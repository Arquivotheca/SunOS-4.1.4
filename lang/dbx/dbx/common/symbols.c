#ifndef lint
static	char sccsid[] = "@(#)symbols.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Symbol management.
 */

#include "defs.h"
#include "symbols.h"
#include "languages.h"
#include "printsym.h"
#include "tree.h"
#include "operators.h"
#include "eval.h"
#include "mappings.h"
#include "events.h"
#include "process.h"
#include "runtime.h"
#include "machine.h"
#include "names.h"
#include "types.h"
#include "object.h"
#include "getshlib.h"
#include "library.h"
#include "dbxmain.h"

#ifndef public
typedef struct Symbol *Symbol;

#define AOUT 0
#define SHLIB 1

#include "machine.h"
#include "names.h"
#include "languages.h"
#include "object.h"

/*
 * Symbol classes
 *	add one here ==> add its name in printsym.c
 */

typedef enum {
    BADUSE, CONST, TYPE, VAR, ARRAY, PTRFILE, RECORD, FIELD,
    PROC, FUNC, FVAR, REF, PTR, FILET, SET, RANGE,
    LABEL, WITHPTR, SCAL, STR, PROG, IMPROPER, UNION,
    FPROC, FFUNC, MODULE, TYPEID, COMMON, TAG, VARIANT, 
    READONLY, CARRAY, CRANGE, CVALUE, FORWARD, 
    FUNCT, IMPORT, MODUS, TYPEREF, BASED, VARYING
} Symclass;

typedef enum { R_CONST, R_TEMP, R_ARG, R_ADJUST } Rangetype;

struct Symbol {
    Name name;
    Language language;
    Integer flag:1;
    Symclass class : 7;
    Integer level : 8;
    Symbol type;
    Symbol chain;
    union {
	int offset;		/* variable address */
	long iconval;		/* integer constant value */
	double fconval;		/* floating constant value */
	struct {		/* import information */
	    Symclass class;	/* import from : IMPORT, MODUS, FUNC */
	    struct Modnode *unit;
	} importv;
	struct {		/* field offset and size (both in bits) */
	    int offset;
	    int length;
	} field;
	struct {		/* (Pascal) varying string */
	    int offset;
	    int length;
	} varying;
	struct {		/* common offset and chain; used to relocate */
	    int offset;		/* vars in global BSS */
	    Symbol chain;
	} common;
	struct {		/* set type */
	    int setsize;		/* size */
	    int startbit;	/* starting bit of the first element */
	} setv;
	struct {		/* enum type size */
	   int enumsize;
	} enumv;
	struct {		/* range bounds */
	    int rngsize : 16;
	    Rangetype lowertype : 8;
	    Rangetype uppertype : 8;
	    long lower;
	    long upper;
	} rangev;
	struct {
	    int offset;		/* offset for of function value */
	    Address beginaddr;	/* address of function code */
	    Address beginoffset; /* offset for "real" code */
	    Boolean src : 1;	/* true if there is source line info */
	    Boolean inline : 1; /* true if no separate act. rec. */
	    Boolean outofdate : 1; 	/* true if out-of-date msg given */
	} funcv;
	struct {		/* Pascal variant record tag info */
	    int varsize;
	    Symbol tagfield;
	    Symbol tagcase;
	} tag;
	struct {
	    int casevalue;
	    Boolean tagfield; /* this field is used temporarily when reading */
			      /* in symbols to pass back info to the TAG rec */
	    Symbol tagtype;   
	} variant;
	struct {
	    Symclass class;	/* Struct, union or enum */
	    Name name;		/* Tag name */
	} forwardref;
    } symvalue;
    Symbol block;		/* symbol containing this symbol */
    Symbol next_sym;		/* hash chain */
};

/*
 * Basic types.
 */

Symbol t_boolean;
Symbol t_char;
Symbol t_int;
Symbol t_real;
Symbol t_shortreal;
Symbol t_short;
Symbol t_nil;
Symbol t_unsigned;

Symbol program;
Symbol curfunc;
Symbol brkfunc;
Symbol endfunc;		/* what curfunc should be when user process ends */
Integer totsyms;

#define symname(s) idnt(s->name)
#define codeloc(f) ((f)->symvalue.funcv.beginaddr)

#ifdef CONSTANT_FCN_PROLOG
#define rcodeloc(f) ((f)->symvalue.funcv.beginaddr - FUNCOFFSET )
#else
#define rcodeloc(f) ( codeloc(f) - (f)->symvalue.funcv.beginoffset)
#endif CONSTANT_FCN_PROLOG

#define isblock(s) (Boolean) ( \
    s->class == FUNC or s->class == PROC or \
    s->class == MODULE or s->class == PROG or \
    s->class == MODUS \
)
#define skipInlineBlocks(s) { \
    while ((s != nil) and isinline(s)) { \
	s = s->block; \
    } \
}

#define nosource(f) (not (f)->symvalue.funcv.src)
#define isinline(f) ((f)->symvalue.funcv.inline)

#include "tree.h"

/*
 * Some macros to make it easier to find symbols with certain attributes.
 */

#define find(s, withname) \
{ \
    s = lookup(withname); \
    while (s != nil and not (s->name == (withname) and

#define where /* qualification */

#define endfind(s) )) { \
	s = s->next_sym; \
    } \
}

#endif		/* end of public section */

#define EVENUP(r) (((r) + 1) & (~ 1))	/* be consistent */

/*
 * Symbol table structure currently does not support deletions.
 */

#define HASHTABLESIZE 2003

private Symbol hashtab[HASHTABLESIZE];

#define hash(name) ((((unsigned) name) >> 2) mod HASHTABLESIZE)

#define SYMBLOCKSIZE 100

typedef struct Sympool {
    struct Symbol sym[SYMBLOCKSIZE];
    struct Sympool *prevpool;
} *Sympool;

private Sympool sympool = nil;
private Integer nleft = 0;

private Boolean samefile();
private Symbol deref();
private Symbol _lookup();

/*
 * Extend the original macro to a routine in order to
 * deal with subrange assignment.
 *
 * Macro to tell if a symbol is unsigned.
 * It should just test if the lower bound is 0, but the
 * bounds for signed chars are wrong.  Hence, the funny
 * check for the size being one byte and then checking
 * the upper bound against 255.  Signed chars have an
 * upper bound of 127.
 *
 * #define isunsigned(sym) ((sym)->class == RANGE and \
 *		((size(sym) == 1)? \
 *		(sym)->symvalue.rangev.upper == 255: \
 *		(sym)->symvalue.rangev.lower == 0))
 */

public Boolean isunsigned(sym)
Symbol sym;
{
    if (sym->class == SET ) return true;
    if (sym->class != RANGE) return false;
    if (sym->language == findlanguage(".c") and
	strcmp(idnt(sym->type->name), "char") == 0)  /* empirically better */
	return false;		/* signed character */
    if (sym->symvalue.rangev.lower == 0 and sym->symvalue.rangev.upper == -1)
	return true;		/* CARDINAL */
    return (Boolean) (sym->symvalue.rangev.lower >= 0 and 
	    sym->symvalue.rangev.upper > sym->symvalue.rangev.lower);
}

/*
 * Allocate a new symbol.
 */

public Symbol symbol_alloc()
{
    register Sympool newpool;

    totsyms++;
    if (nleft <= 0) {
	newpool = new(Sympool);
	if (newpool == 0)
		fatal("No memory available. Out of swap space?");
	bzero((char *) newpool, sizeof(*newpool));
	newpool->prevpool = sympool;
	sympool = newpool;
	nleft = SYMBLOCKSIZE;
    }
    --nleft;
	if(reading_shlib == true)
		(&(sympool->sym[nleft]))->flag = SHLIB;
    return &(sympool->sym[nleft]);
}

public dump_fcn_locals( func, verbose )
Symbol func;
Boolean verbose;
{
    register Symbol s;
    register Integer i;

    printf(" symbols in %s \n",symname(func));
    for (i = 0; i< HASHTABLESIZE; i++) {
	for (s = hashtab[i]; s != nil; s = s->next_sym) {
	    if (s->block == func) {
		psym( s, verbose );
	    }
	}
    }
}

public dump_syms()
{
    register Symbol s;
    register Integer i;

    printf(" SYMBOLS: \n");
    for (i = 0; i< HASHTABLESIZE; i++) {
		for (s = hashtab[i]; s != nil; s = s->next_sym) {
			psym( s, 1 );
		}
    }
    printf(" END OF SYMBOLS \n");
	fflush(stdout);		/* so that calling 'dump_syms()' from dbx works */
}

#ifdef i386
/*
** Given an address, find a symbol at that address.
*/
public findsym(nambuf, destaddr)
char    *nambuf;
Address destaddr;
{
    register Symbol s;
    register int    i;

    *nambuf = '\0';
    for (i = 0; i < HASHTABLESIZE; i++) {
	for (s = hashtab[i]; s != nil; s = s->next_sym) {
	    if (((s->class == CONST) or (s->class == VAR) or (s->class == ARRAY) or
		(s->class == COMMON)) and (s->symvalue.offset == destaddr)) {
		sprintf(nambuf, "%s", s->name->identifier);
		return;
	    }
	}
    }

}
#endif i386


/*
 * Free all the symbols currently allocated.
 */

public symbol_free()
{
    Sympool s, t;
    register Integer i;

    s = sympool;
    while (s != nil) {
	t = s->prevpool;
	dispose(s);
	s = t;
    }
    for (i = 0; i < HASHTABLESIZE; i++) {
	hashtab[i] = nil;
    }
    sympool = nil;
    nleft = 0;
}

/*
 * Create a new symbol with the given attributes.
 */

public Symbol newSymbol(name, blevel, class, type, chain)
Name name;
Integer blevel;
Symclass class;
Symbol type;
Symbol chain;
{
    register Symbol s;

    s = symbol_alloc();
    s->name = name;
    s->level = blevel;
    s->class = class;
    s->type = type;
    s->chain = chain;
    return s;
}

/*
 * Insert a symbol into the hash table.
 */

public Symbol insert(name)
Name name;
{
    register Symbol s;
    register unsigned int h;

    h = hash(name);
    s = symbol_alloc();
    s->name = name;
    s->next_sym = hashtab[h];
    hashtab[h] = s;
    return s;
}

/*
 * An entry must be added to the symbol table - but don't allocate space for an
 * entry; this has already been done to satisfy Pascal forward references.
 */

insert_forw_ref(name, s)
Name name;
register Symbol s;
{
    register unsigned int h;

    h = hash(name);
    s->name = name;
    s->next_sym = hashtab[h];
    hashtab[h] = s;
} 

/*
 * Remove entry from hash table
 */

public unhash(sym)
Symbol sym;
{
	register Symbol s, p;
	register unsigned int h;

	assert(sym != nil);
	h = hash(sym->name);
	s = hashtab[h];
	if (s == sym) {	/* found it the first time */
	    hashtab[h] = s->next_sym;
	} else {	/* check the collision chain */
	    while (s != sym) {
	        if (s == nil) {
		    panic("unhash: can't find entry");
		}
		p = s;
		s = s->next_sym;
	      }
	      p->next_sym = s->next_sym;
	}
      }

/*
 * Symbol lookup.
 * If the symbol is not found and the strip_ flag (smells like FORTRAN)
 * flag is on and the symbol ends in an underscore, then strip the
 * trailing underscore and try again.
 */

public Symbol lookup(name)
Name name;
{
    register Symbol s;
    char buf[256];
    int len;

    s = _lookup(name);
    if (s == nil and strip_) {
	strcpy(buf, idnt(name));
	len = strlen(buf);
	if (buf[len - 1] == '_') {
	    buf[len - 1] = '\0';
	    name = identname(buf, false);
	    s = _lookup(name);
	}
    }
    return s;
}

/*
 * Symbol lookup.
 */
private Symbol _lookup(name)
Name name;
{
    register Symbol s;
    register unsigned int h;

    h = hash(name);
    s = hashtab[h];
    while (s != nil and s->name != name) {
	s = s->next_sym;
    }
    return s;
}

public Symbol findreal(s)
Symbol s;
{
    Symbol t, b;

    if (t = s->chain) return t;
    switch (s->symvalue.importv.class) {
	case IMPORT:
	    t = (s->symvalue.importv.unit)->sym;
	    break;
	case MODUS:
	    b = (s->symvalue.importv.unit)->sym;
	    if (b == nil) return nil;
	    find(t, s->name)
		t->block == b and t->class != FIELD
	    endfind(t);
	    if (t->class == IMPORT) t = findreal(t);
	    break;
	case FUNC:
	    b = s->block;
	    if ( b == nil) return nil;
	    b = b->block;
	    do {
		if (b == nil) return nil;
		find (t, s->name)
		    t->block == b and t->class != FIELD
		endfind(t);
	        if (t and t->class == IMPORT) {
			t = findreal(t);
			if (t == nil) return nil;
		}
		b = b->block;
	    } while (t == nil);
	    break;
    }
    s->chain = t;
    return t;
}

/*
 * zero all the symbols defined in the shlib lib
 */
public clean_up()
{
	register Integer i;
	register Symbol s;

	for (i = 0; i < HASHTABLESIZE; i++) {
		for (s = hashtab[i]; s != nil; s = s->next_sym) {
			if (s->flag == SHLIB)
				s->symvalue.offset = 0;
		}
	}
}

/*
 * Dump out all the variables associated with the given
 * procedure, function, or program (assumes it is not an inline
 * block) at the given recursive level.
 *
 * This is quite inefficient.  We traverse the entire symbol table
 * each time we're called.  The assumption is that this routine
 * won't be called frequently enough to merit improved performance.
 */

public dumpvars(f, frame)
Symbol f;
Frame frame;
{
    register Integer i;
    register Symbol s;

    for (i = 0; i < HASHTABLESIZE; i++) {
	for (s = hashtab[i]; s != nil; s = s->next_sym) {
	    if (container(s) == f) {
		if (should_print(s)) {
		    printv(s, frame);
		    printf("\n");
		} else if (s->class == MODULE) {
		    dumpvars(s, frame);
		}
	    }
	}
    }
}

/*
 * Create base types.
 */

public symbols_init()
{
    t_boolean = maketype("$boolean", 0L, 1L);
    t_int = maketype("$integer", 0x80000000L, 0x7fffffffL);
    t_short = maketype("$short", 0x8000L, 0x7fffL);
    t_char = maketype("$char", 0L, 127L);
    t_real = maketype("$real", 8L, 0L);
    t_shortreal = maketype("$shortreal", 4L, 0L);
    t_nil = maketype("$nil", 0L, 0L);
    t_unsigned = insert(identname("unsigned", true));
    *t_unsigned = *maketype("unsigned", 0x80000000L, 0x7fffffffL);
}

/*
 * Create a builtin type.
 * Builtin types are circular in that btype->type->type = btype.
 */

public Symbol maketype(name, lower, upper)
String name;
long lower;
long upper;
{
    register Symbol s;

    s = newSymbol(identname(name, true), 0, TYPE, nil, nil);
    s->language = findlanguage(".c");
    s->type = newSymbol(nil, 0, RANGE, s, nil);
    s->type->symvalue.rangev.lower = lower;
    s->type->symvalue.rangev.upper = upper;
    return s;
}

/*
 * These functions are now compiled inline.
 *
 * public String symname(s)
Symbol s;
{
    checkref(s);
    return idnt(s->name);
}

 *
 * public Address codeloc(f)
Symbol f;
{
    checkref(f);
    if (not isblock(f)) {
	panic("codeloc: \"%s\" is not a block", idnt(f->name));
    }
    return f->symvalue.funcv.beginaddr;
}
 *
 */

/*
 * Reduce type to avoid worrying about type names.
 */
public Symbol rtype(type)
Symbol type;
{
    register Symclass class;
    register Symbol t;

    t = type;
    if (t != nil) {
	class = t->class;
	if (class == VAR or class == FIELD or class == TAG or class == REF or
	    class == BASED or class == FVAR or class == CVALUE) {
	    t = t->type;
	}
	while (t->class == TYPE or t->class == TYPEID) {
	    if( t == t->type ) {
		return t;	/* opaque or void */
	    }
	    t = t->type;
	}
	if (t->class == IMPORT) {
	    t = findreal(t);
	    if (t == nil) 
                error("import type \"%s\" is not available", symname(type));
	    t = rtype(t);
	}
    }
    return t;
}

public Symbol container(s)
Symbol s;
{
    checkref(s);
    return s->block;
}

/*
 * Return the object address of the given symbol.
 *
 * There are the following possibilities:
 *
 *	globals		- just take offset
 *	locals		- take offset from locals base
 *	arguments	- take offset from argument base
 *	register	- offset is register number
 */

#ifndef public
#define isglobal(s)   ((char)s->level == (char)1 or \
		       (char)s->level == (char)2)
#define islocaloff(s) ((char)s->level >= (char)3 and s->symvalue.offset < 0)
#define isparamoff(s) ((char)s->level >= (char)3 and s->symvalue.offset >= 0)
#endif

public Address address(s, frame)
Symbol s;
Frame frame;
{
    register Frame frp;
    Address addr;
    register Symbol cur;
    register Symclass class;
    register int paramsize;
    Symbol b,ss;

    ss = s;
    checkref(s);
    if (s->class == IMPORT) {
	s = findreal(ss);
        if (s == nil)
	    error("import variable \"%s\" is not available", symname(ss));
    }
    b = s->block;
    if (not isglobal(s) and not isactive(b)) {
	error("\"%s\" is not currently defined", symname(s));
    } else if (isglobal(s)) {
	addr = s->symvalue.offset;
    } else {
	frp = frame;
	if (frp == nil) {
	    frp = symframe(s);
	}
	if( s->class == BASED ) {
	    addr = s->symvalue.offset;
	} else if (islocaloff(s)) {
	    addr = locals_base(frp) + s->symvalue.offset;
	/*
	 * char and short parameters are held in four bytes; the value
	 * is in the low order bytes; however, the symvalue.offset is
	 * the offset get you to the high end of the four bytes.
	 */
	} else if (isparamoff(s)) {
	    paramsize = sizeof(Word) - size(s->type);
	    cur = rtype(s);
	    class = cur->class;
#ifdef i386
	   addr = args_base(frp) + s->symvalue.offset;
#else
	    if (ss->language != findlanguage(".mod") and 
	       (paramsize > 0 and s->class != REF and s->class != CVALUE and
	      class != RECORD and class != ARRAY and class != CARRAY and
	      class != SET)) {
	    	addr = args_base(frp) + s->symvalue.offset + paramsize;
	    } else {
	    	addr = args_base(frp) + s->symvalue.offset;
#ifdef sparc
		/* if it's an aggregate and a parameter (eg struct by value)
		** then, on a sparc, the arg slot address is the address
		** of the address, ie: arg is address of struct. [1010083]
		* I think this specifically applies to (register struct value)
		*/
		/* probably, sparc C should label (struct by value) 
		 * as the (struct by reference) that it really is.
		 * then this whole block could go away.  That would
		 * require a new stabs decl, though...
		 * As is it, fortran really does use struct by reference,
		 * so we must prevent the double de-reference.
		 * jpeck 9/27/89 [1027608] */
		if(s->class != REF) { /* v (REF) params are already indirect */
		    if( rtype(s->type)->class == RECORD ||
		       rtype(s->type)->class == UNION ) {
     			dread(&addr,addr,sizeof(Address));
		    }
		}
#endif
	    }
#endif i386
	} else if (isreg(s)) {
	    panic("attempt to get address of register %d in address()",
		s->symvalue.offset);
	} else {
	    panic("address: bad symbol \"%s\"", symname(s));
	}
    }
    return addr;
}

/*
 * Define a symbol used to access register values.
 */

public defregname(n, r, type)
Name n;
Integer r;
Symbol type;
{
    register Symbol s, t;

    s = insert(n);
    s->language = findlanguage(".c");
    s->class = VAR;
    s->level = -3;
    s->type = type;
    s->block = program;
    s->symvalue.offset = r;
}

/*
 * Resolve an "abstract" type reference.
 *
 * It is possible in C to define a pointer to a type, but never define
 * the type in a particular source file.  Here we try to resolve
 * the type definition.  This is problematic, it is possible to
 * have multiple, different definitions for the same name type.
 */

public findtype(s)
Symbol s;
{
    register Symbol t, u, prev;

    u = s;
    prev = nil;
    while (u != nil and u->class != BADUSE) {
	if (u->name != nil) {
	    prev = u;
	}
	u = u->type;
    }
    if (prev == nil) {
	error("couldn't find link to type reference");
    }
    find(t, prev->name) where
	t->type != nil and t->class == prev->class and
	t->type->class != BADUSE and t->block->class == MODULE
    endfind(t);
    if (t == nil) {
	error("couldn't resolve reference");
    } else {
	prev->type = t->type;
    }
}

/*
 * Find the size in bytes of the given type.
 *
 * This is the WRONG thing to do.  The size should be kept
 * as an attribute in the symbol information as is done for structures
 * and fields.  I haven't gotten around to cleaning this up yet.
 *
 * This comes out WRONG for fields that are odd-length arrays.
 * Because of alignment concerns within and outside of structures,
 * what is really needed is TWO size fields:  the "real" size (i.e.,
 * how much of its memory is meaningful) and the "aligned" size
 * (i.e., how much space it takes up).
 */

#define MINCHAR -128
#define MAXCHAR 127
#define MAXUCHAR 255
#define MINSHORT -32768
#define MAXSHORT 32767
#define MAXUSHORT 65535L
#define MAXULONG -1

public Integer size(sym)
Symbol sym;
{
    register Symbol s, t, u, v;
    register int nel, elsize;
    long lower, upper;
    int r;

    t = sym;
    checkref(t);
    if (t->class == IMPORT) {
	t = findreal(t);
        if (t == nil) 
	    error("import type \"%s\" is not available", symname(sym));
    }
    if (t == t_boolean) {
	return(sizeof(int));
    }
    switch (t->class) {
	case RANGE:
	    if (t->language == findlanguage(".mod")) {
		r = t->symvalue.rangev.rngsize;
		break;
	    }
	    lower = t->symvalue.rangev.lower;
	    upper = t->symvalue.rangev.upper;
	    if (upper == MAXULONG) {
		r = sizeof(long);
	    } else if (upper == 0 and lower > 0) {		/* real */
		r = lower;
	    } else if (lower >= MINCHAR and upper <= MAXCHAR) {
		r = sizeof(char);
	    } else if (lower >= 0 and upper <= MAXUCHAR) {
	    	if (t->language != findlanguage(".p")) {
		    r = sizeof(char);
		} else { /* pc allocates a short for var u: 0..255 */
			if(t->type and istypename(t->type, "char") )
		    	r = sizeof(char);
			else
		    	r = sizeof(short);
		}
	    } else if (lower >= MINSHORT and upper <= MAXSHORT) {
		r = sizeof(short);
	    } else if (lower >= 0 and upper <= MAXUSHORT) {
	    	if (t->language != findlanguage(".p")) {
		    r = sizeof(short);
		} else {
		    r = sizeof(long);
		}
	    } else {
		r = sizeof(long);
	    }
	    break;

	case CRANGE:		/* Pascal conformant array dimension */
	    r = size(t->type);
	    break;

	case ARRAY:
	    elsize = size(t->type);
	    nel = 1;
	    v = t;		/* save for odd test at end */
	    for (s = t->chain; s != nil; s = s->chain) {
		t = rtype(s);
		if (t->class == SCAL) {	/* Pascal indexes can be scalar */
		    nel = 0;
		    for (u = t->chain; u != nil; u = u->chain) {/*# elements?*/
		        nel++;
		    } 
		    if (nel == 0) {
			nel++;
		    }
		} else {
		    lower = get_lowbound(t, t);
		    upper = get_upbound(t, t);

		    /* 
		     * C initialized arrays declared with implicit bounds 
		     * have a bounds of -1..0 in the stab line
		     */
	    	    if ((upper < 0) and (v->language == findlanguage(".c"))) {
		        upper = 0;
		    }
		    nel *= (upper-lower+1);
	        }
	    }
	    r = nel*elsize;
	    break;

	case CARRAY:		/* Pascal conformant array */
	    elsize = size(t->type);
	    nel = 1;
	    s = rtype(t->chain);
	    if (s->class != CRANGE) {
		panic("conformant array symbol table error");
	    }
	    if (getcbound(t, s->symvalue.rangev.lower, &lower) == false) {
		panic("conformant array symbol table error");
	    }
	    if (getcbound(t, s->symvalue.rangev.upper, &upper) == false) {
		panic("conformant array symbol table error");
	    }
	    nel *= (upper-lower+1);
	    r = nel*elsize;
	    EVENUP(r);
	    break;

	case REF:
	case BASED:
	case VAR:
	case FVAR:
	case READONLY:
	case CVALUE:
	    r = size(t->type);
	    /*
	     *
	    if (r < sizeof(Word) and isparam(t)) {
		r = sizeof(Word);
	    }
	     */
	    break;

	case CONST:
	    r = size(t->type);
	    break;

	case FORWARD:  /* fixes 1014550, PEB */
		s = lookup_forward(t);
		if (s != nil) {
		    r = size(s);
		} else {
		    error("Unresolved forward reference for `%s'", symname(t));
		}
		break;

	case TYPE:
	    if (t->type->class == PTR and t->type->type->class == BADUSE) {
		findtype(t);
	    }
	    if( t == t->type ) {
		/* Modula2 Opaque type, or ?maybe void? */ 
		r = sizeof (Word);
	    } else {
		r = size(t->type);
	    }
	    break;

	case TYPEID:
	    r = size(t->type);
	    break;

	case FIELD:
	    /* 8/11/89: We now allow bitfields from other languages than `C'
	     * so we calculate size from the offset and length
	     * if only 3 bytes are used, call it 4, so pop(int) will work
	     */
	    r = ((t->symvalue.field.offset % BITSPERBYTE) +
		 t->symvalue.field.length + (BITSPERBYTE - 1)) div BITSPERBYTE;
	    /* if (size(u) == r + 1) r++ */
	    if ((u = rtype(t))
		and (u->class == SET) or
		((u = u->type) and
		 (istypename(u, "int")   	/* .c bitfields */
		  or istypename(u, "integer") 	/* .p unaligned integer range */
		  or istypename(u, "set")     	/* .p unaligned set */
		  ))) {
		/* if (sizeof(short) < r && r < sizeof(long)) ++r; */
		if (r == (sizeof(short) + 1)) ++r;
	    }
	    break;

	case RECORD:
	case UNION:
	    r = t->symvalue.offset;
	    if (r == 0 and t->chain != nil) {
		panic("missing size information for record");
	    }
	    break;

	/*
	 * Pascal varying strings have a length word,
	 * followed by the string area itself.
	 */
	case VARYING:
	    r = sizeof(Word) + t->symvalue.varying.length;
  	    break;
  
	case PTR:
	case FILET:
	    r = sizeof(Word);
	    break;

	case SCAL:
	    if (t->language == findlanguage(".mod")) {
		r = t->symvalue.enumv.enumsize;
		break;
	    }
	    if (t->language != findlanguage(".p")) {
		r = sizeof(Word);
	    } else {
		nel = 0;
		for (s = t->chain; s != nil; s = s->chain) { /* # elements? */
		    nel++;
		} 
		r = (nel + 255) div 256;
	    }
	    break;

	case SET:
	    if (t->language == findlanguage(".mod")) {
			r = t->symvalue.setv.setsize;
			break;
	    }
		nel = pascal_set_nelem(t);
		if (pascal_xl)
			r = ((nel + 15) div 16) * 2;
		else
			r = ((nel + 31) div 32) * 4;	/* # wds * 4 bytes */
	    break;

	case FPROC:
	case FFUNC:
	    r = sizeof(Word);
	    break;

	case PROC:
	case FUNC:
	case MODULE:
	case PROG:
	    r = sizeof(Symbol);
	    break;

	case FUNCT:
	    r = sizeof(Word);
	    break;

	default:
	    if (ord(t->class) > ord(TYPEREF)) {
		panic("size: bad class (%d)", ord(t->class));
	    } else {
		error("improper operation on %s", aclassname(t));
	    }
	    /* NOTREACHED */
    }
    return r;
}

/*
 * Test if a symbol is a parameter.  This is true if there
 * is a cycle from s->block to s via chain pointers or if
 * it's a register parameter, there is a register entry
 * in the symbol table with the same name as the chained entry.
 */

public Boolean isparam(s)
Symbol s;
{
    register Symbol t;
    register Boolean b;

    b = false;
    t = s->block;
    while (t != nil and t != s) {
	t = t->chain;
    }
    if (t != nil) {
	b = true;
    } else {
    	t = s->block;
	while (t != nil) {
	    if (t->name != s->name or (char)t->level != -(char)s->level) {
		t = t->chain;
	    }
	    else {
		break;
	    }
	}
	if (t != nil) {
	    b = true;
	}
    }
    return b;
}

/*
 * Given a param, return the corresponding register param entry from
 * the symbol table (if it exists).
 */

public Symbol getregparam(s)
Symbol s;
{
    register Symbol t;

    find (t, s->name) where
        s->block == t->block and (char)s->level == -(char)t->level
    endfind(t);
    return t;
}

/*
 * Test if a symbol is accessed like a parameter passed by reference
 * (i.e., like a Pascal "var" param).  This is true if it has class
 * REF, or is a BASED variable (f77), or is a Pascal conformant array.
 */
public Boolean isvarparam(s)
Symbol s;
{
    return (Boolean)
	(s->class == REF or s->class == BASED or s->class == CVALUE);
}

/*
 * Test if a symbol is a variable (actually any addressible quantity
 * with do).
 */
public Boolean isvariable(s)
register Symbol s;
{
    register Symclass class;

    class = s->class;
    return (Boolean) (class == VAR or class == FVAR or class == REF or
      class == BASED or class == READONLY or class == CVALUE);
}

/*
 * Test if a symbol is a block, e.g. function, procedure, or the
 * main program.
 *
 * This function is now expanded inline for efficiency.
 *
 * public Boolean isblock(s)
register Symbol s;
{
    return (Boolean) (
	s->class == FUNC or s->class == PROC or
	s->class == MODULE or s->class == PROG
    );
}
 *
 */

/*
 * Test if a symbol is a module.
 */

public Boolean ismodule(s)
register Symbol s;
{
    return (Boolean) (s->class == MODULE);
}

/*
 * Test if a symbol is builtin, that is, a predefined type or
 * reserved word.
 */

public Boolean isbuiltin(s)
register Symbol s;
{
    if (s == nil) {
	return false;
    } else {
    	return (Boolean)((char)s->level == (char)0 and
	  s->class != PROG and s->class != VAR and s->class != FVAR);
    }
}

/*
 * Test if two types match.
 * Equivalent names implies a match in any language.
 *
 * Special symbols must be handled with care.
 */

public Boolean compatible(t1, t2)
register Symbol t1, t2;
{
    Boolean b;

    if (t1 == t2) {
	b = true;
    } else if (t1 == nil or t2 == nil) {
	b = false;
    } else if (t1 == procsym) {
	b = isblock(t2);
    } else if (t2 == procsym) {
	b = isblock(t1);
    } else if (t1->language == nil) {
	b = (Boolean) (t2->language == nil or typematch(t2, t1));
    } else if (t2->language == nil) {
	b = (Boolean) typematch(t1, t2);
    } else if (isbuiltin(t1) or isbuiltin(t1->type)) {
	b = (Boolean) typematch(t2, t1);
    } else {
	b = (Boolean) typematch(t1, t2);
    }
    return b;
}

/*
 * Check for a type of the given name.
 */

public Boolean istypename(type, name)
Symbol type;
String name;
{
    Symbol t;
    Boolean b;

    t = type;
    if (t == nil) return false; /* checkref(t) would panic! */
    if (t->class == IMPORT) {
	t = findreal(t);
        if (t == nil)
            error("import type \"%s\" is not available", symname(type));
    }
    b = (Boolean) (
	t->class == TYPE and t->name == identname(name, true)
    );
    return b;
}

/*
 * Test if the name of a symbol is uniquely defined or not.
 */

public Boolean isambiguous(s)
register Symbol s;
{
    register Symbol t;

    find(t, s->name) 
	where t != s and 
	(t->class == s->class or 
	 t->class == FUNC and s->class == MODUS or
	 t->class == MODUS and s->class == FUNC)
    endfind(t);
    return (Boolean) (t != nil);
}

typedef char *Arglist;

#define nextarg(type)  ((type *) (ap += sizeof(type)))[-1]

private Symbol mkstring();
private Symbol namenode();

/*
 * Determine the type of a parse tree.  Also make some symbol-dependent
 * changes to the tree such as removing RVAL nodes for constant symbols
 * and changing O_NEG to O_NEGF where appropriate.
 */

public assigntypes(p)
register Node p;
{
    register Node p1;
    register Symbol s;

    switch (p->op) {
	case O_SYM:
	    p->nodetype = namenode(p);
	    break;

	case O_FCON:
	    p->nodetype = t_real;
	    break;

	case O_SCON:
	    p->value.scon = strdup(p->value.scon);
	    s = mkstring(p->value.scon);
	    p->nodetype = s;
	    break;

	case O_AMPER:
	    p1 = p->value.arg[0];
	    p->nodetype = newSymbol(nil, 0, PTR, p1->nodetype, nil);
	    p->nodetype->language = p1->nodetype->language;
	    break;

	case O_INDIR:
	    p1 = p->value.arg[0];
	    chkclass(p1, PTR);
	    p->nodetype = deref(p1);
	    break;
	
	case O_INDEX:
	    p1 = p->value.arg[0];
	    p->nodetype = deref(p1);
	    break;

	case O_DOT:
	    p->nodetype = p->value.arg[1]->value.sym;
	    break;

	case O_RVAL:
	    p1 = p->value.arg[0];
	    p->nodetype = p1->nodetype;
	    if (p1->op == O_SYM) {
		if (p1->value.sym->class == CONST) {
		    if (compatible(p1->value.sym->type, t_real)) {
			p->op = O_FCON;
			p->value.fcon = p1->value.sym->symvalue.fconval;
			p->nodetype = t_real;
			dispose(p1);
		    } else {
			p->op = O_LCON;
			p->value.lcon = p1->value.sym->symvalue.iconval;
			p->nodetype = p1->value.sym->type;
			dispose(p1);
		    }
		} else if( isreg(p1->value.sym)
#ifdef sparc
		/* on a sparc an aggregate in a reg is actually a pointer 
		** to the aggregate - don't remove the O_RVAL
		*/
		&& (rtype(p->nodetype)->class != RECORD)
		&& (rtype(p->nodetype)->class != UNION)
#endif
		) {
		    p->op = O_SYM;
		    p->value.sym = p1->value.sym;
		    dispose(p1);
		}
	    } else if (p1->op == O_INDIR) {
		while (p1->value.arg[0]->op == O_INDIR) {
		    p1 = p1->value.arg[0];
		}
		if (p1->value.arg[0]->op == O_SYM) {
		    s = p1->value.arg[0]->value.sym;
		    if (isreg(s)) {
		        p1->op = O_SYM;
		        dispose(p1->value.arg[0]);
		        p1->value.sym = s;
		        p1->nodetype = s;
		    }
		}
	    }
	    break;

	case O_CALL:
	    p1 = p->value.arg[0];
	    p->nodetype = deref(p1);
	    break;

	case O_TYPERENAME: { 
	        Symbol from, to;
		Symbol intsym;
		Node abs_decl;
		Node type;

	        p->nodetype = p->value.arg[1]->nodetype;
	        to = rtype(p->nodetype);
		if (to->class == ARRAY) {
		    error("Cannot cast to an array");
		} else if (to->class == RECORD) {
		    error("Cannot cast to a structure");
		} else if (to->class == UNION) {
		    error("Cannot cast to a union");
		}

	        from = rtype(p->value.arg[0]->nodetype);
		if (from->class == ARRAY) {
		    error("Cannot cast an array");
		} else if (from->class == RECORD) {
		    error("Cannot cast a structure");
		} else if (from->class == UNION) {
		    error("Cannot cast a union");
		}

	        if (isfloat(from) and not isfloat(to)) {
		    p->value.arg[0] = build(O_FTOI, p->value.arg[0]);
	        } else if (isfloat(to)) { 
		    if (not isfloat(from)) {
			abs_decl = build(O_SYM, newSymbol(nil, 0, TYPE, t_int, nil));
			intsym = which(identname("int", false));
			type = buildtype(build(O_SYM, intsym), abs_decl);
		        p->value.arg[0] = build(O_TYPERENAME, p->value.arg[0], type);
		        p->value.arg[0] = build(O_ITOF, p->value.arg[0]);
		    }
	        } 
	    }
	    break;

	case O_ITOF:
	    p->nodetype = t_real;
	    break;

	case O_NEGF:
	    s = p->value.arg[0]->nodetype;
	    break;

	case O_NEG:
	    s = p->value.arg[0]->nodetype;

	    if( isfloat( rtype(s) ) ) {
		p->op = O_NEGF;
	    } else if (not compatible(s, t_int)) {
		if (not compatible(s, t_real)) {
		    beginerrmsg();
		    prtree(p->value.arg[0]);
		    printf("is improper type");
		    enderrmsg();
		} else {
		    p->op = O_NEGF;
		}
	    }
	    p->nodetype = s;
	    break;

	case O_ADD:
	{
	    /*
	     * special case checks for PTR + int: ok
	     *                         int + PTR: ok
	     *                         PTR + PTR: not ok
	     */
	    Symbol t1, t2, t3;
	    Node tmp;

	    t1 = rtype(p->value.arg[0]->nodetype);
	    t2 = rtype(p->value.arg[1]->nodetype);
	    if (t2->class == PTR){
		if (t1->class == PTR){
		    error("pointer + pointer is not allowed");
		} else {
		    /* put in canonical form */
		    tmp = p->value.arg[0];
		    p->value.arg[0] = p->value.arg[1];
		    p->value.arg[1] = tmp;
		    t3 = t2; t2 = t1; t1 = t3;
		}
	    }
	    if (t1->class == PTR){
		if (!compatible( t2, t_int) ){
		    error("type clash in pointer arithmetic");
		}
		p->nodetype = t1;
	    } else 
		goto arithmetic;
	    break;

	}
	case O_SUB:
	{
	    /*
	     * special case checks for PTR - int: ok
	     *                         PTR - PTR: ok
	     *                         int - PTR: not ok
	     */
	    Symbol t1, t2;

	    t1 = rtype(p->value.arg[0]->nodetype);
	    t2 = rtype(p->value.arg[1]->nodetype);
	    if (t1->class == PTR ){
		if (t2->class == PTR){
		    /* this really should be a stronger test */
		    if (!compatible( t1->type, t2->type)){
			error("pointer type clash");
		    }
		    p->nodetype = t_int;
		} else
		    p->nodetype = t1;
	    } else if (t2->class == PTR){
		error("cannot subtract a pointer from a non-pointer");
		goto arithmetic;
	    }else
		goto arithmetic;
	    break;
	}
	case O_MUL:
	case O_LT:
	case O_LE:
	case O_GT:
	case O_GE:
	case O_EQ:
	case O_NE:
	arithmetic:
	{
	    Boolean t1real, t2real;
	    Symbol t1, t2;

	    t1 = rtype(p->value.arg[0]->nodetype);
	    t2 = rtype(p->value.arg[1]->nodetype);
	    t1real = compatible(t1, t_real);
	    t2real = compatible(t2, t_real);
	    if (t1real or t2real) {
		p->op = (Operator) (ord(p->op) + 1);
		if (not t1real) {
		    p->value.arg[0] = build(O_ITOF, p->value.arg[0]);
		} else if (not t2real) {
		    p->value.arg[1] = build(O_ITOF, p->value.arg[1]);
		}
	    }
	    if (ord(p->op) >= ord(O_LT)) {
		p->nodetype = t_boolean;
	    } else {
		if (t1real or t2real) {
		    p->nodetype = t_real;
		} else {
		    p->nodetype = t_int;
		}
	    }
	    break;
	}

	case O_LTF:
	case O_LEF:
	case O_GTF:
	case O_GEF:
	case O_EQF:
	case O_NEF:
	    p->nodetype = t_boolean;
	    break;

	case O_DIVF:
	case O_MULF:
	case O_ADDF:
	case O_SUBF:
	{
	    Symbol t1, t2;

	    t1 = rtype(p->value.arg[0]->nodetype);
	    t2 = rtype(p->value.arg[1]->nodetype);
	    if (not (isfloat(t1))) {
	    	convert(&(p->value.arg[0]), t_real, O_ITOF);
	    }
	    if (not (isfloat(t2))) {
	    	convert(&(p->value.arg[1]), t_real, O_ITOF);
	    }
	    p->nodetype = t_real;
	    break;
	}

	case O_DIV:
	case O_MOD:
	{
	    Symbol t1, t2;

	    t1 = rtype(p->value.arg[0]->nodetype);
	    t2 = rtype(p->value.arg[1]->nodetype);
	    if (isfloat(t1)) {
		error("expression is floating point - use '/'");
	    	/*convert(&(p->value.arg[0]), t_int, O_NOP);*/
	    }
	    if (isfloat(t2)) {
		error("expression is floating point - use '/'");
	    	/*convert(&(p->value.arg[1]), t_int, O_NOP);*/
	    }
	    p->nodetype = p->value.arg[0]->nodetype;
	    break;
	}

	case O_BITAND:
	case O_BITOR:
	case O_BITXOR:
	case O_RSHIFT:
	case O_LSHIFT:
	case O_NOT:
	case O_COMPL:
	    p->nodetype = p->value.arg[0]->nodetype;
	    break;

	case O_LOGAND:
	case O_LOGOR:
	    chkboolean(p->value.arg[0]);
	    chkboolean(p->value.arg[1]);
	    p->nodetype = t_boolean;
	    break;

	case O_LCON:
	case O_FTOI:
	case O_SIZEOF:
	case O_QLINE:
	    p->nodetype = t_int;
	    break;

	case O_QUEST:
	    p->nodetype = p->value.arg[1]->nodetype;
	    break;

	default:
	    p->nodetype = nil;
	    break;
    }
}

/*
 * Deference a type to produce the type it points to.
 * Check for uncovering a forward reference here.
 * If one is found, look it up, and if it is still
 * undefined, then complain.
 */
private Symbol deref(p)
Node p;
{
    Symbol sym;

    sym = rtype(p->nodetype)->type;
    if (sym->class == FORWARD) {
        sym = lookup_forward(sym);
        if (sym == nil) {
            printf("No type for `");
            prtree(p);
	    printf("'\n");
            erecover();
	}
    }
    return(sym);
}

/*
 * Create a node for a name.  The symbol for the name has already
 * been chosen, either implicitly with "which" or explicitly from
 * the dot routine.
 */

private Symbol namenode(p)
Node p;
{
    register Symbol s;
    register Node np;

    s = p->value.sym;
    if (s->class == REF or s->class == BASED or s->class == CVALUE) {
	np = new(Node);
	if (np == 0)
		fatal("No memory available. Out of swap space?");
	np->op = p->op;
	np->nodetype = s;
	np->value.sym = s;
	p->op = O_INDIR;
	p->value.arg[0] = np;
    }
/*
 * Old way
 *
    if (s->class == CONST or s->class == VAR or s->class == FVAR) {
	r = s->type;
    } else {
	r = s;
    }
 *
 */
    return s;
}

/*
 * Convert a tree to a type via a conversion operator;
 * if this isn't possible generate an error.
 *
 * Note the tree is call by address, hence the #define below.
 */

private convert(tp, typeto, op)
Node *tp;
Symbol typeto;
Operator op;
{
#define tree    (*tp)

    Symbol s;

    s = rtype(tree->nodetype);
    typeto = rtype(typeto);
    if (compatible(typeto, t_real) and compatible(s, t_int)) {
	tree = build(op, tree);
    } else if (not compatible(s, typeto)) {
	beginerrmsg();
	prtree(tree);
	printf(" is improper type");
	enderrmsg();
    } else if (op != O_NOP and s != typeto) {
	tree = build(op, tree);
    }

#undef tree
}

public Node at(func, right)
Node func;
Name right;
{
    register Symbol s;
    Symbol left;
    register Node p;

    left = func->nodetype;
    if (isblock(left)) {
        find(s, right) where
            s->block == left and
            s->class != FIELD and s->class != TYPEID
        endfind(s);
        if (s != nil and s->class == IMPORT) s = findreal(s);
        if (s == nil) {
            beginerrmsg();
            printf("\"%s\" is not defined in ", idnt(right));
            printname(left);
            enderrmsg();
        }
        p = new(Node);
	if (p == 0)
		fatal("No memory available. Out of swap space?");
        p->op = O_SYM;
        p->value.sym = s;
        p->nodetype = namenode(p);
    } else {
	error("syntax error");
    }
    return p;
}

public Node atglobal(right)
Name right;
{
    register Symbol s;
    Symbol left;
    register Node p;

    find(s, right) where
        s->block == program and
        s->class != FIELD and s->class != TYPEID
    endfind(s);
    if (s != nil and s->class == IMPORT) s = findreal(s);
    if (s == nil) {
        beginerrmsg();
        printf("\"%s\" is not defined globally", idnt(right));
        enderrmsg();
    }
    p = new(Node);
    if (p == 0)
	fatal("No memory available. Out of swap space?");
    p->op = O_SYM;
    p->value.sym = s;
    p->nodetype = namenode(p);
    return p;
}

public Symbol curfile;

public Symbol find_file(n)
Name n;
{
	Symbol s;

	find (s, n) where
	    s->class == MODULE and s->block == program
        endfind(s);
        if (s == nil) 
             error("no file named `%s'", idnt(n));
	return s;
}

/*
 * Find one and only one function or procedure with a given name.
 * Complain if more than one or no functions/procs are found.
 */
public Symbol which_func_file(file, funcname)
Symbol file;
Name funcname;
{
    register Symbol block, s, t, p;
    Symbol sym;
    Symbol func;
    int nfound;
    Name filename;
    char filebuf[100];
    char *dot;

    /* find function in the 'file' scope */
    if (file != nil) {
	find (func, funcname) where
	    func->block == file and 
	    (func->class == FUNC or func->class == PROC or 
	     func->class == MODUS or func->class == VAR)
	endfind(func);
	curfile = nil;
	if (func == nil) {
	    curfile = nil;
	    error("no module or function named %s", idnt(file->name));
	}
	return func;
    }

    curfile = nil;

    if (curfunc != nil and curfunc->level != 1) {
	/*
	 * search through the static scope from the
	 * current function to the current file
	 */
	find(s, funcname) where
	    s->class == FUNC or s->class == PROC or s->class == MODUS
	endfind(s);
	if (s == nil) {
	    error("\"%s\" is not defined as a function or module",
		idnt(funcname));
	} else {
	    p = curfunc;
	    do {
		t = lookup(funcname);
		block = t->block;
		skipInlineBlocks(block);
		while (t->name != funcname or block != p or
		  (t->class != FUNC and t->class != PROC and t->class != MODUS)) {
		    t = t->next_sym;
		    if (t != nil) {
			block = t->block;
			skipInlineBlocks(block);
		    } else {
			break;
		    }
		}  
		p = p->block;
	    } while (t == nil and p != program);
	}          
	if (t != nil) return t; 
    }

    /* find a uniquely defined the function named by funcname */
    filename = nil;
    if (cursource != nil) {
	strcpy(filebuf, cursource);
	dot = rindex(filebuf, '.');
	if (dot != nil) *dot = '\0';
	filename = identname(filebuf, false);
    }
    nfound = 0;
    func = nil;
    sym = lookup(funcname);
    while (sym != nil) {
      if ((sym->class == FUNC or sym->class == PROC or sym->class == MODUS) and 
        sym->name == funcname) {
          nfound++;
	  if (sym->block->name == filename) {
	    nfound = 1;
	    func = sym;
	    break;
	  }
          func = sym;
      }
      sym = sym->next_sym;
    }
    if (nfound == 0) {
	if (file == nil) {
	    /*
	     * If not functions where found and we are not using
	     * an explicit file, then look for a file.
	     */
	    find (s, funcname) where 
		s->class == MODULE
	    endfind(s);
	    if (s != nil) {
		func = s;
	    } else {
                error("no module, procedure or file named `%s'", 
		  idnt(funcname));
	    }
	} else {
            error("no module or procedure named `%s'", idnt(funcname));
	}
    } else if (nfound > 1) {
        error("more than one module or procedure named `%s'",
          idnt(funcname));
    }
    return(func);
}

public Node dot(record, fieldname)
Node record;
Name fieldname;
{
    register Symbol s, t;
    register Node p;

    if (record->nodetype->class == MODUS) {
	p = at(record, fieldname);
    } else if (isblock(record->nodetype)) {
	error("syntax error");
    } else {
	p = record;
        t = rtype(p->nodetype);
        s = t;
        if (s->class == PTR) {
            s = s->type;
        }
		if(s->class == TYPEID || s->class == TYPE) {
			s = rtype(s);
		}
        if (s->class == FORWARD) {
            s = lookup_forward(s);
            if (s == nil) {
                printf("Unresolved forward reference for \"");
                prtree(record);
                printf("\"");
                error("");
            }
        }
        s = findfield(fieldname, s);
        if (s == nil) {
            printf("\"%s\" is not a field in \"", idnt(fieldname));
            prtree(record);
            printf("\"");
            error("");
        }
        if (t->class == PTR and not isreg(record->nodetype)) {
            p = build(O_INDIR, record);
        }
        p = build(O_DOT, p, build(O_SYM, s));
    }
    return p;
}

/*
 * Check to see if a tree is boolean-valued, if not it's an error.
 */

public chkboolean(p)
register Node p;
{
    if ((p->nodetype != t_boolean) && (p->nodetype != t_int)) {
	beginerrmsg();
	printf("found ");
	prtree(p);
	printf(", expected boolean expression");
	enderrmsg();
    }
}

/*
 * Check to make sure the given tree has a type of the given class.
 */

private chkclass(p, class)
Node p;
Symclass class;
{
    struct Symbol tmpsym;

    tmpsym.class = class;
    if (rtype(p->nodetype)->class != class) {
	beginerrmsg();
	printf("\"");
	prtree(p);
	printf("\" is not %s", aclassname(&tmpsym));
	enderrmsg();
    }
}

/*
 * Construct a node for the type of a string.  While we're at it,
 * scan the string for '' that collapse to ', and chop off the ends.
 */

private Symbol mkstring(str)
String str;
{
    register char *p, *q;
    register Symbol s;

    p = str;
    q = str;
    while (*p != '\0') {
	if (*p == '\\') {
	    ++p;
	}
	*q = *p;
	++p;
	++q;
    }
    	*q = '\0';
    	s = newSymbol(nil, 0, ARRAY, t_char, nil);
    	s->language = findlanguage(".s");
    	s->chain = newSymbol(nil, 0, RANGE, t_int, nil);
    	s->chain->language = s->language;
    	s->chain->symvalue.rangev.lower = 1;
    	s->chain->symvalue.rangev.upper = p - str + 1;
    return s;
}

/*
 * Free up the space allocated for a string type.
 */

public unmkstring(s)
Symbol s;
{
    dispose(s->chain);
}

/*
 * Figure out the "current" variable or function being referred to,
 * this is either the active one or the most visible from the
 * current scope.
 *
 * Note - if you're stopped in a routine and "print x" where x is
 * defined in an inner block which you haven't reached yet, which will
 * still find it.  The alternative is allowing curfunc to be an
 * inner block - which is a mess.
 */

 /*
  * how 'which()' works --- ivan		Tue Jan 24 1989
  *
  * Below is a partially rewritten version of 'which()'. The original
  * copy is below that and is #ifdef UNDEF'ed out. Most of the differences
  * are syntactical with the exception of 'skipInlineBlocks()'. Since for
  * the time being, "unnamed modules" and the 'inline' field of Symbol isn't
  * being used (The "mess" referred to above), I decided to take it out to
  * improve the readability ... it was forcing us to use the temporary
  * tracking variable 'block'. 
  *
  * The main 'do' loop attempts to find a symbol whose immediate scope 
  * (block) is the current function. If this fails, it tries with the
  * current functions parent and so on.  The iteration variable for 
  * the do loop is 'p'.
  * It might happen that the symbol we're looking for is not in the current
  * scope. The only such symbols are file statics. Although unkosher, we
  * do want to allow users to access such ones. 'module_sym's job is
  * to remember any file statics which are encountered. Concievably the
  * "remembering" of 'module_sym' can be moved out of the do loop entirely.
  *
  * DON'T PUT skipInlineBlocks() BACK IN!
  * I believe that one of the reasons block nested variables weren't resolved
  * properly was that 'skip...()' was *in*. If we skip the inline blocks how
  * could we possibly access something within them?
  */

#define until(s) while(!(s))

public Symbol which(n)
Name n;
{
    register Symbol s;	
    register Symbol p;	/* Predecessor */
    register Symbol t;	/* Target */
	Symbol module_sym;

    find(s, n) where s->class != FIELD and s->class != TYPEID endfind(s);
    if (s == nil or objname == nil) {
		error("\"%s\" is not defined", idnt(n));
    } else {
		p = curfunc;

		do {
			module_sym = nil;
			t = lookup(n);
			while (t and
				   ! (t->name == n and t->block == p and
			          t->class != FIELD and t->class != TYPEID)
				  ) {
				if (t->block and t->block->class == MODULE and
					t->name == n and
					isvariable(t)) {
					module_sym = t;
				}
				t = t->next_sym;
			}

			if (p) {
				p = p->block;
			}
		} until (t or p == nil);	/* until found target or exhausted scope */

		/*
		 * if failed to find a target in the current scope, fall back on
		 * file statics. Such have been conveniently remembered in
		 * 'module_sym', although if there are several of them, which one
		 * we get is non-deterministic.
		 */
		if (!t and module_sym) {
			t = module_sym;
		}

		if (t and t->class == IMPORT) {
			t = findreal(t);
		}

		/*
		 * If we turn up a module, check that it's not hiding
		 * a global. both will be in the program block and what's wanted
		 * is the var
		 */
		if (t and t->class == MODULE) {
			register Symbol global_var;
			find(global_var, n) where 
				/*
				global_var->class == VAR and global_var->level == 1
				*/
				global_var->class == VAR and isglobal(global_var)
			endfind(global_var);
			if(global_var != nil)
				t = global_var;
		}
		if (t == nil)
			t = s;
    }
    return t;
}

#ifdef UNDEF

public Symbol which******(n)
Name n;
{
    register Symbol s, p, t, block;
    Symbol module_sym;

    find(s, n) where s->class != FIELD and s->class != TYPEID endfind(s);
    if (s == nil) {
		error("\"%s\" is not defined", idnt(n));
    } else {
		p = curfunc;
		do {
			module_sym = nil;
			t = lookup(n);
			block = t->block;
			skipInlineBlocks(block);
			while (t->name != n or block != p or
			  t->class == FIELD or t->class == TYPEID) {
				if (block != nil and
					t->name == n and
					block->class == MODULE and
					isvariable(t)) {
					module_sym = t;
				}
				t = t->next_sym;
				if (t != nil) {
					block = t->block;
					skipInlineBlocks(block);
				} else {
					break;
				}
			}

			if (p != nil) {
				p = p->block;
			}

			if (p == program and module_sym != nil) {
				t = module_sym;
			}

		} while (t == nil and p != nil);

		if (t != nil and t->class == IMPORT) {
			t = findreal(t);
		}

		/* similarly, if we turn up a module, check that it's not hiding
		** a global. both will be in the program block and what's wanted
		** is the var
		*/
		if (t != nil and t->class == MODULE) {
			register Symbol global_var;
			find(global_var, n) where 
				/*
				global_var->class == VAR and global_var->level == 1
				*/
				global_var->class == VAR and isglobal(global_var)
			endfind(global_var);
			if(global_var != nil)
				t = global_var;
		}
		if (t == nil)
			t = s;
    }
    return t;
}
#endif UNDEF

/*
 * Try to resolve a forward reference.
 * Look for a symbol of the correct name, and class in the current file.
 * If not found, look for one anywhere in the program.
 */
public Symbol lookup_forward(sym)
Symbol sym;
{
    Symbol s;
    Name n;

    n = sym->symvalue.forwardref.name;
    find (s, n) where 
	(s->class == TYPEID or s->class == TYPE) and
	rtype(s)->class == sym->symvalue.forwardref.class and
	samefile(s)
    endfind(s);
    if (s == nil) {
        find (s, n) where 
    	    (s->class == TYPEID or s->class == TYPE) and
	    rtype(s)->class == sym->symvalue.forwardref.class
        endfind(s);
    }
    return(s);
}

/*
 * Check if a symbol is contained in the current file.
 */
private Boolean samefile(sym)
Symbol sym;
{
    Symbol blk;
    int len;

    if (cursource == nil) {
	return(false);
    }
    len = index(cursource, '.') - cursource;
    for (blk = sym->block; blk != nil; blk = blk->block) {
	if (blk->class == MODULE and 
	  strncmp(idnt(blk->name), cursource, len) == 0) {
	    return(true);
	}
    }
    return(false);
}

/*
 * Go through a fieldlist and look for a field name match
 */

private Symbol fieldlist_search(fieldname, begin, deep_search)
Name fieldname;
Symbol begin;
Boolean deep_search;
{
    register Symbol t, t2, t3;
    register Symbol prevlist;/* previous field list hanging off variant case */

    t2 = begin;
    t = begin;
    while (t != nil and t->name != fieldname) {
	t = t->chain;
    }
    if (t == nil) {		/* Maybe it's a Pascal variant field */
	while (t2 != nil and t2->class != TAG) {
	    t2 = t2->chain;
	}
	if (t2 != nil) {	/* are there are any variant parts? */
	    t2 = t2->symvalue.tag.tagcase;	/* class of t2 is VARIANT */
	    prevlist = nil;
	    do {		/* for each variant case... */
		t3 = t2->type;
	        if (t3 != nil and t3 != prevlist) {
		    t = fieldlist_search(fieldname, t3, false);
	        }
	        prevlist = t2->type;
	        t2 = t2->chain;
	    } while (t == nil and t2 != nil);
	}
    }

    /*
     * The field name we're looking for is neither at the top level
     * of this struct, nor is it a Pascal variant field.  Maybe it
     * is a FORTRAN UNION field-name -- to find it, we need to do
     * a deep search.  (deep_serach is set only if we call ourself
     * recursively after finding a FORTRAN UNION.
     */
    if( t == nil  and  deep_search ) {
	for( t2 = begin; t2 != nil  and  t == nil; t2 = t2->chain ) {
	    if( t2->type  and  
		(t2->type->class == UNION  or  t2->type->class == RECORD) )
	    {
		t = fieldlist_search( fieldname, t2->type->chain, true );
		if( t ) break;
		
	    }
	}
    }

    if( t==nil and begin!=nil and begin->language == findlanguage(".f") ) {
	for( t2 = begin; t2 != nil  and  t == nil; t2 = t2->chain ) {
	    if( t2->type  and  t2->type->class == UNION ) {
		t = fieldlist_search( fieldname, t2->type->chain, true );
		if( t ) break;
	    }
	}
    }
    return t;
}

/*
 * Find the symbol which is has the same name and scope as the
 * given symbol but is of the given field.  Return nil if there is none.
 */

public Symbol findfield(fieldname, record)
Name fieldname;
Symbol record;
{
    register Symbol t;

    t = rtype(record)->chain;
    return (fieldlist_search(fieldname, t, false));
}

/*
 * Get an lower bound.  
 */
public int get_lowbound(s, range)
Symbol s;
Symbol range;
{
    Boolean r;
    int	val;

    r = getbound(s, range->symvalue.rangev.lower, 
      range->symvalue.rangev.lowertype, &val);
    if (r == false) {
        error("Could not find dynamic bounds");
    }
    return(val);
}

/*
 * Get an upper bound.
 */
public int get_upbound(s, range, valp)
Symbol s;
Symbol range;
int *valp;
{
    Boolean r;
    int	val;

    r = getbound(s, range->symvalue.rangev.upper, 
      range->symvalue.rangev.uppertype, &val);
    if (r == false) {
        error("Could not find dynamic bounds");
    }
    return(val);
}

/*
 * If the array is dynamic the bound
 * will be gotten from the stack.  Otherwise, it is gotten
 * from the symbol.
 */
public Boolean getbound(s, off, type, valp)
Symbol s;
int off;
Rangetype type;
int *valp;
{
    Frame frp;
    Address addr;
    Symbol cur;

    if (type != R_TEMP and type != R_ARG) {
	*valp = off;
	return(true);
    }
    if (not isactive(s->block)) {
	return(false);
    }
    frp = symframe(s);
    if (frp == nil) {
	return(false);
    }
    if (type == R_TEMP) {
	addr = locals_base(frp) + off;
    } else if (type == R_ARG) {
	addr = args_base(frp) + off;
    }

    /*
     * If it's a FORTRAN array parameter, and its upper dimension is
     * another parameter (How do we tell?  Is there any other type of
     * dynamic FORTRAN array parameter?), then we need to dereference it
     * again.
    	if( type != R_CONST  and  s->language == findlanguage( ".f" ) ) {
		dread(&addr,addr,sizeof(Address));
    	}
    ** no - R_ARG in fortran is only used for char*(*) array bounds
	** which are by value - all others, eg
	** sub (ar(n)
	** int ar(n)
	** use a temp
     */

    dread (valp,addr,sizeof(long));
    if (read_err_flag) {
	error("bad address for array dimension");
    }
    return (true);
}

/*
 * Get the value of a Pascal conformant array subscript
 */

public Boolean getcbound(array, range, value)
Symbol array, range;
int *value;
{
    Frame frp;
    register int siz;
    register Symbol cur;
    int valp;
    register Address addr;

    if (not isactive(array->block)) {
	return(false);
    }
    cur = array->block;
    frp = findframe(cur);
    if (frp == nil) {
	return(false);
    }
    addr = args_base(frp) + range->symvalue.offset;
    siz = size(range->type);
    if (siz < sizeof(Word)) {
	addr += sizeof(Word) - siz;
    }
    dread (&valp, addr, siz);
    if (read_err_flag) {
	error("bad address for conformant array dimension");
    }
    switch (siz) {
	case sizeof(char):
	    *value = (valp >> 24) & 0xff;
	    break;
	case sizeof(short):
	    *value = (valp >> 16) & 0xffff;
	    break;
	case sizeof(long):
	    *value = valp;
	    break;
	default:
	    return false;
    }
    return true;
}

/*
 * See if a given name is defined as a type anywhere in the
 * program.
 */
public Boolean istype(name)
Name	name;
{
    Symbol s;

    find (s, name) where s->class == TYPE endfind(s);
    return((Boolean) (s != nil));
}

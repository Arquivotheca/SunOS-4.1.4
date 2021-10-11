#ifndef lint
static	char sccsid[] = "@(#)printsym.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Printing of symbolic information.
 */

#include "defs.h"
#include "symbols.h"
#include "languages.h"
#include "printsym.h"
#include "tree.h"
#include "eval.h"
#include "mappings.h"
#include "process.h"
#include "runtime.h"
#include "machine.h"
#include "names.h"
#include "dbxmain.h"

#ifdef mc68000
#  include "cp68881.h"
#  include "fpa_inst.h"
#elif i386
#  include "fpa_inst.h"
#endif mc68000

/*
 * Maximum number of arguments to a function.
 * This is used as a check for the possibility that the stack has been
 * overwritten and therefore a saved argument pointer might indicate
 * to an absurdly large number of arguments.
 */

#ifndef public
#define MAXARGSPASSED 20
#endif

/*
 * Return a pointer to the string for the name of the class that
 * the given symbol belongs to.  These strings must match the
 * "typedef enum { ... } Symclass;" in symbols.c
 */

private String clname[] = {
    "bad use", "constant", "type", "variable", "array", "fileptr",
    "record", "field", "procedure", "function", "funcvar",
    "ref", "pointer", "file", "set", "range", "label", "withptr",
    "scalar", "string", "program", "improper", "union",
    "procparam", "funcparam", "file", "typeid", "common",
    "tag", "variant", "read only", "carray", "crange", "conf value", 
    "forward", "procedure type", "import", "module", "typeref", "based",
    "varying"
};

public String classname(s)
Symbol s;
{
    return clname[ord(s->class)];
}

/* Get the article right */
public String aclassname(s)	
Symbol s;
{
  static char acl[ 18 ];
  char *cln = classname( s );

    /* array, import or improper (but not union!) */
    if( *cln == 'a'  ||  *cln == 'i' )
	sprintf( acl, "an %s", cln );
    else
	sprintf( acl, "a %s", cln );

    return acl;
}

/*
 * Note the entry of the given block, unless it's the main program.
 */

public printentry(s)
Symbol s;
{
    if (s != program and not istool()) {
	printf("\nentering %s %s\n", classname(s), symname(s));
    }
}

/*
 * Note the exit of the given block
 */

public printexit(s)
Symbol s;
{
    if (s != program and not istool()) {
	printf("leaving %s %s\n\n", classname(s), symname(s));
    }
}

/*
 * Note the call of s from t.
 */

public printtrcall(s, t)
Symbol s, t;
{
    printf("calling %s", symname(s));
    printparams(s, nil);
    printf(" from %s %s\n", classname(t), symname(t));
}

/*
 * Note the return from s.  If s is a function, print the value
 * it is returning.  This is somewhat painful, since the function
 * has actually just returned.
 */

public printrtn(s)
Symbol s;
{
    register Symbol t;
    register int len;
    Boolean isindirect;

    printf("returning ");
    if (s->class == FUNC and s->type != nil and
       (!istypename(s->type,"void")) and (!istypename(s->type, "(void)"))) {
	len = size(s->type);
	if (canpush(len)) {
	    t = rtype(s->type);
	    if (t->class == ARRAY or complex_type(t)) {
		rpush(ss_return_addr(), len);
		push_double(t, len);
	    } else {
	    	isindirect = (Boolean) (t->class == RECORD or t->class == UNION);
	    	pushretval(len, isindirect, t, return_regs() );
	    }
	    printval(s->type);
	    printf(" ");
	} else {
	    printf("(value too large) ");
	}
    }
    printf("from %s\n", symname(s));
}

/*
 * Test if a symbol should be printed.  We don't print files,
 * for example, simply because there's no good way to do it.
 * The symbol must be within the given function.
 */

public Boolean should_print(s)
Symbol s;
{
    Boolean b;
    register Symbol t;

    switch (s->class) {
	case VAR:
	case FVAR:
	case REF:
	case BASED:
	case CVALUE:
	    if (isparam(s)) {
		b = true;
	    } else {
		t = rtype(s->type);
		if (t == nil) {
		    b = false;
		} else {
		    switch (t->class) {
			case FILET:
			case BADUSE:
			    b = false;
			    break;

			default:
			    b = true;
			    break;
		    }
		}
	    }
	    break;

	default:
	    b = false;
	    break;
    }
    return b;
}

/*
 * Print the name and value of a variable.
 */

public printv(s, frame)
Symbol s;
Frame frame;
{
    register Symbol t;
    Address addr;
    double d;
    int len;

    if (isambiguous(s) and ismodule(container(s))) {
	printname(s);
	printf(" = ");
    } else {
	printf("%s = ", symname(s));
    }
    t = rtype(s->type);
    if (t->class == ARRAY && (!istypename(t->type, "char"))) {
	printf(" ARRAY ");
    } else if (t->class == CARRAY) {
	printf(" CONFORMANT ARRAY ");
    } else if (t->class == SET) {
	printf(" SET ");
    } else if (t->class == RECORD) {
	printf(" RECORD ");
    } else {
        if (isvarparam(s)) {
	    rpush(address(s, frame), sizeof(Address));
	    addr = pop(Address);
	    len = size(s->type);
        } else if (isreg(s)) {
#ifdef mc68000
	    if (is68881reg(s->symvalue.offset)) {
		findregvar_68881(s, nil, &d);
	        push(double, d);
	    } else if (isfpareg(s->symvalue.offset)) {
		findregvar_fpa(s, nil, &d);
	        push(double, d);
	    } else
#elif i386
#ifdef WEITEK_1167
	    if (isfpareg(s->symvalue.offset)) {
		findregvar_fpa(s, nil, &d);
		push(double, d);
	    } else
#endif WEITEK_1167
#elif sparc
        if(isfpureg(s)) {
            find_fpureg(s, nil, &d);
            push(double, d);
        } else 
#endif mc68000
	    {
		pushsmall(s, findregvar(s, frame));
	    }
	    printval(s->type);
	    return;
	} else {
	    addr = address(s, frame);
	    len = size(s);
	}
        if (canpush(len)) {
	    rpush(addr, len);
	    push_double(rtype(s->type), len);
	    printval(s->type);
        } else {
	    printf("*** expression too large ***");
        }
    }
}

/*
 * Print out the name of a symbol.
 */

public printname(s)
Symbol s;
{
    if (s == nil) {
	printf("(noname)");
    } else if (isambiguous(s)) {
	printwhich(s);
    } else {
	printf("%s", symname(s));
    }
}

/*
 * Print the fully specified variable that is described by the given identifer.
 */

public printwhich(s)
Symbol s;
{
    register Symbol block;

    block = container(s);
    skipInlineBlocks(block);
    printouter(block);
    printf("%s", symname(s));
}

/*
 * Print the fully qualified name of each symbol that has the same name
 * as the given symbol.    (Except in Modula2, where we have generated
 * a "FUNC" entry for each module, for its module-initialization code.)
 */

#define ModuleInit(s) (  streq( language_name((s)->language), "modula2" ) \
	and (s)->type == nil		  and	(s)->block != nil	   \
	and (s)->block->name == (s)->name and	(s)->block->class == MODUS )

public printwhereis(s)
Symbol s;
{
    register Name n;
    register Symbol t;

    checkref(s);
    n = s->name;
    t = lookup(n);

    if( ModuleInit( t ) ) {
	t = t->block;
    }
    printf("%s:\t", classname(t));
    printwhich(t);
    t = t->next_sym;
    while (t != nil) {
	if (t->name == n) {
	    if( !ModuleInit( t ) ) {
		printf("\n");
		printf("%s:\t", classname(t));
		printwhich(t);
	    }
	}
	t = t->next_sym;
    }
    printf("\n");
}

private printouter(s)
Symbol s;
{
    Symbol outer;

    if (s != nil) {
	outer = container(s);
	skipInlineBlocks(outer);
	if (outer == nil or outer == program) {
	    printf("`");
	} else {
	    printouter(outer);
	}
        if (s == program) return;
        if (s->class == RECORD)
            printf("%s.", symname(s));
        else
            printf("%s`", symname(s));
    }
}

/*
 * Straight dump of symbol information.
 */

public psym( s, verbose )
Symbol s;
Boolean verbose;
{
    if( s == nil ) {
	printf( "no symbol\n" );
	return;
    }
    if( !verbose ) {
	printf("%s\n", symname(s) );
	return;
    }
    printf("\nname\t%s\tat 0x%x\n", symname(s), s );
    printf("lang\t%s\n", language_name(s->language));
    printf("class\t%s\t\t", classname(s));
    printf("level\t%d\n", s->level);
    printf("type\t0x%x", s->type);
    if (s->type != nil and s->type->name != nil) {
	printf(" (%s)", symname(s->type));
    }
    printf("\nchain\t0x%x", s->chain);
    if (s->chain != nil and s->chain->name != nil) {
	printf(" (%s)", symname(s->chain));
    }
    printf("\nblock\t0x%x", s->block);
    if (s->block != nil and s->block->name != nil) {
	printf(" (");
	printname(s->block);
	printf(")");
    }
    printf("\nnext\t0x%x", s->next_sym);
    if (s->next_sym != nil and s->next_sym->name != nil) {
	printf(" (");
	printname(s->next_sym);
	printf(")");
    }
    printf("\n");
    printf("\n");
    switch (s->class) {
	case VAR:
	case REF:
	case READONLY:
	case CVALUE:
	    if ((char)s->level >= (char)3) {
		printf("address\t0x%x\n", s->symvalue.offset);
	    } else {
		printf("offset\t%d\n", s->symvalue.offset);
	    }
	    printf("size\t%d\n", size(s));
	    break;

	case RECORD:
	case UNION:
	    printf("offset\t%d\n", s->symvalue.offset);
	    break;

	case FIELD:
	    printf("offset\t%d\n", s->symvalue.field.offset);
	    printf("length\t%d\n", s->symvalue.field.length);
	    printf("size\t%d\n", size(s));
	    break;

	case PROG:
	case PROC:
	case FUNC:
	    printf("address\t0x%x\n", s->symvalue.funcv.beginaddr);
	    if (isinline(s)) {
		printf("inline procedure\n");
	    }
	    if (nosource(s)) {
		printf("does not have source information\n");
	    } else {
		printf("has source information\n");
	    }
	    break;

	case RANGE:
	    prangetype(s->symvalue.rangev.lowertype);
	    printf("lower\t%d\n", s->symvalue.rangev.lower);
	    prangetype(s->symvalue.rangev.uppertype);
	    printf("upper\t%d\n", s->symvalue.rangev.upper);
	    break;

	default:
	    /* do nothing */
	    break;
    }
}


/* Used only by dbxdebug command */
public  dbdb_followtype( sym, verbose )
Symbol sym;
Boolean verbose;
{
  /* This could be done right, but ... */
# define recurlimit 10
  Symbol t, prev_t = nil, pprev_t = nil;
  int level;

    if( sym == nil ) {
	printf( "no symbol\n" );
	return;
    }
    if( (t= sym->type) == nil ) {
	printf( "symbol has no type\n" );
	return;
    }

    for( level = 0;  t != nil  and  level < recurlimit; ++level ) {
	if( t == prev_t  or  t == pprev_t ) {
	    printf( "\tRecursion loop in dbdb_followtype.\n" );
	    break;
	}

	/*
	 * Even if we're not being verbose, we still want to
	 * see the class for any type we're looking at.
	 */

	if( !verbose )
	    printf("(class %s):\t", classname(sym));
	psym( t, verbose );

	pprev_t = prev_t ;
	prev_t = t;
	t = t->type ;
    }
}

private prangetype(r)
Rangetype r;
{
    switch (r) {
	case R_CONST:
	    printf("CONST");
	    break;

	case R_ARG:
	    printf("ARG");
	    break;

	case R_TEMP:
	    printf("TEMP");
	    break;

	case R_ADJUST:
	    printf("ADJUST");
	    break;
    }
}

/*
 * Print out the contents of an array.
 * Haven't quite figured out what the best format is.
 *
 * This is rather inefficient.
 *
 * The "2*elsize" is there since "printval" drops the stack by elsize.
 */

public printarray(a)
Symbol a;
{
    Stack *savesp, *newsp;
    Symbol eltype;
    long elsize;
    String sep;

    savesp = sp;
    sp -= (size(a));
    newsp = sp;
    eltype = rtype(a->type);
    elsize = size(eltype);
    printf("(");
    if (eltype->class == RECORD or eltype->class == ARRAY or
      eltype->class == UNION or eltype->class == CARRAY) {
	sep = "\n";
	printf("\n");
    } else {
	sep = ", ";
    }
    for (sp += elsize; sp <= savesp; sp += 2*elsize) {
	if (sp - elsize != newsp) {
	    printf(sep);
	}
	p_str_arr_val(eltype, elsize);
    }
    sp = newsp;
    if (streq(sep, "\n")) {
	printf("\n");
    }
    printf(")");
}

/*
 * Print out the value of a real number in Pascal notation.
 * This is, unfortunately, different than what one gets
 * from "%g" in printf.
 */

public prtreal(r)
double r;
{
    extern char *index();
    char buf[256];

    sprintf(buf, "%.18g", r);
    if (buf[0] == '.') {
	printf("0%s", buf);
    } else if (buf[0] == '-' and buf[1] == '.') {
	printf("-0%s", &buf[1]);
    } else {
	printf("%s", buf);
    }
    if (index(buf, '.') == nil and 
      not streq(buf, "Infinity") and not streq(buf, "NaN")) {
	printf(".0");
    }
}

/*
 * Print out a character using ^? notation for unprintables > 0.
 */

public Boolean printchar(c)
char c;
{
    Boolean printable;

    printable = true;
    if (c == 0) {
	printf("\\");
	printf("0");
    } else if (c == 0177) {
	printf("^");
	printf("?");
    } else if (c == '\n') {
	printf("\\");
	printf("n");
    } else if (c > 0 and c < ' ') {
	printf("^");
	printf("%c", c - 1 + 'A');
    } else {
	if (c < 0) {
	    printable = false;
	}
	printf("%c", c);
    }
    return printable;
}

/*
 * Print an array or structure element that has been pushed on the stack
 * (along with the rest of the array or structure).  printval() can't
 * be called directly because a real or complex type is pushed as 
 * a 4/8 byte quantity, respectively, rather than a 8/16 byte quantity.
 * (This is because the whole array/structure was pushed on the stack and
 * there was no float -> double or complex -> double complex conversion done.)
 *
 * If a bit field in a structure is to be printed, make sure the value
 * on the stack doesn't change since the next field may be part of this
 * one.
 *
 * Actually, it might be more clear to have two routines for these,
 * the struct printers know they just want the p_str_bitfield,
 * and printarray knows it wants the double conversion...
 */

public p_str_arr_val(eltype, len)
register Symbol eltype;
Integer len;
{
    register Symbol s;
    float fl;

    s = rtype(eltype);
    if (four_byte_float(s, len)) {
    	prtreal(pop(float));
    } else if (sing_prec_complex(s, len)) {
	printf("(");
	fl = pop(float);
	prtreal(pop(float));
	printf(",");
	prtreal(fl);
	printf(")");
    } else if (s->class == SET &&
	       eltype->class == FIELD &&
	       s->language == findlanguage(".p") && 
	       (eltype->symvalue.field.length < 32 ||
		eltype->symvalue.field.offset % 16 != 0)) {
	p_str_bitfield(eltype); /* generic subfield printer */
	/* pascal_print_packed_set(eltype); */
    } else if (isbitfield(eltype)) {
	/*	ivan Wed Jan 11 1989
	 *	we simplify the check to accomodate pascal packed
	 *	enums, booleans, etc. 'p_str_bitfield()' has been modified to call
	 * 	the generic 'printval()' instead of the C specific.
	 *	this isn't as scary as it seems ... all 'isbitfield' does to test
	 *	whether somethings on an odd (as in couple) boundry; then 
	 *	'p_str_bitfield' extracts the appropriate field and and calls 
	 *	printval which does the actuall printing anyway.
	 */
	p_str_bitfield(eltype);
    } else {
	printval(eltype);
    }
}


/*
 * Print out a bit field which is part of a whole structure being
 * printed.  The structure sitting on the stack shouldn't change.
 */

#undef SHIFT_OFF
#ifdef i386
#define SHIFT_OFF (off)
#else
#define SHIFT_OFF ((- (off + len)) & (BITSPERBYTE - 1)) /* ((-(off + len)) mod BITSPERBYTE) */
#endif
/* see also: `eval.c`push_aligned_bitfield */
#define printbitfield(type) \
    orig = pop(type);\
    origsp = sp;\
    i = orig >> k;\
    i &= ((1 << len) - 1);\
    push(type, i);\
    printval(s);\
    savesp = sp;\
    sp = origsp;\
    push(type, orig);\
    sp = savesp;
/*
 * probably: savesp == origsp, Oh well,
 * the callers of p_str_arr_val always reset sp anyway... 
 */

/*
 * this function temporarily aligns the structure subfield (s)
 * can dispatches to printval. then it resets the stack with
 * the original (unshifted) values.
 *
 * when called, the sp should point to the char/short/long after the field,
 * note that for 3-byte fields, this is not obvious...
 * stack:  xxxx byt1 xxxx   xxxx byt2 byt1 xxxx  xxxx byt4 byt3 byt2 xxxx xxxx
 * sp:                ^                     ^                              ^
 *
 * on return, sp is where printval left it, relative to the aligned data.
 */
public p_str_bitfield(s)
Symbol s;
{
    register int i, j, k, orig, len, off;
    Stack *origsp, *savesp;
    
    len = s->symvalue.field.length;
    off = (s->symvalue.field.offset % BITSPERBYTE);
    /* j = (off div BITSPERBYTE) + (size s); */
    j = (off + len + BITSPERBYTE - 1) div BITSPERBYTE;
    k = SHIFT_OFF;
    if (j <= sizeof(  char)){
	printbitfield(char);
    } else if (j <= sizeof(short)){
	printbitfield(short);
    } else {
#ifndef i386
	if (j < sizeof(long)) k += 8;
#endif
	printbitfield(long);
    }
}

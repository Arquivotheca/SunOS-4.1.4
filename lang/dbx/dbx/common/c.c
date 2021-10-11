#ifndef lint
static  char sccsid[] = "@(#)c.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

/*
 * C-dependent symbol routines.
 */

#include "defs.h"
#include "symbols.h"
#include "printsym.h"
#include "languages.h"
#include "c.h"
#include "tree.h"
#include "eval.h"
#include "operators.h"
#include "mappings.h"
#include "process.h"
#include "runtime.h"
#include "machine.h"
#include "types.h"
#include "coredump.h"

#define STRINGLEN 512

private	Integer stringlen = STRINGLEN;		/* Max num of chars to print */

private c_printdecl();
private c_printval();
private Boolean c_typematch();
private Node c_buildaref();
private int c_evalaref();

#define isrange(t, name) (t->class == RANGE and istypename(t->type, name))


/*
 * Initialize C language information.
 */

public c_init()
{
    Language lang;

    lang = language_define("c");
    language_suffix_add (lang, ".c");
    language_setop(lang, L_PRINTDECL, c_printdecl);
    language_setop(lang, L_PRINTVAL, c_printval);
    language_setop(lang, L_PRINTPARAMS, c_printparams);
    language_setop(lang, L_EVALPARAMS, c_evalparams);
    language_setop(lang, L_TYPEMATCH, c_typematch);
    language_setop(lang, L_BUILDAREF, c_buildaref);
    language_setop(lang, L_EVALAREF, c_evalaref);
}

/*
 * Test if two types are compatible.
 */

private Boolean c_typematch(type1, type2)
Symbol type1, type2;
{
    Boolean t2_is_int_char_range ; /* t2 is int or char or range thereof */
    register Symbol t1, t2, tmp;

    t1 = type1;
    t2 = type2;
    if (t1 == t2) {
	return true;
    }

    t1 = rtype(t1);
    t2 = rtype(t2);
    if (t1->type == t_char or t1->type == t_int or t1->type == t_real) {
	tmp = t1;
	t1 = t2;
	t2 = tmp;
    }

    t2_is_int_char_range = (Boolean) (
				t2->type == t_int   or  isrange(t2, "int")
			     or t2->type == t_char  or  isrange(t2, "char") );

    /* picked apart */
    if( t2_is_int_char_range  and
	  ( isrange(t1, "int")  or  isrange(t1, "char" ) ) )
	return true;

    if( t1->class == RANGE and isfloat(t1) and t2->type == t_real )
	return true;
    if( t1->class == PTR and t2->type == t_int )
	return true;
    if( t1->type == t2->type and
	  ( (t1->class == t2->class) or
	    (t1->class == SCAL  and t2->class == CONST) or
	    (t1->class == CONST and t2->class == SCAL ) ) )
	return true;

    return false;
}

/*
 * Decide if a field is a bit field.
 */

public Boolean isbitfield(s)
register Symbol s;
{
    Boolean b;
    register Integer off, len;
    register Symbol t;
    int szt;

	/* 
	 *	ivan Mon Jan 30 1989 
	 * 	Motivated by the fact that am calling *this* routine from 
	 *	'p_str_arr_val()' instead of 'cbitfield()'.
	 */
	if (s->class != FIELD)
		return false;
		
    /* basically, isbitfield should return true if the field needs 
     * bit extraction and/or alignment: if (size(s) != len div 8)
     * being unaligned or partial byte, *always* makes size > len.
     */
    off = s->symvalue.field.offset;
    len = s->symvalue.field.length;

    /* PEB fix 1015283, used to return false for 24 bit fields */
    if ((off mod BITSPERBYTE) != 0 or (len mod BITSPERBYTE) != 0) {
	b = true;
    } else {
	t = rtype(s->type);
	
	if (t == nil) {			/* ivan Mon Jan 30 1989 */
	    b = false;
	} else if( t->class == ARRAY and istypename(t->type, "char")) {
	    b = false; /* VARYING will be aligned for integer count */
	} else if( t->class == SCAL  and  len != (sizeof(int)*BITSPERBYTE) ) {
	    b = true;
	} else {
	    szt = size(t);		/* size(t) may have been rounded up 1 */
	    if( len != szt*BITSPERBYTE /* && len != (szt-1)*BITSPERBYTE */ )
		b = true;
	    else
		b = false;
	}
    }
    return b;
}

/*
 * Decide if a field is a bit field in the C language (obsolete)
 */

public Boolean cbitfield(s)
register Symbol s;
{
    Boolean b;

    if ((s->language == nil) or (s->language != findlanguage(".c"))) {
	b = false;
    } else {
	b = isbitfield(s);
    }
    return b;
}

/*
 * Print out the declaration of a C variable.
 */

private c_printdecl(s)
register Symbol s;
{
    Symbol mem;
    Boolean semicolon, newline;

    semicolon = true;
    newline = true;
    switch (s->class) {
	case TYPE:
	    /*
	     * Typedefs.  For a typedef to a struct/union/enum print
	     * the whole thing.  For one of the basic types, just print
	     * its name.  Otherwise, print the typedef declaration.
	     */
	    if (s->type->class == TYPEID) {
		printf("typedef ");
	        pr_strunem(s->type, s->type->type);
	        printf("{\n");
	        printdecl(s->type->type);
	        printf("} %s", symname(s->type));
	    } else if (s->type->class == RANGE) {
	        printf("%s", symname(s));
	    } else {
		printf("typedef ");
	        pre_type(s->type);
	        printf("%s", symname(s));
	        post_type(s->type);
	    }
	    break;

	case CONST:
	    if (s->type->class == SCAL) {
		printf("member of enum, ord %ld", s->symvalue.iconval);
	    } else {
		printf("const %s = ", symname(s));
		c_printval(s);
	    }
	    break;

	case VAR:
	    pr_storage(s);
	    pre_type(s->type);
	    printf("%s", symname(s));
	    post_type(s->type);
	    break;

	case FIELD:
	    if (s->type->class == RECORD
	      or s->type->class == UNION
	      or s->type->class == SCAL) {
		printf("%s {\n", c_classname(s->type));
		printdecl(s->type);
		indent(indentlevel);
		printf("} %s", symname(s));
	    } else {
	        pre_type(s->type);
	        printf("%s", symname(s));
	        post_type(s->type);
	        if (isbitfield(s)) {
	            printf(" : %d", s->symvalue.field.length);
	        }
	    }
	    break;

	case TYPEID:
	    pr_strunem(s, s->type);
	    printf("{\n");
	    printdecl(s->type);
	    indent(indentlevel);
	    printf("}");
	    break;

	case RECORD:
	case UNION:
	    semicolon = false;
	    newline = false;
	    indentlevel++;
	    for (mem = s->chain; mem != nil; mem = mem->chain) {
		indent(indentlevel);
		printdecl(mem);
	    }
	    indentlevel--;
	    break;

	case SCAL:
	    semicolon = false;
	    newline = false;
	    indentlevel++;
	    for (mem = s->chain; mem != nil; mem = mem->chain) {
		indent(indentlevel);
		printf("%s%s\n", symname(mem), (mem->chain != nil)? ",": "");
	    }
	    indentlevel--;
	    break;

	case PROC:
	    semicolon = false;
	    printf("%s", symname(s));
	    c_listparams(s);
	    printf("\n");
	    c_paramdecls(s);
	    newline = false;
	    break;

	case FUNC:
	    semicolon = false;
	    pre_type(s->type);
	    printf("%s", symname(s));
	    c_listparams(s);
	    post_type(s->type);
	    printf("\n");
	    c_paramdecls(s);
	    newline = false;
	    break;

	case MODULE:
	    semicolon = false;
	    printf("source file \"%s.c\"", symname(s));
	    break;

	case PROG:
	    semicolon = false;
	    printf("executable file \"%s\"", symname(s));
	    break;

	case PTR:
	    pre_type(s);
	    post_type(s);
	    break;

	default:
	    error("class %s in c_printdecl", classname(s));
    }
    if (semicolon) {
	printf(";");
    }
    if (newline) {
	printf("\n");
    }
}

/*
 * Print out a structure, union or enum keyword and
 * optional tag.
 */
public pr_strunem(sym, type)
Symbol sym;
Symbol type;
{
    printf("%s ", c_classname(type));
    if (sym->name != nil) {
	printf("%s ", symname(sym));
    }
}

/*
 * List the parameters of a procedure or function.
 * No attempt is made to combine like types.
 */

public c_listparams(s)
Symbol s;
{
    register Symbol t;

    printf("(");
    for (t = s->chain; t != nil; t = t->chain) {
	printf("%s", symname(t));
	if (t->chain != nil) {
	    printf(", ");
	}
    }
    printf(")");
}

/*
 * Print the parameter declarations for a function/procedure.
 */
private c_paramdecls(s)
Symbol s;
{
    register Symbol t, u;

    if (s->chain != nil) {
	for (t = s->chain; t != nil; t = t->chain) {
	    if (t->class != VAR) {
		panic("unexpected class %d for parameter", t->class);
	    }
	    u = getregparam(t);
	    if (u != nil) {
	    	printdecl(u);
	    } else {
	    	printdecl(t);
	    }
	}
    } else {
	printf("\n");
    }
}

/*
 * Print the values of the parameters of the given procedure or function.
 * The frame distinguishes recursive instances of a procedure.  If
 * printregparam is true, print the register value of the parameter
 * (if it exists); otherwise, print the value on the stack (which is
 * the value the routine was called with).
 */

public c_printparams(f, frame)
Symbol f;
Frame frame;
{
    Symbol param, regparam;
    int n, m, s;

    n = nargspassed(frame);
    param = nil;
    if (f != nil) {
        param = f->chain;
    }
    printf("(");
    if (param != nil or n > 0) {
	m = n;
	if (param != nil) {
	    for (;;) {
		s = size(param) div sizeof(Word);
		if (s == 0) {
		    s = 1;
		}
		m -= s;
		/*
		 * If we have just entered the routine and have not
		 * executed the prologue, any register params will
		 * not have been loaded from the stack yet.  Get params
		 * from the stack in this case.  Else, get from registers.
		 */
		if (f != nil and pc == codeloc(f)) {
		    printv(param, frame);
		} else {
		    regparam = getregparam(param);
		    if (regparam != nil ) {
		    	printv(regparam, frame);
		    } else {
		    	printv(param, frame);
		    }
		}
		param = param->chain;
	        if (param == nil) break;
		printf(", ");
	    }
	}

	/*
	 * The code below is intended to print out any "extra"
	 * arguments (i.e., ones that were supplied by the caller
	 * but not required or declared by the callee).
	 *
	 * This code is skipped for Pascal & Modula2 because they allow
	 * parameters to start on a short word boundary and so the way
	 * `m' is calculated above is wrong.  Anyway, extra parameters
	 * are not allowed in either language.
	 *
	 * On a sparc, we MUST rely on the symbols if they're there:
	 * "nargs" is impossible to discern from the code.  In that case,
	 * we'll enter the code below only where we have no symbols (`m'
	 * will be 6 and f==nil).
	 */
	if ((m > 0) and
	    (f == nil
#ifndef sparc
			or (f->language != findlanguage(".p") and
			  f->language != findlanguage(".mod"))
#endif sparc
	             )) {
	    if (m > MAXARGSPASSED) {
		m = MAXARGSPASSED;
	    }
	    if (f != nil and f->chain != nil) {
		printf(", ");
	    }
	    for (;;) {
		--m;
		printf("0x%x", argn(n - m, frame));
	    if (m <= 0) break;
		printf(", ");
	    }
	}
    }
    printf(")");
}

/*
 * Print out the value on the top of the expression stack
 * in the format for the type of the given symbol.
 */

private c_printval(s)
Symbol s;
{
    register Symbol t;
    register Address a;
    register int i, len, off;
    String str;

    switch (s->class) {
	case CONST:
	case TYPE:
	case VAR:
	case REF:
	case FVAR:
	case TYPEID:
/* D. Teeple  mar 22 1988 */
/* Don't try to print cyclic structures whose type we don't know 
 * bug 1007968
 */
	    if (s->class == s->type->class and istypename(s->type, "void")) {
	    		printf("type unkown\n"); 
	    	} else {
	  		c_printval(s->type);
	  	}
	    break;
/* D. Teeple  mar 2 1988
 *    a structure with a bit alignment constant field is also a field
 *    fixes bug 1008157, the compiler calls the type of such fields
 *    as the declared type; size checks only this size & doesn't
 *    discover that there is a mis-match
 */
	
	case FIELD:
	    if (isbitfield(s) ||
	       ((size(rtype(s->type))*BITSPERBYTE)
	           > s->symvalue.field.length) ) {
		len = s->symvalue.field.length;
		off = ((s->symvalue.field.offset % BITSPERBYTE)
		       + len + BITSPERBYTE - 1) div BITSPERBYTE;
		if (off <= sizeof(char)) {
		    i = pop(unsigned char);
		} else if (off <= sizeof(short)) {
		    i = pop(unsigned short);
		} else {
		    i = pop(unsigned int);
		}
		t = rtype(s->type);
		if (t->class == SCAL) {
		    printenum(i, t);
		} else {
		    printrange(i, t);
		}
	    } else {
		c_printval(s->type);
	    }
	    break;

	case ARRAY:
	    t = rtype(s->type);
	    if (t->class == RANGE and istypename(t->type, "char")) {
		len = size(s);
		sp -= len;
		printf("\"%.*s\"", len, sp);
	    } else {
		printarray(s);
	    }
	    break;

	case RECORD:
	    c_printstruct(s);
	    break;

	case RANGE: {
	    double workaround;
		if (istypename(s->type, "boolean")) {
		    printrange(popsmall(s), s);
		} else if (istypename(s->type, "char")) {
		    printrange(pop(char), s);
		} else if (isfloat(s)) {
		    /* Was just:   prtreal(pop(double)); */
		    workaround = pop(double);
		    prtreal( workaround );
		    break;
		} else {
		    printrange(popsmall(s), s);
		}
	    }
	    break;

	case PTR:
	    t = rtype(s->type);
	    a = pop(Address);
	    if (a == 0) {
		printf("(nil)");
	    } else if (t->class == RANGE and
		       istypename(t->type, "char")) {
		printf("0x%x ", a);
		printstring(a);
	    } else if (t->class == FUNC or t->class == PROC) {
		if ((str = srcprocname(a)) != nil) {
		    printf("&%s() at ", str);
		}
		printf("0x%x", a);
	    } else {
		printf("0x%x", a);
	    }
	    break;

	case SCAL:
	    i = pop(Integer);
	    printenum(i, s);
	    break;

	case FORWARD:
	    c_printval(lookup_forward(s));
	    break;

	default:
	    if (ord(s->class) > ord(TYPEREF)) {
		panic("printval: bad class %d", ord(s->class));
	    }
	    sp -= size(s);
	    printf("<%s>", c_classname(s));
	    break;
    }
}

/*
 * Print out the value of a C structure.
 */

private c_printstruct(s)
Symbol s;
{
    register Symbol f;
    register Stack *savesp;
    register Integer n, off, len;
    String   name;
    Integer  namelen;
    Integer  maxlen;
    Integer  i;

    /*
     * Find the length of the longest field name
     */
    maxlen = 0;
    for (f = s->chain; f != nil; f = f->chain) {
	name = symname(f);
	namelen = strlen(name);
	if (namelen > maxlen) {
	    maxlen = namelen;
	}
    }

    /*
     * Print out the fields' values
     */
    sp -= size(s);
    savesp = sp;
    indentlevel++;
    printf("{\n");
    for (f = s->chain; f != nil; f = f->chain) {
	indent(indentlevel);
	name = symname(f);
	namelen = strlen(name);
	printf("%s", name);
	for (i = namelen; i < maxlen; i++) {
	    printf(" ");
	}
	printf(" = ");
	/* size includes offset % 8, here we add offset div 8 */
	n = size(f);
	sp += (n + (f->symvalue.field.offset div BITSPERBYTE));
	p_str_arr_val(f, n);
	sp = savesp;
	printf("\n");
    }
    indentlevel--;
    indent(indentlevel);
    printf("}");
}
/*
 * Print out a range type (integer, char, or boolean).
 */

private printrange(i, t)
Integer i;
register Symbol t;
{
    if (istypename(t->type, "boolean")) {
	printf(((Boolean) i) == true ? "true" : "false");
    } else if (istypename(t->type, "char")) {
	printf("'");
	printchar(i);
	printf("'");
    } else if (t->symvalue.rangev.lower >= 0) {
	switch (size(t)) {
	    case sizeof(char):
		printf("%lu", i & 0x0ff);
		break;
	    case sizeof(short):
		printf("%lu", i & 0x0ffff);
		break;
	    default:
		printf("%lu", i);
	}
    } else {
	printf("%ld", i);
    }
}

/*
 * Print out a null-terminated string (pointer to char)
 * starting at the given address.
 */
private printstring(addr)
Address addr;
{
    if (stringlen == 0) {
	return;
    }
    printf("\"");
    pr_string(addr);
    printf("\"");
}

/*
 * Do the dirty work of printing out a string.
 */
public pr_string(addr)
Address	addr;
{
    register Address a;
    register Integer i;
    register Boolean endofstring;
    Integer nbytes;
    union {
	char ch[sizeof(Word)];
	int word;
    } u;

    a = addr;
    if (not in_address_space(addr)) {
	return;
    }
    endofstring = false;
    nbytes = 0;
    while (not endofstring and nbytes < stringlen) {
	dread(&u, a, sizeof(u));
	if (read_err_flag) {
	    printf("<bad address>");
	    return;
	}
	i = 0;
	do {
	    if (u.ch[i] == '\0') {
		endofstring = true;
	    } else {
		if (printchar(u.ch[i]) == false) {
		    printf("<unprintable...>");
		    return;
		}
		nbytes++;
	    }
	    ++i;
	} while (i < sizeof(Word) and not endofstring and nbytes < stringlen);
	a += sizeof(Word);
    }
    if (not endofstring) {
	printf("<...>");
    }
}

/*
 * Print out an enumerated value by finding the corresponding
 * name in the enumeration list.
 */

private printenum(i, t)
Integer i;
Symbol t;
{
    register Symbol e;

    e = t->chain;
    while (e != nil and e->symvalue.iconval != i) {
	e = e->chain;
    }
    if (e != nil) {
	printf("%s", symname(e));
    } else {
	printf("%d", i);
    }
}

/*
 * Return the C name for the particular class of a symbol.
 */

public String c_classname(s)
Symbol s;
{
    String str;

    str = c_tagtype(s->class);
    if (str == nil) {
	str = classname(s);
    }
    return str;
}

/*
 * Look for C classes that have tags
 */
public String c_tagtype(class)
Symclass class;
{
    String str;

    switch (class) {
	case RECORD:
	    str = "struct";
	    break;

	case UNION:
	    str = "union";
	    break;

	case SCAL:
	    str = "enum";
	    break;

	default:
	    str = nil;
	    break;
    }
    return(str);
}

private Node c_buildaref(a, slist)
Node a, slist;
{
    register Symbol t;
    register Node p;
    Symbol etype, atype, eltype;
    Node esub, r;

    r = a;
    t = rtype(a->nodetype);
    eltype = t->type;
    if (t->class == PTR) {
	p = slist->value.arg[0];
	if (not compatible(p->nodetype, t_int)) {
	    beginerrmsg();
	    printf("bad type for subscript of ");
	    prtree(a);
	    enderrmsg();
	}
	if (a->op == O_SYM)
	    a = build(O_RVAL, a);
	r = build(O_INDEX, a, p );
	r->nodetype = eltype;
    } else if (t->class != ARRAY) {
	beginerrmsg();
	prtree(a);
	printf(" is not an array");
	enderrmsg();
    } else {
	p = slist;
	t = t->chain;
	for (; p != nil and t != nil; p = p->value.arg[1], t = t->chain) {
	    esub = p->value.arg[0];
	    etype = rtype(esub->nodetype); /* type of subscript expression  */
	    atype = rtype(t);		   /* subscript type the array wants */
	    if (not compatible(atype, etype)) {
		beginerrmsg();
		printf("subscript ");
		prtree(esub);
		printf(" is the wrong type");
		enderrmsg();
	    }
	    r = build(O_INDEX, r, esub);
	    r->nodetype = eltype;
	}
	if (p != nil or t != nil) {
	    beginerrmsg();
	    if (p != nil) {
		printf("too many subscripts for ");
	    } else {
		printf("not enough subscripts for ");
	    }
	    prtree(a);
	    enderrmsg();
	}
    }
    return r;
}

/*
 * Evaluate a subscript index.
 */

private int c_evalaref(s, i)
Symbol s;
long i;
{
    long lb, ub;

    s = rtype(s);
    if (s->class == PTR)
	return i;
    s = s->chain;
    lb = s->symvalue.rangev.lower;
    ub = s->symvalue.rangev.upper;
    if (i < lb or i > ub) {
	if (ub == -1 and lb == 0) {
	    printf("warning: array bounds unknown (not specified in source)\n");
	} else {
	    warning("subscript out of range");
	}
    }
    return(i - lb);
}

/*
 * Set the length of strings to print
 */
public set_stringlen(len)
Integer len;
{
    stringlen = len;
}

/*
 * Return the length of strings to print
 */
public get_stringlen()
{
    return(stringlen);
}

/*
 * This structure is used by c_evalparams to keep track of string
 * constants (and structures, on sparcs) that are pushed onto the
 * stack when dbx has to call a routine in the subprocess.
 */
#define NSTR_ARGS 10

struct str_arg {
	Node	node;			/* O_SCON node */
	Address	pushpoint;		/* Address on stack where the string */
					/* will be pushed */
};


/*
 * Return (& reset) the length (on the stack) of the string-constants
 * that c_evalparams has stacked.  (On sparc, this also includes structures
 * passed "by value" -- the sparc C compiler copies the struct into a
 * temporary on the stack, and then passes that copy by reference anyway.)
 */
private int stacked_strs = 0;

public stk_str_len()
{
    int ss = stacked_strs;
    stacked_strs = 0;
    return(ss);
}

/*
 * Evaluate arguments left-to-right.
 * Look for special cases:
 *
 *	Literal strings - must push the string after all the other
 *			  args, and pass a pointer to the string as
 *			  the parameter.
 *
 *	Arrays - pass the address of the first element.
 *
 * 	Functions/Procedures - pass the address of the routine.
 *
 *	Structures passed by value - On sparc machines, these must
 *	be treated like strings, so that we don't ever attempt to put
 *	them into argument registers.
 */

#define str_err "Too many structure or literal string arguments - max is %d"
#define dalignstack() { \
    sp = (Stack *) (( ((int) sp) + sizeof(double)-1)&~(sizeof(double)-1)); \
}

public c_evalparams(proc, arglist)
Symbol proc;
Node arglist;
{
    Node p;
    Node exp;
    Node slist;
    Symbol arg, rt;
    Address addr;
    Stack *savesp;
    Boolean typechecking;
    int ntypesize, rtypesize; /* node type size & rounded (aligned) version */
    int argsize, i;
    int nstrs;
    Address stackpointer;
    struct str_arg str_s[NSTR_ARGS];
    Boolean toomany;

    toomany = false;
    savesp = sp;
    nstrs = 0;
    stacked_strs = 0;
    stackpointer = reg(REG_SP);
    arg = proc->chain;
    if (nosource(proc)) {
	toomany = true;
    }
    for (p = arglist; p != nil; p = p->value.arg[1]) {
	if (arg == nil) {
	    if (not toomany) {
	        warning("Too many parameters to `%s'", symname(proc));
	        toomany = true;
	    }
	}
	exp = p->value.arg[0];
	if (exp->nodetype->class == FUNC or exp->nodetype->class == PROC) {
	    /*
	     * For a function name, take the address of the routine.
	     */
	    exp = build(O_AMPER, exp);
	    p->value.arg[0] = exp;
	} else if (exp->nodetype->type != nil and
	  exp->nodetype->type->class == ARRAY) {
	    /*
	     * For an array name, take the address of the zero-th element.
	     */
	    slist = build(O_COMMA, build(O_LCON, 0), nil);
	    exp = c_buildaref(exp->value.arg[0], slist);
	    exp = build(O_AMPER, exp);
	    p->value.arg[0] = exp;
	}
	ntypesize = size(exp->nodetype);
	rt= rtype( exp->nodetype );

	if (exp->op == O_SCON) {
	    if (nstrs >= NSTR_ARGS) {
		sp = savesp;
		error(str_err, NSTR_ARGS);
	    }
	    /*
	     * For simplicity's sake, we re-align the stack for each
	     * string parameter.  Not that expensive.
	     */
#ifdef sparc
	    rtypesize = round(ntypesize, sizeof(double));
#else
	    rtypesize = round(ntypesize, sizeof(int));
#endif sparc
	    str_s[nstrs].node = exp;
	    str_s[nstrs].pushpoint = stackpointer - rtypesize;
	    nstrs++;
	    stackpointer -= rtypesize;
	    ntypesize = sizeof(char *);
	}
#ifdef sparc
	else if( rt != nil  &&  rt->class == RECORD ) {
	    if (nstrs >= NSTR_ARGS) {
		sp = savesp;
		error(str_err, NSTR_ARGS );
	    }
	    /*
	     * Structures are already rounded to size int on sparc,
	     * but they need to be aligned to double-word alignment.
	     */
	    rtypesize = round(ntypesize, sizeof(double));
	    str_s[nstrs].node = exp;
	    str_s[nstrs].pushpoint = stackpointer - rtypesize;
	    nstrs++;
	    stackpointer -= rtypesize;
	    ntypesize = sizeof(char *);
	}
#endif
	if (arg != nil) {
	    argsize = size(arg->type);
	    arg = arg->chain;
	}
    }
    if (arg != nil) {
	warning("Not enough parameters to `%s'", symname(proc));
    }
    nstrs = 0;
    arg = proc->chain;
    for (p = arglist; p != nil; p = p->value.arg[1]) {
	exp = p->value.arg[0];
	ntypesize = size(exp->nodetype);
	rt = rtype( exp->nodetype );

	if (exp->op == O_SCON) {
	    push(long, str_s[nstrs].pushpoint);
	    nstrs++;
	}
#ifdef sparc
	else if( rt != nil  &&  rt->class == RECORD ) {
	    push(long, str_s[nstrs].pushpoint);
	    nstrs++;
	}
#endif sparc
	else {
	    eval(exp);
	    if (ntypesize < sizeof(long)) {
		i = popsmall(exp->nodetype);
	        push(long, i);
	    }
	}
	if (arg != nil) {
	    arg = arg->chain;
	}
    }

    /*
     * Finally, push the strings (&structures, on sparc) themselves onto
     * dbx's stack.  Realign to sizeof(int) between each pair.
     */
    {
	Stack * wasp = sp;

	/*
	 * Evaluate the strings onto dbx's stack.  On m68k,
	 * eval will round sp only to an even address each time.
	 */
#ifdef sparc
	    dalignstack();	/* align sp to double */
#else
	    alignstack();	/* align sp to int */
#endif sparc
	for (i = nstrs - 1; i >= 0; i--) {
	    eval(str_s[i].node);
#ifdef sparc
	    dalignstack();	/* align sp to double */
#else
	    alignstack();	/* align sp to int */
#endif sparc
	}

	stacked_strs = sp - wasp;
    }
}

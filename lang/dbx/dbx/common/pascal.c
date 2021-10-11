#ifndef lint
static	char sccsid[] = "@(#)pascal.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Pascal-dependent symbol routines.
 */

#include "defs.h"
#include "symbols.h"
#include "pascal.h"
#include "languages.h"
#include "tree.h"
#include "eval.h"
#include "mappings.h"
#include "process.h"
#include "runtime.h"
#include "machine.h"
#include "dbxmain.h"

private Boolean pascal_typematch();
private pascal_printdecl();
private pascal_printval();
private Node pascal_buildaref();
private int pascal_evalaref();

#ifndef public
#endif

extern c_printparams();

#define isrange(t, name) (t->class == RANGE and istypename(t->type, name))
#define isarray(t, name) ((t->class == ARRAY or t->class == CARRAY) and istypename(t->type, name))

/*
 * Initialize Pascal information.
 */

public pascal_init()
{
    Language lang;

    lang = language_define("pascal");
    language_suffix_add (lang, ".p");
    language_suffix_add (lang, ".pas");
    language_setop(lang, L_PRINTDECL, pascal_printdecl);
    language_setop(lang, L_PRINTVAL, pascal_printval);
    language_setop(lang, L_PRINTPARAMS, c_printparams);
    language_setop(lang, L_EVALPARAMS, pascal_evalparams);
    language_setop(lang, L_TYPEMATCH, pascal_typematch);
    language_setop(lang, L_BUILDAREF, pascal_buildaref);
    language_setop(lang, L_EVALAREF, pascal_evalaref);
}

public int pascal_set_nelem(t)
/*============================================================================*/
Symbol t;
	{
	Symbol s;
	int nel;

	s = rtype(t->type);
	if (s->class == SCAL) 
		{
		nel = 0;
		for (s = s->chain; s != nil; s = s->chain)
			nel++;
		}
	else if (s->class == RANGE)
		nel = s->symvalue.rangev.upper - s->symvalue.rangev.lower + 1;
	else
		error("internal error: can't determine set size");
	return nel;
	}

/*
 * Compatible tests if two types are compatible.  The issue
 * is complicated a bit by ranges.
 *
 * Integers and reals are not compatible since they cannot always be mixed.
 *
 * shouldn't ARRAY of char and VARYING be compatible??
 */

private Boolean pascal_typematch(type1, type2)
Symbol type1, type2;
{
    Boolean b;
    register Symbol t1, t2, tmp;

    t1 = type1;
    t2 = type2;
    if (t1 == t2) {
	b = true;
    } else {
	if (t1->class == READONLY or t1->class == CRANGE) {
	    t1 = t1->type;
	}
	if (t2->class == READONLY or t2->class == CRANGE) {
	    t2 = t2->type;
	}
	t1 = rtype(t1);
	t2 = rtype(t2);
	if (t1->type == t_char or t1->type == t_int or t1->type == t_real) {
	    tmp = t1;
	    t1 = t2;
	    t2 = tmp;
	}
	/*
	 * The evaluation of b, below, is broken up because the compiler
	 * can't handle such a complicated expression.
	 */
	b = (Boolean) (
	    (
		isrange(t1, "integer") and
		  (isrange(t2, "integer") or t2->type == t_int or
		   t2->type == t_char)
	    ) or (
		isrange(t1, "char") and
		  (isrange(t2, "char") or t2->type == t_char or
		   t2->type == t_int)
	    )
	);
	if (b == false) {
	  b = (Boolean) (
	    (
		t1->class == RANGE and isfloat(t1) and t2->type == t_real
	    ) or (
		isarray(t1, "char") and
		  (isarray(t2, "char") or t2->type == t_char)
	    ) or (
		t1->type == t2->type and (
		    (t1->class == t2->class) or
		    (t1->class == SCAL and t2->class == CONST) or
		    (t1->class == CONST and t2->class == SCAL)
		)
	    )
	  );
	}
	if (b == false) {
	  b = (Boolean) (
	    (
	  	t1 == t_nil and t2->class == PTR 	/* why is this here? */
	    ) or (
	  	t1->class == PTR and (t2->type == t_int or t2 == t_nil)
	    ) or (
		t1->class == RANGE and
		t2->class == SCAL and (rtype(t1->type))->class == SCAL
	    )
	  );
	}
    }
    return b;
}

private pascal_printdecl(s)
Symbol s;
{
    register Symbol t;
    Boolean semicolon;

    semicolon = true;
    switch (s->class) {
	case CONST:
	    if (s->type->class == SCAL) {
		printf("(enumeration constant, ord %ld)",
		    s->symvalue.iconval);
	    } else {
		printf("const %s = ", symname(s));
		printval(s);
	    }
	    break;

	case TYPE:
	    printf("type %s = ", symname(s));
	    printtype(s, s->type, 0);
	    if (s->type != nil and s->type->class == RECORD) {
		semicolon = false;
	    }
	    break;

	case VAR:
	case CVALUE:
	    if (isparam(s)) {
		if (s->type->class == FFUNC) {
		    printf("(function parameter) %s : ",
		      symname(s));
		} else if (s->type->class == FPROC) {
		    printf("(procedure parameter) %s", symname(s));
		    break;
		} else {
		   printf("(parameter) %s : ", symname(s));
		}
	    } else {
		printf("var %s : ", symname(s));
	    }
	    printtype(s, s->type, 0);
	    break;

	case FVAR:
	   printf("function variable %s : ", symname(s));
	    printtype(s, s->type, 0);
	    break;

	case REF:
	   printf("(var parameter) %s : ", symname(s));
	    printtype(s, s->type, 0);
	    break;

	case READONLY:
	   printf("(conformant array bounds parameter) %s : ",
	     symname(s));
	    printtype(s, s->type, 0);
	    break;

	case RANGE:
	case ARRAY:
	case CARRAY:
	case RECORD:
	case PTR:
	    printtype(s, s, 0);
	    semicolon = false;
	    break;

	case SET:
	case FILET:
	    printtype(s, s, 0);
	    break;

	case FFUNC:
	   printf("(function parameter) %s : ", symname(s));
	    printtype(s, s->type, 0);
	    break;

	case FPROC:
	   printf("(procedure parameter) %s", symname(s));
	    break;

	case FIELD:
	   printf("(field) %s : ", symname(s));
	    printtype(s, s->type, 0);
	    break;

	case PROC:
	   printf("procedure %s", symname(s));
	    listparams(s);
	    break;

	case PROG:
	   printf("program %s", symname(s));
	    t = s->chain;
	    if (t != nil) {
		printf("(%s", symname(t));
		for (t = t->chain; t != nil; t = t->chain) {
		   printf(", %s", symname(t));
		}
		printf(")");
	    }
	    break;

	case MODULE:
	    semicolon = false;
	   printf("source file \"%s.p\"", symname(s));
	    break;

	case FUNC:
	    if (istypename(s->type, "(void)")) {
		printf("procedure %s", symname(s));
	    	listparams(s);
	    } else {
	    	printf("function %s", symname(s));
	    	listparams(s);
	    	printf(" : ");
	    	printtype(s, s->type, 0);
	    }
	    break;

	default:
	    error("class %s in printdecl", classname(s));
    }
    if (semicolon) {
	printf(";");
    }
    printf("\n");
}

/*
 * Recursive whiz-bang procedure to print the type portion
 * of a declaration.
 *
 * The symbol associated with the type is passed to allow
 * searching for type names without getting "type blah = blah".
 */

private printtype(s, t, indent)
Symbol s;
Symbol t;
Integer indent;
{
    register Symbol tmp, tmp2;
    register int i;
    static int indent_done = 0;

    checkref(s);
    checkref(t);
    if (indent_done) {
	indent_done = 0;
    } else if (indent > 0 ) {
	printf("%*c", indent, ' ');
    }
    switch (t->class) {
	case VAR:
	case CONST:
	case FUNC:
	case PROC:
	    panic("pascal_printtype: class %s", classname(t));
	    break;

	case ARRAY:
	case CARRAY:
	    printf("array[");
	    tmp = t->chain;
	    if (tmp != nil) {
		for (;;) {
		    printtype(t, tmp, 0);
		    tmp = tmp->chain;
		    if (tmp == nil) {
			break;
		    }
		   printf(", ");
		}
	    }
	    printf("] of ");
	    printtype(t, t->type, 0);
	    break;

	case RECORD:
	    printf("record\n");
	    if (t->chain != nil) {
		printtype(t->chain, t->chain, indent+4);
	    }
	    if (indent > 0) {
	    	printf("%*c", indent, ' ');
	    }
	    printf("end;");
	    break;

	case FIELD:
	    if (t->chain != nil and t->chain->class == TAG and
	      t->chain->symvalue.tag.tagfield != nil) {
	    /* this tag field be printed as part of the variant part - so don't print */
		indent_done = 1;
		printtype(t->chain, t->chain, indent);
	    } else {
	    	printf("%s : ", symname(t));
	    	printtype(t, t->type, 0);
	    	printf(";\n");
	    	if (t->chain != nil) {
		    printtype(t->chain, t->chain, indent);
	    	}
	    }
	    break;

	case TAG:
	    printf("case ");
	    if (t->symvalue.tag.tagfield != nil) {
		printf("%s : ", symname(t->symvalue.tag.tagfield));
	    }
	    printtype(t->type, t->type, 0);
	    printf(" of\n");
	    tmp = t->symvalue.tag.tagcase;
	    if (tmp != nil) {
		if (tmp->class != VARIANT) {
		    error("internal error: bad record information");
		} else {
		    printtype(tmp, tmp, indent+4);
		}
	    }
	    break;

	case VARIANT:
	    print_variant_label(t);
	    tmp = t;
	    while (t->chain != nil and t->chain->type == t->type) {
		printf(", ");
		t = t->chain;
		print_variant_label(t);
	    }
	    t = t->chain;
	    printf(" : ");
	    if (tmp->type != nil) {
		printf("(\n");
		printtype(tmp->type, tmp->type, indent+4);
	        if (indent > 0) {
	    	   printf("%*c", indent, ' ');
	        }
		printf(");\n");
	    } else {
		printf("();\n");
	    }
	    if (t != nil) {
		printtype(t, t, indent);
	    }
	    break;

	case RANGE: {
	    long r0, r1;

	    r0 = t->symvalue.rangev.lower;
	    r1 = t->symvalue.rangev.upper;
	    if (istypename(t->type, "char")) {
		if (r0 < 0x20 or r0 > 0x7e) {
		   printf("%ld..", r0);
		} else {
		   printf("'%c'..", (char) r0);
		}
		if (r1 < 0x20 or r1 > 0x7e) {
		   printf("\\%lo", r1);
		} else {
		   printf("'%c'", (char) r1);
		}
	    } else if (r0 > 0 and r1 == 0) {
		printf("%ld byte real", r0);
	    } else if (istypename(t->type, "integer")) {
	        if (r0 >= 0) {
		   printf("%lu..%lu", r0, r1);
	        } else {
		   printf("%ld..%ld", r0, r1);
	        }
	    } else {
		i = size(t);
		switch (i) {
		    case sizeof(char):
			push(char, (char)r0);
			break;

		    case sizeof(short):
			push(short, (short)r0);
			break;

		    default:
			push(long, r0);
			break;
		}
		pascal_printval(t);
		printf("..");
		switch (i) {
		    case sizeof(char):
			push(char, (char)r1);
			break;

		    case sizeof(short):
			push(short, (short)r1);
			break;

		    default:
			push(long, r1);
			break;
		}
		pascal_printval(t);
	    }
	    break;
	}

	case CRANGE: {		/* conformant array range */
	    long r0, r1;

	    tmp = (Symbol) t->symvalue.rangev.lower;
	    tmp2 = (Symbol) t->symvalue.rangev.upper;
	    if (getcbound(s, tmp, &r0) == false) {
		printf("%s..%s", symname(tmp), symname(tmp2));
	    } else {
	        getcbound(s, tmp2, &r1);
	        if (istypename(t, "char")) {
		    if (r0 < 0x20 or r0 > 0x7e) {
		       printf("%ld..", r0);
		    } else {
		       printf("'%c'..", (char) r0);
		    }
		    if (r1 < 0x20 or r1 > 0x7e) {
		       printf("\\%lo", r1);
		    } else {
		       printf("'%c'", (char) r1);
		    }
	        } else if (istypename(t, "integer")) {
	            if (r0 >= 0) {
		       printf("%lu..%lu", r0, r1);
	            } else {
		       printf("%ld..%ld", r0, r1);
	            }
	        } else {
		    i = size(t);
		    switch (i) {
		        case sizeof(char):
			    push(char, (char)r0);
			    break;

		        case sizeof(short):
			    push(short, (short)r0);
			    break;

		        default:
			    push(long, r0);
			    break;
		    }
		    pascal_printval(t);
		    printf("..");
		    switch (i) {
		        case sizeof(char):
			    push(char, (char)r1);
			    break;

		        case sizeof(short):
			    push(short, (short)r1);
			    break;

		        default:
			    push(long, r1);
			    break;
		    }
		    pascal_printval(t);
	        }
	    }
	    printf(": ");
	    printtype(t, t->type, 0);
	    break;
	}

	case PTR:
	    printf("^");
	    printtype(t, t->type, 0);
	    break;

	case TYPE:
	    if (symname(t) != nil) {
		printf("%s", symname(t));
	    } else {
		printtype(t, t->type, indent);
	    }
	    break;

	case SCAL:
	    printf("(");
	    t = t->chain;
	    i = 0;
	    if (t != nil) {
	    	while (1) {
		    printf("%s", symname(t));
		    t = t->chain;
		    if (t == nil) {
			break;
		    }
		    printf(", ");
		    if (i++ == 5) {
			i = 0;
			printf("\n");
    			if (indent > 0 ) {
			    printf("%*c", indent, ' ');
    			}
		    }
		}
	    } else {
		panic("empty enumeration");
	    }
	    printf(")");
	    break;

	case FILET:
	    printf("file of ");
	    printtype(t->type, t->type, 0);
	    break;

	case SET:
	    printf("set of ");
	    printtype(t->type, t->type, 0);
	    break;

	case FFUNC:
	case FPROC:
	    printtype(t, t->type, 0);
	    break;

	case VARYING:
	    printf("varying [%d] of char", t->symvalue.varying.length);
	    break;

	default:
	    printf("(class %d)", t->class);
	    break;
    }
}

/*
 * List the parameters of a procedure or function.
 * No attempt is made to combine like types.
 */

private listparams(s)
Symbol s;
{
    Symbol t;

    if (s->chain != nil) {
	printf("(");
	for (t = s->chain; t != nil; t = t->chain) {
	    switch (t->class) {
		case REF:
		   printf("var ");
		    break;

		case VAR:
		case CVALUE:
		    break;

		default:
		    panic("unexpected class %d for parameter", t->class);
	    }
	    if (t->type->class == FPROC) {
		printf("procedure %s", symname(t));
	    } else if (t->type->class == FFUNC) {
		printf("function %s : ", symname(t));
	    	printtype(t->type, t->type, 0);
	    } else {
	       printf("%s : ", symname(t));
	    	printtype(t->type, t->type, 0);
	    }
	    if (t->chain != nil) {
		printf("; ");
	    }
	}
	printf(")");
    }
}

private set_is_member (base, n, nelem)
/*============================================================================*/
/* Given bitmap starting at 'base' and of length 'nelem' bits,
/* tells whether element 'n' is a member
/*............................................................................*/
void *base;
int n;
int nelem;
	{
	int rnelem = round (nelem, 16);
	int x;								/* index */
	unsigned short sbuf;	
	unsigned long lbuf;

#ifdef i386
	if (n < 16 && rnelem % 32 != 0)
		{
		x = (rnelem - (n+1)) / 16;
		sbuf = ((unsigned short *)base)[x];
		return (sbuf >> (n % 16)) & 0x1;
		}
	else
		{
		x = (rnelem - (n+1)) / 32;
		lbuf = ((unsigned long *)base)[x];

		if (rnelem % 32 != 0)
			n -= 16;
		return (lbuf >> (n % 32)) & 0x1;
		}
#else
	x = (rnelem - (n+1)) / 16;
	sbuf = ((unsigned short *)base)[x];
	return (sbuf >> (n % 16)) & 0x1;
#endif
	}

/*
 * Print out the value on the top of the expression stack
 * in the format for the type of the given symbol.
 */

private pascal_printval(s)
register Symbol s;
{
    register Symbol t;
    register int len, i;
    register Address addr;

    switch (s->class) {
	case TYPE:
	    if (istypename(s, "boolean")) {
		i = popsmall(s);
		printf(((Boolean) i) == true ? "true" : "false");
		break;
	    } /* else fall through */

	case VAR:
	case FVAR:
	case REF:
	case TYPEID:
	case CONST:
	case READONLY:
	case CRANGE:
	case CVALUE:
	    pascal_printval(s->type);
	    break;

	case ARRAY:
	case CARRAY:
	    t = rtype(s->type);
	    if (istypename(t, "char") or ((t->class == RANGE or
	      t->class == CRANGE) and istypename(t->type, "char"))) {
		len = size(s);
		sp -= len;
		printf("'%.*s'", len, sp);
		break;
	    } else {
		printarray(s);
	    }
	    break;

	case RECORD:
	    mp_printrecord(s);	/* in modula.c */
	    break;

	case FIELD:
	    if (size(s) != size(s->type)) {
		/* jpeck 8/11/89
		 * KLUDGE: printval will dispatch on the s->type
		 * and will popsmall(s->type)
		 * so here we must extend the real value to be 
		 * of size (s->type).
		 * and then put things back together afterwards.
		 * [other folks may be playing stack games: p_str_bitfield]
		 */
		/* fix problem with packed_record field 0..255
		 * versus var 0..255  different sizes for same type!
		 * size(field) => 1, size(var) = size(field->type) => 2
		 */
		int actual, orig;

		t = s->type;
		len = size(t);
		i = size(s);
		actual = popsmall(s);
		pushsmall(s, actual);
		sp += len - i;	/* typically len > i */
		orig = popsmall(t);
		pushsmall(t, actual);
		pascal_printval(t);
		pushsmall(t,orig);
		sp += i - len;
		popsmall(s);
	    } else {
		pascal_printval(s->type);
	    }
	    break;

	case RANGE:
	    if (istypename(s->type, "boolean")) {
			printf(((Boolean) popsmall(s)) == true ? "true" : "false");
	    } else if (istypename(s->type, "char")) {
			printf("'");
			printchar(pop(char));
			printf("'");
	    } else if (isfloat(s)) {
			prtreal(pop(double));
	    } else if (istypename(s->type, "integer")) {
	    	if (s->symvalue.rangev.lower >= 0) {
				printf("%lu", popsmall(s));
	    	} else {
				printf("%ld", popsmall(s));
	    	}
	    }  else {		/* range of non-standard type */
			pascal_printval(s->type);
	    }
	    break;

	case FILET:
	case PTR:
	    addr = pop(Address);
	    if (addr == 0) {
		printf("(nil)");
	    } else {
		printf("0x%x", addr);
	    }
	    break;

	case SCAL: {
	    int scalar;
	    Boolean found;

	    scalar = popsmall(s);
	    found = false;
	    for (t = s->chain; t != nil; t = t->chain) {
		if (t->symvalue.iconval == scalar) {
		   printf("%s", symname(t));
		    found = true;
		    break;
		}
	    }
	    if (not found) {
		printf("(scalar = %d)", scalar);
	    }
	    break;
	}

	case FPROC:
	case FFUNC:
	    /*
	     * earlier versions of pc used a descriptor
	     * to represent functional arguments.  This is
	     * changed to work like C and F77. -- (2 Oct 84)
	     */
	    addr = pop(Address);
	    t = whatblock(addr, true);
	    if (t == nil) {
		printf("(proc at 0x%x)", addr);
	    } else {
		printf("%s", symname(t));
	    }
	    break;

	case SET:
		/*
		sun3/sun4:
		the set of lowercase letters:
		|f e d c b a 9 8 7 6 5 4 3 2 1 0|f e d c b a 9 8 7 6 5 4 3 2 1 0|
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|           |z|y|x|w|v|u|t|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|c|b|a|
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|            word 1             |             word 2            |
		*/
		{
		int nbytes = size(s);
		int n;					/* member index */
		int nelem = pascal_set_nelem(s);
		int offset;
		int elsize;
		Stack *savesp;
		int total_printed;
		Stack *base;
		Stack *origsp;
		int orig;

		base = sp - nbytes;

		t = rtype(s->type);
		elsize = size(t);
		offset = (t->class == RANGE)? t->symvalue.rangev.lower: 0;

		savesp = sp - nbytes;  /* we must pop this set */

		total_printed = 0;
		printf("[");

		for (n = 0; n < nelem; n++)
			{
			if (set_is_member(base, n, nelem))
				{
				if ((total_printed % 8) == 0)
					{
					if (total_printed != 0)
						{
						printf(",\n");
						indent (indentlevel);
						}
					}
				else
				   printf(", ");

				total_printed++;

				sp = &stack[64]; /* arb. section of stack */
				orig = pop(long);
				origsp = sp;
				switch (elsize) 
					{
					case sizeof(char):
						push(char, (char) (n + offset));
						break;

					case sizeof(short):	
						push(short, (short) (n + offset));
						break;

					default:
						push(long, (n + offset));
						break;
					}
				pascal_printval(t);
				sp = origsp;
				push(long,orig);

				/* DEBUG
				printf ("=%d", n);
				*/

				}
			}	/* for each element */
		sp = savesp;
		printf("]");
		}
		break;

	case PROC:
	case FUNC:
	    t = pop(Symbol);
	    printf("%s", symname(t));
	    break;

	case VARYING:
	    print_varying_string( s );
	    break;

	default:
	    if (ord(s->class) < ord(BADUSE) or ord(s->class) > ord(TYPEREF)) {
		panic("printval: bad class %d", ord(s->class));
	    }
	    error("don't know how to print %s", aclassname(s));
	    /* NOTREACHED */
    }
}


/*
 * Pascal varying strings have a length word, followed by the
 * string area itself.  No null byte is guaranteed.  Despite the
 * four-byte "current length" word, pc and dbx agree to impose a
 * limit of 32767 characters in a varying string.
 */
private print_varying_string(s)
register Symbol s;
{
    register int maxlen, i;
    register Word curlen;
    register char *chars;

    maxlen = s->symvalue.varying.length;
    sp -= maxlen;
    chars = sp ;
    curlen = pop( Word );

    /*
     * Sanity checks
     */
    if( maxlen > 32767 ) {
	error( "Varying string's maximum %d > 32767.\n", maxlen );
    }

    if( curlen < 0 ) {
	error( "Varying string's current length %d < 0.\n", curlen );
    }

    if( curlen > maxlen ) {
	printf( "Varying string's current length %d < maximum %d -- string truncated.\n",
	    curlen, maxlen );
	curlen = maxlen;
    }

    /*
     * Must print them out one at a time so that
     * we won't stop at a null byte.
     */
    printf( "\"" );
    for( i = 0; i < curlen ; ++i ) {
	printf( "%c", chars[i] );
    }
    printf( "\"" );
}


private print_variant_label(t)
register Symbol t;
{
    register int i;
    register Symbol t2;

    i = t->symvalue.variant.casevalue;
    t2 = t->symvalue.variant.tagtype;
    switch (size(t2)) {
	case sizeof(char):
	    push(char, (char)i);
	    break;

	case sizeof(short):
	    push(short, (short)i);
	    break;

	default:
	    push(long, i);
	    break;
    }
    pascal_printval(t2);
}

private Node pascal_buildaref(a, slist)
Node a, slist;
{
    register Symbol t;
    register Node p;
    Symbol etype, atype, eltype;
    Node esub, r;

    r = a;
    t = rtype(a->nodetype);
    eltype = t;
    do {
	eltype = rtype(eltype->type);
    } while (eltype != nil and
      (eltype->class == ARRAY or eltype->class == CARRAY));
    if (eltype == nil) {
	eltype = rtype(a->nodetype);
    }
    if (t->class != ARRAY and t->class != CARRAY) {
	beginerrmsg();
	prtree(a);
	printf(" is not an array");
	enderrmsg();
    } else {
	p = slist;
	do {
	    esub = p->value.arg[0];
	    etype = rtype(esub->nodetype);
	    atype = rtype(t->chain);
	    if (not compatible(atype, etype)) {
		beginerrmsg();
		printf("subscript ");
		prtree(esub);
		printf(" is the wrong type");
		enderrmsg();
	    }
	    r = build(O_INDEX, r, esub);
	    r->nodetype = t->type;
	    t = rtype(t->type);
	    p = p->value.arg[1];
	} while (p != nil and t != nil and
	  (t->class == ARRAY or t->class == CARRAY));
	if (p != nil or t == nil or t->class == ARRAY or t->class == CARRAY) {
	    beginerrmsg();
	    if (p != nil or t == nil) {
		printf("too many subscripts for ");
	    } else {
		printf("not enough subscripts for ");
	    }
	    prtree(a);
	    enderrmsg();
	}
	r->nodetype = eltype;
    }
    return r;
}

/*
 * Evaluate an array subscript.  This is called from evalindex(), which
 * in turn is called from the O_INDEX case in eval().  The tree built
 * (containing O_INDEX node) was made in pascal_buildaref().
 * 's' is either the ARRAY itself or else the index type (if this is
 * the second+ dimension).
 */

private int pascal_evalaref(s, i)
Symbol s;
long i;
{
    register Symbol t;
    static Symbol array;	/* root node for this array reference */
    long lb, ub;

    t = rtype(s);
    if (s->name != nil and (t->class == ARRAY or t->class == CARRAY)) {
	array = t;
    }
    s = rtype(t->chain);
    if (s->class == SCAL) {	/* Pascal may have a scalar index */
	lb = 0;
	for (t = s->chain; t != nil; t = t->chain) {
	    if (t->symvalue.iconval == i) {
		break;
	    } else {
		lb++;
	    }
	}
	if (t == nil) {
	    error("subscript out of range");
	} else {
	    return(lb);
	}
	/* NOT REACHED */
    }
    if (s->class == CRANGE) {	/* Pascal conformant array */
	if (getcbound(array, s->symvalue.rangev.lower, &lb) == false) {
	    panic("evalaref: conformant array symbol table error");
	}
	if (getcbound(array, s->symvalue.rangev.upper, &ub) == false) {
	    panic("evalaref: conformant array symbol table error");
	}
    } else {
        lb = s->symvalue.rangev.lower;
        ub = s->symvalue.rangev.upper;
    }
    if (i < lb or i > ub) {
	if (ub == -1 and lb == 0 and s->class != CRANGE) {
	   printf("warning: array bounds unknown (not specified in source)\n");
	} else {
	    error("subscript out of range");
	}
    }
    return(i - lb);
}

/*
 * Evaluate arguments left-to-right.
 */

public pascal_evalparams(proc, arglist)
Symbol proc;
Node arglist;
{
    Node p;
    Node exp;
    Symbol arg;
    Symbol t;
    Stack *savesp;
    Integer ntypesize, argsize, i;
    float f;

    savesp = sp;
    i = 0;
    arg = proc->chain;
    for (p = arglist; p != nil; p = p->value.arg[1]) {
	if (arg == nil) {
	    error("too many parameters to %s", symname(proc));
	}
        if ((arg)->class == CVALUE) {
            error("Can't currently call with a conformant array parameter");
        }
	if ((rtype(arg))->class == CARRAY) {
	    error("Can't currently call with a conformant array parameter");
	}
        if ((rtype(arg))->class == SET ) {
            error("Can't currently call with a set parameter");
        }
	exp = p->value.arg[0];
	if (exp->op == O_CALL) {
	    error("Can't currently call with a function/procedure parameter");
	}

	exp = p->value.arg[0];
	ntypesize = size(exp->nodetype);
	argsize = size(arg->type);
	if (arg->class == REF) {
	    if (exp->op != O_RVAL) {
		switch (exp->op) {
		case O_LCON:
		case O_FCON:
		case O_SCON:
		    error("Cannot pass constants to var parameters");
		    break;
		}
	    } else if (ntypesize != argsize) {
	    	warning("expression for parameter %s may be wrong type",
	      	  symname(arg));
	    }
	} else if (exp->op == O_SCON) {
	    /*
	     * Change the size of the constant string to match
	     * the declaration for the parameter.
	     */
	    exp->nodetype->chain->symvalue.rangev.upper = argsize;
	} else if (ntypesize != argsize) {
	    warning("expression for parameter %s may be wrong type",
	      symname(arg));
	}
	arg = arg->chain;
    }
    if (arg != nil) {
	sp = savesp;
	error("not enough parameters to %s", symname(proc));
    }
    arg = proc->chain;
    for (p = arglist; p != nil; p = p->value.arg[1]) {
	exp = p->value.arg[0];
	ntypesize = size(exp->nodetype);
	if (arg->class == REF) {
	    push(Address, lval(exp->value.arg[0]));
	} else {
	    eval(exp);
	    if (ntypesize < sizeof(long)) {
		i = popsmall(exp->nodetype);
	        push(long, i);
	    }
	}
	arg = arg->chain;
    }
}

/*
 * Evaluate arguments left-to-right.
 */


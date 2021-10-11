#ifndef lint
static	char sccsid[] = "@(#)modula.c 1.1 94/10/31 SMI";

#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Modula2-dependent symbol routines.
 */

#include "defs.h"
#include "symbols.h"
#include "modula.h"
#include "languages.h"
#include "tree.h"
#include "eval.h"
#include "mappings.h"
#include "process.h"
#include "runtime.h"
#include "machine.h"

private Boolean modula_typematch();
private modula_printdecl();
private modula_printval();
private Node modula_buildaref();
private int modula_evalaref();

#ifndef public
#endif

extern c_printparams();
extern pascal_evalparams();

#define isrange(t, name) (t->class == RANGE and istypename(t->type, name))
#define isarray(t, name) ((t->class == ARRAY or t->class == CARRAY) and istypename(t->type, name))

/*
 * Initialize Pascal information.
 */

public modula_init()
{
    Language lang;

    lang = language_define("modula2");
    language_suffix_add(lang, ".mod");
    language_setop(lang, L_PRINTDECL, modula_printdecl);
    language_setop(lang, L_PRINTVAL, modula_printval);
    language_setop(lang, L_PRINTPARAMS, c_printparams);
    language_setop(lang, L_EVALPARAMS, pascal_evalparams);
    language_setop(lang, L_TYPEMATCH, modula_typematch);
    language_setop(lang, L_BUILDAREF, modula_buildaref);
    language_setop(lang, L_EVALAREF, modula_evalaref);
}

/*
 * Compatible tests if two types are compatible.  The issue
 * is complicated a bit by ranges.
 *
 * Integers and reals are not compatible since they cannot always be mixed.
 */

private Boolean modula_typematch(type1, type2)
Symbol type1, type2;
{
    Boolean b;
    register Symbol t1, t2, tmp;

    t1 = type1;
    t2 = type2;
    if (t1 == t2) {
	b = true;
    } else {
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
		isrange(t1, "INTEGER") and
		  (isrange(t2, "INTEGER") or t2->type == t_int or
		   t2->type == t_char)
	    ) or (
		isrange(t1, "CHAR") and
		  (isrange(t2, "CHAR") or t2->type == t_char or
		   t2->type == t_int)
	    ) or (
		isrange(t1, "CARDINAL") and
		  (isrange(t2, "CARDINAL") or t2->type == t_char or
		   t2->type == t_int)
	    )
	);
	if (b == false) {
	  b = (Boolean) (
	    (
		t1->class == RANGE and isfloat(t1) and t2->type == t_real
	    ) or (
		isarray(t1, "CHAR") and
		  (isarray(t2, "CHAR") or t2->type == t_char)
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

private modula_printdecl(s)
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
		printf("CONST %s = ", symname(s));
		printval(s);
	    }
	    break;

	case TYPE:
	    printf("TYPE %s = ", symname(s));
	    printtype(s, s->type, 0);
	    if (s->type != nil and s->type->class == RECORD) {
		semicolon = false;
	    }
	    break;

	case VAR:
	case CVALUE:
	    if (isparam(s)) {
		printf("(parameter) %s : ", symname(s));
	    } else {
		printf("VAR %s : ", symname(s));
	    }
	    printtype(s, s->type, 0);
	    break;

	case REF:
	    printf("(var parameter) %s : ", symname(s));
	    printtype(s, s->type, 0);
	    break;

	case RANGE:
	case ARRAY:
	case RECORD:
	case PTR:
	    printtype(s, s, 0);
	    semicolon = false;
	    break;

	case SET:
	    printtype(s, s, 0);
	    break;

	case FIELD:
	    printf("(field) %s : ", symname(s));
	    printtype(s, s->type, 0);
	    break;

	case PROC:
	    printf("PROCEDURE %s", symname(s));
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

	case MODUS:
	    printf("MODULE %s", symname(s));
	    break;

	case IMPORT:
	    printf("IMPORT %s = ", symname(s));
	    s = findreal(s);
	    if (s != nil) {
		printtype(s, s->type, 0);
		if (s->type->class == RECORD) semicolon = false;
	    } else {
		printf("unknown type");
	    }
	    break;

	case MODULE:
	    semicolon = false;
	    printf("source file \"%s.mod\"", symname(s));
	    break;

	case FUNC:
	    if (s->type == nil) {
		/*
		 * This symbol had better be the fake FUNC that is really
		 * the initialization code for the module (MODUS).
		 */
		if( s->block != nil  and
		    s->block->name == s->name  and
		    s->block->class == MODUS ) {
		    printf( "MODULE %s", symname(s));
		} else {
		    error( "symbol %s has nil FUNCTION type\n",
			symname(s) );
		}
	    } else if (istypename(s->type, "(void)")) {
		printf("PROCEDURE %s", symname(s));
	    	listparams(s);
	    } else {
	    	printf("PROCEDURE %s", symname(s));
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
	case MODUS:
	    panic("modula_printtype: class %s", classname(t));
	    break;

	case ARRAY:
	case CARRAY:
	    printf("ARRAY");
	    if (t->chain->symvalue.rangev.uppertype == R_ARG) {
		printf(" OF ");
		printtype(t, t->type, 0);
		break;
	    }
	    printf("[");
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
	    printf("] OF ");
	    printtype(t, t->type, 0);
	    break;

	case RECORD:
	   printf("RECORD\n");
	    if (t->chain != nil) {
		printtype(t->chain, t->chain, indent+4);
	    }
	    if (indent > 0) {
	    	printf("%*c", indent, ' ');
	    }
	   printf("END;");
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
	   printf("CASE ");
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
	   printf(" : ");				/*???*/
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
	    if (istypename(t->type, "CHAR")) {
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
	    } else if (istypename(t->type, "INTEGER")) {
	        if (r0 >= 0) {
		   printf("%lu..%lu", r0, r1);
	        } else {
		   printf("%ld..%ld", r0, r1);
	        }
	    } else if (istypename(t->type, "CARDINAL")) {
		printf("%lu..%lu", r0, r1);
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
		modula_printval(t);
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
		modula_printval(t);
	    }
	    break;
	}

	case PTR:
	    printf("POINTER TO ");
	    printtype(t, t->type, 0);
	    break;

	case TYPE:
	    if (t->name != nil) {
		printf("%s", symname(t));
	    } else {
		if( t == t->type )
		    printf( "Opaque" );
		else
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

	case SET:
	   printf("SET OF ");
	    printtype(t->type, t->type, 0);
	    break;

	case FUNCT:             /* ????? */
	    if (istypename(t, "(void)")) {
		printf("(PROCEDURE TYPE) ");
	        listparams(t);
	    } else {
		printf("(PROCEDURE TYPE) ");
		listparams(t);
		printf(" : ");
		printtype(t, t->type, 0);
	    }
	    break;

	case IMPORT:
	    tmp = t;
	    t = findreal(t);
	    if (t == nil) {
		printf("(IMPORT TYPE) %s", symname(tmp));
	    } else {
		printtype(s, t, indent);
	    }
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
		   printf("VAR ");
		    break;

		case VAR:
		case CVALUE:
		    break;

		default:
		    panic("unexpected class %d for parameter", t->class);
	    }
	    if (s->name != nil) printf("%s : ", symname(t));
	    printtype(t->type, t->type, 0);
	    if (t->chain != nil) {
		printf("; ");
	    }
	}
	printf(")");
    }
}

/*
 * Print out the value on the top of the expression stack
 * in the format for the type of the given symbol.
 */

private modula_printval(s)
register Symbol s;
{
    register Symbol t;
    register int len, i;
    register Address addr;

    switch (s->class) {
	case TYPE:
	    if (istypename(s, "BOOLEAN")) {
		i = popsmall(s);
		printf(((Boolean) i) == true ? "TRUE" : "FALSE");
		break;
	    } /* else fall through */

	case VAR:
	case REF:
	case CONST:
	case CVALUE:
	    if( s == s->type ) {
		/* It's an opaque type.  Print it in Hex. */
		addr = pop(Address);
		if (addr < 10 ) {
		    printf( "%d", addr );
		} else {
		    printf( "0x%x", addr);
		}
	    } else {
		modula_printval(s->type);
	    }
	    break;

	case ARRAY:
	    t = rtype(s->type);
	    if (istypename(t, "CHAR") or (t->class == RANGE
	      and istypename(t->type, "CHAR"))) {
		len = size(s);
		sp -= len;
		printf("\"%.*s\"", len, sp);
		break;
	    } else {
		printarray(s);
	    }
	    break;

	case RECORD:
	    mp_printrecord(s);
	    break;

	case FIELD:
	    modula_printval(s->type);
	    break;

	case RANGE:
	    if (istypename(s->type, "BOOLEAN")) {
		printf(((Boolean) popsmall(s)) == true ? "TRUE" : "FALSE");
	    } else if (istypename(s->type, "CHAR")) {
		printf("'");
		printchar(pop(char));
		printf("'");
	    } else if (isfloat(s)) {
		prtreal(pop(double));
	    } else if (istypename(s->type, "INTEGER") or
		       istypename(s->type, "LONGINT") or
		       istypename(s->type, "LONGCARD") or
		       istypename(s->type, "CARDINAL") or
		       istypename(s->type, "SHORTINT") or
		       istypename(s->type, "SHORTCARD") or
		       istypename(s->type, "BYTE") or
		       istypename(s->type, "ADDRESS") or
		       istypename(s->type, "WORD")) {
	    	if (s->symvalue.rangev.lower >= 0) {
		   printf("%lu", popsmall(s));
	    	} else {
		   printf("%ld", popsmall(s));
	    	}
	    }  else {		/* range of non-standard type */
		modula_printval(s->type);
	    }
	    break;

	case PTR:
	case FUNCT:
	    addr = pop(Address);
	    if (addr == 0) {
		printf("NIL");
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

	case SET: {
	    register int j, k, l, total_printed;
	    int offset, elsize;
	    Stack *savesp;

	    len = size(s);
	    printf("{");
	    len = len div 4;
	    if (len == 0) len = 1;
	    savesp = sp;
	    total_printed = 0;
	    t = rtype(s->type);
	    if (t->class == RANGE) {
		offset = t->symvalue.rangev.lower;
	    } else {
		offset = 0;
	    }
	    elsize = size(t);
	    for (i = 1; i <= len; i++) {	/* one word at a time */
		sp = savesp;
	    	sp -= ((len - i) * 4);
		k = 32 * (i - 1);
		if (len > 1)
		    l = pop(long);
		else
		    l = popsmall(s);
		j = l;
		do {
		    if ((j & 1) != 0) {
			if ((total_printed % 10) == 0) {
			    if (total_printed != 0) {
				printf(",\n");
			    }
			} else {
			   printf(", ");
			}
			total_printed++;
			switch (elsize) { 	/* element size? */
			    case sizeof(char):
				push(char, (char) (k + offset));
				break;

			    case sizeof(short):
				push(short, (short) (k + offset));
				break;

			    default:
				push(long, (k + offset));
				break;
			}
			modula_printval(t);
		    }
		    k++;
		    j = (unsigned int) j >> 1;
		} while (j != 0);
	    }
	    printf("}");
	    sp = savesp - size(s);
	    break;
	}

	case PROC:
	case FUNC:
	    t = pop(Symbol);
	    printf("%s", symname(t));
	    break;

      	case IMPORT:
	    t = findreal(s);
	    if ( t != nil ) {
		printval(t);
		break;
	    }

	default:
	    if (ord(s->class) < ord(BADUSE) or ord(s->class) > ord(TYPEREF)) {
		panic("printval: bad class %d", ord(s->class));
	    }
	    error("don't know how to print %s", aclassname(s));
	    /* NOTREACHED */
    }
}

/*
 * Print out the value of a (Modula-2 or Pascal) record, field by field.

	Somewhat hackish: Whoever designed the variant record stuff for 
	pascal/modula types didn't make the "variations" of a type a recursion
	in the Symbol structure. Instead they are kept in the 'chain'. 
	This means that when we reach a variant field we have to find it, and then
	also skip the rest of the variations up to the next normal field.
	These routines were initially coded in a threaded fashion where recursion
	was the sole means of iteration over the loops, but then getting the 
	indentation and proper newlining becomes a nightmare. Its redone here in
	iterative form.
	The main hack is that printfield returns the next item rather than depending
	on the for() ... this is to ease the skipping of irrelevant variations.
 */

public mp_printrecord(s)
Symbol s;
{
	Symbol printfield();		/* forward ref */

    if (s->chain == nil) {
	error("record has no fields");
    }
	printf("record\n");
	sp -= (size(s));
	++indentlevel;
	for (s = s->chain; s != nil; )
		s = printfield (s);
	indent (indentlevel);
	printf("end");
	--indentlevel;
}

/*
 * Print out a field.
 */

private Symbol printfield(s)
register Symbol s;
{
    register Stack *savesp;
    register Symbol t;
    register int l;
	int dval;			/* discriminat value */
    Boolean found;

    if (s->class == TAG) {
        found = false;
	t = s->symvalue.tag.tagfield;
	if (t == nil) {
		printf("<variant>");			/* variant w/o a discriminant */
	} else {	/* print variant part */
	    savesp = sp;
		sp += ((t->symvalue.field.offset div BITSPERBYTE) + size(t->type));
		indent (indentlevel);
		printf("%s = ", symname(t));

		/* ivan 13 Feb 1989
		/* Do two things:
		/* 	1)	extract the discriminant value and put it into 'dval'
		/*	2)	print that value.
		/* All the stack gymnastics are neccessary because 
		/* 'push_aligned_bitfield()' clobbers the stack.
		/*....................................................................*/
	    switch (size(rtype(t))) {
			case sizeof(char):
				l = (int) pop(char);

				push(char, l);
				push_aligned_bitfield(t);
				dval = (int) pop(char);

				push(char, l);
				p_str_arr_val(t, size(t));

				break;

			case sizeof(short):
				l = (short) pop(short);

				push(short, l);
				push_aligned_bitfield(t);
				dval = (int) pop(short);

				push(short, l);
				p_str_arr_val(t, size(t));

				break;

			default:
				l = pop(int);

				push(long, l);
				push_aligned_bitfield(t);
				dval = (int) pop(long);

				push(int, l);
				p_str_arr_val(t, size(t));

				break;
	    }

	    sp = savesp;
	    s = s->symvalue.tag.tagcase;

		/*
		 * the "variations" are stored along the chain ... go find
		 * the correct variation.
		 */
		for (t = s; t != nil; t = t->chain) {
			if (t->class != VARIANT) {
				error("internal error in printfield() - no variant label");
			}
			if (t->symvalue.variant.casevalue == dval) {
				found = true;
				break;
			}
	    }
	    if (found == true) {
			printf(" (\n");
			++indentlevel;
			for (s = t->type; s != nil; s = s->chain)
				printfield (s);
			indent (indentlevel);
			printf(")");
			printf ("\n");
			--indentlevel;
	    }

		/*
		 *	Now skip the rest of the "variations" ...
		 */
		for (t = s; t != nil and t->class == VARIANT; t = t->chain)
			;

		return t;
	}
    } else if (s->class == FIELD) {
	t = s->chain;
	if (t != nil and t->class == TAG and t->symvalue.tag.tagfield != nil) {
		return t;

	} else {
	        indent (indentlevel);
		printf("%s = ", symname(s));
		savesp = sp;
		/* size includes offset % 8, here we add offset div 8 */
		sp += ((s->symvalue.field.offset div BITSPERBYTE) + size(s));
		p_str_arr_val(s, size(s));
		sp = savesp;
		printf ("\n");
		return t;
    	    }
    } else {
	error("internal error: class of record element incorrect");
    }
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
    modula_printval(t2);
}

private Node modula_buildaref(a, slist)
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
 * (containing O_INDEX node) was made in modula_buildaref().
 * 's' is either the ARRAY itself or else the index type (if this is
 * the second+ dimension).
 */

private int modula_evalaref(s, i)
Symbol s;
long i;
{
    register Symbol os, t;
    static Symbol array;	/* root node for this array reference */
    long lb, ub;

    os = s;
    t = rtype(s);
    if (t->class == IMPORT) {
	t = findreal(t);
	if (t == nil) error("no definition of the import type");
	t = rtype(t);
    }
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
    lb = get_lowbound(os, s);
    ub = get_upbound(os, s);
    if (i < lb or i > ub) {
	error("subscript out of range");
    }
    return(i - lb);
}

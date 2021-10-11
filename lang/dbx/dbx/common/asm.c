#ifndef lint
static	char sccsid[] = "@(#)asm.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Assembly language dependent symbol routines.
 */

#include "defs.h"
#include "symbols.h"
#include "asm.h"
#include "languages.h"
#include "tree.h"
#include "eval.h"
#include "operators.h"
#include "mappings.h"
#include "process.h"
#include "runtime.h"
#include "machine.h"
#include "printsym.h"
#include "c.h"

private asm_printdecl();
private asm_printval();
private Boolean asm_typematch();
private asm_buildaref();

#define isdouble(range) ( \
    range->symvalue.rangev.upper == 0 and range->symvalue.rangev.lower > 0 \
)


/*
 * Initialize assembly language information.
 */

public asm_init()
{
    Language lang;

    lang = language_define("assembler");
    language_suffix_add(lang, ".s");
    language_setop(lang, L_PRINTDECL, asm_printdecl);
    language_setop(lang, L_PRINTVAL, asm_printval);
    language_setop(lang, L_PRINTPARAMS, c_printparams);
    language_setop(lang, L_EVALPARAMS, c_evalparams);
    language_setop(lang, L_TYPEMATCH, asm_typematch);
    language_setop(lang, L_BUILDAREF, asm_buildaref);
    language_setop(lang, L_EVALAREF, asm_buildaref);
}

/*
 * Test if two types are compatible.
 * ARGSUSED
 */

private Boolean asm_typematch(type1, type2)
Symbol type1, type2;
{
    Boolean b;

    b = false;
    return b;
}

private asm_printdecl(s)
Symbol s;
{
    switch (s->class) {
	case VAR:
	case REF:
	    printf("&%s = 0x%x", symname(s), s->symvalue.offset);
	    break;

	case PROC:
	case FUNC:
	    printf("%s (0x%x):", symname(s), codeloc(s));
	    break;

	case MODULE:
	    printf("object file \"%s.o\"", symname(s));
	    break;

	case ARRAY:
	    if( s->name == nil ) {
		/* This can happen if user typed <whatis "">. */
		error( "no symbol \"\".", nil );
		break;
	    }

	    /* Otherwise, fall through to the original obscure error message. */
	default:
	    error("class %s in asm_printdecl", classname(s));
    }
    printf("\n");
}

/*
 * Print out the value on the top of the expression stack
 * in the format for the type of the given symbol.
 */

private asm_printval(s)
register Symbol s;
{
    register Symbol t;
    register Integer len;

    switch (s->class) {
	case ARRAY:
	    t = rtype(s->type);
	    if (t->class == RANGE and istypename(t->type, "$char")) {
		len = size(s);
		sp -= len;
		printf("\"%.*s\"", len, sp);
	    } else {
		printarray(s);
	    }
	    break;

	default:
	    printf("0x%x", pop(Integer));
	    break;
    }
}

/*
 * Error routine called when trying to build or evaluate an array
 * reference that does not have any type information associated with it.
 * ARGSUSED
 */
static
asm_buildaref(array, sub)
Node	array;
Node	sub;
{
	error("No type information - cannot build an array reference");
}

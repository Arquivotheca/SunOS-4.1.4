#ifndef lint
static char sccsid[] = "@(#)languages.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Language management.
 */

#include "defs.h"
#include "languages.h"
#include "tree.h"
#include "c.h"
#include "pascal.h"
#include "asm.h"

#ifndef public
typedef struct Language *Language;

typedef enum {
    L_PRINTDECL,
    L_PRINTVAL,
    L_PRINTPARAMS,
    L_EVALPARAMS,
    L_TYPEMATCH,
    L_BUILDAREF,
    L_EVALAREF
} LanguageOp;

typedef LanguageOperation();

/*
 * Macros to use the language ops vectors
 */
#define printdecl(sym)			(*language_op(sym->language, \
					    L_PRINTDECL))(sym)
#define printval(sym)			(*language_op(sym->language, \
					    L_PRINTVAL))(sym)
#define printparams(func, frame)	(*language_op(func->language, \
					    L_PRINTPARAMS))(func, frame)
#define evalparams(proc, arglist)	(*language_op(proc->language, \
					    L_EVALPARAMS))(proc, arglist)
#define typematch(type1, type2)		(*language_op(type1->language, \
					    L_TYPEMATCH))(type1, type2)
#define subscript(array, subscripts)	(Node) (*language_op( \
					    array->nodetype->language, \
					    L_BUILDAREF))(array, subscripts)
#define evalindex(array, subscripts)	(*language_op(array->language, \
					    L_EVALAREF))(array, subscripts)

Integer indentlevel;			/* Current indentation */

#endif

#define MAXSUFFIX 8

struct Language {
    String name;
    String suffix[MAXSUFFIX];
    LanguageOperation *op[ord(L_EVALAREF) + 1];
    Language next;
};

private Language head;

/*
 * Initialize language information.
 *
 * The last language initialized will be the default one
 * for otherwise indistinguised symbols.
 */

public language_init()
{
    c_init();
    fortran_init();
    pascal_init();
    modula_init();
    asm_init();
}

/*
 * All languages that can print out a nicely indented
 * record structure share one global 'indentlevel' variable.
 * This routine resets it.
 */
public	reset_indentlevel()
{
	indentlevel = 0;
}

/*
 * Indent a given number of levels
 */
public indent(nesting)
int	nesting;
{
    if (nesting > 0) {
	printf("%*c", nesting*8, ' ');
    }
}

private suffix_match(suffix_array, suffix)
String *suffix_array;
String suffix;
	{
	while (*suffix_array)
		if (streq(*suffix_array++, suffix))
			return true;
	return false;
	}

public Language findlanguage(suffix)
String suffix;
{
    Language lang;

    lang = head;
    if (suffix != nil) {
	while (lang != nil and not suffix_match(lang->suffix, suffix)) {
	    lang = lang->next;
	}
	if (lang == nil) {
	    lang = head;
	}
    }
    return lang;
}

public String language_name(lang)
Language lang;
{
    return (lang == nil) ? "(nil)" : lang->name;
}

public Language language_define(name)
String name;
{
    Language p;
    int i;

    p = new(Language);
    if (p == 0)
	fatal("No memory available. Out of swap space?");
    p->name = name;
    for (i=0; i < MAXSUFFIX; i++)
	    p->suffix[i] = NULL;
    p->next = head;
    head = p;
    return p;
}

public void language_suffix_add(lang, suffix)
Language lang;
String suffix;
	{
	String *suffix_p;

	for (suffix_p = lang->suffix; *suffix_p; suffix_p++)
		;
	if (suffix_p - lang->suffix > MAXSUFFIX)
		panic ("too many suffixes for %s", lang->name);
	*suffix_p = suffix;
	}


public language_setop(lang, op, operation)
Language lang;
LanguageOp op;
LanguageOperation *operation;
{
    checkref(lang);
    assert(ord(op) <= ord(L_EVALAREF));
    lang->op[ord(op)] = operation;
}

public LanguageOperation *language_op(lang, op)
Language lang;
LanguageOp op;
{
    LanguageOperation *o;

    checkref(lang);
    o = lang->op[ord(op)];
    checkref(o);
    return o;
}

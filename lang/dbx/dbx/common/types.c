#ifndef lint
static	char sccsid[] = "@(#)types.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Routines to manipulate the types of variables
 */

#include "defs.h"
#include "symbols.h"
#include "tree.h"
#include "names.h"

public Symbol combitypes(t1, t2)
    Symbol t1, t2;
{
    char newname[1024];
    Name n;
    Symbol s;

    strcat(strcat(strcpy(newname, idnt(t1->name)) , " "), idnt(t2->name));
    if ((n = identname(newname, false)) == nil || (s = lookup(n)) == nil)
	error("no such type");
    return s;
}

private Symbol lookup_tag(cl, tag)
    register Symclass  cl;
    Name tag;
{

    register Symbol s;

    find(s, tag) where
	s->class == TYPEID and s->type->class == cl
    endfind(s);
    if (s==nil)
	error("no such type");
    return s;
}

public Symbol lookup_structtag(tag)
    Name tag;
{
    return lookup_tag(RECORD, tag);
}

public Symbol lookup_uniontag(tag)
    Name tag;
{
    return lookup_tag(UNION, tag);
}

public Symbol lookup_enumtag(tag)
    Name tag;
{
    return lookup_tag(SCAL, tag);
}


/*
* build-abstract-declarator
*/
private Node build_abdcl(n, class)
    Node n;
    Symclass class;
{
    register Symbol *s;

    s = &n->value.sym;
    while ((*s)->class != TYPE)
	s = & (*s)->type;
    *s = newSymbol(nil, 0, class, *s, nil);
    return n;
}

public Node build_ptrdcl(n)
    Node n;
{
    return build_abdcl(n, PTR);
}

public Node build_funcdcl(n)
    Node n;
{
    return build_abdcl(n, FUNC);
}

public Node buildtype(typespec, abdcl)
    Node typespec;
    Node abdcl;
{
    /*
     * We want to make a full type out of a typespec and an
     * abstract declarator.  At the bottom of the declarator is
     * an int type.  It is only a placeholder for the real type
     * from the typespec that we want to plug in.  Do it,
     * and give back the spare pieces.
     */
    register Symbol *s, sp;
    Language clang = findlanguage(".c");
    Language modula2 = findlanguage(".mod");

    if (typespec->value.sym == t_unsigned) {
	typespec = build(O_SYM, lookup(identname("unsigned int", true)));
    }
    s = & abdcl->value.sym;
    while ((*s)->class != TYPE)
	s = & ((*s)->type);
    /* give_back_symbol(*s); */
    *s = typespec->value.sym;
    tfree(typespec);

    sp = abdcl->value.sym ;
    abdcl->nodetype = sp ;

    if( sp->class == IMPORT ) {
	    sp->language = modula2;
    } else {
	while( sp  and  sp->class != TYPE  and  sp->class != TYPEID ) {
	    sp->language = clang;
	    sp = sp->type;
	}
    }
    return abdcl;
}

/*
 * Print leading part of type
 */
public void pre_type(s)
    Symbol s;
{
    Symbol t;

    switch (s->class){
    case TYPEID:
	/* struct, union, enum */
	printf("%s %s ", c_classname(s->type),  idnt(s->name));
	break;
    case TYPE:
	printf("%s ", idnt(s->name));
	break;
    case PTR:
	if (s->type->class != BADUSE) {
	    pre_type(s->type);
	}
	if (s->type->class == FUNC or s->type->class == ARRAY) {
	    printf("(");
	}
	printf("*");
	break;
    case FUNC:
    case ARRAY:
	pre_type(s->type);
	break;
    case RECORD:
    case UNION:
    case SCAL:
	printf("%s ", c_classname(s));
	break;
    case FORWARD:
	t = lookup_forward(s);
	if (t != nil) {
	    pre_type(t);
	} else {
	    printf("%s %s ", c_tagtype(s->symvalue.forwardref.class),
	      idnt(s->symvalue.forwardref.name));
	}
	break;
    default:
	error("unexpected class cast");
    }
}

/*
 * Print trailing part of type
 */
public void post_type(s)
    Symbol s;
{
    Symbol b;

    switch (s->class){
    case TYPEID:
    case TYPE:
    case RECORD:
    case UNION:
    case SCAL:
	break;
    case PTR:
	if (s->type->class == FUNC or s->type->class == ARRAY) {
	    printf(")");
	}
	if (s->type->class != BADUSE) {
	    post_type(s->type);
	}
	break;
    case FUNC:
	printf("()");
	post_type(s->type);
	break;
    case ARRAY:
	b = rtype(s->chain);
	if (b->symvalue.rangev.upper >= 0)
	    printf("[%d]", b->symvalue.rangev.upper+1);
	else
	    printf("[]");
	post_type(s->type);
	break;
    case FORWARD:
	s = lookup_forward(s);
	if (s != nil) {
	    post_type(s);
	}
	break;
    default:
	error("unexpected class cast");
    }
}

/*
 * Print a type rename (cast) in C-like syntax
 */
public void prcast(typer)
    Node typer;
{
    printf("(");
    pre_type(typer->value.sym);
    post_type(typer->value.sym);
    printf(") ");
}

/*
 * Print out the storage class of something.
 * Either "static", "register" or nothing.
 */
public pr_storage(s)
    Symbol s;
{
    if ((char)s->level == (char)2) {
        printf("static ");
    } else if ((char)s->level < (char)0) {
        printf("register ");
    }
}

#ifndef lint
static	char sccsid[] = "@(#)bound.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "cpass2.h"
#include "ctype.h"

#include <sun4/trap.h>

extern int chk_ovfl;

bound_test( p, cookie )
    register NODE *p;
{
    NODE *expr   = p->in.left;
    NODE *bounds = p->in.right;
    NODE * v[2];

    char *regname;
    int  helper;
    char ** tname;
    /* enum {LOWER, UPPER} firsttest, test; */
#   define LOWER 0
#   define UPPER 1
    int firsttest, test;

    static char * utraps[] = { "	tlu	%d\n", "	tgu	%d\n" };
    static char * straps[] = { "	tl	%d\n", "	tg	%d\n" };

    v[LOWER] = bounds->in.left;
    v[UPPER] = bounds->in.right;

    regname = rnames[expr->tn.rval];
    helper = resc[0].tn.rval;

    /*
     * interesting special cases are:
     * 1) lower-bound constant 0
     *    ( can do single compare, use unsigned condition )
     * 2) one or both bounds small constants
     *    ( dont' have to evaluate them into registers to compare )
     */
    if ( v[LOWER]->in.op==ICON && v[LOWER]->tn.name[0]=='\0' && v[LOWER]->tn.lval==0 ) {
	firsttest = UPPER;
	tname = utraps;
    } else {
	firsttest = LOWER;
	tname = ISUNSIGNED(expr->tn.type) ? utraps : straps;
    }
    for ( test = firsttest; test <= UPPER; test += 1 ){
	if (v[test]->in.op == ICON && v[test]->tn.name[0]=='\0' && IN_IMMEDIATE_RANGE(v[test]->tn.lval)){
	    printf("	cmp	%s,%d\n", regname, v[test]->tn.lval );
	}else {
	    v[test]->tn.rall = MUSTDO |  helper;
	    order( v[test], INTAREG );
	    printf("	cmp	%s,%s\n", regname, rnames[helper] );
	    reclaim( v[test], RNULL, FOREFF );
	}
	printf(tname[test], ST_RANGE_CHECK); /* defined in trap.h */
    }
    order( expr, cookie );

}

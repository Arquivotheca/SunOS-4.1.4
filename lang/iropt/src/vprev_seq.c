#ifndef lint
static	char sccsid[] = "@(#)vprev_seq.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * This module defines an "iterator" abstraction for routines
 * that must traverse VAR_REFS of a triple in reverse execution
 * order.
 */

#include <stdio.h>
#include "iropt.h"
#include "misc.h"
#include "recordrefs.h"
#include "vprev_seq.h"

/* private */

#define MAX_VAR_REFS_PER_TRIPLE 10	/* actually, it's more like 2 */
static VAR_REF *var_refs[MAX_VAR_REFS_PER_TRIPLE];
static VAR_REF **refptr;

/*
 * init_vprev_seq(tp) -- initialize a backward sequence
 */
VAR_REF *
init_vprev_seq(tp)
    TRIPLE *tp;
{
    VAR_REF *vp;
    int i;

    i = 0;
    FOR_VAR_REFS(vp, tp) {
	if (i >= MAX_VAR_REFS_PER_TRIPLE) {
	    quit("too many var_refs in this triple");
	    /*NOTREACHED*/
	}
	var_refs[i++] = vp;
    }
    refptr = &var_refs[i];
    return vprev_seq();
}

/*
 * vprev_seq() -- return the previous VAR_REF in
 * execution order.  Returns NULL when the first VAR_REF
 * credited to this triple has already been processed.
 */
VAR_REF *
vprev_seq()
{
    VAR_REF *vp;

    if (refptr == var_refs) {
	/* already processed all VAR_REFS */
	vp = NULL;
    } else {
	vp = *--refptr;
    }
    return vp;
}

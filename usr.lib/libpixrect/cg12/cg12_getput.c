#ifndef lint
static char     sccsid[] =      "@(#)cg12_getput.c	1.1 94/10/31 SMI";
#endif

/* Copyright 1989 Sun Microsystems, Inc. */

#include <pixrect/cg12_var.h>

cg12_get(pr, x, y)
Pixrect            *pr;
int                 x, y;
{
    Pixrect             mempr;

    CG12_PR_TO_MEM(pr, mempr);
    return mem_get(pr, x, y);
}

cg12_put(pr, x, y, value)
Pixrect            *pr;
int                 x, y, value;
{
    Pixrect             mempr;

    CG12_PR_TO_MEM(pr, mempr);
    return mem_put(pr, x, y, value);
}

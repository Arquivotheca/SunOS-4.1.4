#ifndef lint
static char         sccsid[] = "@(#)cg12_stencil.c 1.1 94/10/31 SMI";
#endif

/* Copyright 1990, Sun Microsystems, Inc. */

#include <pixrect/cg12_var.h>

cg12_stencil(dpr, dx, dy, dw, dh, op, stpr, stx, sty, spr, sx, sy)
Pixrect            *dpr;
int                 dx, dy, dw, dh, op;
Pixrect            *stpr;
int                 stx, sty;
Pixrect            *spr;
int                 sx, sy;
{
    Pixrect             dmempr, smempr;

    CG12_PR_TO_MEM(dpr, dmempr);
    CG12_PR_TO_MEM(spr, smempr);
    return mem_stencil(dpr, dx, dy, dw, dh, op, stpr, stx, sty, spr, sx, sy);
}

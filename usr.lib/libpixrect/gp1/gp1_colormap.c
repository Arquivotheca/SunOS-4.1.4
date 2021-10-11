#ifndef lint
static char sccsid[] = "@(#)gp1_colormap.c	1.1  94/10/31  SMI";
#endif

/*
 * Copyright 1989 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sun/gpio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>
#include <pixrect/cg9var.h>
#include <pixrect/gp1cmds.h>
#include <pixrect/gp1reg.h>
#include <pixrect/gp1var.h>

extern char *malloc();

#define SWITCH_OPS(fb, ops)                                     \
    if ((fb) == FBTYPE_SUN2COLOR)                               \
	(ops) = &cg2_ops;                                        \
    else if ((fb) == FBTYPE_SUNROP_COLOR)                       \
	(ops) = &cg9_ops;                                        \
    else                                                        \
	return PIX_ERR

#define PR_TO_CG(pr,cg, ops)                                    \
	if ((pr)->pr_ops->pro_rop == gp1_rop) {                 \
	    (cg) = *(pr);                                       \
	    (cg).pr_ops = ops;	                                \
	    (cg).pr_data = (caddr_t) & gp1_d((pr))->cgpr;       \
	    (pr) = &(cg);                                       \
	}


gp1_putattributes(pr, planes)
    Pixrect        *pr;
    int            *planes;
{
    int             ret;
    struct gp1pr   *gpd = gp1_d(pr);
    Pixrect cgpr;

    cgpr = *pr;
    cgpr.pr_data = (caddr_t) &gpd->cgpr;
    if (gpd->fbtype == FBTYPE_SUN2COLOR) {
	if (!(ret = cg2_putattributes(&cgpr, planes)))
	    GP1_COPY_FROM_CG2(cg2_d(pr), gpd);
    } else if (gpd->fbtype == FBTYPE_SUNROP_COLOR) {
	if (!(ret = cg9_putattributes(&cgpr, planes))) {
	    gpd->cgpr_planes = gpd->cgpr.cg9pr.planes & PIX_ALL_PLANES;	
	    pr->pr_depth = cgpr.pr_depth;
	  }
    }
    return ret;
}

gp1_getattributes(pr, planes)
    Pixrect        *pr;
    int            *planes;
{
    Pixrect         cg;
    struct pixrectops *ops;

    SWITCH_OPS(gp1_d(pr)->fbtype, ops);
    PR_TO_CG(pr, cg, ops);
    return pr_getattributes(pr, planes);
}

/* bunch of wrapper routines */

gp1_putcolormap(pr, index, count, red, green, blue)
    Pixrect        *pr;
    int             index,
                    count;
    u_char         *red,
                   *green,
                   *blue;
{
    Pixrect         cg;
    struct pixrectops *ops;

    pr_ioctl(pr, GP1IO_SCMAP, 0);
    SWITCH_OPS(gp1_d(pr)->fbtype, ops);
    PR_TO_CG(pr, cg, ops);
    return pr_putcolormap(pr, index, count, red, green, blue);
}

gp1_getcolormap(pr, index, count, red, green, blue)
    Pixrect        *pr;
    int             index,
                    count;
    u_char         *red,
                   *green,
                   *blue;
{
    Pixrect         cg;
    struct pixrectops *ops;

    SWITCH_OPS(gp1_d(pr)->fbtype, ops);
    PR_TO_CG(pr, cg, ops);
    return pr_getcolormap(pr, index, count, red, green, blue);
}

gp1_put(pr, x, y, value)
    register Pixrect *pr;
    register int    x,
                    y;
    int             value;
{
    Pixrect         cg;
    struct pixrectops *ops;

    SWITCH_OPS(gp1_d(pr)->fbtype, ops);
    PR_TO_CG(pr, cg, ops);
    return pr_put(pr, x, y, value);
}

gp1_get(pr, x, y)
    register Pixrect *pr;
    register int    x,
                    y;
{
    Pixrect         cg;
    struct pixrectops *ops;

    SWITCH_OPS(gp1_d(pr)->fbtype, ops);
    PR_TO_CG(pr, cg, ops);
    return pr_get(pr, x, y);
}

gp1_batchrop(dpr, dx, dy, op, src, count)
    Pixrect        *dpr;
    int             dx,
                    dy,
                    op;
    struct pr_prpos *src;
    int             count;
{
    struct pixrectops *ops;
    Pixrect         cg;

    SWITCH_OPS(gp1_d(dpr)->fbtype, ops);
    PR_TO_CG(dpr, cg, ops);
    return pr_batchrop(dpr, dx, dy, op, src, count);
}

gp1_stencil(dpr, dx, dy, dw, dh, op, stpr, stx, sty, spr, sx, sy)
    Pixrect        *dpr;
    int             dx,
                    dy;
    register int    dw,
                    dh,
                    op;
    Pixrect        *stpr;
    int             stx,
                    sty;
    Pixrect        *spr;
    int             sx,
                    sy;
{
    struct pixrectops *ops;
    Pixrect         cg;

    SWITCH_OPS(gp1_d(dpr)->fbtype, ops);
    PR_TO_CG(dpr, cg, ops);
    return pr_stencil(dpr, dx, dy, dw, dh, op, stpr, stx, sty, spr, sx, sy);
}

gp1_polypoint(pr, dx, dy, n, ptlist, op)
	Pixrect *pr;
	int dx, dy, n;
	register struct pr_pos *ptlist;
	int op;
{
    struct pixrectops *ops;
    Pixrect         cg;


    if (gp1_d(pr)->fbtype == FBTYPE_SUN2COLOR)
	return cg2_polypoint(pr, dx, dy, n, ptlist, op);
    else if (gp1_d(pr)->fbtype == FBTYPE_SUNROP_COLOR) {
	SWITCH_OPS(gp1_d(pr)->fbtype, ops);
	PR_TO_CG(pr, cg, ops);
	return mem_polypoint(pr, dx, dy, n, ptlist, op);
    }
}

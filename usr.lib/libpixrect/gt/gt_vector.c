#ifndef lint
static char	sccsid[] =	"@(#)gt_vector.c	1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1989-1990, Sun Microsystems, Inc.
 */

/*
 * Hawk vector 
 */
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/gtvar.h>
#include <pixrect/pr_planegroups.h>

#define PT_CLIP(x_o, min, max) \
   ( (x_o) < min ? ((x_o) = min) : ( (x_o) > max ? ((x_o) = max) : ((x_o) = (x_o))))

#define _AMIN(x, y)  ((x) < (y)? (x):(y))

#define _ABS(x) ((x) < 0 ? -(x) : (x))

gt_vector(pr, x0, y0, x1, y1, op, color)
    register Pixrect *pr;
    int x0, y0, x1, y1;
    int op;
    int color;
{
    register int    dx, dy;
    register int    minval ;
    u_int           col;
    struct gt_data *gtd;
    gtd = gt_d (pr);


    dx = x1 - x0;
    dy = y1 - y0;

    if ((dx == 0) && (dy == 0))
	return PIX_ERR;

    switch (PIX_ATTRGROUP (gtd->planes)) {
    case PIXPG_8BIT_COLOR:
	col = (color << GT_OVERLAY_SHIFT24);
	if (PIX_OPCOLOR(op)) col = (PIX_OPCOLOR(op) << GT_OVERLAY_SHIFT24);
	gt_set_common_reg (pr, col, HKPBM_HFBAS_CURSOR_BYTE,
			    HKPP_ROP_SRC, gtd->buf,
			    (gtd->planes & PIX_ALL_PLANES));
	break;
    case PIXPG_24BIT_COLOR:
	col = color;
	if (PIX_OPCOLOR(op)) col = PIX_OPCOLOR(op);
	gt_set_common_reg (pr, col, HKPBM_HFBAS_IMAGE,
			    HKPP_ROP_SRC, gtd->buf,
			    (gtd->planes & PIX_ALL_PLANES));
	break;
    default:
	return PIX_ERR;
    }
    return mem_vector (pr, x0, y0, x1, y1, op, color);


}

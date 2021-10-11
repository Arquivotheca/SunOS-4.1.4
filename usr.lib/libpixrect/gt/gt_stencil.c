#ifndef lint
static char     sccsid[] =      "@(#)gt_stencil.c	1.1 94/10/31 SMI";
#endif


/*
 * Copyright 1990-1991 by Sun Microsystems, Inc.
 */

/*
 * Hawk Color get and put single pixel
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_planegroups.h>
#include <gt/gtvar.h>

extern int mem_stencil();

gt_stencil (dpr, dx, dy, dw, dh, op, stpr, stx, sty, spr, sx, sy)
    Pixrect        *dpr;
    int             dx,dy;
    int		    dw, dh, op;
    Pixrect        *stpr;
    int             stx, sty;
    Pixrect        *spr;
    int             sx, sy;
{
    struct gt_data *dprd = gt_d (dpr);
    
    switch (PIX_ATTRGROUP (dprd->planes)) {
	case PIXPG_8BIT_COLOR:
	    gt_set_common_reg(dpr,
		(PIX_OPCOLOR (op) << GT_OVERLAY_SHIFT24), 
		HKPBM_HFBAS_CURSOR_BYTE,
		PIXOP_OP(op), dprd->buf, 
		(PIX_ALL_PLANES & dprd->planes)); 
	break;
	case PIXPG_24BIT_COLOR:
	    gt_set_common_reg(dpr,
		PIX_OPCOLOR (op), 
		HKPBM_HFBAS_IMAGE,
		PIXOP_OP(op), dprd->buf, 
		(PIX_ALL_PLANES & dprd->planes)); 
	break;
	case PIXPG_WID:
	    gt_set_common_reg(dpr,
		PIX_OPCOLOR (op), 
		HKPBM_HFBAS_WINDOW,
		PIXOP_OP(op), dprd->buf, 
		(HKPP_WWM_MASK & HKPP_WWM_WID));
	break;
    }
    return mem_stencil(dpr, dx, dy, dw, dh, op, stpr, stx, sty, spr, sx, sy);
}

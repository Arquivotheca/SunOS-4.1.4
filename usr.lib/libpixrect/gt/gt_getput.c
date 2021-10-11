#ifndef lint
static char     sccsid[] =      "@(#)gt_getput.c	1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1989-1990 by Sun Microsystems, Inc.
 */

/*
 * Hawk Color get and put single pixel
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memreg.h>
#include <gt/gtvar.h>
#include <pixrect/pr_planegroups.h>


gt_get(pr, x, y)
	Pixrect *pr;
	int x, y;
{
    struct gt_data *dprd = gt_d (pr);
    register int    dst_pl = dprd->planes;

   switch (PIX_ATTRGROUP (dst_pl)) {
	case PIXPG_8BIT_COLOR:
	    gt_set_common_reg(pr, 0, HKPBM_HFBAS_CURSOR_BYTE,
		    HKPP_ROP_SRC, dprd->buf, 
		    (PIX_ALL_PLANES & dst_pl));
	break;
	case PIXPG_24BIT_COLOR:
	    gt_set_common_reg(pr, 0, HKPBM_HFBAS_IMAGE,
		    HKPP_ROP_SRC, dprd->buf, 
		    (PIX_ALL_PLANES & dst_pl));
	break;
	case PIXPG_WID:
	    gt_set_common_reg(pr, 0, HKPBM_HFBAS_WINDOW,
		    HKPP_ROP_SRC, 0, (HKPP_WWM_MASK & HKPP_WWM_WID));
	break;
	case (PIXPG_ZBUF):
	    gt_set_common_reg(pr, 0,HKPBM_HFBAS_DEPTH,
		HKPP_ROP_SRC, 0, (PIX_ALL_PLANES & dst_pl));
	break;
        case (PIXPG_CURSOR):
        case (PIXPG_CURSOR_ENABLE):
        {
            int ret_val;
            register int old_as, old_buf;

            gt_get_as_reg(dprd->gt_rp,old_as);
            gt_get_buf_reg(dprd->gt_fbi,old_buf);
            gt_set_as_reg(dprd->gt_rp, HKPBM_HFBAS_CURSOR_BYTE);
            gt_set_buf_reg(dprd->gt_fbi,0);

            gt_waitidle(dprd->gt_rp, dprd->gt_fe_timeout);
            if (mem_get (pr, x, y))
                  ret_val = PIX_ERR;
            else
                 ret_val = 0;

            /* restore the old state */
            gt_set_buf_reg(dprd->gt_fbi,old_buf);
            gt_set_as_reg(dprd->gt_rp, old_as);

            return (ret_val);   
	}
        break;
 
	default:
	    return PIX_ERR;
    }
    return (mem_get(pr, x, y));
}

gt_put(pr, x, y, value)
	Pixrect *pr;
	int x, y;
	register int value;
{
    struct gt_data *dprd = gt_d (pr);
    register int    dst_pl = dprd->planes;

    switch (PIX_ATTRGROUP (dst_pl)) {
	case PIXPG_8BIT_COLOR:
	    gt_set_common_reg(pr, (value << GT_OVERLAY_SHIFT24),
		     HKPBM_HFBAS_CURSOR_BYTE, HKPP_ROP_SRC, dprd->buf, 
		    (PIX_ALL_PLANES & dst_pl));
	break;
	case PIXPG_24BIT_COLOR:
	    gt_set_common_reg(pr, value,
		     HKPBM_HFBAS_IMAGE, HKPP_ROP_SRC, dprd->buf, 
		    (PIX_ALL_PLANES & dst_pl));
	break;
	case PIXPG_WID:
	    gt_set_common_reg(pr, value,
		     HKPBM_HFBAS_WINDOW, HKPP_ROP_SRC, 0, 
		    (HKPP_WWM_MASK & HKPP_WWM_WID));
	break;
	case (PIXPG_ZBUF):
	    gt_set_common_reg(pr, value,HKPBM_HFBAS_DEPTH,
		HKPP_ROP_SRC, 0, (PIX_ALL_PLANES & dst_pl));
	break;
        case (PIXPG_CURSOR):
        case (PIXPG_CURSOR_ENABLE):
        {
            int ret_val;
            register int old_as, old_buf;
                    
            gt_get_as_reg(dprd->gt_rp,old_as);
            gt_get_buf_reg(dprd->gt_fbi,old_buf);
            gt_set_as_reg(dprd->gt_rp, HKPBM_HFBAS_CURSOR_BYTE);
            gt_set_buf_reg(dprd->gt_fbi,0);
                    
            gt_waitidle(dprd->gt_rp, dprd->gt_fe_timeout);
            if (mem_put(pr, x, y, value))
                  ret_val = PIX_ERR;
            else    
                 ret_val = 0;
 
            /* restore the old state */
            gt_set_buf_reg(dprd->gt_fbi,old_buf);
            gt_set_as_reg(dprd->gt_rp, old_as);
 
            return (ret_val);
        }
        break;
	default:
	    return PIX_ERR;
    }
    return (mem_put(pr, x, y, value));
}

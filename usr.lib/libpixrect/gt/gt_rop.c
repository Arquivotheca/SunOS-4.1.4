#ifndef lint
static char        sccsid[] =     "@(#)gt_rop.c	1.1 94/10/31 SMI";
#endif

/* Copyright 1989 Sun Microsystems, Inc. */
#include <sys/types.h>
#include <pixrect/pixrect.h>		       /* PIX		 */
#include <pixrect/gtvar.h>		       /* gt_d(pr)  	 */
#include <pixrect/memvar.h>		       /* mem_ops	 */
#include <pixrect/pr_planegroups.h>
#include <machine/param.h>

/* rop routine starts here */
gt_rop (dpr, dx, dy, dw, dh, op, spr, sx, sy)
    Pixrect        *dpr,*spr;
    int             dx,dy,dw,dh, op, sx, sy;
{
    register struct gt_data	*sprd;
    register struct gt_data	*dprd = gt_d(dpr);
    register int    dst_dep = dpr->pr_depth;
    register int    dst_pl  = dprd->planes;
    register int    dst_pg = PIX_ATTRGROUP(dst_pl);
    int err;

#ifdef KERNEL
    if ((dst_dep >1) || (spr && (spr->pr_depth > 1))) {
	printf("kernel: gt_rop error: attempt at illegal rop\n");
	return (0); /* PIX_ERR??*/
    }
#endif KERNEL    
    /* Ensure all coordinates are within screen/region bounds */
    if (!(op & PIX_DONTCLIP)) {
	struct pr_subregion dst;
	struct pr_prpos src;

	dst.pr = dpr;
	dst.pos.x = dx;
	dst.pos.y = dy;
	dst.size.x = dw;
	dst.size.y = dh;

	if ((src.pr = spr) != 0) {
		src.pos.x = sx;
		src.pos.y = sy;
	}
	(void) pr_clip(&dst, &src);

	dx = dst.pos.x;
	dy = dst.pos.y;
	dw = dst.size.x;
	dh = dst.size.y;

	if (spr != 0) {
		sx = src.pos.x;
		sy = src.pos.y;
	}
    }
    if (dw <= 0 || dh <= 0 )
	return 0;    

/*
 * Case 1 -- we are the source for the rasterop, but not the destination.
 * Pretend to be a memory pixrect and punt to dest's rop routine.
 */
    if (dpr->pr_ops->pro_rop != gt_rop) {
	register    int	src_pl;
	struct pixrect mpr;
		
	if (spr == 0 || (spr->pr_ops->pro_rop != gt_rop)) return PIX_ERR;
	mpr = *spr;
	mpr.pr_ops = &mem_ops;
	sprd = gt_d(spr);
	src_pl = sprd->planes;
	
	switch (PIX_ATTRGROUP(src_pl)) {
	    case (PIXPG_8BIT_COLOR):
		gt_set_common_reg(spr, 0, HKPBM_HFBAS_CURSOR_BYTE,
			PIXOP_OP(op), sprd->buf,(PIX_ALL_PLANES & src_pl));
		err = pr_rop (dpr, dx, dy, dw, dh, op, &mpr, sx, sy);
	    break;
	    case (PIXPG_24BIT_COLOR):
		gt_set_common_reg(spr, 0, HKPBM_HFBAS_IMAGE, 
			PIXOP_OP(op), sprd->buf,(PIX_ALL_PLANES &src_pl));
		err = pr_rop (dpr, dx, dy, dw, dh, op, &mpr, sx, sy);
	    break;
	    case (PIXPG_WID):
		gt_set_common_reg(spr, 0,HKPBM_HFBAS_WINDOW,
			PIXOP_OP(op), 0, (HKPP_WWM_MASK & HKPP_WWM_WID));
		err = pr_rop (dpr, dx, dy, dw, dh, op, &mpr, sx, sy);
	    break;	    
	    case (PIXPG_CURSOR):
	    case (PIXPG_CURSOR_ENABLE):
	    { 
                    register int old_as, old_buf;
		    
                    gt_get_as_reg(sprd->gt_rp,old_as);

                    gt_waitidle(sprd->gt_rp, sprd->gt_fe_timeout);

		    gt_get_buf_reg(sprd->gt_fbi,old_buf);
                    gt_set_as_reg(sprd->gt_rp, HKPBM_HFBAS_CURSOR_BYTE);

		    gt_set_buf_reg(sprd->gt_fbi,0);
		    
                    err = mem_rop (dpr, dx, dy, dw, dh, op, &mpr, sx,sy);
 
                    /* restore the old state */
		    gt_set_buf_reg(sprd->gt_fbi,old_buf);
                    gt_set_as_reg(sprd->gt_rp, old_as);
 
	    }    
	    break;
	    case (PIXPG_ZBUF):
		gt_set_common_reg(spr, 0,HKPBM_HFBAS_DEPTH,
			PIXOP_OP(op), 0, (PIX_ALL_PLANES & src_pl));
		err = pr_rop (dpr, dx, dy, dw, dh, op, &mpr, sx, sy);
	    break;
	    default:
		return PIX_ERR;
	}
	gt_set_z_ctrl(sprd->gt_fbi);
	return (err); 
    }

/*
 * Case 2 -- writing to the hawkcolor frame buffer this consists of 4 
 * different
 * cases depending on where the data is coming from:  from nothing, from
 * memory, from another type of pixrect, and from the frame buffer itself.
 * By the time we get here, destination is definitely hawk. Remember to
 * add the change in control if in stencil mode.  dont know yet how to.
 * 
 */	

    { /* start case 2 series */
	H_rp	 *rp = (H_rp *)dprd->gt_rp;
	H_fbi    *fbi = (H_fbi *)dprd->gt_fbi;
	H_fbi_go *fbi_go = (H_fbi_go *)dprd->gt_fbi_go;
	H_fe_hold *fe_hold = (H_fe_hold *)dprd->gt_fe_h;

	short   dir;
	u_int group_mask = 0;
 	/*
	 * Case 2a: source pixrect is specified as null.
	 * In this case we interpret the source as color.
	 * Use hawk's block fill calls
	 */
	 if (spr == 0) {
	    /* Translate the origin now */
    	    dx += mprp_d(dpr)->mpr.md_offset.x;
            dy += mprp_d(dpr)->mpr.md_offset.y;

	    switch (dst_pg) {
	        case PIXPG_8BIT_COLOR: 
		    group_mask = (HKPF_FDA_PG_MASK & HKPF_FDA_PG_IMAGE);
		    gt_set_common_reg(dpr, 
				(PIX_OPCOLOR(op) << GT_OVERLAY_SHIFT24),
				HKPBM_HFBAS_CURSOR_BYTE, PIXOP_OP(op), 
				dprd->buf, (PIX_ALL_PLANES & dst_pl));
	            gt_set_fill_regs(rp, fbi, fbi_go, fe_hold, dw, dh, dx, 
		       dy, group_mask, dprd->gt_fe_frozen, dprd->gt_fe_timeout);
		    gt_waitidle(rp, dprd->gt_fe_timeout);
                    gt_rel_fe_hold(fe_hold);
		    err = 0;
		break;
		case PIXPG_24BIT_COLOR:
		    group_mask = (HKPF_FDA_PG_MASK & HKPF_FDA_PG_IMAGE);
		    gt_set_common_reg(dpr, PIX_OPCOLOR(op),
				HKPBM_HFBAS_IMAGE, PIXOP_OP(op), 
				dprd->buf,(PIX_ALL_PLANES & dst_pl));
	            gt_set_fill_regs(rp, fbi, fbi_go, fe_hold, dw, dh, dx, 
		       dy, group_mask, dprd->gt_fe_frozen, dprd->gt_fe_timeout);
		    gt_waitidle(rp, dprd->gt_fe_timeout);
                    gt_rel_fe_hold(fe_hold);
		    err = 0;
		break;
		case PIXPG_WID:
		    group_mask = (HKPF_FDA_PG_MASK & HKPF_FDA_PG_WINDOW); 
		    gt_set_common_reg(dpr, PIX_OPCOLOR(op),
				HKPBM_HFBAS_WINDOW, PIXOP_OP(op),
				0, (HKPP_WWM_MASK & HKPP_WWM_WID));
	            gt_set_fill_regs(rp, fbi, fbi_go, fe_hold, dw, dh, dx, 
		       dy, group_mask, dprd->gt_fe_frozen, dprd->gt_fe_timeout);
		    gt_waitidle(rp, dprd->gt_fe_timeout);
                    gt_rel_fe_hold(fe_hold);
		    err = 0;

		break;
		case PIXPG_ZBUF:
		    group_mask = (HKPF_FDA_PG_MASK & HKPF_FDA_PG_DEPTH); 
		    gt_set_common_reg(dpr, PIX_OPCOLOR(op),
				HKPBM_HFBAS_DEPTH, PIXOP_OP(op),
				0, (PIX_ALL_PLANES & dst_pl));
		    gt_set_fill_regs(rp,fbi, fbi_go, fe_hold, dw, dh, 
			    dx, dy, group_mask, dprd->gt_fe_frozen,
			    dprd->gt_fe_timeout);
		    gt_waitidle(rp, dprd->gt_fe_timeout);
		    gt_rel_fe_hold(fe_hold);
		    gt_set_z_ctrl(fbi);
		    err = 0;
		break;
		case PIXPG_CURSOR:		
		case PIXPG_CURSOR_ENABLE:
		{
			register int old_as, old_buf;

			gt_get_as_reg(dprd->gt_rp,old_as);
			gt_set_as_reg(dprd->gt_rp, HKPBM_HFBAS_CURSOR_BYTE);

			gt_waitidle(dprd->gt_rp, dprd->gt_fe_timeout);

			gt_get_buf_reg(dprd->gt_fbi,old_buf);
			gt_set_buf_reg(dprd->gt_fbi,0);
			
			err = mem_rop (dpr, dx, dy, dw, dh, op, spr, sx,sy);

			/* restore the old state */
			gt_set_buf_reg(dprd->gt_fbi,old_buf);
			gt_set_as_reg(dprd->gt_rp,old_as);
		}
		break;
		default:
		    err = PIX_ERR;
	    }
	    return (err);
	 } 
	 /*
	  * Case 2b: rasterop within the frame buffer. 
	  * Use the hawk  block copy mode. this is allowed only if the
	  * src and dest are within the same plane group.  else
	  * report error. 
	 */
         sprd = gt_d(spr);
	 if ((spr->pr_ops->pro_rop == gt_rop) && (sprd->gt_fbi == fbi)){
	    register	u_int   extra = dw % GT_COPY_HACK;
	    register	int src_pl = sprd->planes;
	    register	int src_pg = PIX_ATTRGROUP(src_pl) ;
            int		stereo;
	    
	    /* Translate the origin now */
            dx += mprp_d(dpr)->mpr.md_offset.x;
            dy += mprp_d(dpr)->mpr.md_offset.y;
            sx += mprp_d(spr)->mpr.md_offset.x;
            sy += mprp_d(spr)->mpr.md_offset.y;

	    if (src_pg == dst_pg) {

		stereo = (dprd->gt_stereo & (GT_DISP_STEREO | FB_WIN_STEREO));
		/* To deal with some hw problem, bill's workaround*/
		if (extra) {
		    gt_set_vwclip(fbi, dx, (dx+dw-1));
		    extra = GT_COPY_HACK - extra;
		    dw += extra;
		}	
		/* If the direction is from bottom to top,
		 * sx, sy, dx and dy should change
		 */
		if (dy > sy) {  
		    dir = 1;
		    dx += dw-1; dy += dh-1;
		    sx += dw-1; sy += dh-1;
		} else if (dy < sy) { 
		    dir = 0;
		} else if (dx > sx) {		     /* dy=dx */
		    dir = 1;
		    dx += dw-1; dy += dh-1;
		    sx += dw-1; sy += dh-1;
		} else 
		    dir = 0;		    /* dy = dx & dx < sx */


		switch (src_pg) {
		    case PIXPG_8BIT_COLOR:
			group_mask =(HKPF_CSA_PG_MASK & HKPF_CSA_PG_IMAGE);
			gt_set_common_reg(dpr, PIXOP_COLOR(op), 
				    HKPBM_HFBAS_CURSOR_BYTE, PIXOP_OP(op), 
				    dprd->buf, (PIX_ALL_PLANES & dst_pl));
			if (stereo) {

			    /* Move the left eye */
			    gt_set_stereo(fbi, 
			      (HKPP_SWE_MASK&(HKPP_SWE_STEREO|~HKPP_SWE_RIGHT)));
			    gt_set_copy_regs(rp, fbi, fbi_go, fe_hold, sx, sy,
				group_mask, dw, dh, dir, dx, dy, 
				dprd->gt_fe_frozen, dprd->gt_fe_timeout);
			    gt_waitidle(rp, dprd->gt_fe_timeout);
			    gt_rel_fe_hold(fe_hold);
			    /* Move the right eye */
			    gt_set_stereo(fbi, 
			    (    HKPP_SWE_MASK&(HKPP_SWE_STEREO|HKPP_SWE_RIGHT)));
			    gt_set_copy_regs(rp, fbi, fbi_go, fe_hold, sx, sy,
				group_mask, dw, dh, dir, dx, dy, 
				dprd->gt_fe_frozen, dprd->gt_fe_timeout);
			    gt_waitidle(rp, dprd->gt_fe_timeout);
			    gt_rel_fe_hold(fe_hold);
			    gt_set_stereo(fbi, (HKPP_SWE_MASK&HKPP_SWE_MONO));
			    gt_set_vwclip(fbi, 0, HKPP_VCXY_MASK );

			} else {

			    gt_set_copy_regs(rp, fbi, fbi_go, fe_hold, sx, sy,
			      group_mask, dw, dh, dir, dx, dy, dprd->gt_fe_frozen,
				dprd->gt_fe_timeout);
			    gt_waitidle(rp, dprd->gt_fe_timeout);
			    gt_rel_fe_hold(fe_hold);
			    gt_set_vwclip(fbi, 0, HKPP_VCXY_MASK );
			}
		    break;
		    case PIXPG_24BIT_COLOR:
		        group_mask =(HKPF_CSA_PG_MASK & HKPF_CSA_PG_IMAGE);
			gt_set_common_reg(dpr, PIXOP_COLOR(op), 
				    HKPBM_HFBAS_IMAGE, PIXOP_OP(op), 
				    dprd->buf, (PIX_ALL_PLANES & dst_pl));
			if (stereo) {
			    gt_set_stereo(fbi, 
			    (HKPP_SWE_MASK&(HKPP_SWE_STEREO|~HKPP_SWE_RIGHT)));
			    if (!(sprd->mpg_fcs & HKPP_MPG_FCS_DISABLE))
					gt_set_fcs_copy(sprd->gt_fbi);
			    gt_set_copy_regs(rp, fbi, fbi_go, fe_hold, sx, sy,
				group_mask, dw, dh, dir, dx, dy, 
				dprd->gt_fe_frozen, dprd->gt_fe_timeout);
			    gt_waitidle(rp, dprd->gt_fe_timeout);
			    gt_rel_fe_hold(fe_hold);
			    
			    /* Move the right eye */
			    gt_set_stereo(fbi, 
			    (HKPP_SWE_MASK&(HKPP_SWE_STEREO|HKPP_SWE_RIGHT)));
			    if (!(sprd->mpg_fcs & HKPP_MPG_FCS_DISABLE))
					gt_set_fcs_copy(sprd->gt_fbi);
			    gt_set_copy_regs(rp, fbi, fbi_go, fe_hold, sx, sy,
				group_mask, dw, dh, dir, dx, dy, 
				dprd->gt_fe_frozen, dprd->gt_fe_timeout);
			    gt_waitidle(rp, dprd->gt_fe_timeout);
			    gt_rel_fe_hold(fe_hold);
			    gt_set_stereo(fbi, (HKPP_SWE_MASK&HKPP_SWE_MONO));
			    gt_set_vwclip(fbi, 0, HKPP_VCXY_MASK );

			} else {
			    gt_set_copy_regs(rp, fbi, fbi_go, fe_hold, sx, sy,
			      group_mask, dw, dh, dir, dx, dy, dprd->gt_fe_frozen,
				dprd->gt_fe_timeout);
			    gt_waitidle(rp, dprd->gt_fe_timeout);
			    gt_rel_fe_hold(fe_hold);
			    gt_set_vwclip(fbi, 0, HKPP_VCXY_MASK );
			}
		    break;
		    case PIXPG_WID:
		        group_mask=(HKPF_CSA_PG_MASK & HKPF_CSA_PG_WINDOW);
			gt_set_common_reg(dpr, PIXOP_COLOR(op), 
				    HKPBM_HFBAS_WINDOW, PIXOP_OP(op), 
				    0, (HKPP_WWM_MASK & HKPP_WWM_WID));
			if (!(sprd->mpg_fcs & HKPP_MPG_FCS_DISABLE))
			    gt_set_fcs_copy(sprd->gt_fbi);
			if (stereo)  {

    			    /* Move the left eye */
			    gt_set_stereo(fbi, 
			    (HKPP_SWE_MASK&(HKPP_SWE_STEREO|~HKPP_SWE_RIGHT)));
			    if (!(sprd->mpg_fcs & HKPP_MPG_FCS_DISABLE))
					gt_set_fcs_copy(sprd->gt_fbi);
			    gt_set_copy_regs(rp, fbi, fbi_go, fe_hold, sx, sy,
				group_mask, dw, dh, dir, dx, dy, 
				dprd->gt_fe_frozen, dprd->gt_fe_timeout);
			    gt_waitidle(rp, dprd->gt_fe_timeout);
			    gt_rel_fe_hold(fe_hold);
			    
			    /* Move the right eye */
			    gt_set_stereo(fbi, 
			    (HKPP_SWE_MASK&(HKPP_SWE_STEREO|HKPP_SWE_RIGHT)));
			    if (!(sprd->mpg_fcs & HKPP_MPG_FCS_DISABLE))
					gt_set_fcs_copy(sprd->gt_fbi);
			    gt_set_copy_regs(rp, fbi, fbi_go, fe_hold, sx, sy,
				group_mask, dw, dh, dir, dx, dy, 
				dprd->gt_fe_frozen, dprd->gt_fe_timeout);
			    gt_waitidle(rp, dprd->gt_fe_timeout);
			    gt_rel_fe_hold(fe_hold);
			    gt_set_stereo(fbi, (HKPP_SWE_MASK&HKPP_SWE_MONO));
			    gt_set_vwclip(fbi, 0, HKPP_VCXY_MASK );

			} else {
			    gt_set_copy_regs(rp, fbi, fbi_go, fe_hold, sx, sy,
			      group_mask, dw, dh, dir, dx, dy, dprd->gt_fe_frozen,
				dprd->gt_fe_timeout);
			    gt_waitidle(rp, dprd->gt_fe_timeout);
			    gt_rel_fe_hold(fe_hold);
			    gt_set_vwclip(fbi, 0, HKPP_VCXY_MASK );
			}
		    break;
		    case PIXPG_ZBUF:
		        group_mask=(HKPF_CSA_PG_MASK & HKPF_CSA_PG_DEPTH);
			gt_set_common_reg(dpr, PIXOP_COLOR(op), 
				    HKPBM_HFBAS_DEPTH, PIXOP_OP(op), 
				    0, (PIX_ALL_PLANES & dst_pl));
			if (stereo) {

			    /* Move the left eye */
			    gt_set_stereo(fbi, 
			    (HKPP_SWE_MASK&(HKPP_SWE_STEREO|~HKPP_SWE_RIGHT)));
			    gt_set_copy_regs(rp, fbi, fbi_go, fe_hold, sx, sy,
				group_mask, dw, dh, dir, dx, dy, 
				dprd->gt_fe_frozen, dprd->gt_fe_timeout);
			    gt_waitidle(rp, dprd->gt_fe_timeout);
			    gt_rel_fe_hold(fe_hold);
			    /* Move the right eye */
			    gt_set_stereo(fbi, 
			    (HKPP_SWE_MASK&(HKPP_SWE_STEREO|HKPP_SWE_RIGHT)));
			    gt_set_copy_regs(rp, fbi, fbi_go, fe_hold, sx, sy,
				group_mask, dw, dh, dir, dx, dy, 
				dprd->gt_fe_frozen, dprd->gt_fe_timeout);
			    gt_waitidle(rp, dprd->gt_fe_timeout);
			    gt_rel_fe_hold(fe_hold);
			    gt_set_stereo(fbi, (HKPP_SWE_MASK&HKPP_SWE_MONO));
			    gt_set_vwclip(fbi, 0, HKPP_VCXY_MASK );

			} else {
			    gt_set_copy_regs(rp, fbi, fbi_go, fe_hold, sx, sy,
			      group_mask, dw, dh, dir, dx, dy, dprd->gt_fe_frozen,
			      dprd->gt_fe_timeout);
			    gt_waitidle(rp, dprd->gt_fe_timeout);
			    gt_rel_fe_hold(fe_hold);
			    gt_set_vwclip(fbi, 0, HKPP_VCXY_MASK );
			    gt_set_z_ctrl(fbi);
			}
		    break;
		    default:
			return PIX_ERR;
		}

		return (0);
	    }
	    else {
		    /* the src and dst are in diff planegroups. no! no!*/
		    return PIX_ERR;
	    }
	 }
		
	/*
	 * Case 2c: Source is some other kind of pixrect, but not
	 * memory.  Ask the other pixrect to read itself into
	 * temporary memory to make the problem easier. yes???
	 * This case should not occur as we do not support
	 * multiple color headed systems. or so the story goes.
	 */ 
	 if (MP_NOTMPR(spr)) {
#ifndef KERNEL	   
	    Pixrect *tpr;
	    tpr = spr;
	    if (! (spr = (Pixrect *) mem_create(dw, dh, spr->pr_depth)))
			return (PIX_ERR);
	    if ((*tpr->pr_ops->pro_rop)(spr, 0, 0, dw, dh,
			PIX_SRC | PIX_DONTCLIP, tpr, sx, sy)) {
		(void) mem_destroy(spr);
		return (PIX_ERR);
	    }
	    sx = 0;    sy = 0;
	    if (tpr) (void) mem_destroy(spr);
#endif !KERNEL
	} else { /*yes mempr */
	/*
	 * Case 2d: Source is a memory pixrect.
	 * We can handle this case because the format of memory
	 * pixrects is public knowledge. There are 4 cases here-
	 * Pixel transfer - 32-to-32 bits
	 * byte transfer  - 8-to-8 bits
	 * Stencil mode	  - 1-to-8/32 bits
	 * Mono mode	  - 1-1 - cursor planes
	 * 32-to-8, 8-to-32, 8-to-1 etc are not allowed.
	 */		
	    int	    src_dep = spr->pr_depth;
	    struct pixrect     dmpr, smpr;

	    dmpr = *dpr;
	    dmpr.pr_ops = &mem_ops;
            smpr = *spr;
	    smpr.pr_ops = &mem_ops;

	    switch (src_dep) { /* mem->hawk  */
		case GT_DEPTH_24:
		    if (dst_dep != GT_DEPTH_24) return PIX_ERR;
		    
		    /* Pixel access mode in image planes */
		    switch (dst_pg) {	
			case PIXPG_24BIT_COLOR:
			    gt_set_common_reg(dpr, PIXOP_COLOR(op), 
					HKPBM_HFBAS_IMAGE, HKPP_ROP_SRC, 
					dprd->buf, (PIX_ALL_PLANES & dst_pl));
			break;
			case PIXPG_WID:
			    gt_set_common_reg(dpr, PIXOP_COLOR(op), 
					HKPBM_HFBAS_WINDOW, HKPP_ROP_SRC, 
					0, (HKPP_WWM_MASK & HKPP_WWM_WID));
			break;
			case PIXPG_ZBUF:
			    gt_set_common_reg(dpr, PIXOP_COLOR(op), 
					HKPBM_HFBAS_DEPTH, HKPP_ROP_SRC, 
					0, (PIX_ALL_PLANES & dst_pl));
			break;
			default:
			    return PIX_ERR;
		    }
		    err = mem_rop(&dmpr, dx, dy, dw, dh, op, &smpr, sx, sy); 
		    gt_set_z_ctrl(fbi);
		break;
		case  GT_DEPTH_8 :
		    if (dst_dep != GT_DEPTH_8) return PIX_ERR;
		    gt_set_common_reg(dpr, PIXOP_COLOR(op),
			    (HKPBM_HFBAS_MASK & HKPBM_HFBAS_CURSOR_BYTE), 
			    HKPP_ROP_SRC, dprd->buf, (PIX_ALL_PLANES & dst_pl));
		    err = mem_rop(&dmpr, dx, dy, dw, dh, op, &smpr, sx, sy);
    		break;
		case GT_DEPTH_1:
		{   /* case 1 starts here */
		
		    register int opcode = PIXOP_OP(op);
		    struct mpr_data *mprd = mpr_d(spr);
		    struct mpr_data *smpr2 = &mprp_d(spr)->mpr;
		    register int src_lb = (smpr2->md_linebytes);
    		    
		    /* Translate the origin now */
                    dx += mprp_d(dpr)->mpr.md_offset.x;
                    dy += mprp_d(dpr)->mpr.md_offset.y;
                    sx += mprp_d(spr)->mpr.md_offset.x;
                    sy += mprp_d(spr)->mpr.md_offset.y;

		    if (mprd->md_flags & MP_REVERSEVIDEO) 
				opcode = PIX_OP_REVERSESRC(opcode);

		    switch (dst_dep) { /*1-> */
			case GT_DEPTH_24:
			case GT_DEPTH_8:
			{ 
			   /* use hawk's stencil mode */
			    register u_int lm, rm;
			    register int n, m;
			    register u_char *by_ad, len;
			    u_int   color, by_off, l_off, r_off;

			    dst_pl &= PIX_ALL_PLANES;
			    color = PIX_OPCOLOR(op);
			    if (dst_dep == GT_DEPTH_8) {
				if (!(color = PIX_OPCOLOR(op)))
				    color = 0xff000000;
				else 
				    color <<= GT_OVERLAY_SHIFT24;
				dst_pl <<= GT_OVERLAY_SHIFT24;
			    } else {
				if (!(color = PIX_OPCOLOR(op)))
				    color = 0x00ffffff;
			    }
			    gt_set_common_reg(dpr, color, 
				    HKPBM_HFBAS_ST_IMAGE, opcode,
				    dprd->buf, dst_pl );
			    m = (sy*src_lb + (sx/GT_SRC1_PIX_PER_BYTE));
                            by_ad = (u_char *)PTR_ADD(smpr2->md_image,m);

			    /* hawk stencil is on top of pixel mode */
			    if ((src_lb%4 == 0) && (sx/32 == (sx+dw)/32)) {
				/* Rop is less that 32 pixels wide and 
				 * the memory pixrect is long word aligned
				*/
				u_long *mem_ad, *hawk_ad;
				
				m = (GT_BYTES_PER_WORD*dx)
						+(dy*GT_IMAGE_LINEBYTES); 
			    	hawk_ad = (u_long *)
				    PTR_ADD(dprd->fb[0].mprp.mpr.md_image,m);
				mem_ad = (u_long *)((int)by_ad & ~3);
				by_off = ((int)by_ad - (int)mem_ad);
				l_off = (by_off*8 + (sx%8));
				lm = (((u_int)0xffffffff)<< l_off);
				len = (dw + l_off +31) >> 5;
				r_off = (GT_SRC1_PIX_PER_WORD - 
					 ((dw + l_off)%GT_SRC1_PIX_PER_WORD));
				lm <<= r_off;
				gt_set_sten_mask(fbi, lm);
				n = dh;
				/* Unroll the loop heightwise */
				while (n >= 8) {
				    gt_sten_rop_1(hawk_ad, mem_ad, u_long,
				    l_off, by_ad, src_lb, ~3);
				    gt_sten_rop_1(hawk_ad, mem_ad, u_long,
						    l_off, by_ad, src_lb, ~3);
				    gt_sten_rop_1(hawk_ad, mem_ad, u_long,
						    l_off, by_ad, src_lb, ~3);
				    gt_sten_rop_1(hawk_ad, mem_ad, u_long,
						    l_off, by_ad, src_lb, ~3);
				    gt_sten_rop_1(hawk_ad, mem_ad, u_long,
						    l_off, by_ad, src_lb, ~3);
				    gt_sten_rop_1(hawk_ad, mem_ad, u_long,
						    l_off, by_ad, src_lb, ~3);
				    gt_sten_rop_1(hawk_ad, mem_ad, u_long,
						    l_off, by_ad, src_lb, ~3);
				    gt_sten_rop_1(hawk_ad, mem_ad, u_long,
						    l_off, by_ad, src_lb, ~3);
				    n -= 8;
				}
			        switch (n) {
                                    case 7: 
                                        gt_sten_rop_1(hawk_ad, mem_ad, u_long,
						    l_off, by_ad, src_lb, ~3);
                                    case 6:
					gt_sten_rop_1(hawk_ad, mem_ad, u_long,
						    l_off, by_ad, src_lb, ~3);
                                    case 5:
					gt_sten_rop_1(hawk_ad, mem_ad, u_long,
						    l_off, by_ad, src_lb, ~3);
                                    case 4:
					gt_sten_rop_1(hawk_ad, mem_ad, u_long,
						    l_off, by_ad, src_lb, ~3);
                                    case 3:
					gt_sten_rop_1(hawk_ad, mem_ad, u_long,
						    l_off, by_ad, src_lb, ~3);
                                    case 2:
					gt_sten_rop_1(hawk_ad, mem_ad, u_long,
						    l_off, by_ad, src_lb, ~3);
                                    case 1: 
                                        *hawk_ad = (*mem_ad << l_off);
				    break;
				  }
			    } else if ((src_lb%2 == 0) &&
						 (sx/16 == (sx+dw)/16)) {
				 /*Rop is less that 32 pixels wide and 
			          * the src is short word aligned 
			       	  */
				 u_short *mem_ad, *hawk_ad;
				
				m = (GT_BYTES_PER_WORD*dx)
						+(dy*GT_IMAGE_LINEBYTES); 
			    	hawk_ad = (u_short *)
				    PTR_ADD(dprd->fb[0].mprp.mpr.md_image,m);
				mem_ad = (u_short *)((int)by_ad & ~1);
				by_off = ((int)by_ad - (int)mem_ad);
				l_off = (by_off*8 + (sx%8));
				lm = (((u_int)0xffffffff)<< l_off);
				len = (dw + l_off + 15)/16;
				r_off = (GT_SRC1_PIX_PER_WORD - 
					 ((dw + l_off)%GT_SRC1_PIX_PER_WORD));
				lm <<= r_off;
				gt_set_sten_mask(fbi, lm);
				n = dh;
				while (n >= 8) {
				    gt_sten_rop_1(hawk_ad, mem_ad, u_short,
						    l_off, by_ad, src_lb, ~1);
				    gt_sten_rop_1(hawk_ad, mem_ad, u_short,
						    l_off, by_ad, src_lb, ~1);
				    gt_sten_rop_1(hawk_ad, mem_ad, u_short,
						    l_off, by_ad, src_lb, ~1);
				    gt_sten_rop_1(hawk_ad, mem_ad, u_short,
						    l_off, by_ad, src_lb, ~1);
				    gt_sten_rop_1(hawk_ad, mem_ad, u_short,
						    l_off, by_ad, src_lb, ~1);
				    gt_sten_rop_1(hawk_ad, mem_ad, u_short,
						    l_off, by_ad, src_lb, ~1);
				    gt_sten_rop_1(hawk_ad, mem_ad, u_short,
						    l_off, by_ad, src_lb, ~1);
				    gt_sten_rop_1(hawk_ad, mem_ad, u_short,
						    l_off, by_ad, src_lb, ~1);
				    n -= 8;
				}
				switch (n) {
				    case 7: 
					gt_sten_rop_1(hawk_ad, mem_ad, u_short,
						    l_off, by_ad, src_lb, ~1);
				    case 6:
					gt_sten_rop_1(hawk_ad, mem_ad, u_short,
					    l_off, by_ad, src_lb, ~1);
				    case 5:
					gt_sten_rop_1(hawk_ad, mem_ad, u_short,
						    l_off, by_ad, src_lb, ~1);
				    case 4:
					gt_sten_rop_1(hawk_ad, mem_ad, u_short,
						    l_off, by_ad, src_lb, ~1);
				    case 3:
					gt_sten_rop_1(hawk_ad, mem_ad, u_short,
						    l_off, by_ad, src_lb, ~1);
				    case 2:
					gt_sten_rop_1(hawk_ad, mem_ad, u_short,
						    l_off, by_ad, src_lb, ~1);
				    case 1: 
					*hawk_ad = (*mem_ad << l_off);
				    break;
				}

			    } else {

			     	register u_long *m_ad, *h_ad;	
				u_long *mem_ad, *hawk_ad;

			    	m = (GT_BYTES_PER_WORD*dx)
						+(dy*GT_IMAGE_LINEBYTES); 
			    	h_ad = hawk_ad = (u_long *)
				   PTR_ADD(dprd->fb[0].mprp.mpr.md_image,m);

			    	for (n=dh; --n >=0;) {
				    m_ad = mem_ad = 
						(u_long *) ((int)by_ad & ~3);
				    by_off = ((int)by_ad - (int)mem_ad);
				    l_off = (by_off*8 + (sx%8));
				    lm = (((u_int)0xffffffff)<< l_off);
				    len = (dw + l_off +31) >> 5;
				    r_off = (GT_SRC1_PIX_PER_WORD - 
					 ((dw + l_off)%GT_SRC1_PIX_PER_WORD));
				    if (len == 1) {
					lm <<= r_off;
					gt_set_sten_mask(fbi, lm);
					*h_ad = (*m_ad << l_off);
					h_ad = (u_long *)PTR_ADD(h_ad, 
							GT_STEN_INCR);
				    } else {  
					if (l_off) {
					    gt_set_sten_mask(fbi, lm);
					    *h_ad = (*m_ad++ << l_off);
					    h_ad = (u_long *)
						PTR_ADD(h_ad, (GT_STEN_FAC*
					     (GT_SRC1_PIX_PER_WORD -l_off)));
					    --len;
					}  
					/* write 32 pixels at a time */
					gt_set_sten_mask(fbi, 0xffffffff);
					while (len > 8) {
					    gt_sten_rop_2(h_ad, m_ad);
					    gt_sten_rop_2(h_ad, m_ad);
					    gt_sten_rop_2(h_ad, m_ad);
					    gt_sten_rop_2(h_ad, m_ad);
					    gt_sten_rop_2(h_ad, m_ad);
					    gt_sten_rop_2(h_ad, m_ad);
					    gt_sten_rop_2(h_ad, m_ad);
					    gt_sten_rop_2(h_ad, m_ad);
					    len -= 8;
					}
					switch (len) {
					    case 8:
						gt_sten_rop_2(h_ad, m_ad);
					    case 7:
                                                gt_sten_rop_2(h_ad, m_ad);
                                            case 6:
                                                gt_sten_rop_2(h_ad, m_ad);
                                            case 5:
						 gt_sten_rop_2(h_ad, m_ad);
                                            case 4:
						gt_sten_rop_2(h_ad, m_ad);
                                            case 3:
                                                gt_sten_rop_2(h_ad, m_ad);
                                            case 2:
                                                gt_sten_rop_2(h_ad, m_ad);
                                            case 1:
						rm =
					        (((u_int)0xffffffff)<< r_off);
                                                gt_set_sten_mask(fbi, rm);
                                                *h_ad = *m_ad;
                                            break;
					}
				    }
				    by_ad += src_lb;
				    h_ad = (hawk_ad = (u_long *)
					PTR_ADD(hawk_ad, GT_IMAGE_LINEBYTES));
				}
			    }
			} /***/
			break;
			case GT_DEPTH_1:
			{                        
			    register int old_as, old_buf;

			    gt_get_as_reg(dprd->gt_rp,old_as);
			    gt_set_as_reg(dprd->gt_rp, 
						HKPBM_HFBAS_CURSOR_BYTE);

			    gt_waitidle(dprd->gt_rp, dprd->gt_fe_timeout);

			    gt_get_buf_reg(dprd->gt_fbi,old_buf);
			    gt_set_buf_reg(dprd->gt_fbi,0);
			
			    err = mem_rop (dpr, dx, dy, dw, dh, op, spr, sx,sy);

			    /* restore the old state */
			    gt_set_buf_reg(dprd->gt_fbi,old_buf);
			    gt_set_as_reg(dprd->gt_rp,old_as);
			}
			break;
			default:
			    err =  PIX_ERR;
		      }/* end switch (dst_dep) */
		    }/* end of 1->n switch */
		    return (err);
		}/* mem->hawk */
	    } /* end yes mempr */
	} /* end case 2 series*/
    return (0);   
}


#ifndef lint
static char        sccsid[] =      "@(#)gt_batch.c	1.1 94/10/31 SMI";
#endif

#include <sys/types.h>
#include <pixrect/pixrect.h>		       /* PIX		 */
#include <pixrect/gtvar.h>		       /* gt_d(pr)  	 */
#include <pixrect/memvar.h>		       /* mem_ops	 */
#include <pixrect/pr_planegroups.h>
#include <machine/param.h>

int
gt_batchrop(dpr, dx, dy, op, src, count)
	Pixrect *dpr;
	register int dx, dy;
	int op;
	register struct pr_prpos *src;
	register int count;
{   
    register int clip, col_sten;
    int xmin, xmax, ymin, ymax, group_mask;
    register int dposx, dposy, dsizew, dsizeh;
    register struct gt_data    *dprd; 
    register int dst_pl, dst_pg;
    H_rp	 *rp;
    H_fbi	*fbi;
    H_fbi_go	*fbi_go;
    H_fe_hold	*fe_h;
     	
    /* dst != gt */
    if (dpr->pr_ops->pro_rop != gt_rop) 
	    return pr_gen_batchrop(dpr, dx, dy, op, src, count);
    
    /* Dst = hawk; Initial set up from dst side */
    dprd = gt_d(dpr);	
    rp = (H_rp *) dprd->gt_rp;
    fbi = (H_fbi *)dprd->gt_fbi;
    fbi_go = (H_fbi_go *)dprd->gt_fbi_go;
    dst_pl = dprd->planes;
    dst_pg = PIX_ATTRGROUP(dst_pl);

    switch (dst_pg) {
        case PIXPG_8BIT_COLOR: 
	    if (!(col_sten = (PIX_OPCOLOR(op) << GT_OVERLAY_SHIFT24)))
    		    col_sten = (0xff000000);
	    gt_set_common_reg(dpr,
		    (PIX_OPCOLOR(op) << GT_OVERLAY_SHIFT24),
		    HKPBM_HFBAS_CURSOR_BYTE,
		    PIXOP_OP(op),
		    dprd->buf, (PIX_ALL_PLANES & dst_pl));
	break;
	case PIXPG_24BIT_COLOR:
	    if (!(col_sten = PIX_OPCOLOR(op))) 
		    col_sten = 0x00ffffff;
	    group_mask = (HKPF_FDA_PG_MASK & HKPF_FDA_PG_IMAGE); 
	    gt_set_common_reg(dpr,PIX_OPCOLOR(op),
			    HKPBM_HFBAS_IMAGE,
			    PIXOP_OP(op),
			    dprd->buf, (PIX_ALL_PLANES & dst_pl));
	break;
	case PIXPG_WID:
	    group_mask = (HKPF_FDA_PG_MASK & HKPF_FDA_PG_WINDOW); 
	    gt_set_common_reg(dpr,PIX_OPCOLOR(op),
			    HKPBM_HFBAS_WINDOW,
			    PIXOP_OP(op), 0,
			    (HKPP_WWM_MASK & HKPP_WWM_WID));
	break;
	default:
	    return PIX_ERR;
    }
    /* Clipping */
    dsizew = dpr->pr_size.x;
    dsizeh = dpr->pr_size.y;
    xmin = dprd->mprp.mpr.md_offset.x;
    ymin = dprd->mprp.mpr.md_offset.y;
    dx += xmin;			/* pen position */
    dy += ymin;
    xmax = xmin + dpr->pr_size.x - 1;
    ymax = ymin + dpr->pr_size.y - 1;
    if( op & PIX_DONTCLIP) {
        xmin = ymin = 0 ;
        xmax = 1280;
        ymax = 1024;
    }
    /* now loop through the src pixrects */
    for (; count > 0; count--, src++) {	/* count loop */
	
	register Pixrect *spr;
	int dw, dh;	    
	register int sx = 0;
	register int sy = 0;
	
	dx += src->pos.x;
	dy += src->pos.y;
	spr = src->pr;
	dw = spr->pr_size.x;
	dh = spr->pr_size.y;
	dposx = dx;
	dposy = dy;
	    
	if( dposx < xmin ) {
	    dw  -= (xmin - dposx) ;
	    sx = (xmin - dposx);
	    dposx = xmin ;
	}
	if( dposy < ymin ) {
	    dh -= (ymin - dposy) ;
	    sy = (ymin - dposy);
	    dposy = ymin ;
	}
	if( dposx + dw - 1 > xmax )
	    dw = xmax - dposx + 1 ;

	if( dposy + dh - 1 > ymax )
	    dh = ymax - dposy + 1 ;

	if (dw <= 0 || dh <= 0 )
	    continue;    
	if ((spr = src->pr) == 0) {
	
	    /* Hawk block fill case */
	    gt_set_fill_regs(rp, fbi, fbi_go,fe_h, dw, dh, dposx, 
		dposy, group_mask, dprd->gt_fe_frozen, dprd->gt_fe_timeout);
	    gt_waitidle(rp, dprd->gt_fe_timeout);
	    gt_rel_fe_hold(fe_h);
	    
	} else { /* src != NULL */
	
	    register struct mpr_data *sprd = mpr_d(spr);
	    register int src_lb = (sprd->md_linebytes);

	    if ((spr->pr_ops->pro_rop == gt_rop)) {
		/* Hawk block copy --- LAter */
	    } else {	/* memory case */

		if (MP_NOTMPR(spr) || spr->pr_depth != 1 || 
			    sprd->md_flags & MP_REVERSEVIDEO) {
		    return pr_gen_batchrop(dpr, dposx, dposy, op, src, count);
		    
		} else {  
		    /* use hawk's stencil mode */

		    register u_int lm, rm;
		    register int n, m;
		    register u_char *by_ad, len;
		    u_int   color, by_off, l_off, r_off;
		    int	    dst_dep = dpr->pr_depth;
		    struct mpr_data *smpr = &mprp_d(spr)->mpr;

		    dst_pl = dprd->planes & PIX_ALL_PLANES;
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
			    HKPBM_HFBAS_ST_IMAGE, PIXOP_OP(op),
			    dprd->buf, dst_pl );
		    m = (sy*src_lb + (sx/GT_SRC1_PIX_PER_BYTE));
                           by_ad = (u_char *)PTR_ADD(smpr->md_image,m);
		    
		    /* hawk stencil is on top of pixel mode */
		    if ((src_lb%4 == 0) && (sx/32 == (sx+dw)/32)) {
			register u_long *m_ad, *h_ad,tmp;	
			u_long *mem_ad, *hawk_ad;
			
			/* Memory pixrect is long word aligned 
			 * and is less than 32 pixels wide    
  			 */	
			m = (GT_BYTES_PER_WORD*dposx)
					+(dposy*GT_IMAGE_LINEBYTES); 
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
			/* Unroll the loop a bit- heightwise */
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
				/* The last rop may have some masking */
                                 *hawk_ad = (*mem_ad << l_off);
			    break;
			}
		    } else if ((src_lb%2 == 0) && (sx/16 == (sx+dw)/16)) {
		 	/* Memory pixrect is short word aligned 
			 * and is less than 32 pixels wide - most
		 	 * text characters fall into this case
			*/		
			register u_short *m_ad, *h_ad;	
			u_short *mem_ad, *hawk_ad;

			m = (GT_BYTES_PER_WORD*dposx)
				    +(dposy*GT_IMAGE_LINEBYTES); 
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
			/* Unroll the loop - heightwise */
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
			/* The general case */
			register u_long *m_ad, *h_ad,tmp;	
			u_long *mem_ad, *hawk_ad;

			m = (GT_BYTES_PER_WORD*dposx)
				+(dposy*GT_IMAGE_LINEBYTES); 
			h_ad = hawk_ad = (u_long *)
				   PTR_ADD(dprd->fb[0].mprp.mpr.md_image,m);
			for (n=dh; --n >=0;) {
			    m_ad = mem_ad = (u_long *) ((int)by_ad & ~3);
			    by_off = ((int)by_ad - (int)mem_ad);
			    l_off = (by_off*8 + (sx%8));
			    lm = (((u_int)0xffffffff)<< l_off);
			    len = (dw + l_off +31)/32;
			    r_off = (GT_SRC1_PIX_PER_WORD - 
				 ((dw + l_off)%GT_SRC1_PIX_PER_WORD));
			    if (len == 1) {
				/* Width of the rod <= 32 pixels &&
				 * memory is not well aligned
				*/
				lm <<= r_off;
				gt_set_sten_mask(fbi, lm);
				*h_ad = (*m_ad << l_off);
    				h_ad = (u_long *)PTR_ADD(h_ad, GT_STEN_INCR);
			    } else {  
				/* Wide rops */
				if (l_off) {
				    /* The start may be offset from a
				     * long word boundary 
			 	    */
				    gt_set_sten_mask(fbi, lm);
				    *h_ad = (*m_ad++ << l_off);
				    h_ad = (u_long *)
					 PTR_ADD(h_ad, (GT_STEN_FAC*
				         	(GT_SRC1_PIX_PER_WORD -l_off)));
				    --len;
				}  
				/* The middle - write 32 pixels at a time
				 * Unroll the loop widthwise
				 */
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
				        /* The end - may have masking */
					rm =(((u_int)0xffffffff)<< r_off);
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
		}
	    }
	}
    }
    return (0);
}

#ifndef lint
static char sccsid[] = "@(#)cg6_polygon2.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1988-1989, Sun Microsystems, Inc.
 */


#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/cg6var.h>

cg6_polygon_2(dpr, dx, dy, nbnds, npts, vlist, op, spr, sx, sy)
	struct pixrect *dpr;		/* destination pixrect */
	int dx, dy;			/* dest pixrect offset position */
	int nbnds;			/* number of closed boundaries */
	int *npts;			/* number of points in each boundary */
	register struct pr_pos *vlist;	/* list of points in all boundaries */
	int op;				/* pixrect op to use in painting */
	struct pixrect *spr;		/* source pixrect */
	int sx, sy;			/* source pixrect offset position */
{
	register struct fbc *fbc;
	register int n;
	int h;

	if (nbnds != 1)
		return PIX_ERR;

	if ((n = npts[0]) < 2)
		return 0;

	if (n > 4)
		return PIX_ERR;

	if (spr) {
		register struct mpr_data *mprd = mpr_d(spr);
		if (spr->pr_width != 16
		   || (spr->pr_depth > 1) 
		   || (dx - sx) < 0
		   || (dy - sy) < 0
		   || ((mprd->md_offset.x & 15) != 0)
	  	   || MP_NOTMPR(spr)) {
			return (PIX_ERR);
		}

		h = spr->pr_height;
	
		switch (h) {
		default:
			return (PIX_ERR);
		case 16:
		case 8:
		case 4:
		case 2:
		case 1:
			break;
		}
	}

	/* do hardware initialization - rop, color,  & clip */
	{
		register struct cg6pr *prd = cg6_d(dpr);
		register int color = PIXOP_COLOR(op);

		fbc = prd->cg6_fbc;

		if (spr && (spr->pr_depth == 8 || color == 0))
			color = ~0;

		cg6_setregs(fbc, 
			dx += prd->mprp.mpr.md_offset.x,
			dy += prd->mprp.mpr.md_offset.y,
			PIXOP_OP(op),			/* rop */
			prd->mprp.planes,		/* planemask */
			color,				/* fcolor (bcol = 0) */
			spr ? L_FBC_RASTEROP_PATTERN_MASK 
			    : L_FBC_RASTEROP_PATTERN_ONES,	/* pattern */
			L_FBC_RASTEROP_POLYG_NONOVERLAP	/* polygon overlap */
		);

		/* set clip rectangle */
		if (op & PIX_DONTCLIP)
			cg6_clip(fbc, 0, 0, prd->cg6_size.x-1,
				 prd->cg6_size.y-1);
		else
			cg6_clip(fbc, prd->mprp.mpr.md_offset.x, 
			    prd->mprp.mpr.md_offset.y,
			    prd->mprp.mpr.md_offset.x + dpr->pr_size.x - 1,
			    prd->mprp.mpr.md_offset.y + dpr->pr_size.y - 1);

		/*
	 	 * If source given, set up pattern registers
		 */
		if (spr) {
			register int ss;
			register int i;
			{
				register struct mpr_data *mprd = mpr_d(spr);
	
                		ss = (int) PTR_ADD(mprd->md_image,
                      		(short)mprd->md_offset.y * 
			    	(short)mprd->md_linebytes + (mprd->md_offset.x >> 4 << 1));
			}

			switch (h) {
	
			case 16:
				if (ss & 3) {
					/* not int aligned - extract words */
					register unsigned short *sp =
						 (unsigned short*) ss;

					for (i = 0; i < 8; i++) {
					    write_cg6_fbc((int *)fbc+L_FBC_PATTERN0+i, 
						 	sp[0] << 16 | sp[1]);
					    sp += 2;	
					}
				}else{
					register unsigned int *sp =
						 (unsigned int*) ss;

					for (i = 0; i < 8; i++) {
					    write_cg6_fbc((int *)fbc+L_FBC_PATTERN0+i,
							 *sp++);
					}
				}
				break;

			case 8:
			case 4:
				if (ss & 3) {
					/* not int aligned - extract words */
					register unsigned short *sp;
					register j, k;

					for (k = 0, j = 16/h; j; j--) {
						sp = (unsigned short*) ss;
						for (i = h/2; i; i--, k++) {
							write_cg6_fbc(
							   (int *)fbc+L_FBC_PATTERN0+k,
						 	   sp[0] << 16 | sp[1]);
							sp += 2;
						}
					}
				}else{
					register unsigned int *sp;
					register j, k;
	
					for (k = 0, j = 16/h; j; j--) {
						sp = (unsigned int*) ss;
						for (i = h/2; i; i--, k++) {
							write_cg6_fbc(
							   (int *)fbc+L_FBC_PATTERN0+k,
						 	    *sp++);
						}
					}
				}
				break;
			
			case 2:
				{
					register unsigned int x;
	
					x = ((unsigned short*) ss)[0] << 16
				  		| ((unsigned short*) ss)[1];
	
					for (i = 0; i < 8; i++) {
						write_cg6_fbc(
						    (int *)fbc+L_FBC_PATTERN0+i, x);
					}
				}
				break;
	
			case 1:
				{
					register unsigned int x;

					x = *((unsigned short*) ss);
					x |= (x << 16);
	
					for (i = 0; i < 8; i++) {
						write_cg6_fbc(
						     (int *)fbc+L_FBC_PATTERN0+i, x);
					};
				}
				break;
			}

               /* XXX pattalign usage is different from FBC Ref Manual v 3.0. 
                   pattalign x and y are added to framebuffer x and y  
                   instead of subtracted as described in manual.  Correct
                   usage requires computing 16 - pattalign x,y */

			fbc->l_fbc_pattalign.word =
                                ((16 - ((dx - sx) & 0xF)) << 16) |
                                 (16 - ((dy - sy) & 0xF));
		}
	}

	{
		register int r;

		fbc->l_fbc_ipointabsy = (u_int) vlist->y;
		fbc->l_fbc_ipointabsx = (u_int) (vlist++)->x;

		switch (n) {
		case 4:
			fbc->l_fbc_iquadabsy = (u_int) vlist->y;
			fbc->l_fbc_iquadabsx = (u_int) (vlist++)->x;
		case 3:
			fbc->l_fbc_iquadabsy = (u_int) vlist->y;
			fbc->l_fbc_iquadabsx = (u_int) (vlist++)->x;
		case 2:
			fbc->l_fbc_iquadabsy = (u_int) vlist->y;
			fbc->l_fbc_iquadabsx = (u_int) (vlist++)->x;
		}

		cg6_draw_done(fbc, r);
		if (fbc_exception(r))
			return PIX_ERR;
	}

	return 0;
}

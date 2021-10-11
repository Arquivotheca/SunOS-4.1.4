#ifndef lint
static char sccsid[] = "@(#)cg6_replrop.c 1.1 %E SMI";
#endif

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memreg.h>
#include <pixrect/cg6var.h>
#include <pixrect/memvar.h>
#include "pr_impl_util.h"

cg6_replrop(dpr, dx, dy, dw, dh, op, spr, sx, sy)
	Pixrect *dpr;
	register int dx, dy;
	int dw, dh, op;
	register Pixrect *spr;
	int sx, sy;
{
	register struct fbc *fbc;
	register u_int r;
	int h;
	int cg6_rop();

	/*
	 * Weed out the hard cases.
	 */ 
	if (spr) {
		register struct mpr_data *mprd = mpr_d(spr);
		if (spr->pr_width != 16
		   || (spr->pr_depth > 1 
		   || (dx - sx) < 0
		   || (dy - sy) < 0
		   || ((mprd->md_offset.x & 15) != 0)
	  	   || MP_NOTMPR(spr))) {
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
	} else
	   return PIX_ERR;   /* return PIX_ERR, no source given.  */

	/* do hardware initialization - rop, color,  & clip */
	{
		register int rop = op;
		register struct cg6pr *dprd = cg6_d(dpr);
		register int color;

		fbc = dprd->cg6_fbc;

		dx += dprd->mprp.mpr.md_offset.x;
		dy += dprd->mprp.mpr.md_offset.y;

		color = PIX_OPCOLOR(rop);

		if (color == 0)
		    color = ~0;

		cg6_setregs(fbc, 
			0, 0,					/* offset */
			(rop >> 1) & 0xf,			/* rop */
			dprd->mprp.planes,			/* planemask */
			color,					/* fcolor (bcol = 0) */
			spr ? L_FBC_RASTEROP_PATTERN_MASK 
			    : L_FBC_RASTEROP_PATTERN_ONES,
			L_FBC_RASTEROP_POLYG_OVERLAP		/* polygon overlap */
		);
	if (op & PIX_DONTCLIP)
	  cg6_clip(fbc,0,0,dprd->cg6_size.x - 1, dprd->cg6_size.y -1);
	else
	  cg6_clip(fbc, dx, dy, dx+dw, dy+dh);
	}

	/*
	 * set up pattern ram
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
				register unsigned short *sp = (unsigned short*) ss;

				for (i = 0; i < 8; i++) {
					write_cg6_fbc((int *)fbc+L_FBC_PATTERN0 + i, 
						 sp[0] << 16 | sp[1]);
					sp += 2;
				}
			}else{
				register unsigned int *sp = (unsigned int*) ss;

				for (i = 0; i < 8; i++) {
					write_cg6_fbc((int *)fbc+L_FBC_PATTERN0+i, *sp++);
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
						write_cg6_fbc((int *)fbc+L_FBC_PATTERN0+k,
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
						write_cg6_fbc((int *)fbc+L_FBC_PATTERN0+k,
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
					write_cg6_fbc((int *)fbc+L_FBC_PATTERN0+i, x);
				}
			}
			break;

		case 1:
			{
				register unsigned int x;

				x = *((unsigned short*) ss);
				x |= (x << 16);
	
				for (i = 0; i < 8; i++) {
					write_cg6_fbc((int *)fbc+L_FBC_PATTERN0+i, x);
				};
			}
			break;
		}

		/* XXX pattalign usage is different from FBC Ref Manual v 3.0. 
		   pattalign x and y are added to framebuffer x and y
		   instead of subtracted as described in manual.  Correct
	 	   usage requires computing 16 - pattalign x,y */

                fbc->l_fbc_pattalign.word = (16 - ((dx-sx) & 0xF)) << 16 | (16 - ((dy-sy) & 0xF));
	}

	fbc->l_fbc_irectabsy = dy;
	fbc->l_fbc_irectabsx = dx;

	fbc->l_fbc_irectabsy = dy + dh - 1;
	fbc->l_fbc_irectabsx = dx + dw - 1;

	/* now read DRAW_STATUS register to effect the DRAW op */
		
	cg6_draw_done(fbc, r);
	if (r & L_FBC_DRAW_EXCEPTION)  {
		return (PIX_ERR);
	}

	return (0);
}

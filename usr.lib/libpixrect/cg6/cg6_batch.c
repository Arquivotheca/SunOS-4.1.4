#ifndef lint
static char sccsid[] = "@(#)cg6_batch.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1985, 1987 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <sundev/cg6reg.h>
#include <pixrect/cg6var.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_impl_util.h>

#include <stdio.h>

cg6_batchrop(dpr, dx, dy, op, src, count)
	Pixrect *dpr;
	register int dx, dy;
	int op;
	register struct pr_prpos *src;
	register int count;
{

	register struct fbc *fbc;
	int xoff, yoff;
	int dsizex, dsizey;	/* dst pixrect size */
	int cdy;		/* clipped dy */
	int clip;		/* clipping flag */

	{
		register struct cg6pr *dprd;
		register int opcode = PIXOP_OP(op);
		register int color;

		clip =  ! (op & PIX_DONTCLIP);

		dprd = cg6_d(dpr);
		fbc = dprd->cg6_fbc;
	
		color = PIX_OPCOLOR(op);
		if (color == 0)
			color = ~0;

		xoff = dprd->mprp.mpr.md_offset.x;
		yoff = dprd->mprp.mpr.md_offset.y;

		dsizex = xoff + dpr->pr_size.x;
		dsizey = yoff + dpr->pr_size.y;

		cg6_setregs(fbc, 
			0, 0,					/* offset */
			opcode,
			dprd->mprp.planes,			/* planemask */
			color,					/* fcolor (bcol = 0) */
			L_FBC_RASTEROP_PATTERN_ONES,		/* pattern */
			L_FBC_RASTEROP_POLYG_OVERLAP		/* polygon overlap */
		);

		dx += xoff;
		dy += yoff;

		if (op & PIX_DONTCLIP) 
			/* set clip rectangle (all of dest) */ 
			cg6_clip(fbc, 0, 0, dprd->cg6_size.x-1,
				 dprd->cg6_size.y-1);
		else 
			/* set clip rectangle */
			cg6_clip(fbc, xoff, yoff, dsizex-1, dsizey-1); 
	}

	cg6_color_mode(fbc, L_FBC_MISC_DATA_COLOR1);

	cdy = dy;

	for (; count > 0; count--, src++) {
		register int dw, dh;	/* clipped width and height */
		register ushort *s;	/* first pixel of src */

		{
			register Pixrect *spr;
			register struct mpr_data *sprd;

			dx += src->pos.x;
			if (src->pos.y != 0) {
				dy += src->pos.y;
				cdy = dy;
			}

			if ((spr = src->pr) == 0)
				continue;

			dw = spr->pr_size.x;
			dh = spr->pr_size.y;

			sprd = mpr_d(spr);

			/*
			 * If a source pixrect is too hard for us
			 * to deal with, we have to draw it and all
			 * remaining sprs with pr_rop (we don't
			 * know what the first pr_rop will do to 
			 * the frame buffer state).
			 */
			if (MP_NOTMPR(spr) || 
				spr->pr_depth != 1 ||
				sprd->md_linebytes != 2 ||
				sprd->md_offset.x != 0 ||
				sprd->md_flags & MP_REVERSEVIDEO) { 
				return pr_gen_batchrop(dpr,
					dx - xoff - src->pos.x,
					dy - yoff - src->pos.y,
					op, src, count);
			}

			/* 
			 * compute address of first line of src
			 * src linebytes must equal sizeof(*sfirst) !
			 */
			s = (ushort *) sprd->md_image + 
				sprd->md_offset.y;

			if (clip) {
				register int extra;

				/* clipped on left? */
				if (dx < 0) {
					/*
					 * If partially clipped, use pr_rop.
					 * We happen to known that it won't
					 * screw up the frame buffer state.
					 * (If you change cg2_rop or gp1_rop
					 * check this!)
					 */
					/* XXX */
					if (dx + dw > 0) {
						(void) pr_rop(dpr, 
							dx, dy, dw, dh, 
							op, spr, 0, 0);
					}
					continue;
				}

				/* clipped on right? */
				if ((extra = dx + dw - dsizex) > 0)
					dw -= extra;

                                /* clipped on bottom? */
                                if ((extra = dy + dh - dsizey) > 0)
                                        dh -= extra;

				/* clipped on top? */
				if ((extra = dy) < 0) {
					dh += extra;
					cdy = 0;
					s -= extra;
				}

			}


			/* clipped to nothing? */
			if (--dw < 0 || --dh < 0) 
				continue;
		}

		cg6_setinx(fbc, 0, 1);
		cg6_setfontxy(fbc, dx, dx+dw, cdy);
		PR_LOOPP(dh, fbc->l_fbc_font = *s++ << 16);
	}

	return 0;
}


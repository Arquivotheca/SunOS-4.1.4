#ifndef lint
static char sccsid[] = "@(#)cg2_batch.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1985, 1987 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/memvar.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>


cg2_batchrop(dpr, dx, dy, op, src, count)
	Pixrect *dpr;
	int dx, dy, op;
	struct pr_prpos *src;
	int count;
{
	register struct memropc *allrop;

	short *dleft;		/* first word of current line in dst */
	int dlinebytes;		/* dst bytes per line */
	int doffx;		/* dst pixrect x offset */

	int clip;		/* clipping flag */
	int dsizex, dsizey;	/* dst pixrect size */

	{
		register struct cg2pr *dprd;
		register struct cg2fb *fb;
		register int rop = op;
		register int color;

		{
			register Pixrect *rdpr = dpr;

			/* 
	 	 	 * If clipping, compute destination limits 
	 	 	 * for later comparisons. 
	 	 	 */
			if (rop & PIX_DONTCLIP)
				clip = 0;
			else { 
				clip = 1;
				dsizex = rdpr->pr_size.x;
				dsizey = rdpr->pr_size.y;
			}

			dprd = cg2_d(rdpr);
		}

		GP1_PRD_SYNC(dprd, return PIX_ERR);
		fb = dprd->cgpr_va;
		allrop = &fb->ropcontrol[CG2_ALLROP].ropregs;

		if ((color = PIXOP_COLOR(rop)) == 0 ||
			(color = ~color & 255) == 0) {
			fb->ppmask.reg = dprd->cgpr_planes;
			allrop->mrc_op = PIXOP_OP(rop);
		}
		else {
			rop = PIXOP_OP(rop);

			fb->ppmask.reg = color;
			allrop->mrc_op = rop << 2 | rop & 3;
			fb->ppmask.reg = ~color;
			allrop->mrc_op = rop;
			fb->ppmask.reg = dprd->cgpr_planes;
		}

		allrop->mrc_pattern = 0;
		fb->status.reg.ropmode = PWWWRD;

		/* save dst pixrect x offset, linebytes */
		doffx = dprd->cgpr_offset.x;
		dlinebytes = cg2_linebytes(fb, PWWWRD);

		/* compute address of left pixel on first line of dst */
		dleft = cg2_ropwordaddr(fb, 0, 0, dy + dprd->cgpr_offset.y);
	}

	while (--count >= 0) {
		register int dw, dh;	/* clipped width and height */
		short *dfirst;		/* first pixel of dst */
		UMPR_T *sfirst;		/* first pixel of src */

		{
			register struct pr_prpos *rsrc;
			register Pixrect *spr;
			register struct mpr_data *sprd;

			rsrc = src;
			dx += rsrc->pos.x;
			if (rsrc->pos.y != 0) {
				dy += rsrc->pos.y;
				PTR_INCR(short *, dleft,
					pr_product(dlinebytes,
						rsrc->pos.y));
			}
			dfirst = dleft;

			if ((spr = rsrc->pr) == 0)
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
				sprd->md_flags & MP_REVERSEVIDEO) 
				return pr_gen_batchrop(dpr,
					dx - rsrc->pos.x,
					dy - rsrc->pos.y,
					op, rsrc, count + 1);

			src = ++rsrc;

			/* 
			 * compute address of first line of src
			 * src linebytes must equal sizeof(*sfirst) !
			 */
			sfirst = (UMPR_T *) sprd->md_image + 
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
					if (dx + dw > 0)
						(void) pr_rop(dpr, 
							dx, dy, dw, dh, 
							op, spr, 0, 0);
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
					PTR_INCR(short *, dfirst,
						-pr_product(dlinebytes, 
							extra));
					sfirst -= extra;
				}
			}

			/* clipped to nothing? */
			if (--dw < 0 || --dh < 0)
				continue;
		}

		{
			register UMPR_T *s = sfirst;
			register short *d;
			register int doffset = dlinebytes;
			register int skew;

			d = dfirst + ((skew = dx + doffx) >> 4);

			allrop->mrc_mask1 = 
				mrc_lmask(skew = cg2_prskew(skew));
			allrop->mrc_shift = (1 << 8) | skew;
			allrop->mrc_mask2 = 
				mrc_rmask(cg2_prskew(dw += skew));

			/* char fits in one dst word */
			if (dw < 16) {
				allrop->mrc_width = 0;
				allrop->mrc_opcount = 0;
				if (skew) 
					PR_LOOPP(dh,
						*d = *s++;
						PTR_INCR(short *, d, doffset));
				else {
					allrop->mrc_source1 = *s++;
					PR_LOOP(dh,
						*d = *s++;
						PTR_INCR(short *, d, doffset));
						*d = doffset;
				}
			}
			/* char straddles two dst words */
			else {
				allrop->mrc_width = 1;
				allrop->mrc_opcount = 1;
				doffset -= 2;
				PR_LOOPP(dh,
					*d++ = *s++;
					*d = doffset;
					PTR_INCR(short *, d, doffset));
			}
		}
	}

	return 0;
}


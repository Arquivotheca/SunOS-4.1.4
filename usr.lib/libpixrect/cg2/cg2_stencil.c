#ifndef lint
static char sccsid[] = "@(#)cg2_stencil.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1984-1989 Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>
#include <pixrect/memvar.h>

/* minimum # words to make ropmode swapping worthwhile */
#define	SWAP_ROPMODE_MIN	6

/* magic ropmode xor mask*/
#define	SWAP_ROPMODE_MASK	((PRWWRD ^ PWWWRD) << 3)


cg2_stencil(dpr, dx, dy, dw, dh, op, stpr, stx, sty, spr, sx, sy)
	Pixrect *dpr;
	int dx, dy;
	register int dw, dh, op;
	Pixrect *stpr;
	int stx, sty;
	Pixrect *spr;
	int sx, sy;
{
	struct cg2fb *fb;
	register LOOP_T w;

	int dskew, doffset;

	short *sfirst;
	int sskew, soffset, sdepth;

	short *stfirst;
	int stskew, stoffset, stprime;

	Pixrect *tpr = 0;

#define	dummy	IF68000(w, 0)


	/* stencil must be a depth-1 memory pixrect */
	if (MP_NOTMPR(stpr) || stpr->pr_depth != 1)
		return PIX_ERR;

	/* poor way to clip */
	if (!(op & PIX_DONTCLIP)) {
		struct pr_subregion dst;
		struct pr_prpos src, sten;

		dst.pr = dpr;
		dst.pos.x = dx;
		dst.pos.y = dy;
		dst.size.x = dw;
		dst.size.y = dh;

		if (src.pr = spr) {
			src.pos.x = sx;
			src.pos.y = sy;
			(void) pr_clip(&dst, &src);
			dst.pos.x = dx;
			dst.pos.y = dy;
			dst.size.x = dw;
			dst.size.y = dh;
		}

		sten.pr = stpr;
		sten.pos.x = stx;
		sten.pos.y = sty;
		(void) pr_clip(&dst, &sten);
		stx = sten.pos.x;
		sty = sten.pos.y;

		if (spr) {
			(void) pr_clip(&dst, &src);
			sx = src.pos.x;
			sy = src.pos.y;
		}

		dx = dst.pos.x;
		dy = dst.pos.y;
		dw = dst.size.x;
		dh = dst.size.y;
	}

	if (dw <= 0 || dh <= 0 )
		return 0;

	{
		register struct cg2pr *dprd;

		dprd = cg2_d(dpr);
		GP1_PRD_SYNC(dprd, return PIX_ERR);
	}

	{
		register struct cg2pr *dprd;
		register struct cg2fb *rfb;
		register Pixrect *rspr = spr;
		register struct mpr_data *mprd;
		register int rop = op, color, planes;

		dprd = cg2_d(dpr);
		dx += dprd->cgpr_offset.x;
		dy += dprd->cgpr_offset.y;
		fb = rfb = dprd->cgpr_va;
		planes = dprd->cgpr_planes & 255;

		if (PIXOP_NEEDS_SRC(rop)) {
			/* identify weird type of fill: src is 1 x 1 */
			if (rspr && 
				rspr->pr_size.x == 1 && rspr->pr_size.y == 1) {
				color = pr_get(rspr, 0, 0);
				if (rspr->pr_depth == 1 && 
					color && (color = 
						PIXOP_COLOR(rop)) == 0)
						color = ~color;

				rspr = 0;
			}
			else
				color = PIXOP_COLOR(rop);
		}
		else {
			color = 0;
			rspr = 0;
		}

		rop = PIXOP_OP(rop);

		if (rspr) {
			/* non-memory pixrect source */
			if (MP_NOTMPR(rspr)) {
				if (rspr->pr_depth != CG2_DEPTH &&
					rspr->pr_depth != 1 ||
					!(rspr = mem_create(dw, dh, 
						rspr->pr_depth)))
					return PIX_ERR;

				if ((*spr->pr_ops->pro_rop)(rspr, 
					0, 0, dw, dh, 
					PIX_SRC | PIX_DONTCLIP, 
					spr, sx, sy)) {
					(void) mem_destroy(spr);
					return PIX_ERR;
				}

				sx = 0; 
				sy = 0;

				tpr = rspr;
			}

			mprd = mpr_d(stpr);
			stoffset = mprd->md_linebytes;
			stskew = mprd_skew(mprd, stx, sty);
			stfirst = mprd_addr(mprd, stx, sty);

			rop = mprd->md_flags & MP_REVERSEVIDEO ?
				CG_DEST & CG_MASK | rop :
				rop << 4 | CG_DEST & CG_NOTMASK;

			rfb->ppmask.reg = planes;
			cg2_setfunction(rfb, CG2_ALLROP, rop);

			mprd = mpr_d(rspr);

			switch (sdepth = rspr->pr_depth) {
			case CG2_DEPTH:
				break;

			case 1:
				/* zero color becomes ~0 */
				if (color == 0)
					color = ~color;

				/* check for reverse video spr */
				if (mprd->md_flags & MP_REVERSEVIDEO) {
					rop = rop << 2 & CG_SRC | 
						rop >> 2 & ~CG_SRC;
					cg2_setfunction(rfb, CG2_ALLROP, rop);
				}

				/* set function for 0 bits in color */
				if ((color = ~color) & planes) {
					rfb->ppmask.reg = color;
					rop &= ~CG_SRC;
					rop |= rop << 2;
					cg2_setfunction(rfb, CG2_ALLROP, rop);
					rfb->ppmask.reg = planes;
				}
				break;

			default:
				return PIX_ERR;
			}

		}
		else {
			rspr = stpr;
			sdepth = 1;
			sx = stx;
			sy = sty;
			stpr = 0;

			/* check for reverse video stencil */
			mprd = mpr_d(rspr);
			rop = mprd->md_flags & MP_REVERSEVIDEO ?
				CG_DEST & CG_SRC | 
					(rop << 2 | rop) & ~CG_SRC :
				(rop << 2 | rop) << 2 & CG_SRC | 
					CG_DEST & ~CG_SRC;

			rfb->ppmask.reg = planes;
			cg2_setfunction(rfb, CG2_ALLROP, rop);

			/* set pattern for 0 bits in color */
			cg2_setpattern(rfb, CG2_ALLROP, 0);

			/* set pattern for 1 bits in color */
			if (color & planes) {
				rfb->ppmask.reg = color;
				cg2_setpattern(rfb, CG2_ALLROP, ~0);
				rfb->ppmask.reg = planes;
			}
		}

		soffset = mprd->md_linebytes;
		if (sdepth == 1) {
			sskew = mprd_skew(mprd, sx, sy);
			sfirst = mprd_addr(mprd, sx, sy);
		}
		else
			sfirst = (short *) 
				((int) mprd8_addr(mprd, sx, sy, 8) & ~1);

		sx += mprd->md_offset.x;

		cg2_setlmask(rfb, CG2_ALLROP, 
			mrc_lmask(dskew = cg2_prskew(dx)));
		cg2_setrmask(rfb, CG2_ALLROP,
			mrc_rmask(cg2_prskew(dskew + dw - 1)));
	}

	switch (sdepth) {
	case CG2_DEPTH: {
		register short *d, *s = sfirst;
		int lw, rw;

		{
			register struct cg2fb *rfb = fb;

			stprime = stskew > dskew ? 2 : 0;
			stskew = cg2_prskew(dskew - stskew);

			w = (dw |= dx & 1) - 1 >> 1;
			cg2_setwidth(rfb, CG2_ALLROP, w, w);

			d = (short *) cg2_roppixaddr(rfb, dx & ~1, dy);
			doffset = cg2_linebytes(rfb, SWWPIX) - w - w;
			rfb->status.reg.ropmode = SWWPIX;

			soffset = soffset - w - w - 2;

			if (lw = dskew) 
				lw = 16 - lw;

			if ((rw = (dx + dw) & 15) == 0)
				rw = 16;

			if ((w = dw - lw - rw >> 4) < 0) {
				lw = 0;
				w = 0;
				rw = dw;
			}

			/* convert lw, rw to 16 bit words */
			lw = lw + 1 >> 1;
			rw = rw + 1 >> 1;

			stoffset = stoffset - stprime - 
				((lw > 0) + w << 1) - 2;
		}

		/* Src and dst are word aligned. */
		if (!((dx ^ sx) & 1)) {
			register u_short *st = (u_short *) stfirst;
			register u_short *pattern =
				&fb->ropcontrol[CG2_ALLROP].
					ropregs.mrc_pattern;
			register u_long stword;
			register int stshift = stskew;
			register u_short *rsource_pix = 
				&fb->ropcontrol[CG2_ALLROP].prime
					.ropregs.mrc_source1;

			cg2_setshift(fb, CG2_ALLROP, 0, 1);

			/* adjust rw for last write */
			rw--;

			PR_LOOPP(dh - 1,
				stword = 0;
				if (stprime)
					stword |= *st++;

				*rsource_pix = *s++;

				if (lw > 0) {
					stword = stword << 16 | *st++;
					*pattern = stword >> stshift;
					PR_LOOP(lw, *d++ = *s++);
				}

				PR_LOOP(w,
					stword = stword << 16 | *st++;
					*pattern = stword >> stshift;
					PR_LOOPP(7, *d++ = *s++));

				stword = stword << 16 | *st++;
				*pattern = stword >> stshift;
				PR_LOOP(rw, *d++ = *s++);

				*d = dummy;

				PTR_INCR(short *, d, doffset);
				PTR_INCR(short *, s, soffset);
				PTR_INCR(u_short *, st, stoffset));
		}

		/* 
		 * Src and dst are misaligned -- use FIFO.
		 *
		 * Using the FIFO in pixel mode is very tricky -- we have
		 * to adjust the shift for each write so that the LSB
		 * of the left source register and the MSB of the right
		 * source register are written to the two destination pixels.
		 */
		else {
			register struct memropc *allrop = 
				&fb->ropcontrol[CG2_ALLROP].ropregs;
			register u_short *st = (u_short *) stfirst;
			register u_long stword;
			register int stshift = stskew;
			register int shift;
			int initshift;
			register u_short *rsource_pix = 
				&fb->ropcontrol[CG2_ALLROP].prime
					.ropregs.mrc_source1;

			/* compute initial shift, set direction bit */
			initshift = 1 << 8 | (dx & 14) + 1;

			/*
			 * If dx is even and sx is odd, make initial shift
			 * negative to flag need to prime FIFO.
			 */
			if (!(dx & 1)) {
				soffset -= 2;
				initshift |= -1 << 16;
			}

			doffset -= 2;

			do {
				stword = 0;
				if (stprime)
					stword |= *st++;

				if ((shift = initshift) < 0)
					*rsource_pix = *s++;

				if (lw > 0) {
					stword = stword << 16 | *st++;
					allrop->mrc_pattern = 
						stword >> stshift;
					PR_LOOP(lw,
						allrop->mrc_shift = shift;
						shift += 2;
						*d++ = *s++);
				}

				PR_LOOP(w,
					stword = stword << 16 | *st++;
					allrop->mrc_pattern = 
						stword >> stshift;
					shift &= 1 << 8 | 15;
					PR_LOOPP(7,
						allrop->mrc_shift = shift;
						shift += 2;
						*d++ = *s++));

				stword = stword << 16 | *st++;
				allrop->mrc_pattern = stword >> stshift;
				PR_LOOP(rw, 
					allrop->mrc_shift = shift;
					shift += 2;
					*d++ = *s++);

				PTR_INCR(short *, d, doffset);
				PTR_INCR(short *, s, soffset);
				PTR_INCR(u_short *, st, stoffset);
			} while (--dh > 0);
		}
	}
		break;

	case 1: {
		register short *d, *s = sfirst;

		{
			register struct cg2fb *rfb = fb;

			d = cg2_ropwordaddr(rfb, 0, dx, dy);

			w = dskew;
			cg2_setshift(rfb, CG2_ALLROP, 
				cg2_prskew(w - sskew), 1);
			sskew = sskew >= w ? 2 : 0;

			stprime = stskew > w ? 2 : 0;
			stskew = cg2_prskew(w - stskew);

			w = w + dw - 1 >> 4;
			cg2_setwidth(rfb, CG2_ALLROP, w, w);

			doffset = cg2_linebytes(rfb, PWWWRD) - w - w - 2;
			rfb->status.reg.ropmode = PWWWRD;
			soffset = soffset - w - w - sskew - 2;
			stoffset = stoffset - w - w - stprime - 2;
		}

		if (stpr) {
			register u_short *st = (u_short *) stfirst;
			register u_short *pattern =
				&fb->ropcontrol[CG2_ALLROP].
					ropregs.mrc_pattern;
			register u_long stword;
			register int stshift = stskew;
			register PTR_T rsource = 0;

			if (sskew)
				rsource = (PTR_T) 
					&fb->ropcontrol[CG2_ALLROP].
						ropregs.mrc_source1;

			PR_LOOPP(dh - 1,
				stword = 0;
				if (stprime)
					stword |= *st++;
				if (rsource)
					* (u_short *) rsource = *s++;
				PR_LOOPP(w,
					stword = stword << 16 | *st++;
					*pattern = stword >> stshift;
					*d++ = *s++);
				PTR_INCR(short *, d, doffset);
				PTR_INCR(short *, s, soffset);
				PTR_INCR(u_short *, st, stoffset));
		}
		else {
			register int rdoffset = doffset;
			register int rsoffset = soffset;

			if (sskew) {
				register u_short *rsource =
					&fb->ropcontrol[CG2_ALLROP].
						ropregs.mrc_source1;
				PR_LOOPP(dh - 1,
					*rsource = *s++;
					PR_LOOPP(w, *d++ = *s++);
					PTR_INCR(short *, d, rdoffset);
					PTR_INCR(short *, s, rsoffset));
			}
			else 
				PR_LOOPP(dh - 1,
					PR_LOOPP(w, *d++ = *s++);
					PTR_INCR(short *, d, rdoffset);
					PTR_INCR(short *, s, rsoffset));
		}
	}
		break;
	}
	
	if (tpr)
		(void) mem_destroy(tpr);

	return 0;
}

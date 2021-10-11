#ifndef lint
static char sccsid[] = "@(#)cg2_vec.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1983, 1987, 1990 Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>

/* largest delta we can handle without overflow in clip code */
#define	MAXDELTA	(32767)

/* minimum # words to make ropmode swapping worthwhile */
#define	SWAP_ROPMODE_MIN	6

/* magic ropmode xor mask*/
#define	SWAP_ROPMODE_MASK	((PRWWRD ^ PWWWRD) << 3)

#define swap(a, b, t)	_STMT((t) = (a); (a) = (b); (b) = (t);)

#define	cg2_roppix(cgd, fb, ax, ay) \
	cg2_roppixaddr((fb), \
		(cgd)->cgpr_offset.x + (ax), (cgd)->cgpr_offset.y + (ay))


cg2_vector(pr, x0, y0, x1, y1, op, color)
	Pixrect *pr;
	int x0, y0, x1, y1;
	int op;
	int color;
{
	register struct cg2pr *prd = cg2_d(pr);
	register struct cg2fb *fb = prd->cgpr_va;
	register int dx, dy;
	int noclip;
	int w, h, startx, starty;
	int reflect, initerror;

#define	dummy	IF68000(dx, 0)

	{
		register struct memropc *allrop = 
			&fb->ropcontrol[CG2_ALLROP].ropregs;
		register int rop = op, rcolor, planes;

		noclip = rop & PIX_DONTCLIP;
		if ((rcolor = PIXOP_COLOR(rop)) == 0)
			rcolor = color;
		rop = PIXOP_OP(rop);

		fb->status.reg.ropmode = 
			PIX_OP_NEEDS_DST(rop) ? SWWPIX : SRWPIX;

		fb->ppmask.reg = planes = prd->cgpr_planes & 255;

		/* set function for 1 bits in color */
		allrop->mrc_op = rop & 0xC | rop >> 2;

		/* set function for 0 bits in color */
		if ((rcolor = ~rcolor) & planes) {
			fb->ppmask.reg = rcolor;
			allrop->mrc_op = rop << 2 | rop & 3;
			fb->ppmask.reg = planes;
		}

		allrop->mrc_pattern = 0;
		allrop->mrc_mask1 = 0;
		allrop->mrc_mask2 = 0;
	}

	dx = x1 - x0;
	dy = y1 - y0;

	/* vertical vectors */
	if (dx == 0) {
		register u_char *d;

		{
			register int y;

			if (dy >= 0)
				y = y0;
			else {
				y = y1;
				dy = -dy;
			}

			if (!noclip) {
				register int h;

				if ((u_int) x0 >= pr->pr_size.x)
					return 0;

				if (y < 0) {
					if ((dy += y) < 0)
						return 0;
					y = 0;
				}

				if (y + dy >= (h = pr->pr_size.y)) {
					if ((dy = h - y - 1) < 0)
						return 0;
				}
			}

			d = cg2_roppix(prd, fb, x0, y);
		}

		{
			register int offset = cg2_linebytes(fb, SRWPIX);

			PR_LOOPP(dy, *d = dummy; d += offset);
		}

		return 0;
	}

	/* handle length 2 vectors by just drawing endpoints */
	if ((u_int) (dx + 1) <= 2 && (u_int) (dy + 1) <= 2) {
		if (noclip) {
			*cg2_roppix(prd, fb, x0, y0) = dummy;
			*cg2_roppix(prd, fb, x1, y1) = dummy;
		}
		else {
			register int w = pr->pr_size.x;
			register int h = pr->pr_size.y;

			if ((u_int) x0 < w && (u_int) y0 < h)
				*cg2_roppix(prd, fb, x0, y0) = dummy;
			if ((u_int) x1 < w && (u_int) y1 < h)
				*cg2_roppix(prd, fb, x1, y1) = dummy;
		}

		return 0;
	}

	/* horizontal vectors */
	if (dy == 0) {
		register short *d;
		register LOOP_T words;

		{
			register int x;

			if (dx >= 0)
				x = x0;
			else {
				x = x1;
				dx = -dx;
			}

			if (!noclip) {
				register int w;

				if ((u_int) y0 >= pr->pr_size.y)
					return 0;

				if (x < 0) {
					if ((dx += x) < 0)
						return 0;
					x = 0;
				}

				if (x + dx >= (w = pr->pr_size.x)) {
					if ((dx = w - x - 1) < 0)
						return 0;
				}
			}

			d = cg2_ropwordaddr(fb, 0, 
				x += prd->cgpr_offset.x, 
				y0 + prd->cgpr_offset.y);

			cg2_setlmask(fb, CG2_ALLROP,
				mrc_lmask(words = cg2_prskew(x)));

			cg2_setrmask(fb, CG2_ALLROP,
				mrc_rmask(cg2_prskew(words += dx)));
		}

		words >>= 4;
		cg2_setwidth(fb, CG2_ALLROP, words, words);
		cg2_setshift(fb, CG2_ALLROP, 0, 1);
		fb->status.reg.ropmode = PWRWRD;

		if (words < SWAP_ROPMODE_MIN || PIXOP_NEEDS_DST(op)) 
			PR_LOOPVP(words, *d++ = dummy);
		else {
			cg2_setwidth(fb, CG2_ALLROP, 2, 2);
			*d++ = dummy;
			*d++ = dummy;
			/* PRRWRD */
			fb->status.word ^= SWAP_ROPMODE_MASK;
			words -= 3;
			PR_LOOPVP(words, *d++ = dummy);
			/* PWRWRD */
			fb->status.word ^= SWAP_ROPMODE_MASK;
			*d = dummy;
		}

		return 0;
	}

	/*
	 * Need to normalize the vector so that the following
	 * algorithm can limit the number of cases to be considered.
	 * We can always interchange the points in x, so that
	 * x0, y0 is to the left of x1, y1.
	 */
	if (dx < 0) {
		register int tmp;

		dx = -dx;
		dy = -dy;
		swap(x0, x1, tmp);
		swap(y0, y1, tmp);
	}

	/*
	 * The clipping routine needs to work with a vector which
	 * increases in y from y0 to y1.  We want to change
	 * y0 and y1 so that there is an increase in y
	 * from y0 to y1 without affecting the clipping
	 * and bounds checking that we will do.  We accomplish
	 * this by reflecting the vector around the horizontal
	 * centerline of pr, and remember that we did this by
	 * incrementing reflect by 2.  The reflection will be undone
	 * before the vector is drawn.
	 */
	w = pr->pr_size.x;
	h = pr->pr_size.y;
	if (dy < 0) {
		dy = -dy;
		y0 = h - 1 - y0;
		y1 = h - 1 - y1;
		reflect = 2;
	}
	else
		reflect = 0;

	/*
	 * Can now do bounds check, since the vector increasing in
	 * x and y can check easily if it has no chance of intersecting
	 * the destination rectangle: if the vector ends before the
	 * beginning of the target or begins after the end!
	 */
	if (!noclip && 
		(y1 < 0 || y0 >= h || x1 < 0 || x0 >= w))
		return 0;

	/*
	 * One more reflection: we want to assume that dx >= dy.
	 * So if this is not true, we reflect the vector around
	 * the diagonal line x = y and remember that we did
	 * this by adding 1 to reflect.
	 */
	if (dx < dy) {
		register int tmp;

		swap(x0, y0, tmp);
		swap(x1, y1, tmp);
		swap(dx, dy, tmp);
		swap(w, h, tmp);
		reflect++;
	}

	initerror = - (dx >> 1);		/* error at x0, y0 */
	startx = x0;
	starty = y0;

	if (!noclip) {

	/*
	 * Begin hard part of clipping.
	 *
	 * We have insured that we are clipping on a vector
	 * which has dx > 0, dy > 0 and dx >= dy.  The error is
	 * the vertical distance from the true line to the approximation
	 * in units where each pixel is dx by dx.  Moving one
	 * to the right (increasing x by 1) subtracts dy from
	 * the error.  Moving one pixel down (increasing y by 1)
	 * adds dx to the error.
	 *
	 * Bresenham functions by restoring the error to the
	 * range (-dx,0] whenever the error leaves it.  The
	 * algorithm increases x and increases y when
	 * it needs to constrain the error.
	 */

	/*
 	 * If dx or dy are too large, scale to avoid overflow in
	 * multiplications below.
 	 */
	while (dx > MAXDELTA || dy > MAXDELTA) {
		dx >>= 1;
		dy >>= 1;
	}

	/*
	 * Clip left end to yield start of visible vector.
	 * If the starting x coordinate is negative, we
	 * must advance to the first x coordinate which will
	 * be drawn (x=0).  As we advance (-startx) units in the
	 * x direction, the error increases by (-startx)*dy.
	 * This means that the error when x=0 will be
	 *	-(dx/2)+(-startx)*dy
	 * For each y advance in this range, the error is reduced
	 * by dx, and should be in the range (-dx,0] at the y
	 * value at x=0.  Thus to compute the increment in y we
	 * should take
	 *	(-(dx/2)+(-startx)*dy+(dx-1))/dx
	 * where the numerator represents the length of the interval
	 *	[-dx+1,-(dx/2)+(-startx)*dy]
	 * The number of dx steps by which the error can be reduced
	 * and stay in this interval is the y increment which would
	 * result if the vector were drawn over this interval.
	 */
	if (startx < 0) {
		register int tmp;

		initerror += (-startx * dy);
		startx = 0;
		tmp = (initerror + (dx-1)) / dx;
		if ((starty += tmp) >= h)
			return 0;

		initerror -= (tmp * dx);
	}

	/*
	 * After having advanced x to be at least 0, advance
	 * y to be in range.  If y is already too large (and can
	 * only get larger!), just give up.  Otherwise, if starty < 0,
	 * we need to compute the value of x at which y is first 0.
	 * In advancing y to be zero, the error decreases by (-starty)*dx,
	 * in the y steps.
	 *
	 * Immediately after an advance in y the error is in the range
	 * (-dx,-dx+dy].  This can be seen by noting that what last
	 * happened was that the error was in the range (-dy,0],
	 * the error became positive by adding dy, to be in the range (0,dy],
	 * and we subtracted dx to get into the range (-dx,-dx+dy].
	 *
	 * Thus we need to advance x to cause the error to change
	 * to be at most (-dx+dy), or, in steps of dy, at most:
	 *	((-dx+dy)-error)/dy
	 * which is the number of dy steps in the interval [error,-dx+dy].
	 */

	/* skip to dst top edge */
	if (starty < 0) {
		register int tmp;

		initerror += (starty * dx);
		starty = 0;
		tmp = ((-dx+dy)-initerror)/dy;
		startx += tmp;
		initerror += (tmp * dy);
		if (startx >= w)
			return 0;
	}

	/*
	 * Now clip right end.
	 *
	 * If the last x position is outside the rectangle,
	 * then clip the vector back to within the rectangle
	 * at x=w-1.  The corresponding y value has initerror
	 *	-(dx/2)+((w-1)-x0)*dy
	 * We need an error in the range (-dx,0], so compute
	 * the corresponding number of steps in y from the number
	 * of dy's in the interval:
	 *	(-dx,-(dx/2)+((w-1)-x0)*dy]
	 * which is:
	 *	[-dx+1,-(dx/2)+((w-1)-x0)*dy]
	 * or:
	 *	(-(dx/2)+((w-1)-x0)*dy+dx-1)/dx
	 * Note that:
	 *	dx - (dx/2) != (dx/2)
	 * (consider dx odd), so we can't simplify much further.
	 */
	if (x1 >= w) {
		x1 = w - 1;
		y1 = y0 +
		    (-(dx>>1) + (((w-1)-x0) * dy) +
			dx-1)/dx;
	}
	/*
	 * If the last y position is outside the rectangle, then
	 * clip it back at y=h-1.  The corresponding x value
	 * has error
	 *	-(dx/2)-((h-1)-y0)*dx
	 * We want the last x value with this y value, which
	 * will have an error in the range (-dy,0] (so that increasing
	 * x one more time will make the error > 0.)  Thus
	 * the amount of error allocatable to dy steps is from
	 * the length of the interval:
	 *	[-(dx/2)-((h-1)-y0)*dx,0]
	 * that is,
	 *	(0-(-(dx/2)-((h-1)-y0)*dx))/dy
	 * or:
	 *	((dx/2)+((h-1)-y0)*dx)/dy
	 */
	if (y1 >= h) {
		y1 = h - 1;
		x1 = x0 +
		    ((dx>>1)+(((h-1)-y0) * dx))/dy;
	}

	} /* end of clipping */

	{
		register u_char *d;
		register LOOP_T count = x1 - startx;
		register int error = initerror;
		register int offset = cg2_linebytes(fb, SRWPIX);
#ifndef mc68000
		register int offset2;
#endif

		/* major axis is y */
		if (reflect & 1) {
			if (reflect & 2) {
				/* unreflect major axis (misnamed x) */
				startx = ((w-1) - startx);
				offset = -offset;
			}
			d = cg2_roppix(prd, fb, starty, startx);
#ifndef mc68000
			offset2 = 1;
#else   mc68000
			do {
				do {
					*d = dummy; 
					d += offset;
				} while ((error += dy) <= 0 && 
					LOOP_DECR(count));
				error -= dx;
				d++;
			} while (--count >= 0);
#endif  mc68000
		} 
		/* major axis is x */
		else {
			if (reflect & 2) {
				/* unreflect minor axis */
				starty = ((h-1) - starty);
				offset = -offset;
			}
			d = cg2_roppix(prd, fb, startx, starty);
#ifndef mc68000
			offset2 = offset;
			offset = 1;
#else   mc68000
			do {
				do {
					*d++ = dummy; 
				} while ((error += dy) <= 0 && 
					LOOP_DECR(count));
				error -= dx;
				d += offset;
			} while (--count >= 0);
#endif  mc68000
		}

#ifndef mc68000
		/* non-680x0 generic inner loop */
		do {
			*d = dummy;
			if ((error += dy) > 0) {
				d += offset2;
				error -= dx;
			}
			d += offset;
		} while (--count >= 0);
#endif !mc68000

	}

	return 0;
}

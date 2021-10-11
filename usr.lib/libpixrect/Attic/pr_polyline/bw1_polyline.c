#ifndef lint
static	char sccsid[] = "@(#)bw1_polyline.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * bw1_vec.c: Sun-1 Black-and-white pixrect vector
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/bw1reg.h>
#include <pixrect/bw1var.h>

#define swap(a,b,t)	(t) = (a); (a) = (b); (b) = (t);

extern char	pr_reversedst[];

extern	bw1_brespos();
extern	bw1_bresminneg();
extern	bw1_bresmajneg();
extern	bw1_horizln();
extern	bw1_vertln();

static (*bres[])() = {
	bw1_brespos, bw1_brespos, bw1_bresminneg, bw1_bresmajneg
};

/*
 * Draw a vector in dst from pos0 to pos1.  Hard part
 * here is clipping, which is done against the rectangle
 * of dst quickly and with proper interpolation so that
 * the jaggies in the vectors are independent of clipping.
 *
 * After eliminating the special cases where the vector
 * is horizontal or vertical, there remain to be considered
 * the cases where delta(x) > delta(y) and delta(x) < delta(y),
 * and also the difference in having delta(y) negative or positive.
 * We remember the four cases in the variable b, encoded as follows:
 *
 *		 minor	     major
 *	b	movement    movement
 *
 *	0	   +Y	       +X
 *	1	   +X	       +Y
 *	2	   -Y	       +X
 *	3	   +X	       -Y
 *
 * We define the correct vector to be one where, if delta(x) is
 * greater than delta(y) there are never 2 pixels drawn with the
 * same delta(x).  This is not the complete definition of the
 * vector (which is given below in the context of explaining
 * vector clipping).
 */
bw1_polyline(dst, x,y, n, op, color)
	struct pixrect *dst;
	int *x, *y, n;
	int op, color;
{
	register d7_dx, d6_dy, d5_count, d4_error, b = 0, col;
	register short *a5_rmaj, *a4_rmin;
	register struct bw1fb *fb;
	register struct bw1pr *dbd;
	struct pr_size size;
	struct pr_pos tempos, start;
	int clip;
	struct pr_pos pos0, pos1;

	dbd = bw1_d(dst);
	fb = dbd->bwpr_va;
	clip = !(op & PIX_DONTCLIP);
	col = PIX_OPCOLOR(op);
	if (col) color = col;
	op = (op>>1)&0xf;
	if (dbd->bwpr_flags & BW_REVERSEVIDEO)
		op = pr_reversedst[op];
	bw1_setfunction(fb, op);
	bw1_setmask(fb, 0);
	col = color ? ~0 : 0;		/* all 1 or 0 bits for horizln */
	while(--n) {
	    bw1_setwidth(fb, 1);
	    size = dst->pr_size;
	    b =0;
	    pos0.x = *x++;
	    pos0.y = *y++;
	    pos1.x = *x;
	    pos1.y = *y;
	d7_dx = pos1.x - pos0.x;
	d6_dy = pos1.y - pos0.y;

	/*
	 * Handle length 1 or 2 vectors by just drawing endpoints.
	 */
	if ((u_int)(d7_dx+1) <= 2 && (u_int)(d6_dy+1) <= 2) {
		if (clip == 0 ||
		    (u_int)pos0.x < size.x && (u_int)pos0.y < size.y) {
			bw1_touch(fb->update[0].reg[BW_REGNONE].cursor[0].x.
					xory[pos0.x + dbd->bwpr_offset.x]);
			fb->update[1].reg[BW_REGSOURCE].cursor[0].y.
					xory[pos0.y + dbd->bwpr_offset.y] = col;
		}
		if ((d7_dx != 0 || d6_dy != 0) &&
		    (clip == 0 ||
		    (u_int)pos1.x < size.x && (u_int)pos1.y < size.y)) {
			bw1_touch(fb->update[0].reg[BW_REGNONE].cursor[0].x.
					xory[pos1.x + dbd->bwpr_offset.x]);
			fb->update[1].reg[BW_REGSOURCE].cursor[0].y.
					xory[pos1.y + dbd->bwpr_offset.y] = col;
		}
/* 		return (0); */
		goto end;
	}

	/*
	 * Need to normalize the vector so that the following
	 * algorithm can limit the number of cases to be considered.
	 * We can always interchange the points in x, so that
	 * pos0 is to the left of pos1.
	 */
	if (d7_dx < 0) {
		/* force vector to scan to right */
		d7_dx = -d7_dx;
		d6_dy = -d6_dy;
		swap(pos0, pos1, tempos);
	}

	/*
	 * The clipping routine needs to work with a vector which
	 * increases in y from pos0.y to pos1.y.  We want to change
	 * pos0.y and pos1.y so that there is an increase in y
	 * from pos0.y to pos1.y without affecting the clipping
	 * and bounds checking that we will do.  We accomplish
	 * this by reflecting the vector around the horizontal
	 * centerline of dst, and remember that we did this by
	 * incrementing b by 2.  The reflection will be undone
	 * before the vector is drawn.
	 */
	if (d6_dy < 0) {
		d6_dy = -d6_dy;
		pos0.y = (size.y-1) - pos0.y;
		pos1.y = (size.y-1) - pos1.y;
		b += 2;
	}

	/*
	 * Can now do bounds check, since the vector increasing in
	 * x and y can check easily if it has no chance of intersecting
	 * the destination rectangle: if the vector ends before the
	 * beginning of the target or begins after the end!
	 */
	if (clip && 
	    (pos1.y < 0 || pos0.y >= size.y || pos1.x < 0 || pos0.x >= size.x))
/* 		return (0); */
		goto end;

	/*
	 * If vector is horizontal, use fast algorithm.
	 */
	if (d6_dy == 0) {
		bw1_touch(fb->update[0].reg[BW_REGNONE].cursor[0].y.
		    xory[pos0.y + dbd->bwpr_offset.y]);
		if (clip) {
			if (pos0.x < 0)
				pos0.x = 0;
			if (pos1.x >= size.x)
				pos1.x = size.x-1;
			d7_dx = pos1.x - pos0.x;
		}
		a5_rmaj =
		    &fb->update[1].reg[BW_REGSOURCE].cursor[0].x.
			xory[pos0.x + dbd->bwpr_offset.x];
		a4_rmin =	/* minor is actually width register */
		    &fb->update[0].reg[BW_REGOTHERS].cursor[3].x.
			xory[BWXY_WIDTH];
		bw1_horizln();
/* 		return (0); */
		goto end;
	}

	/*
	 * If vector is vertical, use fast algorithm.
	 */
	if (d7_dx == 0) {
		if (clip) {
			if (pos0.y < 0)
				pos0.y = 0;
			if (pos1.y >= size.y)
				pos1.y = size.y-1;
			d6_dy = pos1.y - pos0.y;
		}
		if (b & 2)	/* undo reflection */
			pos0.y = size.y - 1 - pos1.y;
		bw1_touch(fb->update[0].reg[BW_REGNONE].cursor[0].x.
		    xory[pos0.x + dbd->bwpr_offset.x]);
		a4_rmin =
		    &fb->update[1].reg[BW_REGSOURCE].cursor[0].y.
			xory[pos0.y + dbd->bwpr_offset.y];
		bw1_vertln();
/* 		return (0); */
		goto end;
	}

	/*
	 * One more reflection: we want to assume that d7_dx >= d6_dy.
	 * So if this is not true, we reflect the vector around
	 * the diagonal line x = y and remember that we did
	 * this by adding 1 to b.
	 */
	if (d7_dx < d6_dy) {
		swap(pos0.x, pos0.y, d5_count);
		swap(pos1.x, pos1.y, d5_count);
		swap(d7_dx, d6_dy, d5_count);
		swap(size.x, size.y, d5_count);
		b += 1;
	}
	d4_error = -(d7_dx>>1);		/* error at pos0 */
	start = pos0;
	if (!clip)
		goto clipdone;

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
	 * Clip left end to yield start of visible vector.
	 * If the starting x coordinate is negative, we
	 * must advance to the first x coordinate which will
	 * be drawn (x=0).  As we advance (-start.x) units in the
	 * x direction, the error increases by (-start.x)*dy.
	 * This means that the error when x=0 will be
	 *	-(dx/2)+(-start.x)*dy
	 * For each y advance in this range, the error is reduced
	 * by dx, and should be in the range (-dx,0] at the y
	 * value at x=0.  Thus to compute the increment in y we
	 * should take
	 *	(-(dx/2)+(-start.x)*dy+(dx-1))/dx
	 * where the numerator represents the length of the interval
	 *	[-dx+1,-(dx/2)+(-start.x)*dy]
	 * The number of dx steps by which the error can be reduced
	 * and stay in this interval is the y increment which would
	 * result if the vector were drawn over this interval.
	 */
	if (start.x < 0) {
		d4_error += pr_product(-start.x, d6_dy);
		start.x = 0;
		d5_count = (d4_error + (d7_dx-1)) / d7_dx;
		start.y += d5_count;
		d4_error -= pr_product(d5_count, d7_dx);
	}

	/*
	 * After having advanced x to be at least 0, advance
	 * y to be in range.  If y is already too large (and can
	 * only get larger!), just give up.  Otherwise, if start.y < 0,
	 * we need to compute the value of x at which y is first 0.
	 * In advancing y to be zero, the error decreases by (-start.y)*dx,
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
	if (start.y >= size.y)
/* 		return (0); */
		goto end;
	if (start.y < 0) {
		/* skip to dst top edge */
		d4_error += pr_product(start.y,d7_dx);
		start.y = 0;
		d5_count = ((-d7_dx+d6_dy)-d4_error)/d6_dy;
		start.x += d5_count;
		d4_error += pr_product(d5_count,d6_dy);
		if (start.x >= size.x)
/* 			return (0); */
		goto end;
	}

	/*
	 * Now clip right end.
	 *
	 * If the last x position is outside the rectangle,
	 * then clip the vector back to within the rectangle
	 * at x=size.x-1.  The corresponding y value has d4_error
	 *	-(dx/2)+((size.x-1)-pos0.x)*dy
	 * We need an error in the range (-dx,0], so compute
	 * the corresponding number of steps in y from the number
	 * of dy's in the interval:
	 *	(-dx,-(dx/2)+((size.x-1)-pos0.x)*dy]
	 * which is:
	 *	[-dx+1,-(dx/2)+((size.x-1)-pos0.x)*dy]
	 * or:
	 *	(-(dx/2)+((size.x-1)-pos0.x)*dy+dx-1)/dx
	 * Note that:
	 *	dx - (dx/2) != (dx/2)
	 * (consider dx odd), so we can't simplify much further.
	 */
	if (pos1.x >= size.x) {
		pos1.x = size.x - 1;
		pos1.y = pos0.y +
		    (-(d7_dx>>1) + pr_product((size.x-1)-pos0.x,d6_dy) +
			d7_dx-1)/d7_dx;
	}
	/*
	 * If the last y position is outside the rectangle, then
	 * clip it back at y=size.y-1.  The corresponding x value
	 * has error
	 *	-(dx/2)-((size.y-1)-pos0.y)*dx
	 * We want the last x value with this y value, which
	 * will have an error in the range (-dy,0] (so that increasing
	 * x one more time will make the error > 0.)  Thus
	 * the amount of error allocatable to dy steps is from
	 * the length of the interval:
	 *	[-(dx/2)-((size.y-1)-pos0.y)*dx,0]
	 * that is,
	 *	(0-(-(dx/2)-((size.y-1)-pos0.y)*dx))/dy
	 * or:
	 *	((dx/2)+((size.y-1)-pos0.y)*dx)/dy
	 */
	if (pos1.y >= size.y) {
		/* pos1.y = size.y - 1; Don't need to compute */
		pos1.x = pos0.x +
		    ((d7_dx>>1)+pr_product((size.y-1)-pos0.y,d7_dx))/d6_dy;
	}

clipdone:
	d5_count = pos1.x - start.x;
	/*
	 * Now have to set up the cursors and unreflect.
	 * There are really 2*2 cases.
	 *	X or Y as major axis
	 *		The major axis cursor is a5, the minor a4
	 *	Y increment positive or negative
	 *		If Y increment is negative, then we will
	 *		be pre-decrementing the cursor in the
	 *		bresenham routine, and have to both
	 *		un-reflect it and pre-increment it.
	 * Set up the cursors and call out the routine.
	 */
	if (b&1) {
		/* major axis is y */
		if (b&2)
			/* unreflect maj axis (misnamed x) and pre-increment */
			start.x = ((size.x-1) - start.x) + 1;
		a5_rmaj =
		    &fb->update[1].reg[BW_REGSOURCE].cursor[0].y.
			xory[start.x+dbd->bwpr_offset.y];
		a4_rmin =
		    &fb->update[0].reg[BW_REGNONE].cursor[0].x.
			xory[start.y + dbd->bwpr_offset.x];
	} else {
		/* major axis is x */
		if (b&2)
			/* unreflect minor axis and pre-increment */
			start.y = ((size.y-1) - start.y) + 1;
		a5_rmaj =
		    &fb->update[1].reg[BW_REGSOURCE].cursor[0].x.
			xory[start.x + dbd->bwpr_offset.x];
		a4_rmin =
		    &fb->update[0].reg[BW_REGNONE].cursor[0].y.
			xory[start.y+dbd->bwpr_offset.y];
	}
	(*bres[b])();
/* 	return (0); */
		goto end;
end: ;
}
}

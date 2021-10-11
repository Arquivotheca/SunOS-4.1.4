#ifndef lint
static	char sccsid[] = "@(#)mem_vec_v1.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Memory pixrect vector
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>

#define swap(a,b,t)	{(t) = (a); (a) = (b); (b) = (t);}

extern mem_vertln_clr(), mem_vertln_not(), mem_vertln_set();
extern mem_horizln_clr(), mem_horizln_not(), mem_horizln_set();
extern mem_bres_colseg_clr(), mem_bres_colseg_not(), mem_bres_colseg_set();
extern mem_bres_rowsegs_clr(), mem_bres_rowsegs_not(), mem_bres_rowsegs_set();
extern mem_bres_rowsegl_clr(), mem_bres_rowsegl_not(), mem_bres_rowsegl_set();

#define MEM_VCODESIZE	16
extern short			/* 8bit mem_vec code generators */
	*mem_vec_majorx[17], *mem_vec_majory[17],
	mem_addla4a5, mem_addwd6d4, mem_addql1a5, mem_subwd7d4,
	mem_tstwd5, mem_dbgtd5, mem_dbltd5, mem_dbfd6,
	mem_dbfd7, mem_vec_rts;

#define CODE(table, op) {						\
    short *cs, *ce;							\
    cs = table[op]; ce = table[(op)+1];					\
    while (cs < ce)							\
	*pc++ = *cs++;							\
}


static (*vertln[])() =
	{
	mem_vertln_clr, mem_vertln_not, mem_vertln_set
	};

static (*horizln[])() =
	{
	mem_horizln_clr, mem_horizln_not, mem_horizln_set
	};

static (*bres[])() =
	{
	mem_bres_colseg_clr, mem_bres_colseg_not, mem_bres_colseg_set,
	mem_bres_rowsegs_clr, mem_bres_rowsegs_not, mem_bres_rowsegs_set,
	mem_bres_rowsegl_clr, mem_bres_rowsegl_not, mem_bres_rowsegl_set
	};

static int optable[4] = {0, 1, -1, 2};
extern char	pr_reversedst[];

mem_vector(dst, pos0, pos1, op, color)
struct pixrect *dst;
struct pr_pos pos0, pos1;
int op, color;
	{
	register d7_dx, d6_dy, d5_count, d4_initerror, d3_skew;
							/* d7,d6,d5,d4,d3 */
	register short *a5_adr, *a4_vert, *a3_enderror, *a2_dxclip;
							/* a5,a4,a3,a2 */
	int clip, reflect = 0, col;
	struct mpr_data *dmd;
	struct pr_size size;
	struct pr_pos tempos, start;
	short mem_codespace[MEM_VCODESIZE];
	short depth1;

	depth1 = (dst->pr_depth == 1);
	size = dst->pr_size;
	dmd = mpr_d(dst);
	clip = !(op & PIX_DONTCLIP);
	col = PIX_OPCOLOR(op);
	if (col) color = col;
	op = (op>>1)&0xf;
	if (dmd->md_flags & MP_REVERSEVIDEO)
		op = (pr_reversedst[op]);
	if (depth1) {
		if ( color) op >>= 2;
		op = optable[op & 3];
		if (op < 0)
			return(0);
	}

	d7_dx = pos1.x - pos0.x;
	d6_dy = pos1.y - pos0.y;

	/*
	 * Handle length 1 or 2 vectors by just drawing endpoints.
	 */
	if ((u_int)(d7_dx+1) <= 2 && (u_int)(d6_dy+1) <= 2 && depth1) {
		if (clip == 0 ||
		    (u_int)pos0.x < size.x && (u_int)pos0.y < size.y) {
			a5_adr = mprd_addr(dmd, pos0.x, pos0.y);
			d3_skew = 0x8000 >> mprd_skew(dmd, pos0.x, pos0.y);
			switch (op)
				{
			case 0:	*a5_adr &= ~d3_skew;
				break;
			case 1:	*a5_adr ^= d3_skew;
				break;
			case 2:	*a5_adr |= d3_skew;
				break;
				}
		}
		if ((d7_dx != 0 || d6_dy != 0) &&
		    (clip == 0 ||
		    (u_int)pos1.x < size.x && (u_int)pos1.y < size.y)) {
			a5_adr = mprd_addr(dmd, pos1.x, pos1.y);
			d3_skew = 0x8000 >> mprd_skew(dmd, pos1.x, pos1.y);
			switch (op)
				{
			case 0:	*a5_adr &= ~d3_skew;
				break;
			case 1:	*a5_adr ^= d3_skew;
				break;
			case 2:	*a5_adr |= d3_skew;
				break;
				}
		}
		return (0);
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
	 * incrementing reflect by 2.  The reflection will be undone
	 * before the vector is drawn.
	 */
	if (d6_dy < 0) {
		d6_dy = -d6_dy;
		pos0.y = (size.y-1) - pos0.y;
		pos1.y = (size.y-1) - pos1.y;
		reflect += 2;
	}

	/*
	 * Can now do bounds check, since the vector increasing in
	 * x and y can check easily if it has no chance of intersecting
	 * the destination rectangle: if the vector ends before the
	 * beginning of the target or begins after the end!
	 */
	if (clip && 
	    (pos1.y < 0 || pos0.y >= size.y || pos1.x < 0 || pos0.x >= size.x))
		return (0);

	/*
	 * If vector is horizontal, use fast algorithm.
	 */
	if (d6_dy == 0) {
		if (clip) {
			if (pos0.x < 0)
				pos0.x = 0;
			if (pos1.x >= size.x)
				pos1.x = size.x-1;
			d7_dx = pos1.x - pos0.x;
		}
		if (depth1) {
			a5_adr = mprd_addr(dmd, pos0.x, pos0.y);
			d3_skew = mprd_skew(dmd, pos0.x, pos0.y);
			(*horizln[op])();
		} else {		/* compile and run depth8 code */
			short *pc, *ipc;
			d3_skew = color;
			a5_adr = (short *)mprd8_addr
				(dmd,pos0.x,pos0.y,dst->pr_depth);
			pc = mem_codespace;
			ipc = pc;
			CODE( mem_vec_majorx, op);
			*pc++ = mem_dbfd7;
			*pc = (int)ipc - (int)pc; pc++;
			*pc++ = mem_vec_rts;
			(*((int (*)())mem_codespace))();
		}
		return (0);
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
		if (reflect & 2)	/* undo reflection */
			pos0.y = size.y - 1 - pos1.y;
		if (depth1) {
			a5_adr = mprd_addr(dmd, pos0.x, pos0.y);
			d3_skew = mprd_skew(dmd, pos0.x, pos0.y);
			a4_vert = (short *) dmd->md_linebytes;
			(*vertln[op])();
		} else {		/* compile and run depth8 code */
			short *pc, *ipc;
			d3_skew = color;
			a5_adr = (short *)mprd8_addr
				(dmd,pos0.x,pos0.y,dst->pr_depth);
			a4_vert = (short*)dmd->md_linebytes;
			pc = mem_codespace;
			ipc = pc;
			CODE( mem_vec_majory, op);
			*pc++ = mem_addla4a5;
			*pc++ = mem_dbfd6;
			*pc = (int)ipc - (int)pc; pc++;
			*pc++ = mem_vec_rts;
			(*((int (*)())mem_codespace))();
		}
		return (0);
	}

	/*
	 * One more reflection: we want to assume that d7_dx >= d6_dy.
	 * So if this is not true, we reflect the vector around
	 * the diagonal line x = y and remember that we did
	 * this by adding 1 to reflect.
	 */
	if (d7_dx < d6_dy) {
		swap(pos0.x, pos0.y, d5_count);
		swap(pos1.x, pos1.y, d5_count);
		swap(d7_dx, d6_dy, d5_count);
		swap(size.x, size.y, d5_count);
		reflect += 1;
	}

	d4_initerror = -(d7_dx>>1);		/* error at pos0 */
	start = pos0;
	if (!clip)
		{
		d5_count = d6_dy;
		a2_dxclip = (short *) d7_dx;
		a3_enderror = (short *) d4_initerror;
		goto clipdone;
		}

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
		d4_initerror += pr_product(-start.x, d6_dy);
		start.x = 0;
		d5_count = (d4_initerror + (d7_dx-1)) / d7_dx;
		start.y += d5_count;
		d4_initerror -= pr_product(d5_count, d7_dx);
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
		return (0);
	if (start.y < 0) {
		/* skip to dst top edge */
		d4_initerror += pr_product(start.y,d7_dx);
		start.y = 0;
		d5_count = ((-d7_dx+d6_dy)-d4_initerror)/d6_dy;
		start.x += d5_count;
		d4_initerror += pr_product(d5_count,d6_dy);
		if (start.x >= size.x)
			return (0);
	}

	/*
	 * Now clip right end.
	 *
	 * If the last x position is outside the rectangle,
	 * then clip the vector back to within the rectangle
	 * at x=size.x-1.  The corresponding y value has d4_initerror
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
		pos1.y = size.y - 1;
		pos1.x = pos0.x +
		    ((d7_dx>>1)+pr_product((size.y-1)-pos0.y,d7_dx))/d6_dy;
	}

	if (depth1)
		d5_count = pos1.y - start.y;
	else	d5_count = pos1.x - start.x;
	a2_dxclip = (short *) (pos1.x - start.x);
	a3_enderror = (short *) (d4_initerror +
	    pr_product((int) a2_dxclip, d6_dy) - pr_product(d5_count, d7_dx));

clipdone:
	/*
	 * Now have to set up for the Bresenham routines.
	 */

	a4_vert = (short *) dmd->md_linebytes;
	if (reflect & 1) {
		/* major axis is y */
		if (reflect & 2) {
			/* unreflect major axis (misnamed x) */
			start.x = ((size.x-1) - start.x);
			a4_vert = (short *) -((int) a4_vert);
		}
		if (depth1) {
			a5_adr = mprd_addr(dmd, start.y, start.x);
			d3_skew = mprd_skew(dmd, start.y, start.x);
			(*bres[op])();
		} else {		/* compile and run depth8 code */
			short *pc, *min, *maj;
			d3_skew = color;
			a5_adr = (short *)mprd8_addr
				(dmd,start.y,start.x,dst->pr_depth);
			pc = mem_codespace;
			min = pc;
			*pc++ = mem_addql1a5;
			maj = pc;
			CODE( mem_vec_majory, op);
			*pc++ = mem_addla4a5;
			*pc++ = mem_addwd6d4;
			*pc++ = mem_dbgtd5;
			*pc = (int)maj - (int)pc; pc++;
			*pc++ = mem_subwd7d4;
			*pc++ = mem_tstwd5;
			*pc++ = mem_dbltd5;
			*pc = (int)min - (int)pc; pc++;
			*pc++ = mem_vec_rts;
			(*((int (*)())maj))();
		}
	} else {
		/* major axis is x */
		if (reflect & 2) {
			/* unreflect minor axis */
			start.y = ((size.y-1) - start.y);
			a4_vert = (short *) -((int) a4_vert);
		}
		if (depth1) {
			a5_adr = mprd_addr(dmd, start.x, start.y);
			d3_skew = mprd_skew(dmd, start.x, start.y);
			if (d7_dx <= (d6_dy << 4)) {
				(*bres[op + 3])();
			} else {
				(*bres[op + 6])();
			}
		} else {		/* compile and run depth8 code */
			short *pc, *min, *maj;
			d3_skew = color;
			a5_adr = (short *)mprd8_addr
				(dmd,start.x,start.y,dst->pr_depth);
			pc = mem_codespace;
			min = pc;
			*pc++ = mem_addla4a5;
			maj = pc;
			CODE( mem_vec_majorx, op);
			*pc++ = mem_addwd6d4;
			*pc++ = mem_dbgtd5;
			*pc = (int)maj - (int)pc; pc++;
			*pc++ = mem_subwd7d4;
			*pc++ = mem_tstwd5;
			*pc++ = mem_dbltd5;
			*pc = (int)min - (int)pc; pc++;
			*pc++ = mem_vec_rts;
			(*((int (*)())maj))();
		}
	}
	return (0);
	}

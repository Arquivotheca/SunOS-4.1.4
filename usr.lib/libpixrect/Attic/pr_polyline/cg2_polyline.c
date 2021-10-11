#ifndef lint
static	char sccsid[] = "@(#)cg2_polyline.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Sun 2 color pixrect vector
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>

#define swap(a,b,t)	{(t) = (a); (a) = (b); (b) = (t);}

extern short mrc_lmasktable[];
extern short mrc_rmasktable[];

extern cg2_vertln();
extern cg2_bres_majx();
extern cg2_bres_majy();

cg2_polyline(dst, x,y, n, op, color)
	struct pixrect *dst;
	int *x, *y, n;
	int op, color;
{
	register d7_dx, d6_dy, d5_count, d4_initerror, d3_color;
							/* d7,d6,d5,d4,d3 */
	register u_char *a5_adr, *a4_vert, *a3_enderror, *a2_dxclip;
							/* a5,a4,a3,a2 */
	int clip, reflect = 0, col;
	struct cg2pr *dmd;
	struct cg2fb *fb;
	struct pr_size size;
	struct pr_pos tempos, start;
    struct pr_pos pos0, pos1;

	if (dst->pr_depth != 8)
 		return(-1); 
	dmd = cg2_d(dst);
	fb  = dmd->cgpr_va;
	clip = !(op & PIX_DONTCLIP);
	col = PIX_OPCOLOR(op);
	if (col) color = col;
	op = (op>>1) & 0xf;

	while(--n) {
	    size = dst->pr_size;
	    reflect =0;
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
		if (PIXOP_NEEDS_DST(op<<1))		/* setup rasterop */
			fb->status.reg.ropmode = SWWPIX;
		else	fb->status.reg.ropmode = SRWPIX;
		fb->ppmask.reg = dmd->cgpr_planes;
		cg2_setfunction( fb, CG2_ALLROP, op);
		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_pattern = 0;
		fb->ropcontrol[CG2_ALLROP].prime.ropregs.mrc_source1 =
			color | (color<<8);	/* load color in src */
		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 = 0;
		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 = 0;
		cg2_setwidth(fb, CG2_ALLROP, 0, 0);
		cg2_setshift( fb, CG2_ALLROP, 0, 1);
		if (clip == 0 ||
		    (u_int)pos0.x < size.x && (u_int)pos0.y < size.y) {
			a5_adr = cg2_roppixel( dmd, pos0.x, pos0.y);
			*a5_adr = color;
		}
		if ((d7_dx != 0 || d6_dy != 0) &&
		    (clip == 0 ||
		    (u_int)pos1.x < size.x && (u_int)pos1.y < size.y)) {
			a5_adr = cg2_roppixel( dmd, pos1.x, pos1.y);
			*a5_adr = color;
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
/* 		return (0); */
		goto end;

	/*
	 * If vector is horizontal, use fast algorithm.
	 */
	if (d6_dy == 0) {
		short nodst, ropmode;
		fb->ppmask.reg = dmd->cgpr_planes;
		cg2_setfunction( fb, CG2_ALLROP, op);
		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_pattern = 0;
		fb->ropcontrol[CG2_ALLROP].prime.ropregs.mrc_source2 =
			color | (color<<8);	/* load color in src */

		if (clip) {
			if (pos0.x < 0) pos0.x = 0;
			if (pos1.x >= size.x) pos1.x = size.x-1;
			d7_dx = pos1.x - pos0.x;
		}
		d5_count = cg2_prd_skew( dmd, pos1.x);
		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 =
			mrc_rmasktable[d5_count];
		d5_count = cg2_prd_skew( dmd, pos0.x);
		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 =
			mrc_lmasktable[d5_count];

		nodst = (PIXOP_NEEDS_DST(op<<1) == 0);
		if (nodst)
			ropmode = PRRWRD;	/* load dst when necessary */
		else    ropmode = PWRWRD;	/* but never load src */
		fb->status.reg.ropmode = PWRWRD; /* ld dst for mask */

		d5_count = (d7_dx + d5_count)>>4;
		cg2_setwidth(fb, CG2_ALLROP, d5_count, d5_count);
		cg2_setshift( fb, CG2_ALLROP, 0, 1);

		a5_adr = (u_char *)cg2_ropword( dmd, 0, pos0.x, pos0.y);
		switch (d5_count) {
		case 0:
			*((short*)a5_adr) = d3_color;
			break;
		case 1:
			*((short*)a5_adr)++ = d3_color;
			*((short*)a5_adr)++ = d3_color;
			break;
		default:
			if (nodst) {
			    cg2_setwidth( fb, CG2_ALLROP, 2, 2);
			    d5_count -= 2;
			    *((short*)a5_adr)++ = d3_color;
			    *((short*)a5_adr)++ = d3_color;
			    fb->status.reg.ropmode = ropmode;
			    rop_fastloop( d5_count,
				*((short *)a5_adr)++ = d3_color);
			    fb->status.reg.ropmode = PWRWRD;
			    *((short*)a5_adr)++ = d3_color;
			} else {
			    rop_fastloop( d5_count,
				*((short *)a5_adr)++ = d3_color);
			    *((short*)a5_adr)++ = d3_color;
			}
		}
/* 		return (0); */
		goto end;
	}

	/*
	 * If vector is vertical, use fast algorithm.
	 */
	if (d7_dx == 0) {
		fb->ppmask.reg = dmd->cgpr_planes;
		cg2_setfunction( fb, CG2_ALLROP, op);
		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_pattern = 0;
		fb->ropcontrol[CG2_ALLROP].prime.ropregs.mrc_source1 =
			color | (color<<8);	/* load color in src */

		if (PIXOP_NEEDS_DST(op<<1))		/* setup rasterop */
			fb->status.reg.ropmode = SWWPIX;
		else	fb->status.reg.ropmode = SRWPIX;

		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 = 0;
		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 = 0;
		cg2_setshift( fb, CG2_ALLROP, 0, 1);
		cg2_setwidth( fb, CG2_ALLROP, 0, 0);
		if (clip) {
			if (pos0.y < 0)
				pos0.y = 0;
			if (pos1.y >= size.y)
				pos1.y = size.y-1;
			d6_dy = pos1.y - pos0.y;
		}
		if (reflect & 2)	/* undo reflection */
			pos0.y = size.y - 1 - pos1.y;
		a5_adr = cg2_roppixel( dmd, pos0.x, pos0.y);
		a4_vert = (u_char *)cg2_linebytes( fb, SRWPIX);
		d3_color = color;
		cg2_vertln();
/* 		return (0); */
		goto end;
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
		a2_dxclip = (u_char *) d7_dx;
		a3_enderror = (u_char *) d4_initerror;
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
/* 		return (0); */
		goto end;
	if (start.y < 0) {
		/* skip to dst top edge */
		d4_initerror += pr_product(start.y,d7_dx);
		start.y = 0;
		d5_count = ((-d7_dx+d6_dy)-d4_initerror)/d6_dy;
		start.x += d5_count;
		d4_initerror += pr_product(d5_count,d6_dy);
		if (start.x >= size.x)
/* 			return (0); */
		goto end;
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

	d5_count = pos1.y - start.y;
	a2_dxclip = (u_char *) (pos1.x - start.x);
	a3_enderror = (u_char *) (d4_initerror +
	    pr_product((int) a2_dxclip, d6_dy) - pr_product(d5_count, d7_dx));

clipdone:
	/*
	 * Now have to set up for the Bresenham routines.
	 */

	d5_count = pos1.x - start.x;
	fb->ppmask.reg = dmd->cgpr_planes;
	cg2_setfunction( fb, CG2_ALLROP, op);
	fb->ropcontrol[CG2_ALLROP].ropregs.mrc_pattern = 0;
	fb->ropcontrol[CG2_ALLROP].prime.ropregs.mrc_source1 =
		color | (color<<8);	/* load color in src */

	if (PIXOP_NEEDS_DST(op<<1))		/* setup rasterop */
		fb->status.reg.ropmode = SWWPIX;
	else	fb->status.reg.ropmode = SRWPIX;

	fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 = 0;
	fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 = 0;
	cg2_setshift( fb, CG2_ALLROP, 0, 1);
	cg2_setwidth( fb, CG2_ALLROP, 0, 0);
	a4_vert = (u_char *)cg2_linebytes( fb, SRWPIX);
	d3_color = color;
	if (reflect & 1) {
		/* major axis is y */
		if (reflect & 2) {
			/* unreflect major axis (misnamed x) */
			start.x = ((size.x-1) - start.x);
			a4_vert = (u_char *) -((int) a4_vert);
		}
		a5_adr = cg2_roppixel(dmd, start.y, start.x);
		cg2_bres_majy();
	} else {
		/* major axis is x */
		if (reflect & 2) {
			/* unreflect minor axis */
			start.y = ((size.y-1) - start.y);
			a4_vert = (u_char *) -((int) a4_vert);
		}
		a5_adr = cg2_roppixel(dmd, start.x, start.y);
		cg2_bres_majx();
	}
/* 	return (0); */
		goto end;
end: ;
}
}

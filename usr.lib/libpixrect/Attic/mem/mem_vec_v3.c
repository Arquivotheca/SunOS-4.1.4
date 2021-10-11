#ifndef lint
static char sccsid[] = "@(#)mem_vec_v3.c 1.1 94/10/31";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

/*
 * Memory pixrect vector.
 *
 * Mike Shantz,  Sun Microsystems
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_util.h>

#define swap(a,b,t)  {(t) = (a); (a) = (b); (b) = (t);}

extern char pr_reversedst[];

/*
 * The 16 possible rasterops.  Given opcode i, the correct function to
 * execute it is Fi.  These are used for 8bit memory pixrects.
 */
#define F0(d,s) (0)
#define F1(d,s) (~((d)|(s)))
#define F2(d,s) ((d)&~(s))
#define F3(d,s) (~(s))
#define F4(d,s) (~(d)&(s))
#define F5(d,s) (~(d))
#define F6(d,s) ((d)^(s))
#define F7(d,s) (~((d)&(s)))
#define F8(d,s) ((d)&(s))
#define F9(d,s) (~((d)^(s)))
#define FA(d,s) (d)
#define FB(d,s) ((d)|~(s))
#define FC(d,s) (s)
#define FD(d,s) (~(d)|(s))
#define FE(d,s) ((d)|(s))
#define FF(d,s) (~0)

/* Rasterops for depth 1, source is all zeros: CAT(SF, (op & 3))
 * 			or   all ones: CAT(SF, (op >> 2))
 */
#define SF0(d,s) ((d)&~(s))
#define SF1(d,s) ((d)^(s))
#define SF2(d,s) (d)
#define SF3(d,s) ((d)|(s))   

/* Rasterops for depth 8, source is all zeros: CAT(SF8, (op & 3))
 * 			or   all ones: CAT(SF8, (op >> 2))
 */
#define SF80(d,s) (0)
#define SF81(d,s) (~(d))
#define SF82(d,s) (d)
#define SF83(d,s) (~0)   

/* Macros for rasterops when planes != -1 and source is
 *			all zeros: CAT(PMS, (op & 3))
 *                      or   all ones: CAT(PMS, (op >> 2))
 */

#define PMS0(d,s) ((d) & ~planes)
#define PMS1(d,s) ((d) ^ planes)
#define PMS2(d,s) (d)
#define PMS3(d,s) ((d) | planes)

/* Macros for rasterops when planes != -1 and source is arbitrary */

#define PM0(d,s) ((d) & ~planes)
#define PM1(d,s) ((d) ^ (planes  & ((d) | ~(s))))
#define PM2(d,s) ((d) & ~(planes & (s)))
#define PM3(d,s) ((d) ^ (planes  & ((d) ^ ~(s))))
#define PM4(d,s) ((d) ^ (planes  & ((d) | (s))))
#define PM5(d,s) ((d) ^ planes)
#define PM6(d,s) ((d) ^ (planes  & (s)))
#define PM7(d,s) ((d) ^ (planes  & (~(d) | (s))))
#define PM8(d,s) ((d) & (~planes | (s)))
#define PM9(d,s) ((d) ^ (planes  & ~(s)))
#define PMA(d,s) (d)
#define PMB(d,s) ((d) | (planes  & ~(s)))
#define PMC(d,s) ((d) ^ (planes  & ((d) ^ (s))))
#define PMD(d,s) ((d) ^ (planes  & ~((d) & (s))))
#define PME(d,s) ((d) | (planes  & (s)))
#define PMF(d,s) ((d) | planes)

/* Macros to mask the ends of depth 1 horizontal vectors */
#define M0(m)   (*dst &= ~(m))
#define M1(m)   (*dst ^= (m))
#define M2(m)
#define M3(m)   (*dst |= (m))

/* Booleans to do special cases for depth, planes, color */
#define depth1(x,y) x
#define depth8(x,y) y
#define allbits(x,y) x
#define arbval(x,y) y

/* Macro to execute a macro for an opcode. */
#define CASE_BINARY_OP(op,macro,depth,wrtenab,source)\
    switch(op) {\
	CASEOP(0,macro,depth,wrtenab,source)\
	CASEOP(1,macro,depth,wrtenab,source)\
	CASEOP(2,macro,depth,wrtenab,source)\
	CASEOP(3,macro,depth,wrtenab,source)\
	source(,\
	CASEOP(4,macro,depth,wrtenab,source)\
	CASEOP(5,macro,depth,wrtenab,source)\
	CASEOP(6,macro,depth,wrtenab,source)\
	CASEOP(7,macro,depth,wrtenab,source)\
	CASEOP(8,macro,depth,wrtenab,source)\
	CASEOP(9,macro,depth,wrtenab,source)\
	CASEOP(B,macro,depth,wrtenab,source)\
	CASEOP(C,macro,depth,wrtenab,source)\
	CASEOP(D,macro,depth,wrtenab,source)\
	CASEOP(E,macro,depth,wrtenab,source)\
	CASEOP(F,macro,depth,wrtenab,source)\
	)\
    }

#define CASEOP(op,macro,depth,wrtenab,source)\
    case CAT(0x,op): macro(op,depth,wrtenab,source);\
	break;

#define VISIBLE(ax,ay)\
    (clip == 0 || (u_int)(ax) < size.x && (u_int)(ay) < size.y)

#define rolledloop(count,s) {\
	register short cnt;\
	if ((cnt = (count)-1) >= 0)\
	do {s;} while (--cnt != -1);	\
}

/* Macro to draw points in 1 and 8 bit memory pixrects.  This macro
 * does special cases for depth 1 or 8, plane mask of allones, allzeros
 * or arbitrary, and source color of allones, allzeros, or arbitrary.
 * It assumes that op has been adjusted for special source cases.
 */
#define POINT(op, depth, wrtenab, source)\
depth(*dst =CAT(SF,op)(*dst,color),\
    wrtenab(source(*dst =CAT(SF8,op)(*dst,color),\
	    *dst =CAT(F,op)(*dst,color)),\
	source(*dst = CAT(PMS,op)(*dst, color),\
	    *dst = CAT(PM,op)(*dst, color))\
))

/* Macro to do depth 1 and 8 horizontal vectors.  When doing depth 1,
 * source MUST be allbits.  If source is allbits then if source is all
 * zeros, op = op&3, else if source is all ones op = op>>2.  wrtenab has
 * no effect on depth1.
 */
#define HORIZ(op,depth,wrtenab,source) {\
    depth(CAT(M,op)(mask1); dst++;,)\
    wrtenab(source(\
	rolledloop(count,((*dst = depth(CAT(SF,op)(*dst,color),\
		CAT(SF8,op)(*dst,color))), dst++));,\
	rolledloop(count,((*dst = CAT(F,op)(*dst,color)), dst++));),\
	source(\
	rolledloop(count,((*dst = depth(CAT(SF,op)(*dst,color),\
		CAT(PMS,op)(*dst,color))),dst++));,\
	rolledloop(count,((*dst = depth(CAT(SF,op)(*dst,color),\
		CAT(PM,op)(*dst,color))),dst++));)\
    )\
    depth(if (count >= 0) {CAT(M,op)(mask2); dst++;},)\
}

/* Macro to do depth 1 and 8 vertical vectors.  When doing depth 1,
 * source MUST be allbits.  If source is allbits then if source is all
 * zeros, op = op&3, else if source is all ones op = op>>2.  wrtenab has
 * no effect on depth1.
 */
#define VERT(op,depth,wrtenab,source) {\
wrtenab(source(\
    rolledloop(count,depth(*dst = CAT(SF,op)(*dst,color);\
	    (u_char *)dst += (int)vrtstep;,\
	    *dst = CAT(SF8,op)(*dst,color); dst += (int)vrtstep;)),\
    rolledloop(count,depth(*dst = CAT(SF,op)(*dst,color);\
	    (u_char *)dst += (int)vrtstep;,\
	    *dst = CAT(F,op)(*dst,color); dst += (int)vrtstep;))),\
    source(\
    rolledloop(count,depth(*dst = CAT(SF,op)(*dst,color);\
	    (u_char *)dst += (int)vrtstep;,\
	    *dst = CAT(PMS,op)(*dst,color); dst += (int)vrtstep;)),\
    rolledloop(count,depth(*dst = CAT(SF,op)(*dst,color);\
	    (u_char *)dst += (int)vrtstep;,\
	    *dst = CAT(PM,op)(*dst,color); dst += (int)vrtstep;)))\
)\
}

#define MAJORX(op,depth,wrtenab,source)\
    do {\
        do {\
	    POINT(op,depth,wrtenab,source);\
	    depth(\
		if((color >>= 1) == 0) {color = 0x8000; dst++;},\
		dst++;)\
	} while (((error += dy) <= 0) && (--count != -1));\
	error -= dx;\
	depth((u_char *)dst += (int)vrtstep;, dst += (int)vrtstep;)\
    } while ((count >= 0) && (--count != -1));

#define MAJORY(op,depth,wrtenab,source)\
    do {\
	do {\
	    POINT(op,depth,wrtenab,source);\
	    depth((u_char *)dst += (int)vrtstep;, dst += (int)vrtstep;)\
	} while (((error += dy) <= 0) && (--count != -1));\
	error -= dx;\
	depth(\
	    if((color >>= 1) == 0) {color = 0x8000; dst++;},\
	    dst++;)\
    } while ((count >= 0) && (--count != -1));







mem_vector (dpr, x0, y0, x1, y1, op, colour)
    struct pixrect *dpr;
{
    struct mprp_data *mprd = (struct mprp_data *) dpr->pr_data;
    register	dx, dy;
    register	count, error, color;
    register	u_char *vrtstep;
    struct pr_size size;
    int xs, ys;			/* start position */
    int clip, reflect, mprplanes;

    color = PIX_OPCOLOR(op);
    if (!color) color = colour;
    clip = !(op & PIX_DONTCLIP);
    op = (op >> 1) & 0xF;
    if (mprd->mpr.md_flags & MP_REVERSEVIDEO)
	op = (pr_reversedst[op]);
    if (mprd->mpr.md_flags & MP_PLANEMASK)
	    mprplanes = mprd->planes;
    else mprplanes = ~0;
    if ((op == (PIX_DST >> 1)) || (mprplanes == 0))
	return (0);
    dx = x1 - x0;
    dy = y1 - y0;
    size = dpr->pr_size;


/* 
 * Handle length 1 or 2 vectors by just drawing endpoints.
 */
    if ((u_int) (dx + 1) <= 2 && (u_int) (dy + 1) <= 2) {
	if (dpr->pr_depth == 1) {
	    register short *dst;
	    op = (color) ? op>>2 : op&3;
	    if (VISIBLE(x0, y0)) {
		dst = mprd_addr (&mprd->mpr, x0, y0);
		color = 0x8000 >> mprd_skew (&mprd->mpr, x0, y0);
	        CASE_BINARY_OP(op,POINT,depth1,allbits,allbits)
	    }
	    if ((dx != 0 || dy != 0) && VISIBLE(x1, y1)) {
		dst = mprd_addr (&mprd->mpr, x1, y1);
		color = 0x8000 >> mprd_skew (&mprd->mpr, x1, y1);
	        CASE_BINARY_OP(op,POINT,depth1,allbits,allbits)
	    }
	} else /* 8 bit */ {
	    register u_char *dst;
	    register u_char planes = mprplanes;
	    if (VISIBLE(x0, y0)) {
		dst = mprd8_addr(&mprd->mpr, x0, y0, dpr->pr_depth);
		CASE_BINARY_OP(op,POINT,depth8,arbval,arbval)
	    }
	    if ((dx != 0 || dy != 0) && VISIBLE(x1, y1)) {
		dst = mprd8_addr (&mprd->mpr, x1, y1, dpr->pr_depth);
		CASE_BINARY_OP(op,POINT,depth8,arbval,arbval)
	    }
	}
	return (0);
    }

/* 
 * Need to normalize the vector so that the following algorithm can limit
 * the number of cases to be considered.  We can always interchange the
 * points in x, so that pos0 is to the left of pos1.
 */
    if (dx < 0) {
        /* force vector to scan to right */
	dx = -dx;
	dy = -dy;
	swap (x0, x1, count);
	swap (y0, y1, count);
    }

/* 
 * The clipping routine needs to work with a vector which increases in y
 * from y0 to y1.  We want to change y0 and y1 so that there
 * is an increase in y from y0 to y1 without affecting the clipping
 * and bounds checking that we will do.  We accomplish this by reflecting
 * the vector around the horizontal centerline of dst, and remember that
 * we did this by incrementing reflect by 2.  The reflection will be undone
 * before the vector is drawn.
 */
    reflect = 0;
    if (dy < 0) {
	dy = -dy;
	y0 = (size.y - 1) - y0;
	y1 = (size.y - 1) - y1; 
	reflect += 2;
    }

/* 
 * Can now do bounds check, since the vector increasing in x and y can check
 * easily if it has no chance of intersecting the destination rectangle: if
 * the vector ends before the beginning of the target or begins after the end!
 */
    if (clip &&
	    (y1 < 0 || y0 >= size.y || x1 < 0 || x0 >= size.x))
	return (0);

/* 
 * Special case for Horizontal lines
 */
    if (dy == 0) {
	if (clip) {
	    if (x0 < 0) x0 = 0;
	    if (x1 >= size.x) x1 = size.x - 1;
	    dx = x1 - x0;
	}
	if (dpr->pr_depth == 1) {
	    register unsigned long *dst;
	    register unsigned long mask1, mask2;
	    register unsigned long px = x0;

	    op = (color) ? op>>2 : op&3;
	    color = -1;

	    dst = (unsigned long *) ((caddr_t) mprd->mpr.md_image +
	    	pr_product(y0 + mprd->mpr.md_offset.y, 
			mprd->mpr.md_linebytes));
			
	    if ((u_long) dst & 2) {
		dst = (unsigned long *) ((caddr_t) dst - 2);
		px += 16;
	    }

	    dst += (px += mprd->mpr.md_offset.x) >> 5;

	    mask1 = ((unsigned long) 0xFFFFFFFF) >> (px & 31);
	    mask2 = 0xFFFFFFFF << (0x1F - (px + dx & 31));
	    if ((count = ((px & 31) + dx >> 5) - 1) < 0)
		mask1 &= mask2;

	    CASE_BINARY_OP(op,HORIZ,depth1,allbits,allbits)
	} else {			/* depth8 code */ 
	    register u_char *dst;
	    register u_char planes = mprplanes;

	    count = dx + 1;
	    dst = mprd8_addr (&mprd->mpr, x0, y0, dpr->pr_depth);
	    if (color == 0) {
		op &= 3; goto hconst;
	    } else if ((color&255) == 255) {
		op >>= 2; goto hconst;
	    } else {
	        CASE_BINARY_OP(op,HORIZ,depth8,arbval,arbval)
		return (0);
	    }
	    hconst: CASE_BINARY_OP(op,HORIZ,depth8,arbval,allbits)
	}
	return (0);
    }

/* 
 * Special case for vertical lines.
 */
    if (dx == 0) {
	if (clip) {
	    if (y0 < 0) y0 = 0;
	    if (y1 >= size.y) y1 = size.y - 1;
	    dy = y1 - y0;
	}
	vrtstep = (u_char *)mprd->mpr.md_linebytes;
	if (reflect & 2)	
	    y0 = size.y - y1-1; 
	count =  dy + 1;
	if (dpr->pr_depth == 1) {
	    register short *dst = mprd_addr (&mprd->mpr, x0, y0);

	    op = (color) ? op>>2 : op&3;
	    color = 0x8000 >> mprd_skew (&mprd->mpr, x0, y0);
	    CASE_BINARY_OP(op,VERT,depth1,arbval,allbits)

	} else {		/* depth8 code */
	    register u_char *dst;
	    register u_char planes = mprplanes;

	    dst = mprd8_addr (&mprd->mpr, x0, y0, dpr->pr_depth);
	    CASE_BINARY_OP(op,VERT,depth8,arbval,arbval)
	}
	return (0);
    }

/* 
 * One more reflection: we want to assume that dx >= dy.  So if this
 * is not true, we reflect the vector around the diagonal line x = y and
 * remember that we did this by adding 1 to reflect.
 */
    if (dx < dy) {
	swap (x0, y0, count);
	swap (x1, y1, count);
	swap (dx, dy, count);
	swap (size.x, size.y, count);
	reflect += 1;
    }

    error = -(dx >> 1);		/* error at pos0 */
    xs = x0; ys = y0;
    if (!clip)
	goto clipdone;

/* 
 * Begin hard part of clipping.
 * We have insured that we are clipping on a vector which has dx > 0,
 * dy > 0 and dx >= dy.  The error is the vertical distance from the true
 * line to the approximation in units where each pixel is dx by dx.  Moving
 * one to the right (increasing x by 1) subtracts dy from the error.  Moving
 * one pixel down (increasing y by 1) adds dx to the error.
 * Bresenham functions by restoring the error to the range (-dx,0] whenever
 * the error leaves it.  The algorithm increases x and increases y when
 * it needs to constrain the error.
 */

/* 
 * Clip left end to yield start of visible vector.  If the starting x
 * coordinate is negative, we must advance to the first x coordinate
 * which will be drawn (x=0).  As we advance (-xs) units in the x
 * direction, the error increases by (-xs)*dy.  This means that the
 * error when x=0 will be
 *	-(dx/2)+(-xs)*dy
 * For each y advance in this range, the error is reduced by dx, and should
 * be in the range (-dx,0] at the y value at x=0.  Thus to compute the
 * increment in y we should take
 *	(-(dx/2)+(-xs)*dy+(dx-1))/dx
 * where the numerator represents the length of the interval
 *	[-dx+1,-(dx/2)+(-xs)*dy]
 * The number of dx steps by which the error can be reduced and stay in this
 * interval is the y increment which would result if the vector were drawn
 * over this interval.
 */
    if (xs < 0) {
	error += -xs * dy;
	xs = 0;
	count = (error + (dx - 1)) / dx;
	ys += count;
	error -= count * dx;
    }

/* 
 * After having advanced x to be at least 0, advance
 * y to be in range.  If y is already too large (and can
 * only get larger!), just give up.  Otherwise, if ys < 0,
 * we need to compute the value of x at which y is first 0.
 * In advancing y to be zero, the error decreases by (-ys)*dx,
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
    if (ys >= size.y)
	return (0);
    if (ys < 0) {			/* skip to dst top edge */
	error += ys * dx;
	ys = 0;
	count = ((-dx + dy) - error) / dy;
	xs += count;
	error += count * dy;
	if (xs >= size.x)
	    return (0);
    }

/* 
 * Now clip right end.
 * If the last x position is outside the rectangle, then clip the vector
 * back to within the rectangle at x=size.x-1.  The corresponding y value
 * has error -(dx/2)+((size.x-1)-x0)*dy
 * We need an error in the range (-dx,0], so compute the corresponding number
 * of steps in y from the number of dy's in the interval:
 *	(-dx,-(dx/2)+((size.x-1)-x0)*dy]
 * which is:
 *	[-dx+1,-(dx/2)+((size.x-1)-x0)*dy]
 * or:
 *	(-(dx/2)+((size.x-1)-x0)*dy+dx-1)/dx
 * Note that:
 *	dx - (dx/2) != (dx/2)
 * (consider dx odd), so we can't simplify much further.
 */
    if (x1 >= size.x) {
	x1 = size.x - 1;
	y1 = y0 +
	    (-(dx >> 1) + (((size.x - 1) - x0) * dy) +
		dx - 1) / dx;
    }
/* 
 * If the last y position is outside the rectangle, then
 * clip it back at y=size.y-1.  The corresponding x value
 * has error
 *	-(dx/2)-((size.y-1)-y0)*dx
 * We want the last x value with this y value, which
 * will have an error in the range (-dy,0] (so that increasing
 * x one more time will make the error > 0.)  Thus
 * the amount of error allocatable to dy steps is from
 * the length of the interval:
 *	[-(dx/2)-((size.y-1)-y0)*dx,0]
 * that is,
 *	(0-(-(dx/2)-((size.y-1)-y0)*dx))/dy
 * or:
 *	((dx/2)+((size.y-1)-y0)*dx)/dy
 */
    if (y1 >= size.y) {
	y1 = size.y - 1;
	x1 = x0 +
	    ((dx >> 1) + (((size.y - 1) - y0) * dx)) / dy;
    }

clipdone: 
/* 
 * Now have to set up for the Bresenham routines.
 */
    count = x1 - xs;
    vrtstep = (u_char *)mprd->mpr.md_linebytes;

    if (reflect & 1) {				/* major axis is y */
	if (reflect & 2) {
	    /* unreflect major axis (misnamed x) */
 	    xs = ((size.x - 1) - xs); 
	    vrtstep = (u_char *)(-mprd->mpr.md_linebytes);
	}
	if (dpr->pr_depth == 1) {
	    register short *dst;

	    dst = mprd_addr(&mprd->mpr, ys, xs);
	    op = (color) ? op>>2 : op&3;
	    color = 0x8000 >> mprd_skew (&mprd->mpr, ys, xs);
	    CASE_BINARY_OP(op,MAJORY,depth1,arbval,allbits)
	} else /* if (dpr->pr_depth == 8) */ {
	    register u_char *dst;
	    register u_char planes = mprplanes;

	    dst = mprd8_addr (&mprd->mpr, ys, xs, dpr->pr_depth);
	    CASE_BINARY_OP(op,MAJORY,depth8,arbval,arbval)
	}
    } else {					/* major axis is x */
	if (reflect & 2) {
	    /* unreflect minor axis */
 	    ys = ((size.y - 1) - ys); 
	    vrtstep = (u_char *)(-mprd->mpr.md_linebytes);
	}
	if (dpr->pr_depth == 1) {
	    register short *dst;

	    dst = mprd_addr (&mprd->mpr, xs, ys);
	    op = (color) ? op>>2 : op&3;
	    color = 0x8000 >> mprd_skew (&mprd->mpr, xs, ys);
	    CASE_BINARY_OP(op,MAJORX,depth1,arbval,allbits)
	} else /* if (dpr->pr_depth == 8) */ {
	    register u_char *dst;
	    register u_char planes = mprplanes;

	    dst = mprd8_addr (&mprd->mpr, xs, ys, dpr->pr_depth);
	    CASE_BINARY_OP(op,MAJORX,depth8,arbval,arbval)
	}
    }
    return (0);
}

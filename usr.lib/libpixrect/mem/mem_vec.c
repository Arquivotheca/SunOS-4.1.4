#ifndef lint
static char sccsid[] = "@(#)mem_vec.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1987-1990 Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/memvar.h>
#include <pixrect/mem_rop_impl_ops.h>

/* largest delta we can handle without overflow in clip code */
#define	MAXDELTA	(32767)

/* types */
typedef	u_short	PIX1;
typedef	u_char	PIX8;
typedef	u_short	PIX16;
typedef	u_long	PIX32;

typedef	IFSPARC(u_long,PIX1) TMP1;
typedef IFSPARC(u_long,PIX8) TMP8;
typedef	IFSPARC(u_long,PIX16) TMP16;

/* leftmost bit of specified type */
#define	LEFTBIT(type)	((type) (1 << (sizeof (type) * 8 - 1)))

/* reflect flag bits */
#define	REFLECT_YMAJOR	1
#define	REFLECT_XAXIS	2

/* use generic loop for x-major and y-major vector */
#ifndef GENERIC_XY
#define	GENERIC_XY	!defined(mc68000)
#endif

/* define LINE_TAG to tag certain lines */
#ifdef LINE_TAG
#undef	LINE_TAG
#define	LINE_TAG(args)	(_line_tag(__LINE__), _line_tag_args args)
#else
#define	LINE_TAG(args)
#endif

#define swap(a, b, t)	_STMT((t) = (a); (a) = (b); (b) = (t);)

/* align pointer to 32 bit boundary */
#define	DST_ALIGN(dp, xbits) \
	if ((u_int) (dp) & 2) { \
		PTR_INCR(PIX32 *, (dp), -2); \
		(xbits) += 16; \
	} \
	else

/* opcode dispatch macro */
#define	CASE_OP(op,depth1,macro,arg1,arg2) _STMT( \
	depth1( \
		switch (op) { \
		case 15: macro(F,IFFALSE,arg1,arg2); break; \
		case  5: macro(5,IFFALSE,arg1,arg2); break; \
		case  0: macro(0,IFFALSE,arg1,arg2); break; \
		} \
	, \
		if ((op) == 12) \
			macro(C,IFFALSE,arg1,arg2); \
		else \
			switch (op) { \
			case  6: macro(6,IFFALSE,arg1,arg2); break; \
			case 14: macro(E,IFFALSE,arg1,arg2); break; \
			case  8: macro(8,IFFALSE,arg1,arg2); break; \
			case  4: macro(4,IFTRUE,arg1,arg2); break; \
			case  7: macro(7,IFTRUE,arg1,arg2); break; \
			} \
	))
	
/* x/y-major dispatch macro */
#define	CASE_XY(op,depth1,macro,arg1,arg2) \
	if (reflect & REFLECT_YMAJOR) \
		CASE_OP(op,depth1,_CAT(macro,Y),arg1,arg2); \
	else \
		CASE_OP(op,depth1,_CAT(macro,X),arg1,arg2) \

#if GENERIC_XY
#define	CASE_G(op,depth1,macro,arg1,arg2) \
	CASE_OP(op,depth1,_CAT(macro,XY),arg1,arg2)
#else
#define	CASE_G	CASE_XY
#endif

/* point operation macros */

#define	MPOINTT(op,usetmp,d,m,k,t) _STMT( \
	usetmp((t) = *(d);,) \
	*(d) = _CAT(OP_mfill,op)(usetmp((t), *(d)), (m), (k)); )

#define	MPOINT(op,d,m,k) \
	(*(d) = _CAT(OP_mfill,op)(*(d), (m), (k)))

#define	UPOINT(op,d,k) \
	(*(d) = _CAT(OP_ufill,op)(*(d), (k)))

/* vertical vectors */
#define	VEC_V(op,usetmp,dtype,nomask) _STMT( \
	nomask(k = _CAT(OP_ufgen,op)(k), \
		k = _CAT(OP_mfgen,op)(m,k); m = _CAT(OP_mfmsk,op)(m)); \
	PR_LOOPVP(count, \
		nomask(UPOINT(op,d,k), MPOINTT(op,usetmp,d,m,k,t)); \
		PTR_INCR(dtype *, d, (int) offset)); )

/* horizontal vectors */
#define	VEC_H(op,usetmp,arg1,arg2) _STMT( \
	if (lm) { \
		t = *d; \
		*d++ = _CAT(OP_mrop,op)(t, _CAT(OP_mmsk,op)((PIX32) lm), k); \
	} \
	if (--count >= 0) { \
		if (m == ~0) { \
			k = _CAT(OP_ufgen,op)(k); \
			PR_LOOPVP(count, UPOINT(op,d,k); d++); \
		} \
		else { \
			k = _CAT(OP_mfgen,op)(m,k); \
			m = _CAT(OP_mfmsk,op)(m); \
			PR_LOOPVP(count, MPOINTT(op,usetmp,d,m,k,t); d++); \
		} \
	} \
	if (m = (PIX32) rm) { \
		k = (PIX32) k0; \
		t = *d; \
		*d = _CAT(OP_mrop,op)(t, _CAT(OP_mmsk,op)(m), k); \
	} )

/* depth 1 x-major */
#define	VEC_1X(op,usetmp,arg1,arg2) _STMT( \
	do { \
		t = *d; \
		do { \
			if (m == 0) { \
				m = (TMP1) mleft; \
				*d++ = t; \
				t = *d; \
			} \
			t = _CAT(OP_mfill,op)(t,_CAT(OP_mfmsk,op)(m),dummy); \
			m >>= 1; \
		} while ((error += dy) <= 0 && LOOP_DECR(count)); \
		*d = t; \
		PTR_INCR(PIX1 *, d, (int) offset); \
		error -= dx; \
	} while (--count >= 0); )

/* depth 1 y-major */
#define	VEC_1Y(op,usetmp,arg1,arg2) _STMT( \
	m = (PIX1) _CAT(OP_mfmsk,op)(m); \
	mleft = (INT_T) _CAT(OP_mfmsk,op)((PIX1) mleft); \
	do { \
		if (m == (PIX1) _CAT(OP_mfmsk,op)(0)) { \
			m = (TMP1) mleft; \
			d++; \
		} \
		do { \
			*d = _CAT(OP_mfill,op)(*d,m,dummy); \
			PTR_INCR(PIX1 *, d, (int) offset); \
		} while ((error += dy) <= 0 && LOOP_DECR(count)); \
		m >>= 1; \
		m |= _CAT(OP_mfmsk,op)(0) ? LEFTBIT(PIX1) : 0; \
		error -= dx; \
	} while (--count >= 0); )

#if GENERIC_XY

/* depth 8/16/32 generic */
#define	VEC_GXY(op,usetmp,dtype,nomask) _STMT( \
	nomask(k = _CAT(OP_ufgen,op)(k), \
		k = _CAT(OP_mfgen,op)(m,k); m = _CAT(OP_mfmsk,op)(m)); \
	do { \
		usetmp(dtype t; ,) \
		nomask(UPOINT(op,d,k), MPOINTT(op,usetmp,d,m,k,t)); \
		if ((error += dy) > 0) { \
			PTR_INCR(dtype *, d, (int) min_offset); \
			error -= dx; \
		} \
		PTR_INCR(dtype *, d, (int) maj_offset); \
	} while (--count >= 0); )

#else GENERIC_XY

/* depth 8/16/32 x-major */
#define	VEC_GX(op,usetmp,dtype,nomask) _STMT( \
	nomask(k = _CAT(OP_ufgen,op)(k), \
		k = _CAT(OP_mfgen,op)(m,k); m = _CAT(OP_mfmsk,op)(m)); \
	do { \
		do { \
			nomask(UPOINT(op,d,k), MPOINT(op,d,m,k)); \
			d++; \
		} while ((error += dy) <= 0 && LOOP_DECR(count)); \
		PTR_INCR(dtype *, d, (int) offset); \
		error -= dx; \
	} while (--count >= 0); )

/* depth 8/16/32 y-major */
#define	VEC_GY(op,usetmp,dtype,nomask) _STMT( \
	nomask(k = _CAT(OP_ufgen,op)(k), \
		k = _CAT(OP_mfgen,op)(m,k); m = _CAT(OP_mfmsk,op)(m)); \
	do { \
		do { \
			nomask(UPOINT(op,d,k), MPOINT(op,d,m,k)); \
			PTR_INCR(dtype *, d, (int) offset); \
		} while ((error += dy) <= 0 && LOOP_DECR(count)); \
		d++; \
		error -= dx; \
	} while (--count >= 0); )

#endif GENERIC_XY


mem_vector(pr, x0, y0, x1, y1, op, color)
	Pixrect *pr;
	int x0, y0, x1, y1;
	int op;
	int color;
{
	int noclip;
	int w, h, startx, starty;
	int reflect, initerror;

	u_int depth;
	u_long planes;
	struct mpr_data *prd;

	{
		register struct mprp_data *mprd;
		register int rop, rcolor, rdepth;

		mprd = mprp_d(pr);
		prd = &mprd->mpr;

		rop = op;
		noclip = rop & PIX_DONTCLIP;
		if ((rcolor = PIXOP_COLOR(rop)) == 0)
			rcolor = color;
		rop = PIXOP_OP(rop);

		if ((rdepth = pr->pr_depth) == 1) {
			depth = 0;

			if (mprd->mpr.md_flags & MP_REVERSEVIDEO)
				rop = PIX_OP_REVERSEDST(rop);

			if (rcolor)
				rop >>= 2;
			else
				rop &= 3;

			rop |= rop << 2;
		}
		else {
			register int rplanes = ~0, allplanes;

#define	INV	16
#define	CLR	32
#define	SET	(CLR | INV)

			static char optab[16 * 2] = {
				 8|CLR,	 4|INV,	 8|INV,	12|INV,
				 4,	 6|SET,	 6,	 7,
				 8,	 6|INV,	10,	14|INV,
				12,	 7|INV,	14,	14|SET,
				12|CLR,	 4|INV,	 8|INV,	12|INV,
				 4,	 6|SET,	 6,	 7,
				 8,	 6|INV,	10,	14|INV,
				12,	 7|INV,	14,	12|SET
			};

			if (mprd->mpr.md_flags & MP_PLANEMASK)
				rplanes = mprd->planes;

			switch (rdepth) {
			case 8: 
				depth = 1;
				allplanes = 0xff;
				break;
			case 16:
				depth = 2;
				allplanes = 0xffff;
				break;
			case 32:
				depth = 3;
				allplanes = 0xffffffff;
				break;
			default:
				return PIX_ERR;
			}

			rplanes &= allplanes;
			rcolor &= rplanes;

			if (rcolor == 0) {
				rop &= 3;
				rop |= rop << 2;
			}
			else if (rcolor == rplanes) {
				rcolor = 0;
				rop >>= 2;
				rop |= rop << 2;
			}

			if (rplanes == allplanes)
				rop += 16;

			rop = optab[rop];
			if (rop & CLR)
				rcolor = 0;
			if (rop & INV)
				rcolor = ~rcolor & rplanes;
			rop &= 15;

			planes = rplanes;	
			color = rcolor;
#undef	SET
#undef	CLR
#undef	INV
		}

		op = rop;
	}


	/* indentation is wrong here */
{
	register int dx, dy;

	/* vertical vectors */
	if ((dx = x1 - x0) == 0) {
		register caddr_t daddr;
		register INT_T offset;

		{
			register int y;
			register struct mpr_data *mprd;

			if ((y = y0) <= (dy = y1))
				dy -= y;
			else {
				dy = y - dy;
				y -= dy;
			}

			dx = x0;
			if (!noclip) {
				if ((u_int) dx >= pr->pr_size.x)
					return 0;

				if (y < 0) {
					if ((dy += y) < 0)
						return 0;
					y = 0;
				}

				if (y + dy >= pr->pr_size.y) {
					if ((dy = pr->pr_size.y - y - 1) < 0)
						return 0;
				}
			}

			mprd = prd;
			offset = (INT_T) mprd->md_linebytes;
			daddr = (caddr_t) mprd->md_image;
			dx += mprd->md_offset.x;
			y += mprd->md_offset.y;
			daddr += pr_product((int) offset, y);
		}

		{
			register LOOP_T count = dy;

			switch (depth) {
			case 0: {
				register PIX1 *d;
				register TMP1 m, t;
				register TMP1 k;	/* dummy */

				d = (PIX1 *) daddr + (dx >> 4);
				m = LEFTBIT(PIX1) >> (dx &= 15);

				CASE_OP(op,IFTRUE,VEC_V,PIX1,IFFALSE);
				break;
			}

			case 1: {
				register PIX8 *d;
 				register TMP8 m, k, t;

				d = (PIX8 *) daddr + dx;
				k = color;
				if ((m = planes) == 255) {
					LINE_TAG((d, k, count, offset));
					CASE_OP(op,IFFALSE,VEC_V,PIX8,IFTRUE);
				}
				else
					CASE_OP(op,IFFALSE,VEC_V,PIX8,IFFALSE);
				break;
			}

			case 2: {
				register PIX16 *d;
				register TMP16 m, k, t;

				d = (PIX16 *) daddr + dx;
				k = color;
				m = planes;

				CASE_OP(op,IFFALSE,VEC_V,PIX16,IFFALSE);
				break;
			}

			case 3: {
				register PIX32 *d;
				register PIX32 m, k, t;

				d = (PIX32 *) daddr + dx;
				k = color;
				m = planes;

				CASE_OP(op,IFFALSE,VEC_V,PIX32,IFFALSE);
				break;
			}
			} /* switch */
		}

		return 0;
	}

	/* horizontal vectors */
	if ((dy = y1 - y0) == 0) {
		register PIX32 *d, m, k;
		register INT_T lm = 0, rm = 0;
		{
			register int x;
			register struct mpr_data *mprd;

			if (dx > 0)
				x = x0;
			else {
				dx = -dx;
				x = x1;
			}

			dx++;
			dy = y0;

			if (!noclip) {
				if ((u_int) dy >= pr->pr_size.y)
					return 0;

				if (x < 0) {
					if ((dx += x) <= 0)
						return 0;
					x = 0;
				}

				if (x + dx >= pr->pr_size.x) {
					if ((dx = pr->pr_size.x - x) <= 0)
						return 0;
				}
			}

			mprd = prd;
			x += mprd->md_offset.x;
			d = (PIX32 *) PTR_ADD(mprd->md_image,
				pr_product(mprd->md_linebytes, 
					dy + mprd->md_offset.y));

			k = color;
			m = planes;
			switch (depth) {
			case 0:
				m = ~0;
				switch (op) {
				case  0: op = 12; k =  0; break;
				case  5: op =  6; k = ~0; break;
				case 15: op = 12; k = ~0; break;
				}
				break;
			case 1:
				x <<= 3;
				dx <<= 3;
				k |= k << 8;
				k |= k << 16;
				m |= m << 8;
				m |= m << 16;
				break;
			case 2:
				x <<= 4;
				dx <<= 4;
				k |= k << 16;
				m |= m << 16;
				break;
			case 3:
				x <<= 5;
				dx <<= 5;
				break;
			}

			DST_ALIGN(d, x);
			d += x >> 5;

			if (x &= 31) {
				lm = (INT_T) ((PIX32) ~0 >> x & m);
				dx -= 32 - x;
			}

			if (x = dx & 31) {
				rm = (INT_T) (~0 << 32 - x & m);
				dx -= x;
			}

			if (dx < 0) {
				lm = (INT_T) ((PIX32) lm & (PIX32) rm);
				rm = 0;
				dx = 0;
			}
			dx >>= 5;
		}

		{
			register PIX32 t;
			register LOOP_T count = dx;
			register INT_T k0 = (INT_T) k;

			LINE_TAG((d, k, count));
			CASE_OP(op,IFFALSE,VEC_H,0,0);
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
		swap(x0, x1, tmp);

		if (dy = -dy)
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
		reflect = REFLECT_XAXIS;
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
		reflect |= REFLECT_YMAJOR;
	}

	initerror = - (dx >> 1);		/* error at x0, y0 */
	startx = x0;
	starty = y0;

	/* not indented here either */
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

	} /* clip block */

	{
		register LOOP_T count = x1 - startx;
		register caddr_t daddr;
		register INT_T offset;

		{
			register struct mpr_data *mprd = prd;
			register int y;

			offset = (INT_T) mprd->md_linebytes;
			daddr = (caddr_t) mprd->md_image;

			/* major axis is y */
			if (reflect & REFLECT_YMAJOR) {
				y = startx;
				startx = starty + mprd->md_offset.x;
				h = w;
			}
			else {
				y = starty;
				startx += mprd->md_offset.x;
			}

			if (reflect & REFLECT_XAXIS) {
				y = y - h + 1 - mprd->md_offset.y;
				offset = (INT_T) (- (int) offset);
			}
			else
				y += mprd->md_offset.y;

			daddr += pr_product((int) offset, y);
		}

		{
			register int error = initerror;

#if GENERIC_XY

#define min_offset offset

			register INT_T maj_offset;

			{
				register int t;

				if ((t = depth - 1) >= 0) {
					t = 1 << t;
					maj_offset = (INT_T) t;

					if (reflect & REFLECT_YMAJOR) {
						maj_offset = min_offset;
						min_offset = (INT_T) t;
					}
				}
			}
#endif GENERIC_XY

			switch (depth) {
			case 0: {
				register PIX1 *d;
				register TMP1 m, t;
				register INT_T mleft;

				d = (PIX1 *) daddr + (startx >> 4);
				m = LEFTBIT(PIX1);
				mleft = (INT_T) m;
				m >>= (startx & 15);

				CASE_XY(op,IFTRUE,VEC_1,PIX1,IFFALSE);
				break;
			}

			case 1: {
				register PIX8 *d;
				register TMP8 m, k;

				d = (PIX8 *) daddr + startx;
				k = color;
				if ((PIX8) (m = planes) == (PIX8) ~0) {
					LINE_TAG((d, k, count, error, dx, dy, 
						maj_offset, min_offset));
					CASE_G(op,IFFALSE,VEC_G,PIX8,IFTRUE);
				}
				else
					CASE_G(op,IFFALSE,VEC_G,PIX8,IFFALSE);
				break;
			}

			case 2: {
				register PIX16 *d;
				register TMP16 m, k;

				d = (PIX16 *) daddr + startx;
				k = color;
				m = planes;

				CASE_G(op,IFFALSE,VEC_G,PIX16,IFFALSE);
				break;
			}

			case 3: {
				register PIX32 *d;
 				register PIX32 m, k;

				d = (PIX32 *) daddr + startx;
				k = color;
				m = planes;

				CASE_G(op,IFFALSE,VEC_G,PIX32,IFFALSE);
				break;
			}
			} /* switch */
		}
	}
}
	return 0;
}

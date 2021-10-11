#ifndef lint
static char sccsid[] = "@(#)mem_curve.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include "chain.h"

#define region(d, l) (d<0? -1: d<l? 0: 1)


/****	Depth 1 Macros ****/

/* Macros implementing the 12 copies of the inner loop */
/* Third choice (chooseop) made inline */
#define choosedir(rop)							\
	if (cur->x < 0) {						\
		chooseaxis(rop, <<);					\
	} else {							\
		chooseaxis(rop, >>);					\
	}

#define chooseaxis(rop, shiftop)					\
	if (xmajor) {							\
		innerloop(shiftop, *dadr rop dbit, )			\
	} else {							\
		innerloop(shiftop, , *dadr rop dbit)			\
	}

#define innerloop(shiftop, writex, writey)				\
	do {								\
		if ((src<<=1) < 0) {					\
			dbit shiftop= 1;				\
			writex;						\
		} else {						\
			(char*)dadr += lb;				\
			writey;						\
		}							\
	} while (--count);


/****	Depth 8 Macros ****/


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


/* Macro to do rasterops when the bit plane mask is not -1 */
#define PM(rop,dst,col,planes)\
	 ((dst) & ~planes) | (rop(dst,col) & planes)

/* Macro to execute a macro for an opcode. */
#define CASE_BINARY_OP(rop,macro,dst,col,planes)\
    switch(rop) {\
	CASEOP(0,macro,dst,col,planes)\
	CASEOP(1,macro,dst,col,planes)\
	CASEOP(2,macro,dst,col,planes)\
	CASEOP(3,macro,dst,col,planes)\
	CASEOP(4,macro,dst,col,planes)\
	CASEOP(5,macro,dst,col,planes)\
	CASEOP(6,macro,dst,col,planes)\
	CASEOP(7,macro,dst,col,planes)\
	CASEOP(8,macro,dst,col,planes)\
	CASEOP(9,macro,dst,col,planes)\
	CASEOP(B,macro,dst,col,planes)\
	CASEOP(C,macro,dst,col,planes)\
	CASEOP(D,macro,dst,col,planes)\
	CASEOP(E,macro,dst,col,planes)\
	CASEOP(F,macro,dst,col,planes)\
    }

#define CASEOP(rop,macro,dst,col,planes)\
    case CAT(0x,rop): macro(rop,dst,col,planes);\
	break;

/* Macro to do a rop operation on one pixel */
#define PIXELROP(rop,dst,col,planes)\
	CAT(*,dst) = PM(CAT(F,rop),CAT(*,dst),col,planes)

/* Macro to do no operation on one pixel */
#define NOP(rop,dst,col,planes)

/* Macros implementing the 64 copies of the inner loop */
/* Third choice (chooseop) made by CASE_BINARY_OP */
#define choosedir8(rop,dst,col,planes)					\
	if (cur->x < 0) {						\
		chooseaxis8(--,rop,dst,col,planes);			\
	} else {							\
		chooseaxis8(++,rop,dst,col,planes);			\
	}

#define chooseaxis8(addrinc,rop,dst,col,planes)				\
	if (xmajor) {							\
		innerloop8(addrinc,PIXELROP,NOP,rop,dst,col,planes)	\
	} else {							\
		innerloop8(addrinc,NOP,PIXELROP,rop,dst,col,planes)	\
	}

#define innerloop8(addrinc,writex,writey,rop,dst,col,planes)		\
	do {								\
		if ((src<<=1) < 0) {					\
			CAT(dst,addrinc);				\
			writex(rop,dst,col,planes);			\
		} else {						\
			dst += lb;					\
			writey(rop,dst,col,planes);			\
		}							\
	} while (--count);


extern char pr_reversedst[];

enum logops {or, and, xor, nop};

mem_curve (dst, cu, n, op, colour)
	struct pr_prpos dst;
	struct pr_curve cu[];
	int n, op, colour;
{
	int x, y, nx, ny, X, Y, nX, nY, inside, ninside, linebytes;
	int xlim, ylim, absx, absy, yinc, xmajor=1, oxmajor = -1;
	register lb, count;
	register short src = 0;
	register struct pr_curve *cur;
	struct pr_curve *cun;
	struct mprp_data *mprd = mprp_d(dst.pr);

	if (dst.pr->pr_depth == 8)
		goto depth8;

depth1:
	{
	register unsigned dbit;
	register *dadr;
	int color;

	color = PIX_OPCOLOR(op);
	if (!color)
		color = colour;
	op = (op >> 1) & 0xF;
	if (mprd->mpr.md_flags & MP_REVERSEVIDEO)
		op = (pr_reversedst[op]);
	op = ((color?op>>2:op)+1)&3;  /* 0: or, 1: and, 2: xor, 3: nop */
	if (op == (int) nop)
		return(0);
	linebytes = mprd->mpr.md_linebytes;
	nx = dst.pos.x, ny = dst.pos.y;
	xlim = dst.pr->pr_size.x, ylim = dst.pr->pr_size.y;
	nX = region(nx, xlim), nY = region(ny, ylim);
	dadr = (int *)mprd_addr(&mprd->mpr, nx, ny);
	dbit = (unsigned)0x80000000 >> mprd_skew(&mprd->mpr, nx, ny);
	ninside = !nX && !nY;
	cun = cu + n;
	for (cur = cu; cur < cun; cur++) {
		x = nx, y = ny, X = nX, Y = nY, inside = ninside;
		/* position dbit for unrestricted motion */
		if (cur->x < 0) {
			absx = -cur->x;
			if (dbit >= 0x10000)
				dbit >>= 16, (char *)dadr -= 2;
		} else {
			absx = cur->x;
			if (dbit < 0x10000)
				dbit <<= 16, (char *)dadr += 2;
		}
		if (cur->y < 0)
			absy = -cur->y, lb = -linebytes;
		else
			absy = cur->y, lb = linebytes;
		if (!(count = absx + absy))
			continue;	  /* guarantees count != 0 later */
		nx = x + cur->x, ny = y + cur->y;
		nX = region(nx, xlim), nY = region(ny, ylim);
		ninside = !(nX || nY);
		if (absx != absy) {
			xmajor = absx > absy;
		if ((src<0) != oxmajor && oxmajor != xmajor)
			/* at axis change convert minor to major pixel */
			switch(op) {
				case or:  *dadr |= dbit; break;
				case and: *dadr &= ~dbit; break;
				case xor: *dadr ^= dbit; break;
			}
		oxmajor = xmajor;
		if ((src = cur->bits) < 0) {
			/* penup - recompute dadr, dbit from scratch */
			dadr = (int *)mprd_addr(&mprd->mpr, nx, ny);
			dbit = (unsigned)0x80000000 >> mprd_skew(&mprd->mpr, nx, ny);
			continue;
		}
		if (inside? !ninside: !((X&&(X==nX)) || (Y&&(Y==nY)))) {
			/* transition or mixed-axis - plot cautiously */
			yinc = cur->y < 0? -1: 1;
			do {
				if ((src<<=1) < 0)
					if (cur->x < 0)
						dbit <<= 1, x--;
					else
						dbit >>= 1, x++;
				else
					(char *)dadr += lb, y += yinc;
				if (xmajor == (src<0) &&
					   (unsigned)x<xlim && (unsigned)y<ylim)
					switch(op) {
						case or:  *dadr |= dbit; break;
						case and: *dadr &= ~dbit; break;
						case xor: *dadr ^= dbit; break;
					}
			} while (--count);
		} else if (inside) {
			/* whole curve inside - plot obliviously */
			/* chooseop - chooseoperation */
			switch (op) {
				case or:	choosedir(|=); break;
				case and:	choosedir(&=~); break;
				case xor:	choosedir(^=); break;
			}
			}			
			else { /* clip */
			dadr = (int *)mprd_addr(&mprd->mpr, nx, ny);
			dbit = (unsigned)0x80000000 >> mprd_skew(&mprd->mpr, nx, ny);
			x = nx;
			y = ny;
			}
loopend:	;
		}
		else /* special diagonal case */ {
		if ((src = cur->bits) < 0) {
			/* penup - recompute dadr, dbit from scratch */
			dadr = (int *)mprd_addr(&mprd->mpr, nx, ny);
			dbit = (unsigned)0x80000000 >> mprd_skew(&mprd->mpr, nx, ny);
			continue;
		} else {
		    count = cur -> x;
		    if(count < 0) count = -count;
		    do {
					if (cur->x < 0)
						dbit <<= 1, x--;
					else
						dbit >>= 1, x++;
					(char *)dadr += lb, y += yinc;
					switch(op) {
						case or:  *dadr |= dbit; break;
						case and: *dadr &= ~dbit; break;
						case xor: *dadr ^= dbit; break;
					}
		    } while (--count);
		}
		}
	}
	return;
	}

depth8:
	{
	register u_char *dadr;
	register u_char color, mprplanes;

	color = PIX_OPCOLOR(op);
	if (!color)
		color = colour;
	op = (op >> 1) & 0xF;
	if (mprd->mpr.md_flags & MP_REVERSEVIDEO)
		op = (pr_reversedst[op]);
	if (mprd->mpr.md_flags & MP_PLANEMASK)
		mprplanes = mprd->planes;
	else
		mprplanes = ~0;
	if ((op == (PIX_DST >> 1)) || (mprplanes == 0))
		return(0);
	linebytes = mprd->mpr.md_linebytes;
	nx = dst.pos.x, ny = dst.pos.y;
	xlim = dst.pr->pr_size.x, ylim = dst.pr->pr_size.y;
	nX = region(nx, xlim), nY = region(ny, ylim);
	dadr = mprd8_addr(&mprd->mpr, nx, ny, 8);
	ninside = !nX && !nY;
	cun = cu + n;
	for (cur = cu; cur < cun; cur++) {
		x = nx, y = ny, X = nX, Y = nY, inside = ninside;
		if (cur->x < 0)
			absx = -cur->x;
		else
			absx = cur->x;
		if (cur->y < 0)
			absy = -cur->y, lb = -linebytes;
		else
			absy = cur->y, lb = linebytes;
		if (!(count = absx + absy))
			continue;	  /* guarantees count != 0 later */
		nx = x + cur->x, ny = y + cur->y;
		nX = region(nx, xlim), nY = region(ny, ylim);
		ninside = !(nX || nY);
		if (absx != absy) {
			xmajor = absx > absy;
		if ((src<0) != oxmajor && oxmajor != xmajor)
			/* at axis change convert minor to major pixel */
			CASE_BINARY_OP(op, PIXELROP, dadr, color, mprplanes)
		oxmajor = xmajor;
		if ((src = cur->bits) < 0) {
			/* penup - recompute dadr from scratch */
			dadr = mprd8_addr(&mprd->mpr, nx, ny, 8);
			continue;
		}
		if (inside? !ninside: !((X&&(X==nX)) || (Y&&(Y==nY)))) {
			/* transition or mixed-axis - plot cautiously */
			yinc = cur->y < 0? -1: 1;
			do {
				if ((src<<=1) < 0)
					if (cur->x < 0)
						dadr--, x--;
					else
						dadr++, x++;
				else
					dadr += lb, y += yinc;
				if (xmajor == (src<0) &&
					   (unsigned)x<xlim && (unsigned)y<ylim)
					CASE_BINARY_OP(op, PIXELROP, dadr,
					    color, mprplanes)
			} while (--count);
		} else if (inside) {
			/* whole curve inside - plot obliviously */
			/* chooseop - chooseoperation */
			CASE_BINARY_OP(op, choosedir8, dadr, color, mprplanes)
			}			
			else { /* clip */
			dadr = mprd8_addr(&mprd->mpr, nx, ny, 8);
			x = nx;
			y = ny;
			}
loopend8:	;
		}
		else /* special diagonal case */ {
		if ((src = cur->bits) < 0) {
			/* penup - recompute dadr from scratch */
			dadr = mprd8_addr(&mprd->mpr, nx, ny, 8);
			continue;
		} else {
		    count = cur -> x;
		    if(count < 0) count = -count;
		    do {
					if (cur->x < 0)
						dadr--, x--;
					else
						dadr++, x++;
					dadr += lb, y += yinc;
					CASE_BINARY_OP(op, PIXELROP, dadr,
					    color, mprplanes)
		    } while (--count);
		}
		}
	}
	}
}

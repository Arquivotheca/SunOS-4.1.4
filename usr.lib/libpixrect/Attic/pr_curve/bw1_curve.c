#ifndef lint
static char sccsid[] = "@(#)bw1_curve.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * bw1_curve.c: Sun-1 Black-and-white pixrect curverop
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/bw1reg.h>
#include <pixrect/bw1var.h>
#include "chain.h"

#define region(d, l) (d<0? -1: d<l? 0: 1)

#define choosexdir()							\
	if (cur->x < 0)	{						\
		chooseydir(--xreg);					\
	} else {							\
		xreg++;							\
		chooseydir(xreg++);					\
		xreg--;							\
	}

#define chooseydir(xdir)						\
	if (cur->y < 0) {						\
		innerloop(xdir,--yreg);					\
	} else {							\
		yreg++;							\
		innerloop(xdir,yreg++);					\
		yreg--;							\
	}

#define innerloop(xdir,ydir)						\
	do {								\
		if ((src<<=1) < 0)					\
			*xdir = data;					\
		else							\
			*ydir = data;					\
	} while (--count != -1)
/* why oh why doesn't the compiler generate dbra here???!!! */
/* it would also be nice if it used a separate dbra for each arm of if */

extern char	pr_reversedst[];

bw1_curve(dst, pos0, cu, n, op, color)
	struct pixrect *dst;
	struct pr_pos pos0;
	struct pr_curve cu[];
	int n, op, color;
{
	register short *xreg, *yreg;
	register struct bw1fb *fb;
	register struct pr_curve *cur;
	register xinc, yinc;
	register short count, src=0, data;
	struct bw1pr *dbd;
	struct pr_size lim;
	int x, y, nx, ny, X, Y, nX, nY, inside, ninside;
	int absx, absy, xmajor = 1, oxmajor = -1;

	fb = (dbd = bw1_d(dst))->bwpr_va;
	data = PIX_OPCOLOR(op);
	if (data) 
		color = data;
	data = color ? ~0 : 0;
	op = (op>>1)&0xf;
	if (dbd->bwpr_flags & BW_REVERSEVIDEO)
		op = pr_reversedst[op];
	bw1_setfunction(fb, op);
	bw1_setmask(fb, 0);
	bw1_setwidth(fb, 1);
	nx = pos0.x, ny = pos0.y;
	lim = dst->pr_size;
	nX = region(nx, lim.x), nY = region(ny, lim.y);
	xreg = &fb->update[0].reg[BW_REGNONE].cursor[0].x.
			xory[nx + dbd->bwpr_offset.x];
	yreg = &fb->update[0].reg[BW_REGNONE].cursor[0].y.
			xory[ny + dbd->bwpr_offset.y];
	ninside = !nX && !nY;
	for (cur = cu; cur < cu+n; cur++) {
		x = nx, y = ny, X = nX, Y = nY, inside = ninside;
		absx = abs(cur->x), absy = abs(cur->y);
		if (!(count = absx + absy))
			continue;	  /* guarantees count != 0 later */
		nx = x + cur->x, ny = y + cur->y;
		nX = region(nx, lim.x), nY = region(ny, lim.y);
		ninside = !(nX || nY);
		if (absx != absy) {
			xmajor = absx > absy;
		if (xmajor)
			(char *)xreg += bw1_activatex, bw1_touch(*yreg);
		else
			bw1_touch(*xreg), (char *)yreg += bw1_activatex;
		if ((src<0) != oxmajor && oxmajor != xmajor) {
			/* at axis change convert minor to major pixel */
			if (xmajor)
				*xreg = data;
			else
				*yreg = data;
		}
		oxmajor = xmajor;
		if ((src = cur->bits) < 0) {
			/* penup - reset position */
			xreg += cur->x, yreg += cur->y;
			goto loopend;
		}
		if (inside? !ninside: !((X&&(X==nX)) || (Y&&(Y==nY)))) {
			/* transition - plot cautiously */
			xinc = cur->x < 0? -1: 1;
			yinc = cur->y < 0? -1: 1;
			do {
				if ((src<<=1) < 0)
					x += xinc, xreg += xinc;
				else
					y += yinc, yreg += yinc;
				if ((unsigned)x<lim.x && (unsigned)y<lim.y &&
					    xmajor == (src<0))
					if (xmajor)
						bw1_touch(*yreg), *xreg = data;
					else
						bw1_touch(*xreg), *yreg = data;
			} while (--count);
		} else if (inside) {
			count--;	/* for funny do-loop test */
			choosexdir();
		}
		else { /*clip */
					xreg += cur->x, yreg += cur->y;
		}
loopend:	if (xmajor)
			(char *)xreg -= bw1_activatex;
		else
			(char *)yreg -= bw1_activatex;
		}
		else /* special diagonal case */ {
		    if ((src = cur->bits) < 0) {
			/* penup - reset position */
			xreg += cur->x, yreg += cur->y;
			continue;
		    }
		    if (xmajor)
			(char *)xreg += bw1_activatex;
		    else
			(char *)yreg += bw1_activatex;
		    xinc = cur->x < 0? -1: 1;
		    yinc = cur->y < 0? -1: 1;
		    count = absx;
		    do {
				x += xinc, xreg += xinc;
				y += yinc, yreg += yinc;
				if (xmajor)
					bw1_touch(*yreg), *xreg = data;
				else
					bw1_touch(*xreg), *yreg = data;
		    } while (--count);
		    if (xmajor)
			(char *)xreg -= bw1_activatex;
		    else
			(char *)yreg -= bw1_activatex;
		}
	}
}

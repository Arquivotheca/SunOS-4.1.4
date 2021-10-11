#ifndef lint
static char sccsid[] = "@(#)cg1_curve.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * cg1_curve.c: Sun-1 Black-and-white pixrect curverop
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/cg1reg.h>
#include <pixrect/cg1var.h>
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

#define cg1_activatex							\
((u_char *) (&((struct cg1fb *) 0) -> update[1].xysel.x.cursor[0].xory[0]) \
-(u_char *) (&((struct cg1fb *) 0) -> update[0].xysel.x.cursor[0].xory[0]))


cg1_curve (dst, pos0, cu, n, op, color)
struct pixrect *dst;
struct pr_pos   pos0;
struct pr_curve cu[];
int     n, op, color;
{
    register u_char *xreg, *yreg;
    register struct cg1fb  *fb;
    register struct pr_curve   *cur;
    register    xinc, yinc;
    register short  count, src = 0, data;
    struct cg1pr   *dbd;
    struct pr_size  lim;
    int     x, y, nx, ny, X, Y, nX, nY, inside, ninside;
    int     absx, absy, xmajor = 1, oxmajor = -1;

    fb = (dbd = cg1_d (dst)) -> cgpr_va;
    data = color;
    color = PIX_OPCOLOR (op);
    if (color)
	data = color;
    op = (op >> 1) & 0xf;
    op = (CG_NOTMASK & CG_DEST) | (CG_MASK & ((op << 4) | op));
    cg1_setfunction(fb, op);
    cg1_setmask(fb, dbd->cgpr_planes);
    nx = pos0.x, ny = pos0.y;
    lim = dst -> pr_size;
    nX = region (nx, lim.x), nY = region (ny, lim.y);
    xreg = &fb -> update[0].xysel.x.cursor[0].
	xory[nx + dbd -> cgpr_offset.x];
    yreg = &fb -> update[0].xysel.y.cursor[0].
	xory[ny + dbd -> cgpr_offset.y];
    ninside = !nX && !nY;
    for (cur = cu; cur < cu + n; cur++)
    {
	x = nx, y = ny, X = nX, Y = nY, inside = ninside;
	absx = abs (cur -> x), absy = abs (cur -> y);
	if (!(count = absx + absy))
	    continue;		/* guarantees count != 0 later */
	nx = x + cur -> x, ny = y + cur -> y;
	nX = region (nx, lim.x), nY = region (ny, lim.y);
	ninside = !(nX || nY);
	if (absx != absy)
	{
	    xmajor = absx > absy;
	    if (xmajor)
		xreg += cg1_activatex, cg1_touch (*yreg);
	    else
		cg1_touch (*xreg), yreg += cg1_activatex;
	    if ((src < 0) != oxmajor && oxmajor != xmajor)
	    {
	    /* at axis change convert minor to major pixel */
		if (xmajor)
		    *xreg = data;
		else
		    *yreg = data;
	    }
	    oxmajor = xmajor;
	    if ((src = cur -> bits) < 0)
	    {
	    /* penup - reset position */
		xreg += cur -> x, yreg += cur -> y;
		goto loopend; 
	    }
	    if (inside ? !ninside : !((X && (X == nX)) || (Y && (Y == nY))))
	    {
	    /* transition - plot cautiously */
		xinc = cur -> x < 0 ? -1 : 1;
		yinc = cur -> y < 0 ? -1 : 1;
		do
		{
		    if ((src <<= 1) < 0)
			x += xinc, xreg += xinc;
		    else
			y += yinc, yreg += yinc;
		    if ((unsigned) x < lim.x && (unsigned) y < lim.y &&
			    xmajor == (src < 0))
			if (xmajor)
			    cg1_touch (*yreg), *xreg = data;
			else
			    cg1_touch (*xreg), *yreg = data;
		} while (--count);
	    }
	    else
		if (inside)
		{
		    count--;	/* for funny do-loop test */
		    choosexdir ();
		}
		else
		{		/* clip */
		    xreg += cur -> x, yreg += cur -> y;
		}
loopend: if (xmajor)
		xreg -= cg1_activatex;
	    else
		yreg -= cg1_activatex;
	}
		else /* special diagonal case */ {
		    if ((src = cur->bits) < 0) {
			/* penup - reset position */
			xreg += cur->x, yreg += cur->y;
			continue;
		    }
		    count = absx;
		    xinc = cur->x < 0? -1: 1;
		    yinc = cur->y < 0? -1: 1;
		    if (xmajor)
			xreg += cg1_activatex;
		    else
			yreg += cg1_activatex; 
		    do {
				x += xinc, xreg += xinc;
				y += yinc, yreg += yinc;
				if (xmajor)
					cg1_touch(*yreg), *xreg = data;
				else
					cg1_touch(*xreg), *yreg = data;
		    } while (--count);
 		    if (xmajor) 
			xreg -= cg1_activatex;
 		    else 
			yreg -= cg1_activatex; 
		}
	}
}

#ifndef lint
static char sccsid[] = "@(#)gp1_curve.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * gp1_curve.c: Graphics Processor pixrect curverop
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>
#include <pixrect/gp1var.h>
#include "chain.h"

#define region(d, l) (d<0? -1: d<l? 0: 1)

/* why oh why doesn't the compiler generate dbra here???!!! */
/* it would also be nice if it used a separate dbra for each arm of if */

extern char pr_reversedst[];
extern short    gpmrc_lmasktable[];
extern short    gpmrc_rmasktable[];

gp1_curve (dst, pos0, cu, n, op, color)
struct pixrect *dst;
struct pr_pos   pos0;
struct pr_curve cu[];
int     n, op, color;
{
    register    u_char * a5_adr;
    register struct cg2fb  *fb;
    register struct pr_curve   *cur;
    register    xinc, yinc;
    register short  count, src = 0, data;
    struct gp1pr   *dmd;
    struct pr_size  lim;
    int     x, y, nx, ny, X, Y, nX, nY, inside, ninside, linebytes;
    int     opn, absx, absy, xmajor = 1, b, oxmajor = -1;
    u_char * ixreg, *iyreg;
    int     yinc2;

    fb = (dmd = gp1_d (dst)) -> cgpr_va;
    data = color;
    color = PIX_OPCOLOR (op);
    if (color)
	data = color;
    op = (op >> 1) & 0xf;

    /* first sync with the GP */
    if(gp1_sync(dmd->gp_shmem, dmd->ioctl_fd))
        return(-1);

    if (PIXOP_NEEDS_DST (op << 1))/* setup rasterop */
	fb -> status.reg.ropmode = SWWPIX;
    else
	fb -> status.reg.ropmode = SRWPIX;
    fb -> ppmask.reg = dmd -> cgpr_planes;
    cg2_setfunction (fb, CG2_ALLROP, op);
    fb -> ropcontrol[CG2_ALLROP].ropregs.mrc_pattern = 0;
    fb -> ropcontrol[CG2_ALLROP].prime.ropregs.mrc_source1 =
	data | (data << 8);	/* load color in src */
    fb -> ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 = 0;
    fb -> ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 = 0;
    cg2_setwidth (fb, CG2_ALLROP, 0, 0);
    cg2_setshift (fb, CG2_ALLROP, 0, 1);
    cur = cu;
    nx = pos0.x, ny = pos0.y;
    lim = dst -> pr_size;
    nX = region (nx, lim.x), nY = region (ny, lim.y);
    a5_adr = cg2_roppixel (dmd, pos0.x, pos0.y);
    ninside = !nX && !nY;
    for (cur = cu; cur < cu + n; cur++)
    {
	x = nx, y = ny, X = nX, Y = nY, inside = ninside;
/* 	printf ("nx %d ny %d \n", nx, ny); */
	absx = abs (cur -> x), absy = abs (cur -> y);
	if (!(count = absx + absy))
	    continue;		/* guarantees count != 0 later */
	yinc2 = cg2_linebytes (fb, fb -> status.reg.ropmode);
	if ((int) cur -> y < 0)
	    yinc2 = -yinc2;
	nx = x + cur -> x, ny = y + cur -> y;
	nX = region (nx, lim.x), nY = region (ny, lim.y);
	ninside = !(nX || nY);
	if (absx != absy)
	{
	    xmajor = absx > absy;
	    if ((src < 0) != oxmajor && oxmajor != xmajor)
	    {
	    /* at axis change convert minor to major pixel */
	    *a5_adr = data;
	    }
	    oxmajor = xmajor;
	    if ((src = cur -> bits) < 0)
	    {
	    /* penup - reset position */
		a5_adr = cg2_roppixel (dmd, nx, ny);
		continue;
	    }
	    if (inside ? !ninside : !((X && (X == nX)) || (Y && (Y == nY))))
	    {
	    /* transition or mixed-axis - plot cautiously */
		xinc = cur -> x < 0 ? -1 : 1;
		yinc = cur -> y < 0 ? -1 : 1;
		do
		{
		    if ((src <<= 1) < 0)
			x += xinc, a5_adr += xinc;
		    else
		    {
			y += yinc;
			a5_adr += yinc2;
		    }
		    if ((unsigned) x < lim.x && (unsigned) y < lim.y &&
			    xmajor == src < 0)
			*a5_adr = data;
		} while (--count);
	    }
	    else
		if (inside)
		{
		    if (cur -> x < 0)
		    {
			if (xmajor)
			    do
			    {
				if ((src <<= 1) < 0)
				    *--a5_adr = data;
				else
				    a5_adr += yinc2;
			    } while (--count);
			else
			    do
			    {
				if ((src <<= 1) < 0)
				    a5_adr--;
				else
				{
				    a5_adr += yinc2;
				    *a5_adr = data;
				}
			    } while (--count);
		    }
		    else
		    {
			if (xmajor)
			    do
			    {
				if ((src <<= 1) < 0)
				    *++a5_adr = data;
				else
				    a5_adr += yinc2;
			    } while (--count);
			else
			    do
			    {
				if ((src <<= 1) < 0)
				    a5_adr++;
				else
				{
				    a5_adr += yinc2;
				    *a5_adr = data;
				}
			    } while (--count);
		    }
		}
		else
		{		/* clip */
		    a5_adr = cg2_roppixel (dmd, nx, ny);
		}
    loopend: ;
	}
	else
	{
	    if ((src = cur -> bits) < 0)
	    {
	    /* penup - recompute dadr, dbit from scratch */
		a5_adr = cg2_roppixel (dmd, nx, ny);
	    }
	    else
	    {
		count = cur -> x;
		if (count < 0)
		    count = -count;
		xinc = cur -> x < 0 ? -1 : 1;
		yinc = cur -> y < 0 ? -1 : 1;
		do
		{
		    x += xinc, a5_adr += xinc;
		    y += yinc, a5_adr += yinc2;
		    *a5_adr = data;
		} while (--count);
	    }
	}
    }
}

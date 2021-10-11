#ifndef	lint
static char     sccsid[] = "@(#)mem32_vec.c	1.1 94/10/31 SMI";
#endif

/* Copyright 1990 Sun Microsystems, Inc. */

/* 8 bit emulation in 24/32 bit memory pixrect */

#include <pixrect/pixrect.h>
#include <pixrect/mem32_var.h>
#include <pixrect/pr_planegroups.h>	/* PIX_ALL_PLANES */

mem32_vector(pr, x0, y0, x1, y1, op, color)
Pixrect            *pr;
int                 x0,
		    y0,
		    x1,
		    y1,
		    op,
		    color;
{
    /* If indexed, then resort to our own (mem_vector compatible) version. */

    if (mprp32_d(pr)->windowfd >= -1 && pr->pr_depth > 1 &&
	strcmp(mprp32_d(pr)->cms.cms_name, "monochrome") &&
	strncmp(mprp32_d(pr)->cms.cms_name,"rootcolor",9))
    {

	/*
	 * Variable usage:
	 * t		- the variable to keep track of the slope
	 * n		- used to know when to stop drawing
	 * dx		- the distance to be moved in the x direction
	 * dy		- the distance to be moved in the y direction
	 * x_inc	- the increment in the x direction
	 * y_inc	- the increment in the y direction
	 */

	int                 t,
			    n,
			    dx,
			    dy,
			    x_inc,
			    y_inc;
	unsigned int       *dst;
	int                 bytes,
			    planes,
			    not_planes,
			    old_color,
			    xsize,
			    ysize,
			    xlow,
			    ylow,
			    xy_major,
			    dst_val;
	unsigned char       red[MEM32_8BIT_CMAPSIZE],
			    green[MEM32_8BIT_CMAPSIZE],
			    blue[MEM32_8BIT_CMAPSIZE];
	struct colormapseg  cms;
	struct cms_map      cmap;
	unsigned int        int_cmap[MEM32_8BIT_CMAPSIZE];
	int                 i,
			    cms_addr,
			    cms_size;

	/*
	 * If the operation is Clear, Set, Source, or ~Source, and no special
	 * plane mask is used, then mem_vector can handle it with a little
	 * help.
	 */

	if (PIX_OPCOLOR(op))
	    color = PIX_OPCOLOR(op);
	op = PIX_OP(op);
	planes = mprp32_d(pr)->mprp.planes & PIX_ALL_PLANES;
	if (planes > MEM32_8BIT_CMAPSIZE)
	    planes = mprp32_d(pr)->cms.cms_size - 1;
	not_planes = ~planes;

	if ((op==PIX_CLR || op==PIX_SET || op==PIX_SRC || op==PIX_NOT(PIX_SRC))
	    && (planes == mprp32_d(pr)->cms.cms_size - 1))
	{
	    switch (op)
	    {
		case PIX_CLR:
		    color = mem32_get_true(pr, 0, planes);
		    break;
		case PIX_SET:
		    color = mem32_get_true(pr, ~0, planes);
		    break;
		case PIX_SRC:
		    color = mem32_get_true(pr, color, planes);
		    break;
		case PIX_NOT(PIX_SRC):
		    color = mem32_get_true(pr, ~color, planes);
		    break;
	    }
	    op = PIX_SRC;
	    mprp32_d(pr)->mprp.planes = PIX_ALL_PLANES;
	    return mem_vector(pr, x0, y0, x1, y1, op, color);
	}

	/*
	 * Always move x from left to right.  The position of x0,y0 with
	 * respect to x1,y1 indicates a positive or negative y increment.
	 */

	if (x1 < x0)
	{
	    dx = x0;
	    dy = y0;
	    x0 = x1;
	    y0 = y1;
	    x1 = dx;
	    y1 = dy;
	}

	/* Translate to pr_region coordinates */

	x0 += mprp32_d(pr)->mprp.mpr.md_offset.x;
	x1 += mprp32_d(pr)->mprp.mpr.md_offset.x;
	y0 += mprp32_d(pr)->mprp.mpr.md_offset.y;
	y1 += mprp32_d(pr)->mprp.mpr.md_offset.y;

	/* set up variables that don't vary inside the big loop */

	xlow = mprp32_d(pr)->mprp.mpr.md_offset.x;
	ylow = mprp32_d(pr)->mprp.mpr.md_offset.y;
	xsize = pr->pr_size.x + mprp32_d(pr)->mprp.mpr.md_offset.x;
	ysize = pr->pr_size.y + mprp32_d(pr)->mprp.mpr.md_offset.y;

	cmap.cm_red = red;
	cmap.cm_green = green;
	cmap.cm_blue = blue;
	mem32_get_index_cmap(pr, &cms, &cmap);

	for (i = 0; i < MEM32_8BIT_CMAPSIZE; i++)
	    int_cmap[i] = (int) (blue[i] << 16 | green[i] << 8 | red[i]);

	cms_addr = cms.cms_addr;
	cms_size = cms.cms_addr + cms.cms_size;
	dst = (unsigned int *) mprp32_d(pr)->mprp.mpr.md_image;
	bytes = mprp32_d(pr)->mprp.mpr.md_linebytes / sizeof(int);

	x_inc = 1;
	y_inc = (y1 > y0) ? 1 : -1;

	dx = x1 - x0;
	dy = abs(y1 - y0);

	/*
	 * t is initially set to half the longest distance to be traveled. n
	 * is set to the longest distance.  The longest axis is always
	 * incremented. The other axis is incremented everytime the slope
	 * variable, t, indicates that a point would have been > 1 from the
	 * last axis increment in floating point. This is repeated n times.
	 */

	if (dx > dy)
	{
	    xy_major = 0;
	    t = dx >> 1;
	    n = dx;
	}
	else
	{
	    xy_major = 1;
	    t = dy >> 1;
	    n = dy;
	}

	for (;; n--)
	{

	    /*
	     * This is dumb, slow, inefficient, and even embarassing BUT a
	     * quick and dirty clipping by not writing the pixel if outside
	     * the pr_region bounds.  It does insure EXACT pixel accuracy
	     * with mem_vector.  To be followed by something more efficient
	     * if we get the time. (how hard are you laughing?)
	     */

	    if (!(op & PIX_DONTCLIP) && x0 >= xlow && x0 < xsize &&
		y0 >= ylow && y0 < ysize)
	    {
		old_color = dst[x0 + y0 * bytes];
		for (i = cms_addr; i < cms_size; i++)
		{
		    if (old_color == int_cmap[i])
		    {
			old_color = i;
			break;
		    }
		}
		if (i == cms_size)
		    old_color = (old_color == int_cmap[0])
			? 0 : MEM32_8BIT_CMAPSIZE - 1;
		dst_val = old_color;

		switch (op >> 1)
		{
		    case 0:		/* CLR */
			dst_val = 0;
			break;

		    case 1:		/* ~s&~d or ~(s|d) */
			dst_val = ~(color | dst_val);
			break;

		    case 2:		/* ~s&d */
			dst_val = ~color & dst_val;
			break;

		    case 3:		/* ~s */
			dst_val = ~color;
			break;

		    case 4:		/* s&~d */
			dst_val = color & ~dst_val;
			break;

		    case 5:		/* ~d */
			dst_val = ~dst_val;
			break;

		    case 6:		/* s^d */
			dst_val = color ^ dst_val;
			break;

		    case 7:		/* ~s|~d or ~(s&d) */
			dst_val = ~(color & dst_val);
			break;

		    case 8:		/* s&d */
			dst_val = color & dst_val;
			break;

		    case 9:		/* ~(s^d) */
			dst_val = ~(color ^ dst_val);
			break;

		    case 10:		/* DST */
			break;

		    case 11:		/* ~s|d */
			dst_val = ~color | dst_val;
			break;

		    case 12:		/* SRC */
			dst_val = color;
			break;

		    case 13:		/* s|~d */
			dst_val = color | ~dst_val;
			break;

		    case 14:		/* s|d */
			dst_val = color | dst_val;
			break;

		    case 15:		/* SET */
			dst_val = PIX_ALL_PLANES;
			break;
		}

		dst[x0 + y0 * bytes] =
		    int_cmap[(not_planes & old_color) | (planes & dst_val)];
	    }

	    if (n == 0)
		return 0;

	    if (xy_major)
	    {
		y0 += y_inc;
		t += dx;
		if (t > dy)
		{
		    x0 += x_inc;
		    t -= dy;
		}
	    }
	    else
	    {
		x0 += x_inc;
		t += dy;
		if (t > dx)
		{
		    y0 += y_inc;
		    t -= dx;
		}
	    }
	}
    }

    /* otherwise, let mem_vector take it. */
    return mem_vector(pr, x0, y0, x1, y1, op, color);
}

#ifndef lint
static char sccsid[] = "@(#)mem_colormap.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * Memory pixrect "colormap" routines.
 */
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>

/*ARGSUSED*/
mem_putcolormap(pr, index, count, red, green, blue)
	Pixrect *pr;
	int index, count;
	u_char red[], green[], blue[];
{
	if (pr && index == 0 && count > 0)
		if (red[0])
			mpr_d(pr)->md_flags &= ~MP_REVERSEVIDEO;
		else
			mpr_d(pr)->md_flags |= MP_REVERSEVIDEO;

	return 0;
}

mem_putattributes(pr, planes)
	Pixrect *pr;
	int *planes;
{
	if (pr && planes && (mprp_d(pr)->mpr.md_flags & MP_PLANEMASK))
		mprp_d(pr)->planes = *planes;

	return 0;
}

#ifndef	KERNEL
mem_getattributes(pr, planes)
	Pixrect *pr;
	int *planes;
{
	if (pr && planes)
		if (mprp_d(pr)->mpr.md_flags & MP_PLANEMASK)
			*planes = mprp_d(pr)->planes;
		else
			*planes = ~0;

	return 0;
}

mem_getcolormap(pr, index, count, red, green, blue)
	Pixrect *pr;
	register int index, count;
	u_char red[], green[], blue[];
{
	int start;

	if (pr && index >= 0) {
		register u_char rgb = 
			mpr_d(pr)->md_flags & MP_REVERSEVIDEO ? 0 : 255;

		if (index)
			rgb = ~rgb;

		start = 0;
		while (--count >= 0) {
			red[start] = rgb;
			green[start] = rgb;
			blue[start] = rgb;

			if (index == 0)
				rgb = ~rgb;

			index++, start++;
		}
	}

	return 0;
}
#endif !KERNEL

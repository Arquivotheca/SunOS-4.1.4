#ifndef lint
static char sccsid[] = "@(#)bw1_colormap.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun1 Black-and-White pixrect colormap routines
 */
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/bw1reg.h>
#include <pixrect/bw1var.h>

/*ARGSUSED*/
bw1_putcolormap(pr, index, count, red, green, blue)
	Pixrect *pr;
	int index, count;
	u_char red[], green[], blue[];
{
	if (pr && index == 0 && count > 0)
		/*
		 * Note: BW_REVERSEVIDEO gives black on white.
		 */
		if (red[0])
			bw1_d(pr)->bwpr_flags |= BW_REVERSEVIDEO;
		else
			bw1_d(pr)->bwpr_flags &= ~BW_REVERSEVIDEO;

	return 0;
}

/*ARGSUSED*/
bw1_putattributes(pr, planes)
	Pixrect *pr;
	int *planes;
{
	return 0;
}

#ifndef	KERNEL

/*ARGSUSED*/
bw1_getattributes(pr, planes)
	Pixrect *pr;
	int *planes;
{
	if (planes)
		*planes = 1;

	return 0;
}

bw1_getcolormap(pr, index, count, red, green, blue)
	Pixrect *pr;
	register int index, count;
	u_char red[], green[], blue[];
{
	if (pr && index >= 0) {
		register u_char rgb = 
			bw1_d(pr)->bwpr_flags & BW_REVERSEVIDEO ? 255 : 0;

		if (index)
			rgb = ~rgb;

		while (--count >= 0) {
			red[index] = rgb;
			green[index] = rgb;
			blue[index] = rgb;

			if (index == 0)
				rgb = ~rgb;

			index++;
		}
	}

	return 0;
}
#endif !KERNEL

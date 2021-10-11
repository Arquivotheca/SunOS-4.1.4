#ifndef lint
static	char sccsid[] = "@(#)cg1_colormap.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/cg1reg.h>
#include <pixrect/cg1var.h>

/*
 * Read and Write portions of the Sun 1 color frame buffer's color map
 * (color look-up table).  The hardware will not modify the values in the
 * color map except during vertical retrace so the software must rewrite
 * until the transfer starts and ends in vertical retrace.  Any portion of
 * the map may be accessed.  The color map consists of three arrays of byte
 * values (0..255) namely red, green, and blue.  A given index thus selects
 * a 24 bit color value.  The data is passed in 3 separate arrays for
 * convenience in hardware access.
 */

cg1_putcolormap(pr, index, count, red, green, blue)
	struct pixrect *pr;
	int index, count;
	u_char red[], green[], blue[];
{
	register u_char *color,*color_map;
	register short rgb, n;
	struct cg1fb *fb;

	fb = (cg1_d(pr))->cgpr_va;
	if (index > 255 || index < 0)
		return (-1);
	n = ((index + count) > 256) ? 256 - index : count; 
	cg1_setmap_rdwr(fb, 0);
	for (rgb = 0; rgb < 3; rgb++)  {
		do {	/* entire color map within vertical retrace */
			while (!cg1_retrace(fb))
				;
			switch (rgb) {
			case 0:
				color = red;
				color_map = cg1_map(fb, CG_REDMAP, index);
				break;
			case 1:
				color = green;
				color_map = cg1_map(fb, CG_GREENMAP, index);
				break;
			case 2:
				color = blue;
				color_map = cg1_map(fb, CG_BLUEMAP, index);
				break;
			} 
			rop_fastloop(n, *color_map++ = *color++);
		} while (!cg1_retrace(fb));
	}
	return (0);
}

cg1_putattributes(pr, planes)
	struct pixrect *pr;
	int *planes;
{

	if (planes)
		(cg1_d(pr))->cgpr_planes = *planes;
	return (0);
}

#ifndef	KERNEL
cg1_getcolormap(pr, index, count, red, green, blue)
	struct pixrect *pr;
	int index, count;
	u_char red[], green[], blue[];
{
	register u_char *color, *color_map;
	register short rgb, n;
	struct cg1fb *fb;

	fb = (cg1_d(pr))->cgpr_va;
	if (index > 255 || index < 0)
		return (-1);
	n = ((index + count) > 256) ? 256 - index : count; 
	cg1_setmap_rdwr(fb, 0);
	for (rgb = 0; rgb < 3; rgb++)  {
		do {	/* entire map must be written during vertical retrace */
			while (!cg1_retrace(fb))
				;
			switch (rgb) {
			case 0:
				color = red;
				color_map = cg1_map(fb, CG_REDMAP, index);
				break;
			case 1:
				color = green;
				color_map = cg1_map(fb, CG_GREENMAP, index);
				break;
			case 2:
				color = blue;
				color_map = cg1_map(fb, CG_BLUEMAP, index);
				break;
			} 
			rop_fastloop(n, *color++ = *color_map++);
		} while (!cg1_retrace(fb));
	}
	return (0);
}

cg1_getattributes(pr, planes)
	struct pixrect *pr;
	int *planes;
{

	if (planes)
		*planes = (cg1_d(pr))->cgpr_planes;
	return (0);
}
#endif !KERNEL

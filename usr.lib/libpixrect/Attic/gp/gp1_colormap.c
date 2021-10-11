#ifndef lint
static char sccsid[] = "@(#)gp1_colormap.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>
#include <pixrect/gp1var.h>

/*
 * Read and Write portions of the gp frame buffer's color map
 * (color look-up table).  Any portion of the map may be accessed.  The
 * color map consists of three arrays of byte values (0..255) namely red,
 * green, and blue.  A given index thus selects a 24 bit color value.  The
 * data is passed in 3 separate arrays for convenience in hardware access.
 */

gp1_putcolormap(pr, index, count, red, green, blue)
	struct pixrect *pr;
	int index, count;
	u_char red[], green[], blue[];
{
	register u_char *color;
	register u_short *color_map;
	register short rgb, n;
	struct cg2fb *fb;

	fb = (gp1_d(pr))->cgpr_va;
	if (index > 255 || index < 0)
		return(-1);
	n = ((index + count) > 256) ? 256 - index : count; 

        /* first sync with the GP */

#ifdef KERNEL
	if (gp1_kern_sync((gp1_d(pr))->gp_shmem))
		kernsyncrestart((gp1_d(pr))->gp_shmem);
#else
	if(gp1_sync( (gp1_d(pr))->gp_shmem, (gp1_d(pr))->ioctl_fd))
		return(-1);
#endif

	fb->status.reg.update_cmap = 0;		/* enable write to cmap */
	for (rgb = 0; rgb < 3; rgb++)  {
		switch (rgb) {
		case 0:
			color = red;
			color_map = &(fb->redmap[index]);
			break;
		case 1:
			color = green;
			color_map = &(fb->greenmap[index]);
			break;
		case 2:
			color = blue;
			color_map = &(fb->bluemap[index]);
			break;
		} 
		rop_fastloop(n, *color_map++ = *color++);
	}
	fb->status.reg.update_cmap = 1;		/* copy TTL cmap to ECL cmap */
	return (0);
}

gp1_putattributes(pr, planes)
	struct pixrect *pr;
	int *planes;
{

	if (planes)
		(gp1_d(pr))->cgpr_planes = *planes;
	return (0);
}

#ifndef	KERNEL
gp1_getcolormap(pr, index, count, red, green, blue)
struct pixrect *pr;
int index, count;
u_char red[], green[], blue[];
{
	register u_char *color;
	register u_short *color_map;
	register short rgb, n;
	struct cg2fb *fb;

	fb = (gp1_d(pr))->cgpr_va;
	if (index > 255 || index < 0)
		return(-1);
	n = ((index + count) > 256) ? 256 - index : count; 

        /* first sync with the GP */

#ifdef KERNEL
	if (gp1_kern_sync((gp1_d(pr))->gp_shmem))
		kernsyncrestart((gp1_d(pr))->gp_shmem);
#else
	if(gp1_sync( (gp1_d(pr))->gp_shmem, (gp1_d(pr))->ioctl_fd))
		return(-1);
#endif

	fb->status.reg.update_cmap = 0;		/* enable read from cmap */
	for (rgb = 0; rgb < 3; rgb++)  {
		switch (rgb) {
		case 0:
			color = red;
			color_map = &(fb->redmap[index]);
			break;
		case 1:
			color = green;
			color_map = &(fb->greenmap[index]);
			break;
		case 2:
			color = blue;
			color_map = &(fb->bluemap[index]);
			break;
		} 
		rop_fastloop(n, *color++ = 0xFF & *color_map++);
	}
	fb->status.reg.update_cmap = 1;		/* copy TTL cmap to ECL cmap */
	return (0);
}

gp1_getattributes(pr, planes)
	struct pixrect *pr;
	int *planes;
{

	if (planes)
		*planes = (gp1_d(pr))->cgpr_planes;
	return (0);
}
#endif !KERNEL

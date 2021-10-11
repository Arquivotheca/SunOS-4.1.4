#if !defined(lint) && !defined(NOID)
static char sccsid[] = "@(#)cg3_colormap.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1989, Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <pixrect/pixrect.h>
#include <pixrect/cg3var.h>
#include <pixrect/pr_planegroups.h>

cg3_putcolormap(pr, index, count, red, green, blue)
	Pixrect *pr;
	int index, count;
	unsigned char *red, *green, *blue;
{
	struct fbcmap cmap;

	cmap.index = index & PIX_ALL_PLANES;
	cmap.count = count;
	cmap.red   = red;
	cmap.green = green;
	cmap.blue  = blue;
	return ioctl(cg3_d(pr)->fd, FBIOPUTCMAP, &cmap);
}

cg3_getcolormap(pr, index, count, red, green, blue)
	Pixrect *pr;
	int index, count;
	unsigned char *red, *green, *blue;
{
	struct fbcmap cmap;

	cmap.index = index & PIX_ALL_PLANES;
	cmap.count = count;
	cmap.red   = red;
	cmap.green = green;
	cmap.blue  = blue;
	return ioctl(cg3_d(pr)->fd, FBIOGETCMAP, &cmap);
}

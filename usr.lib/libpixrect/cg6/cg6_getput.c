#ifndef lint
static char sccsid[] = "@(#)cg6_getput.c	1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1983, 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun1 Color get and put single pixel
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memreg.h>
#include <pixrect/cg6var.h>


cg6_get(pr, x, y)
	Pixrect *pr;
	int x, y;
{
	register struct fbc *fbc = cg6_d(pr)->cg6_fbc;

	cg6_waitidle(fbc);

	return (mem_get(pr, x, y));
}

cg6_put(pr, x, y, value)
	Pixrect *pr;
	int x, y;
	int value;
{
	register struct fbc *fbc = cg6_d(pr)->cg6_fbc;

	cg6_waitidle(fbc);

	return (mem_put(pr, x, y, value));
}

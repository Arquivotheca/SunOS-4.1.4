#ifndef lint
static char     sccsid[] = "@(#)mem_polyline.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Memory pixrect polyline
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>


mem_polyline(dst, x,y, n, op, color)
	struct pixrect *dst;
	int *x, *y, n;
	int op, color;
{
	int x0, y0;

	while(--n) {
		x0 = *x++; y0 = *y++;
		mem_vector(dst, x0, y0, *x, *y, op, color);
	}
}

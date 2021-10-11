#ifndef lint
static char     sccsid[] =      "@(#)gt_lint.c	1.1 94/10/31 SMI";
#endif
 
#ifndef LINT_INCLUDE
#define LINT_INCLUDE
#include <pixrect/pixrect_hs.h>
#include <stdio.h>
#endif

/* gt specific functions */

Pixrect *
gt_make(fd, size, depth)
	int fd, depth;
	struct pr_size size;
	{ return (Pixrect *) 0; }

gt_rop(dpr, dx, dy, w, h, op, spr, sx, sy) 
	Pixrect *dpr, *spr;
	int dx, dy, w, h, op, sx, sy;
	{ return 0; }

gt_stencil(dpr, dx, dy, w, h, op, stpr, stx, sty, spr, sx, sy) 
	Pixrect *dpr, *stpr, *spr;
	int dx, dy, w, h, op, stx, sty, sx, sy;
	{ return 0; }

gt_destroy(pr) 
	Pixrect *pr;
	{ return 0; }

gt_get(pr, x, y) 
	Pixrect *pr;
	int x, y;
	{ return 0; }

gt_put(pr, x, y, value) 
	Pixrect *pr;
	int x, y, value;
	{ return 0; }

gt_vector(pr, x0, y0, x1, y1, op, color) 
	Pixrect *pr;
	int x0, y0, x1, y1, op, color;
	{ return 0; }

Pixrect *
gt_region(pr, x, y, w, h) 
	Pixrect *pr;
	int x, y, w, h;
	{ return (Pixrect *) 0; }

gt_putcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

gt_getcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

gt_putattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }

gt_getattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }

/* 	bw1_lint.c 1.1 of 10/31/94
*	@(#)bw1_lint.c	1.1
*/
#ifndef LINT_INCLUDE
#define LINT_INCLUDE
#include <pixrect/pixrect_hs.h>
#include <stdio.h>
#endif

/* bw1 specific functions */

Pixrect *
bw1_make(fd, size, depth)
	int fd, depth;
	struct pr_size size;
	{ return (Pixrect *) 0; }

bw1_rop(dpr, dx, dy, w, h, op, spr, sx, sy) 
	Pixrect *dpr, *spr;
	int dx, dy, w, h, op, sx, sy;
	{ return 0; }

bw1_batchrop(dpr, x, y, op, items, n) 
	Pixrect *dpr;
	int x, y, op, n;
	struct pr_prpos items[];
	{ return 0; }

bw1_destroy(pr) 
	Pixrect *pr;
	{ return 0; }

bw1_get(pr, x, y) 
	Pixrect *pr;
	int x, y;
	{ return 0; }

bw1_put(pr, x, y, value) 
	Pixrect *pr;
	int x, y, value;
	{ return 0; }

bw1_vector(pr, x0, y0, x1, y1, op, color) 
	Pixrect *pr;
	int x0, y0, x1, y1, op, color;
	{ return 0; }

Pixrect *
bw1_region(pr, x, y, w, h) 
	Pixrect *pr;
	int x, y, w, h;
	{ return (Pixrect *) 0; }

bw1_putcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

bw1_getcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

bw1_putattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }

bw1_getattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }

/* 	mem_lint.c 1.1 of 10/31/94
*	@(#)mem_lint.c	1.1
*/
#ifndef LINT_INCLUDE
#define LINT_INCLUDE
#include <pixrect/pixrect_hs.h>
#include <stdio.h>
#endif

/* memory pixrect specific functions */

mem_rop(dpr, dx, dy, w, h, op, spr, sx, sy) 
	Pixrect *dpr, *spr;
	int dx, dy, w, h, op, sx, sy;
	{ return 0; }

mem_stencil(dpr, dx, dy, w, h, op, stpr, stx, sty, spr, sx, sy) 
	Pixrect *dpr, *stpr, *spr;
	int dx, dy, w, h, op, stx, sty, sx, sy;
	{ return 0; }

mem_batchrop(dpr, x, y, op, items, n) 
	Pixrect *dpr;
	int x, y, op, n;
	struct pr_prpos items[];
	{ return 0; }

mem_destroy(pr) 
	Pixrect *pr;
	{ return 0; }

mem_get(pr, x, y) 
	Pixrect *pr;
	int x, y;
	{ return 0; }

mem_put(pr, x, y, value) 
	Pixrect *pr;
	int x, y, value;
	{ return 0; }

mem_vector(pr, x0, y0, x1, y1, op, color) 
	Pixrect *pr;
	int x0, y0, x1, y1, op, color;
	{ return 0; }

Pixrect *
mem_region(pr, x, y, w, h) 
	Pixrect *pr;
	int x, y, w, h;
	{ return (Pixrect *) 0; }

mem_putcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

mem_getcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

mem_putattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }

mem_getattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }
/* 
 * memory pixrects 
 */

struct pixrectops mem_ops;

Pixrect *
mem_create(w, h, depth) 
	int w, h, depth;
	{ return (Pixrect *) 0; }

Pixrect *
mem_point(w, h, depth, data)
	int w, h, depth;
	short *data;
	{ return (Pixrect *) 0; }

short *
_mprs_addr(memprs)
	struct pr_prpos *memprs;
	{ return (short *) 0; }

u_char *
_mprs8_addr(memprs)
	struct pr_prpos *memprs;
	{ return (u_char *) 0; }

_mprs_skew(memprs)
	struct pr_prpos *memprs;
	{ return 0; }


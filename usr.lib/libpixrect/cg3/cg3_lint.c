/*
 * @(#)cg3_lint.c 1.1 94/10/31 SMI
 */

#ifndef LINT_INCLUDE
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/cg3var.h>
#endif LINT_INCLUDE

struct pixrectops cg3_ops;

Pixrect *
cg3_make(w, h, depth) 
	int w, h, depth;
	{ return (Pixrect *) 0; }

cg3_destroy(pr) 
	Pixrect *pr;
	{ return 0; }

Pixrect *
cg3_region(pr, x, y, w, h) 
	Pixrect *pr;
	int x, y, w, h;
	{ return (Pixrect *) 0; }

cg3_putcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

cg3_getcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }


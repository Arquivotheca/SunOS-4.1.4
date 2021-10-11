/* 	tv1_lint.c 1.1 of 10/31/94
*	@(#)tv1_lint.c	1.1
*/
#ifndef LINT_INCLUDE
#define LINT_INCLUDE
#include <pixrect/pixrect_hs.h>
#include <stdio.h>
#endif

/* tv1 specific functions */

Pixrect *
tv1_make(fd, size, depth)
	int fd, depth;
	struct pr_size size;
	{ return (Pixrect *) 0; }

tv1_destroy(pr) 
	Pixrect *pr;
	{ return 0; }

Pixrect *
tv1_region(pr, x, y, w, h) 
	Pixrect *pr;
	int x, y, w, h;
	{ return (Pixrect *) 0; }

tv1_putcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

tv1_getcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

tv1_putattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }

tv1_getattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }

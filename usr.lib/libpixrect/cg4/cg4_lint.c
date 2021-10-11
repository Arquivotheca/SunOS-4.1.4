/* 	cg4_lint.c 1.1 of 10/31/94
*	@(#)cg4_lint.c	1.1
*/
#ifndef LINT_INCLUDE
#define LINT_INCLUDE
#include <pixrect/pixrect_hs.h>
#include <stdio.h>
#endif

/* cg4 specific functions */

Pixrect *
cg4_make(fd, size, depth)
	int fd, depth;
	struct pr_size size;
	{ return (Pixrect *) 0; }

cg4_destroy(pr) 
	Pixrect *pr;
	{ return 0; }

Pixrect *
cg4_region(pr, x, y, w, h) 
	Pixrect *pr;
	int x, y, w, h;
	{ return (Pixrect *) 0; }

cg4_putcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

cg4_getcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

cg4_putattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }

cg4_getattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }

cg4_ioctl(pr, cmd, data, flag)
	Pixrect        *pr;
	int             cmd;
	char           *data;
	int             flag;
	{ return 0; }

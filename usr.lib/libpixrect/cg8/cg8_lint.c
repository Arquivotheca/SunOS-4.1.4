/* 	@(#)cg8_lint.c	1.1 94/10/31 */
#ifndef LINT_INCLUDE
#define LINT_INCLUDE
#include <pixrect/pixrect_hs.h>
#include <stdio.h>
#endif

/* cg8 specific functions */

Pixrect *
cg8_make(fd, size, depth)
	int fd, depth;
	struct pr_size size;
	{ return (Pixrect *) 0; }

cg8_destroy(pr) 
	Pixrect *pr;
	{ return 0; }

cg8_putcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

cg8_getcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

cg8_putattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }

cg8_getattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }

Pixrect *
cg8_region(pr, x, y, w, h) 
      Pixrect *pr;
      int x, y, w, h;
      { return (Pixrect *) 0; }

cg8_rop(dpr, dx, dy, w, h, op, spr, sx, sy) 
      Pixrect *dpr, *spr;
      int dx, dy, w, h, op, sx, sy;
      { return 0; }

cg8_ioctl(pr, cmd, data, flag)
	Pixrect        *pr;
	int             cmd;
	char           *data;
	int             flag;
	{ return 0; }

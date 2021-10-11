/* @(#)cg12_lint.c	1.1 94/10/31 SMI */

#ifndef LINT_INCLUDE
#define LINT_INCLUDE
#include <pixrect/pixrect_hs.h>
#include <stdio.h>
#endif	LINT_INCLUDE

/* cg12 specific functions */

Pixrect            *
cg12_make(fd, size, depth, attr)
int                 fd;
struct pr_size      size;
int                 depth;
struct fbgattr     *attr;
{ return (Pixrect *) 0; }

cg12_destroy(pr)
Pixrect            *pr;
{ return 0; }

cg12_batchrop(dpr, dx, dy, op, src, count)
Pixrect            *dpr;
int                 dx, dy, op;
struct pr_prpos    *src;
int                 count;
{ return 0; }

cg12_putcolormap(pr, index, count, red, green, blue)
Pixrect            *pr;
int                 index, count;
unsigned char       red[], green[], blue[];
{ return 0; }

cg12_getcolormap(pr, index, count, red, green, blue)
Pixrect            *pr;
int                 index, count;
unsigned char       red[], green[], blue[];
{ return 0; }

cg12_putattributes(pr, planes)
Pixrect            *pr;
int                *planes;
{ return 0; }

cg12_getattributes(pr, planes)
Pixrect            *pr;
int                *planes;
{ return 0; }

cg12_get(pr, x, y)
Pixrect            *pr;
int                 x, y;
{ return mem_get(pr, x, y); }

cg12_put(pr, x, y, value)
Pixrect            *pr;
int                 x, y, value;
{ return 0; }

cg12_ioctl(pr, cmd, data, flag)
Pixrect            *pr;
int                 cmd;
char               *data;
int                 flag;
{ return 0; }

Pixrect            *
cg12_region(pr, x, y, w, h)
Pixrect            *pr;
int                 x, y, w, h;
{ return (Pixrect *) 0; }

cg12_replrop(dpr, dx, dy, dw, dh, op, spr, sx, sy)
Pixrect            *dpr;
int                 dx, dy, dw, dh, op;
Pixrect            *spr;
int                 sx, sy;
{ return 0; }

cg12_rop(dpr, dx, dy, w, h, op, spr, sx, sy)
Pixrect            *dpr, *spr;
int                 dx, dy, w, h, op, sx, sy;
{ return 0; }

cg12_stencil(dpr, dx, dy, dw, dh, op, stpr, stx, sty, spr, sx, sy)
Pixrect            *dpr;
int                 dx, dy, dw, dh, op;
Pixrect            *stpr;
int                 stx, sty;
Pixrect            *spr;
int                 sx, sy;
{ return 0; }

cg12_vector(pr, x0, y0, x1, y1, op, color)
Pixrect            *pr;
int                 x0, y0, x1, y1, op, color;
{ return 0; }

/* 	gp1_lint.c 1.1 of 10/31/94
*	@(#)gp1_lint.c	1.1
*/
#ifndef LINT_INCLUDE
#define LINT_INCLUDE
#include <pixrect/pixrect_hs.h>
#include <stdio.h>
#endif

/* gp1 specific functions */

Pixrect *
gp1_make(fd, size, depth)
	int fd, depth;
	struct pr_size size;
	{ return (Pixrect *) 0; }

gp1_rop(dpr, dx, dy, w, h, op, spr, sx, sy) 
	Pixrect *dpr, *spr;
	int dx, dy, w, h, op, sx, sy;
	{ return 0; }

gp1_stencil(dpr, dx, dy, w, h, op, stpr, stx, sty, spr, sx, sy) 
	Pixrect *dpr, *stpr, *spr;
	int dx, dy, w, h, op, stx, sty, sx, sy;
	{ return 0; }

gp1_batchrop(dpr, x, y, op, items, n) 
	Pixrect *dpr;
	int x, y, op, n;
	struct pr_prpos items[];
	{ return 0; }

gp1_destroy(pr) 
	Pixrect *pr;
	{ return 0; }

gp1_get(pr, x, y) 
	Pixrect *pr;
	int x, y;
	{ return 0; }

gp1_put(pr, x, y, value) 
	Pixrect *pr;
	int x, y, value;
	{ return 0; }

gp1_vector(pr, x0, y0, x1, y1, op, color) 
	Pixrect *pr;
	int x0, y0, x1, y1, op, color;
	{ return 0; }

Pixrect *
gp1_region(pr, x, y, w, h) 
	Pixrect *pr;
	int x, y, w, h;
	{ return (Pixrect *) 0; }

gp1_putcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

gp1_getcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

gp1_putattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }

gp1_getattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }

gp1_line(pr, x0, y0, x1, y1, brush, tex, op)
	Pixrect *pr;
	int x0, y0, x1, y1;
	struct pr_brush *brush;
	Pr_texture *tex;
	{ return 0; }

gp1_polygon_2(dpr, dx, dy, nbnds, npts, vlist, op, spr, sx, sy)
	Pixrect *dpr, *spr;
	int dx, dy, nbnds, npts[], op, sx, sy;
	struct pr_pos *vlist;
	{ return 0; }

gp1_polyline(pr, dx, dy, npts, ptlist, mvlist, brush, tex, op)
	Pixrect *pr;
	int dx, dy, npts;
	struct pr_pos *ptlist;
	u_char *mvlist;
	struct pr_brush *brush;
	Pr_texture *tex;
	{ return 0; }

gp1_replrop(dpr, dx, dy, dw, dh, op, spr, sx, sy)
	Pixrect *dpr, *spr;
	int dx, dy, dw, dh, op, sx, sy;
	{ return 0; }

gp1_ioctl(pr, cmd, data, flag)
	Pixrect *pr;
	int cmd;
	char *data;
	int flag;
	{ return 0; }

/* gp1 low level functions */

gp1_alloc(shmem, nblocks, bitvec, minordev, fd)
	caddr_t shmem;
	int nblocks, minordev, fd;
	unsigned int *bitvec;
	{ return 0; }

gp1_post(shmem, offset, fd)
	caddr_t shmem;
	short offset;
	int fd;
	{ return 0; }

gp1_sync(shmem, fd)
	caddr_t shmem;
	int fd;
	{ return 0; }


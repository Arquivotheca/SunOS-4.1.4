#ifndef lint
static char sccsid[] = "@(#)cg1_polypoint.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

/*
 * Draw a list of points on the screen
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/cg1reg.h>
#include <pixrect/cg1var.h>

cg1_polypoint(pr, dx, dy, n, ptlist, op)
	Pixrect *pr;
	register dx, dy, n;
	register struct pr_pos *ptlist;
	int op;
{
	register color;
	int noclip;
	register struct cg1pr *prd = cg1_d(pr);
	register struct cg1fb *fb = prd->cgpr_va;

	if (--n < 0)
		return PIX_ERR;

	dx += prd->cgpr_offset.x;
	dy += prd->cgpr_offset.y;

	color = PIX_OPCOLOR(op);
	noclip = op & PIX_DONTCLIP;
	op = (op>>1) & 0xf;
	op = (CG_NOTMASK & CG_DEST)|(CG_MASK & ((op<<4)|op));
	cg1_setfunction(fb, op);
	cg1_setmask(fb, prd->cgpr_planes);

	if (noclip) {
		do {
			cg1_touch(*cg1_xdst(fb, ptlist->x + dx));
			*cg1_ydst(fb, ptlist->y + dy) = (u_char) color;
			ptlist++;
		} while (--n != -1);
	} 
	else {
		register int x_min, x_max, y_min, y_max;

		x_min = prd->cgpr_offset.x - dx;
		x_max = pr->pr_size.x + x_min;
		y_min = prd->cgpr_offset.y - dy;
		y_max = pr->pr_size.y + y_min;

		do {
			if (ptlist->x >= x_min && ptlist->x < x_max &&
				ptlist->y >= y_min && ptlist->y < y_max) {
				cg1_touch(*cg1_xdst(fb, ptlist->x + dx));
				*cg1_ydst(fb, ptlist->y + dy) = (u_char) color;
			}
			ptlist++;
		} while (--n != -1);
	}

	return 0;
}

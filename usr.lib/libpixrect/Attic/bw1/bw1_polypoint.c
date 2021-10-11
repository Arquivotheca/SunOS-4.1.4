#ifndef lint
static char sccsid[] = "@(#)bw1_polypoint.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

/*
 * Draw a list of points on the screen
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/bw1reg.h>
#include <pixrect/bw1var.h>

extern char pr_reversedst[];

bw1_polypoint(pr, dx, dy, n, ptlist, op)
	Pixrect *pr;
	register dx, dy, n;
	register struct pr_pos *ptlist;
	int op;
{
	register color;
	int noclip;
	struct bw1pr *prd = bw1_d(pr);
	register struct bw1fb *fb = prd->bwpr_va;

	if (--n < 0)
		return PIX_ERR;

	dx += prd->bwpr_offset.x;
	dy += prd->bwpr_offset.y;

	color = PIX_OPCOLOR(op);
	noclip = op & PIX_DONTCLIP;
	op = (op>>1) & 0xf;
	if (prd->bwpr_flags & BW_REVERSEVIDEO)
	    op = pr_reversedst[op];

	bw1_setfunction(fb, op);
	bw1_setmask(fb, 0);
	bw1_setwidth(fb, 1);

	color = color ? ~0 : 0;

	if (noclip) {
		do {
			bw1_touch(*bw1_xdst(fb, ptlist->x + dx));
			*bw1_ydst(fb, ptlist->y + dy) = color;
			ptlist++;
		} while (--n != -1);
	} 
	else {
		register int x_min, x_max, y_min, y_max;

		x_min = prd->bwpr_offset.x - dx;
		x_max = pr->pr_size.x + x_min;
		y_min = prd->bwpr_offset.y - dy;
		y_max = pr->pr_size.y + y_min;

		do {
			if (ptlist->x >= x_min && ptlist->x < x_max &&
				ptlist->y >= y_min && ptlist->y < y_max) {
				bw1_touch(*bw1_xdst(fb, ptlist->x + dx));
				*bw1_ydst(fb, ptlist->y + dy) = color;
			}
			ptlist++;
		} while (--n != -1);
	}

	return 0;
}

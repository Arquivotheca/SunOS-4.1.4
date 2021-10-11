#ifndef lint
static char sccsid[] = "@(#)cg2_polypoint.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * Draw a list of points on the screen
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>

cg2_polypoint(pr, x, y, n, ptlist, op)
	Pixrect *pr;
	register x, y;
	int n;
	register struct pr_pos *ptlist;
	int op;
{
	register struct cg2pr *prd = cg2_d(pr);
	register struct cg2fb *fb = prd->cgpr_va;

#define dummy	IF68000(x, 0)

	x += prd->cgpr_offset.x;
	y += prd->cgpr_offset.y;

	GP1_PRD_SYNC(prd, return PIX_ERR);

	{
		register struct memropc *allrop = 
			&fb->ropcontrol[CG2_ALLROP].ropregs;
		register int rop = op, color, planes;

		color = PIXOP_COLOR(rop);
		rop = PIXOP_OP(rop);

		fb->status.reg.ropmode = 
			PIX_OP_NEEDS_DST(rop) ? SWWPIX : SRWPIX;

		fb->ppmask.reg = planes = prd->cgpr_planes & 255;

		/* set function for 1 bits in color */
		allrop->mrc_op = rop & 0xC | rop >> 2;

		/* set function for 0 bits in color */
		if ((color = ~color) & planes) {
			fb->ppmask.reg = color;
			allrop->mrc_op = rop << 2 | rop & 3;
			fb->ppmask.reg = planes;
		}

		allrop->mrc_pattern = 0;
		allrop->mrc_mask1 = 0;
		allrop->mrc_mask2 = 0;
	}

	if (op & PIX_DONTCLIP)
		PR_LOOP(n, 
			*cg2_roppixaddr(fb, 
				ptlist->x + x, ptlist->y + y) = dummy;
			ptlist++);
	else {
		register int x_min, x_max, y_min, y_max;

		x_min = prd->cgpr_offset.x - x;
		x_max = pr->pr_size.x + x_min;
		y_min = prd->cgpr_offset.y - y;
		y_max = pr->pr_size.y + y_min;

		PR_LOOP(n,
			if (ptlist->x >= x_min && ptlist->x < x_max &&
				ptlist->y >= y_min && ptlist->y < y_max) {
				*cg2_roppixaddr(fb, 
					ptlist->x + x, ptlist->y + y) = dummy;
			}
			ptlist++);
	}

	return 0;
}

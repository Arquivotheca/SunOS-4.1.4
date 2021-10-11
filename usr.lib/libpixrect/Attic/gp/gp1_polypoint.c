#ifndef lint
static char sccsid[] = "@(#)gp1_polypoint.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

/*
 * Draw a list of points on the screen
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>
#include <pixrect/gp1var.h>

gp1_polypoint(pr, dx, dy, n, ptlist, op)
	Pixrect *pr;
	register dx, dy, n;
	register struct pr_pos *ptlist;
	int op;
{
	register color;
	int noclip;
	register struct gp1pr *prd = gp1_d(pr);
	register struct cg2fb *fb = prd->cgpr_va;

	if (--n < 0)
		return PIX_ERR;

	dx += prd->cgpr_offset.x;
	dy += prd->cgpr_offset.y;

	color = PIX_OPCOLOR(op) & 255;
	noclip = op & PIX_DONTCLIP;
	op = (op>>1) & 0xf;

	/* first sync on the graphics processor */
	if (gp1_sync(prd->gp_shmem, prd->ioctl_fd))
		return PIX_ERR;

	if (PIXOP_NEEDS_DST(op<<1))
		fb->status.reg.ropmode = SWWPIX;
	else	
		fb->status.reg.ropmode = SRWPIX;

	cg2_setfunction(fb, CG2_ALLROP, op);
	cg2_setpattern(fb, CG2_ALLROP, 0);
	cg2_setrsource(fb, CG2_ALLROP, color | color << 8);
	cg2_setshift(fb, CG2_ALLROP, 0, 1);
	cg2_setwidth(fb, CG2_ALLROP, 0, 0);
	fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 = 0;
	fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 = 0;
	fb->ppmask.reg = prd->cgpr_planes;

	if (noclip) {
		do {
			*cg2_roppixaddr(fb, 
				ptlist->x + dx, ptlist->y + dy) =
				(u_char) color;
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
				*cg2_roppixaddr(fb, 
					ptlist->x + dx, ptlist->y + dy) =
					(u_char) color;
			}
			ptlist++;
		} while (--n != -1);
	}

	return 0;
}

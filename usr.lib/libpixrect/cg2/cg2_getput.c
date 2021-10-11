#ifndef lint
static char sccsid[] = "@(#)cg2_getput.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1983, 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun1 Color get and put single pixel
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>
#include <pixrect/pr_impl_util.h>


cg2_get(pr, x, y)
	register Pixrect *pr;
	register int x, y;
{
	register struct cg2fb *fb;

 	if (x < 0 || x >= pr->pr_size.x || 
		y < 0 || y >= pr->pr_size.y)
		return PIX_ERR;

	{
		register struct cg2pr *prd = cg2_d(pr);

		x += prd->cgpr_offset.x;
		y += prd->cgpr_offset.y;
		fb = prd->cgpr_va;

		GP1_PRD_SYNC(prd, return PIX_ERR);

		if (prd->flags & CG2D_FASTREAD)
			fb->misc.nozoom.dblbuf.reg.fast_read = 0;
	}

	fb->ppmask.reg = ~0;
	fb->status.reg.ropmode = SRWPIX;

	return *cg2_roppixaddr(fb, x, y);
}

cg2_put(pr, x, y, value)
	register Pixrect *pr;
	register int x, y;
	int value;
{
	register struct cg2fb *fb;
	register int planes;

#define	dummy	IF68000(planes, 0)

 	if (x < 0 || x >= pr->pr_size.x || 
		y < 0 || y >= pr->pr_size.y)
		return PIX_ERR;

	{
		register struct cg2pr *prd = cg2_d(pr);

		x += prd->cgpr_offset.x;
		y += prd->cgpr_offset.y;
		fb = prd->cgpr_va;
		planes = prd->cgpr_planes;

		GP1_PRD_SYNC(prd, return PIX_ERR);
	}

	fb->status.reg.ropmode = SRWPIX;

	{
		register struct memropc *allrop = 
			&fb->ropcontrol[CG2_ALLROP].ropregs;

		fb->ppmask.reg = planes;
		allrop->mrc_op = 0;
		fb->ppmask.reg = value;
		allrop->mrc_op = ~0;
		fb->ppmask.reg = planes;

		allrop->mrc_mask1 = 0;
		allrop->mrc_mask2 = 0;
	}

	*cg2_roppixaddr(fb, x, y) = dummy;

	return 0;
}

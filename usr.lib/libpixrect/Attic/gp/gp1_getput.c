#ifndef lint
static char sccsid[] = "@(#)gp1_getput.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>
#include <pixrect/gp1var.h>

gp1_get(src)
	struct pr_prpos src;
{
	register u_char *by;
	struct cg2fb *fb = gp1_fbfrompr(src.pr);
	struct gp1pr *sbd = gp1_d(src.pr);

 	if (src.pos.x < 0 || src.pos.x >= src.pr->pr_size.x || 
	    src.pos.y < 0 || src.pos.y >= src.pr->pr_size.y)
		return (-1);

	/* first sync on the graphics processor */
	if(gp1_sync(sbd->gp_shmem, sbd->ioctl_fd))
		return(-1);

	fb->status.reg.ropmode = SRWPIX;
	by = cg2_roppixel(sbd, src.pos.x, src.pos.y);
	return ( (int)((sbd->cgpr_planes) & *by) );
}

gp1_put(dst, val)
	struct pr_prpos dst;
	int val;
{
	struct cg2fb *fb = gp1_fbfrompr(dst.pr);
	struct gp1pr *dbd = gp1_d(dst.pr);

 	if (dst.pos.x < 0 || dst.pos.x >= dst.pr->pr_size.x || 
	    dst.pos.y < 0 || dst.pos.y >= dst.pr->pr_size.y)
		return (-1);

	/* first sync on the graphics processor */
	if(gp1_sync(dbd->gp_shmem, dbd->ioctl_fd))
		return(-1);

	fb->ppmask.reg = dbd->cgpr_planes;
	cg2_setfunction(fb, CG2_ALLROP, CG_SRC);
	fb->status.reg.ropmode = SRWPIX;
	fb->ropcontrol[CG2_ALLROP].prime.ropregs.mrc_source1 =
		val | (val<<8);		/* load color in src */
	cg2_setshift( fb, CG2_ALLROP, 0, 1);
	cg2_setwidth(fb, CG2_ALLROP, 0, 0);
	fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 = 0;
	fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 = 0;
	*cg2_roppixel(dbd, dst.pos.x, dst.pos.y) = (u_char)val;
	return (0);
}

#ifndef lint
static char sccsid[] = "@(#)gp1_region.c 1.1 94/10/31 Copyr 1985 Sun Micro";
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

struct pixrect *
gp1_region(src)
	struct pr_subregion src;
{
	register struct pixrect *pr;
	register struct gp1pr *sgppr = gp1_d(src.pr), *gppr;
	int zero = 0;

	pr_clip(&src, &zero);
	if ((pr = alloctype(struct pixrect)) == 0)
		return (0);
	if ((gppr = alloctype(struct gp1pr)) == 0) {
		free(pr);
		return (0);
	}
	pr->pr_ops = &gp1_ops;
	pr->pr_size = src.size;
	pr->pr_depth = CG2_DEPTH;
	pr->pr_data = (caddr_t)gppr;
	gppr->cgpr_fd = -1;	/* -> this pr does not own fb */
	gppr->cgpr_va = sgppr->cgpr_va;
	gppr->gp_shmem = sgppr->gp_shmem;
	gppr->cgpr_planes = sgppr->cgpr_planes;
	gppr->cgpr_offset.x = sgppr->cgpr_offset.x + src.pos.x;
	gppr->cgpr_offset.y = sgppr->cgpr_offset.y + src.pos.y;
	gppr->cg2_index = sgppr->cg2_index;
	gppr->minordev = sgppr->minordev;
	gppr->gbufflag = sgppr->gbufflag;
	gppr->ioctl_fd = sgppr->ioctl_fd;
	gppr->ncmd = sgppr->ncmd;
	gppr->cmdver = sgppr->cmdver;
	return (pr);
}

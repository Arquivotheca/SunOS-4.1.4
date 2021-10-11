#ifndef lint
static	char sccsid[] = "@(#)cg1_region.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Sun1 Color region-to-pixrect conversion
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/cg1reg.h>
#include <pixrect/cg1var.h>

struct pixrect *
cg1_region(src)
	struct pr_subregion src;
{
	register struct pixrect *pr;
	register struct cg1pr *scgpr = cg1_d(src.pr), *cgpr;
	int zero = 0;

	pr_clip(&src, &zero);
	if ((pr = alloctype(struct pixrect)) == 0)
		return (0);
	if ((cgpr = alloctype(struct cg1pr)) == 0) {
		free(pr);
		return (0);
	}
	pr->pr_ops = &cg1_ops;
	pr->pr_size = src.size;
	pr->pr_depth = CG1_DEPTH;
	pr->pr_data = (caddr_t)cgpr;
	cgpr->cgpr_fd = -1;	/* -> this pr does not own fb */
	cgpr->cgpr_va = scgpr->cgpr_va;
	cgpr->cgpr_planes = scgpr->cgpr_planes;
	cgpr->cgpr_offset.x = scgpr->cgpr_offset.x + src.pos.x;
	cgpr->cgpr_offset.y = scgpr->cgpr_offset.y + src.pos.y;
	return (pr);
}

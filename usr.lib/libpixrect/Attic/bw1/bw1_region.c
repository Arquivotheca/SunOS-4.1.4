#ifndef lint
static	char sccsid[] = "@(#)bw1_region.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Sun1 Black-and-White region-to-pixrect conversion
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/bw1reg.h>
#include <pixrect/bw1var.h>

struct pixrect *
bw1_region(src)
	struct pr_subregion src;
{
	register struct pixrect *pr;
	register struct bw1pr *sbwpr = bw1_d(src.pr), *bwpr;
	int zero = 0;

	pr_clip(&src, &zero);
	if ((pr = alloctype(struct pixrect)) == 0)
		return (0);
	if ((bwpr = alloctype(struct bw1pr)) == 0) {
		free(pr);
		return (0);
	}
	pr->pr_ops = &bw1_ops;
	pr->pr_size = src.size;
	pr->pr_depth = 1;
	pr->pr_data = (caddr_t)bwpr;
	bwpr->bwpr_fd = -1;	/* -> this pr does not own fb */
	bwpr->bwpr_va = sbwpr->bwpr_va;
	bwpr->bwpr_flags = sbwpr->bwpr_flags;
	bwpr->bwpr_offset.x = sbwpr->bwpr_offset.x + src.pos.x;
	bwpr->bwpr_offset.y = sbwpr->bwpr_offset.y + src.pos.y;
	return (pr);
}

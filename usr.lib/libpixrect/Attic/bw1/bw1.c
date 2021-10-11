#ifndef lint
static char sccsid[] = "@(#)bw1.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1983, 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun1 Black-and-White pixrect create and destroy
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_make.h>
#include <pixrect/bw1reg.h>
#include <pixrect/bw1var.h>

static struct pr_devdata *bw1devdata;

struct pixrect *
bw1_make(fd, size, depth)
	int	fd;
	struct	pr_size size;
	int	depth;
{
	register Pixrect *pr = 0;
	register struct bw1pr *bwpr;
	struct	pr_devdata *dd;

	/*
	 * Allocate/initialize pixrect and get virtual address for it.
	 */
	if (!(pr = pr_makefromfd(fd, size, depth, &bw1devdata, &dd,
		sizeof (struct bw1fb), sizeof (struct bw1pr), 0)))
		return pr;

	/* initialize pixrect data */
	pr->pr_ops = &bw1_ops;

	bwpr = bw1_d(pr);
	bwpr->bwpr_fd = dd->fd;
	bwpr->bwpr_va = (struct bw1fb *)dd->va;
	bwpr->bwpr_flags = BW_REVERSEVIDEO;
	bwpr->bwpr_offset.x = 0;
	bwpr->bwpr_offset.y = 0;

	return pr;
}

bw1_destroy(pr)
	Pixrect *pr;
{

	if (pr) {
		register struct bw1pr *bwpr;

		if (bwpr = bw1_d(pr)) {
			if (bwpr->bwpr_fd != -1) 
				(void) pr_unmakefromfd(bwpr->bwpr_fd, 
					&bw1devdata);
			free((char *) bwpr);
		}
		free((char *) pr);
	}

	return 0;
}

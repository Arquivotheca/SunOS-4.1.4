#ifndef lint
static char sccsid[] = "@(#)cg1.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1983, 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun1 color pixrect create and destroy
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_make.h>
#include <pixrect/cg1reg.h>
#include <pixrect/cg1var.h>

static	struct pr_devdata *cg1devdata;

struct pixrect *
cg1_make(fd, size, depth)
	int	fd;
	struct	pr_size size;
	int	depth;
{
	struct pixrect *pr;
	register struct cg1pr *cgpr;
	struct	pr_devdata *dd;

	/*
	 * Allocate/initialize pixrect and get virtual address for it.
	 */
	if (!(pr = pr_makefromfd(fd, size, depth, &cg1devdata, &dd,
	    sizeof(struct cg1fb), sizeof(struct cg1pr), 0)))
		return (0);
	pr->pr_ops = &cg1_ops;
	/*
	 * Initialize pixrect private data.
	 */
	cgpr = (struct cg1pr *)pr->pr_data;
	cgpr->cgpr_fd = dd->fd;
	cgpr->cgpr_va = (struct cg1fb *)dd->va;
	cgpr->cgpr_planes = 255;
	cgpr->cgpr_offset.x = cgpr->cgpr_offset.y = 0;
	cg1_setreg(cgpr->cgpr_va, CG_STATUS, CG_VIDEOENABLE);
	return (pr);
}

cg1_destroy(pr)
	struct pixrect *pr;
{
	register struct cg1pr *cgpr;

	if (pr == 0)
		return (0);
	if (cgpr = cg1_d(pr)) {
		if (cgpr->cgpr_fd != -1) {
			/*
			 * Don't free this part if region
			 */
			pr_unmakefromfd(cgpr->cgpr_fd, &cg1devdata);
		}
		free(cgpr);
	}
	free(pr);
	return (0);
}

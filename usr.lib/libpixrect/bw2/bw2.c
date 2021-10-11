#ifndef lint
static	char sccsid[] = "@(#)bw2.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1987, 1989 Sun Microsystems, Inc.
 */

/*
 * Monochrome frame buffer pixrect create and destroy
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_make.h>
#include <pixrect/memvar.h>
#include <pixrect/bw2var.h>

static	struct pr_devdata *bw2devdata;

Pixrect *
bw2_make(fd, size, depth)
	int	fd;
	struct	pr_size size;
	int	depth;
{
	register int w = size.x, h = size.y;
	register Pixrect *pr;
	struct	pr_devdata *dd;
	register int linebytes;

	/*
	 * Allocate/initialize pixrect and get virtual address for it.
	 */
	linebytes = mpr_linebytes(w, depth);
	if ((pr = pr_makefromfd(fd, size, depth, &bw2devdata, &dd, 
		h * linebytes, sizeof(struct mpr_data), 0)) != 0) {
			
		register struct mpr_data *md;
		
		pr->pr_ops = &bw2_ops;

		md = (struct mpr_data *) pr->pr_data;
		md->md_linebytes = linebytes;
		md->md_image = (short *) dd->va;
		md->md_offset.x = 0;
		md->md_offset.y = 0;
		md->md_primary = -1 - dd->fd;
		md->md_flags = MP_DISPLAY;	/* pr is display dev */
	}
	return pr;
}


bw2_destroy(pr)
	Pixrect *pr;
{
	if (pr != 0) {
		register struct mpr_data *md;

		if ((md = mpr_d(pr)) != 0) {
			if (md->md_primary) {
				(void) pr_unmakefromfd(-1 - md->md_primary, 
					&bw2devdata);

				/*
 				 * bw2_make (really pr_makefromfd) allocates
 				 * mpr_data separately, mem_region does not.
				 */
				free((char *) md);
			}
		}
		free((char *) pr);
	}
	return 0;
}

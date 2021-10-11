#if !defined(lint) && !defined(NOID)
static char sccsid[] = "@(#)cg3_region.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1989, Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/cg3var.h>

extern char *malloc();

Pixrect *
cg3_region(pr, x, y, w, h)
	register Pixrect *pr;
	int x, y, w, h;
{
	register Pixrect *newpr;
	register struct cg3_data *cgd, *newcgd;
	struct pr_subregion srcreg;
	static struct pr_prpos nulpos = {
		0, { 0, 0 }
	};

	/* allocate new pixrect and pixrect data */
	if ((newpr = (Pixrect *) malloc(sizeof (Pixrect))) == 0)
		return 0;

	if (!(newcgd = (struct cg3_data *) malloc(sizeof (struct cg3_data)))) {
		free((char *) newpr);
		return 0;
	}

	/* compute region offset, size */
	srcreg.pr = pr;
	srcreg.pos.x = x;
	srcreg.pos.y = y;
	srcreg.size.x = w;
	srcreg.size.y = h;

	pr_clip(&srcreg, &nulpos);

	/* set up new pixrect, data */
	newpr->pr_ops = pr->pr_ops;
	newpr->pr_size = srcreg.size;
	newpr->pr_depth = pr->pr_depth;
	newpr->pr_data = (caddr_t) newcgd;
	cgd = cg3_d(pr);
	newcgd->mprp.mpr.md_linebytes = cgd->mprp.mpr.md_linebytes;
	newcgd->mprp.mpr.md_image = cgd->mprp.mpr.md_image;
	newcgd->mprp.mpr.md_offset.x =
		cgd->mprp.mpr.md_offset.x + srcreg.pos.x;
	newcgd->mprp.mpr.md_offset.y =
		cgd->mprp.mpr.md_offset.y + srcreg.pos.y;
	newcgd->mprp.mpr.md_primary = 0;
	newcgd->mprp.mpr.md_flags = cgd->mprp.mpr.md_flags;
	newcgd->mprp.planes = cgd->mprp.planes;
	newcgd->fd = cgd->fd;

	return newpr;
}

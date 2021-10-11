#ifndef lint
static	char sccsid[] = "@(#)cg4_region.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

/*
 * cg4 region creation
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/cg4var.h>

Pixrect *
cg4_region(pr, x, y, w, h)
	Pixrect	*pr;
	int x, y, w, h;
{
	struct pr_subregion srcreg;
	static struct pr_prpos nulpos = {
		0, { 0, 0 }
	};
	register struct cg4fb *fbp;
	register Pixrect *newpr;
	register struct cg4_data *cgd;

	if ((newpr = alloctype(Pixrect)) == 0)
		return 0;

	if ((cgd = alloctype(struct cg4_data)) == 0) {
		free(newpr);
		return 0;
	}

	srcreg.pr = pr;
	srcreg.pos.x = x;
	srcreg.pos.y = y;
	srcreg.size.x = w;
	srcreg.size.y = h;

	pr_clip(&srcreg, &nulpos);

	/* copy old pixrect; set size, data pointer */
	*newpr = *pr;
	newpr->pr_size = srcreg.size;
	newpr->pr_data = (caddr_t) cgd;

	/* copy old pixrect data */
	*cgd = *cg4_d(pr);

	/* this is a secondary pixrect */
	cgd->flags &= ~(CG4_PRIMARY);

	/* fix up all the offsets (ugh) */
	cgd->mprp.mpr.md_offset.x += srcreg.pos.x;
	cgd->mprp.mpr.md_offset.y += srcreg.pos.y;
	for (fbp = cgd->fb; fbp < &cgd->fb[CG4_NFBS]; fbp++)
		fbp->mprp.mpr.md_offset = cgd->mprp.mpr.md_offset;

	return newpr;
}

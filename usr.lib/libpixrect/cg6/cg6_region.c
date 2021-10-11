#ifndef lint
static	char sccsid[] = "@(#)cg6_region.c	1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1984, 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun2 Color region-to-pixrect conversion
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memreg.h>
#include <pixrect/cg6var.h>

extern char *malloc();

Pixrect *
cg6_region(pr, x, y, w, h)
	Pixrect *pr;
	int x, y, w, h;
{
	register Pixrect *newpr;
	struct pr_subregion srcreg;
	static struct pr_prpos nulpos = {
		0, { 0, 0 }
	};
	struct cg6pr *cg6pr;

	/* allocate cg6 pixrect */
	newpr = alloctype(Pixrect);
	if (newpr == 0) 
		return (0);

	cg6pr = alloctype(struct cg6pr);
	if (cg6pr == 0) {
		free(newpr);
		return (0);
	}

	/* compute region offset, size */
	srcreg.pr = pr;
	srcreg.pos.x = x;
	srcreg.pos.y = y;
	srcreg.size.x = w;
	srcreg.size.y = h;

	pr_clip(&srcreg, &nulpos);

	*newpr = *pr;
	newpr->pr_size = srcreg.size;
	newpr->pr_data = (char*) cg6pr;

	*cg6pr = *cg6_d(pr);
	cg6pr->mprp.mpr.md_offset.x += srcreg.pos.x;
	cg6pr->mprp.mpr.md_offset.y += srcreg.pos.y;
	cg6pr->mprp.mpr.md_primary = 0;
	
	return newpr;
}

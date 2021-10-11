#ifndef lint
static	char sccsid[] = "@(#)cg2_region.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1984, 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun2 Color region-to-pixrect conversion
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>

extern char *malloc();

Pixrect *
cg2_region(pr, x, y, w, h)
	Pixrect *pr;
	int x, y, w, h;
{
	struct pr_subregion srcreg;
	static struct pr_prpos nulpos = {
		0, { 0, 0 }
	};
	register Pixrect *newpr;
	register struct cg2pr *newprd;

	/* allocate new pixrect and pixrect data */
	if (!(newpr = (Pixrect *) malloc(sizeof (Pixrect))))
		return 0;

	if (!(newprd = (struct cg2pr *) malloc(sizeof (struct cg2pr)))) {
		free ((char *) newpr);
		return 0;
	}

	/* compute region offset, size */
	srcreg.pr = pr;
	srcreg.pos.x = x;
	srcreg.pos.y = y;
	srcreg.size.x = w;
	srcreg.size.y = h;

	pr_clip(&srcreg, &nulpos);

	/* copy old pixrect; set size, data pointer */
	*newpr = *pr;
	newpr->pr_size = srcreg.size;
	newpr->pr_data = (caddr_t) newprd;

	/* copy old pixrect data */
	*newprd = *cg2_d(pr);

	/* this is a secondary pixrect */
	newprd->cgpr_fd = -1;

	/* set offsets */
	newprd->cgpr_offset.x += srcreg.pos.x;
	newprd->cgpr_offset.y += srcreg.pos.y;

	return newpr;
}

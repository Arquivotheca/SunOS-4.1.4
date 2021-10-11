#ifndef lint
static	char sccsid[] = "@(#)mem_region.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1989 Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>

extern char *malloc();

Pixrect *
mem_region(pr, x, y, w, h)
	register Pixrect *pr;
	int x, y, w, h;
{
	struct pr_subregion srcreg;
	static struct pr_prpos nulpos = {
		0, { 0, 0 }
	};
	register Pixrect *npr;
	register struct mprp_data *md, *nmd;
	register int flags;

	md = mprp_d(pr);
	flags = md->mpr.md_flags;

	/* allocate new pixrect and pixrect data */
	flags = md->mpr.md_flags;
	if (!(npr = (Pixrect *) malloc((unsigned)
		(sizeof (Pixrect) + sizeof (struct mpr_data) +
		(flags & MP_PLANEMASK ? sizeof (int) : 0)))))
		return npr;

	/* compute region offset, size */
	srcreg.pr = pr;
	srcreg.pos.x = x;
	srcreg.pos.y = y;
	srcreg.size.x = w;
	srcreg.size.y = h;

	pr_clip(&srcreg, &nulpos);

	npr->pr_ops = pr->pr_ops;
	npr->pr_size = srcreg.size;
	npr->pr_depth = pr->pr_depth;
	npr->pr_data = (caddr_t) (nmd =
		(struct mprp_data *) ((caddr_t) npr + sizeof (Pixrect)));

	nmd->mpr.md_linebytes = md->mpr.md_linebytes;
	nmd->mpr.md_image = md->mpr.md_image;
	nmd->mpr.md_offset.x = md->mpr.md_offset.x + srcreg.pos.x;
	nmd->mpr.md_offset.y = md->mpr.md_offset.y + srcreg.pos.y;
	nmd->mpr.md_primary = 0;
	nmd->mpr.md_flags = flags;

	if (flags & MP_PLANEMASK)
		nmd->planes = md->planes;

	return npr;
}

#ifndef lint
static	char sccsid[] = "@(#)mem.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * Memory pixrect creation/destruction
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>

extern char *malloc();

/*
 * Create a memory pixrect that points to an existing (non-pixrect) image.
 */
Pixrect *
mem_point(w, h, depth, image)
	int w, h, depth;
	short *image;
{
	register Pixrect *pr;
	register struct mpr_data *md;

	if (!(pr = (Pixrect *) 
		malloc(sizeof (Pixrect) + sizeof(struct mpr_data))))
		return pr;

	md = (struct mpr_data *) ((caddr_t) pr + sizeof (Pixrect));

	pr->pr_ops = &mem_ops;
	pr->pr_size.x = w;
	pr->pr_size.y = h;
	pr->pr_depth = depth;
	pr->pr_data = (caddr_t) md;

	md->md_linebytes = mpr_linebytes(w, depth);
	md->md_offset.x = 0;
	md->md_offset.y = 0;
	md->md_primary = 0;
	md->md_flags = 0;
	md->md_image = image;

	return pr;
}

/*
 * Create a memory pixrect, allocate space, and clear it.
 */
Pixrect *
mem_create(w, h, depth)
	int w, h, depth;
{
	register Pixrect *pr;
	register struct mpr_data *md;

	if ((pr = mem_point(w, h, depth, (short *) 0)) == 0)
		return pr;

	md = mpr_d(pr);

	/*
	 * If compiled for a 32-bit machine, pad linebytes to
	 * a multiple of 4 bytes.
	 */
#ifndef mc68010
	if (md->md_linebytes & 2 && md->md_linebytes > 2)
		md->md_linebytes += 2;
#endif mc68010

	if (!(md->md_image =
		(short *) malloc((unsigned) (h *=md->md_linebytes)))) {
		(void) pr_destroy(pr);
		return 0;
	}

	bzero((char *) md->md_image, h);
	md->md_primary = 1;

	return pr;
}

/*
 * Destroy a memory pixrect
 */
mem_destroy(pr)
	Pixrect *pr;
{
	if (pr) {
		register struct mpr_data *md;

		if ((md = mpr_d(pr)) &&
			md->md_primary && md->md_image)
				free((char *) md->md_image);

		free((char *) pr);
	}

	return 0;
}

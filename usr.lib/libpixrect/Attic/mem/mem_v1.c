#ifndef lint
static	char sccsid[] = "@(#)mem_v1.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Memory pixrect creation
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <pixrect/pixrect.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_util.h>

struct	pixrectops mem_ops = {
	mem_rop,
	mem_stencil,
	mem_batchrop,
	0,
	mem_destroy,
	mem_get,
	mem_put,
	mem_vector,
	mem_region,
	mem_putcolormap,
	mem_getcolormap,
	mem_putattributes,
	mem_getattributes,
};

struct	memropc *mem_ropc;		/* Pointer to ROP chip, or zero */

/*
 * Create a memory pixrect that points to an existing (non-pixrect) image.
 */
struct pixrect *
mem_point(size, depth, data)
	struct pr_size size;
	int depth;				/* XXX */
	short *data;
{
	register struct pixrect *pr;
	register struct mpr_data *md;

	pr = alloctype(struct pixrect);
	if (pr == 0)
		return (0);
	md = alloctype(struct mpr_data);
	if (md == 0) {
		free(pr);
		return (0);
	}
	pr->pr_ops = &mem_ops;
	pr->pr_size = size;
	pr->pr_depth = depth;
	pr->pr_data = (caddr_t)md;
	md->md_linebytes = mpr_linebytes(size.x, depth);
	md->md_offset.x = 0;
	md->md_offset.y = 0;
	md->md_primary = 0;
	md->md_flags = 0;
	md->md_image = data;
	return (pr);
}

/*
 * Create a memory pixrect, allocate space, and clear it.
 */
struct pixrect *
mem_create(size, depth)
	struct pr_size size;
	int depth;				/* XXX */
{
	struct pixrect *pr;
	register struct mpr_data *md;

	pr = mem_point(size, depth, (short *)0);
	if (pr == 0)
		return 0;
	md = mpr_d(pr);
	md->md_image = (short *)calloc(1,pr_product(md->md_linebytes, size.y));
	if (md->md_image == 0) {
		/* No room at the inn */
		mem_destroy(pr);
		return (0);
	}
	md->md_primary = 1;		/* cfree when mem_destroy */
	return (pr);
}

mem_destroy(pr)
	struct pixrect *pr;
{
	register struct mpr_data *md;

	if (pr == 0)
		return;
	md = mpr_d(pr);
	if (md) {
		 if (md->md_primary && md->md_image)
			free(md->md_image);
		free(md);
	}
	free(pr);
}

int	mem_needinit = 1;

mem_init()
{
	int mf;
	struct memropc *mrc;

	mem_needinit = 0;
	mf = open("/dev/ropc", 2);
	if (mf < 0)
		return;
	mrc = (struct memropc *)valloc(sizeof (*mrc));
	if (mrc == 0 ||
	    mmap(mrc, sizeof (*mrc), PROT_READ|PROT_WRITE,
	      MAP_SHARED, mf, 0) < 0) {
		close(mf);
		return;
	}
	mem_ropc = mrc;
}

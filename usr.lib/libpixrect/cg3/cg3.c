#if !defined(lint) && !defined(NOID)
static char sccsid[] = "@(#)cg3.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1989, Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_make.h>
#include <pixrect/memvar.h>
#include <pixrect/cg3var.h>
#include <sun/fbio.h>

/* device data list head */
static struct pr_devdata *cg3devdata;

Pixrect *
cg3_make(fd, size, depth)
	int fd;
	struct pr_size size;
	int depth;
{
	Pixrect *pr;
	struct pr_devdata *dd;
	int linebytes = mpr_linebytes(size.x, depth);
	register struct cg3_data *cgd;

	if (pr = pr_makefromfd(fd, size, depth, &cg3devdata, &dd,
		size.y * linebytes,
		sizeof (struct cg3_data),
		CG3_MMAP_OFFSET)) {
		pr->pr_ops = &cg3_ops;
		cgd = cg3_d(pr);
		cgd->mprp.mpr.md_linebytes = linebytes;
		cgd->mprp.mpr.md_image = (short *) dd->va;
		cgd->mprp.mpr.md_offset.x = 0;
		cgd->mprp.mpr.md_offset.y = 0;
		cgd->mprp.mpr.md_primary = 1;
		cgd->mprp.mpr.md_flags = MP_DISPLAY | MP_PLANEMASK;
		cgd->mprp.planes = (1 << depth) - 1;
		cgd->fd = dd->fd;
	}
	return pr;
}

cg3_destroy(pr)
	Pixrect *pr;
{
	register struct cg3_data *cgd;
	register int err = 0;

	if (pr) {
		if (cgd = cg3_d(pr)) {
			if (cgd->mprp.mpr.md_primary)
				err = pr_unmakefromfd(cgd->fd, &cg3devdata);
			free((char *) cgd);
		}
		free ((char *) pr);
	}

	return err;
}

#ifndef lint
static char sccsid[] = "@(#)cg2.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1983, 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun2 color pixrect create and destroy
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_make.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>
#include <sun/fbio.h>

/* device data list head */
struct pr_devdata *_cg2devdata;

Pixrect *
cg2_make(fd, size, depth, attr)
	int fd;
	struct pr_size size;
	int depth;
	struct fbgattr *attr;
{
	register Pixrect *pr;
	struct pr_devdata *dd;
	int pagesize = getpagesize();

	/*
	 * Allocate/initialize pixrect and get virtual address for it.
	 */
	if (!(pr = pr_makefromfd(fd, size, depth, &_cg2devdata, &dd,
		CG2_MAPPED_SIZE + pagesize,
		sizeof(struct cg2pr),
		CG2_MAPPED_OFFSET - pagesize)))
		return pr;

	/* initialize pixrect data */
	pr->pr_ops = &cg2_ops;
	_cg2_prd_init(cg2_d(pr), dd->fd, dd->va + pagesize, attr);

	return pr;
}

_cg2_prd_init(prd, fd, addr, attr)
	register struct cg2pr *prd;
	int fd;
	caddr_t addr;
	register struct fbgattr *attr;
{
	register struct cg2fb *fb;

	prd->ioctl_fd = prd->cgpr_fd = fd;
	prd->cgpr_va = fb = (struct cg2fb *) addr;
	prd->cgpr_planes = 255;
	prd->cgpr_offset.x = 0;
	prd->cgpr_offset.y = 0;

	if (attr->fbtype.fb_width == CG2_WIDTH) 
		prd->flags = CG2D_STDRES;

	prd->linebytes = attr->fbtype.fb_width;
			
	/* get flags if valid */
	if (attr->real_type >= 0 &&
		attr->sattr.flags & FB_ATTR_DEVSPECIFIC)
		if (attr->sattr.dev_specific[FB_ATTR_CG2_FLAGS] & 
			FB_ATTR_CG2_FLAGS_PRFLAGS)
			prd->flags = 
				attr->sattr.dev_specific[FB_ATTR_CG2_PRFLAGS];
		/* double buffer detection for 3.5 driver */
		else if (attr->sattr.dev_specific[FB_ATTR_CG2_FLAGS] & 
			FB_ATTR_CG2_FLAGS_SUN3) {
			prd->flags |= CG2D_NOZOOM;
			if (attr->sattr.dev_specific[FB_ATTR_CG2_BUFFERS] > 1)
				prd->flags |= CG2D_DBLBUF;
		}

	/*
	 * Initialize zoom/pan registers for no pan or zoom.  This is
	 * a little questionable because some other program may be
	 * doing something with them, but this is an easy way to
	 * recover from some screwup.
	 */
	if (prd->flags & CG2D_ZOOM) {
		fb->misc.zoom.wordpan.reg = 0;	/* hi pixel adr = 0 */
		fb->misc.zoom.zoom.word = 0;	/* zoom=0, yoff=0 */
		fb->misc.zoom.pixpan.word = 0;	/* pix adr=0, xoff=0 */
		fb->misc.zoom.varzoom.reg = 255; /* unzoom at line 4*255 */
	}
}

cg2_destroy(pr)
	Pixrect *pr;
{
	if (pr) {
		register struct cg2pr *cgpr;

		if (cgpr = cg2_d(pr)) {
			if (cgpr->cgpr_fd != -1) 
				(void) pr_unmakefromfd(cgpr->cgpr_fd, 
					&_cg2devdata);
			free((char *) cgpr);
		}
		free((char *) pr);
	}

	return 0;
}

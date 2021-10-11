#ifndef lint
static char	sccsid[] =	"@(#)cg9.c	1.1 94/10/31 SMI";
#endif

/* Copyright (c) 1988, 1989 by Sun Microsystems, Inc. */

/* CG-9 Frame Buffer pixrect create and destroy */

#include	<sys/types.h>
#include	<stdio.h>
#include	<sys/ioctl.h>
#include	<sun/fbio.h>
#include	<pixrect/pixrect.h>
#include	<pixrect/pr_planegroups.h>
#include	<pixrect/pr_impl_make.h>
#include	<pixrect/cg9var.h>

	struct	pr_size		cg9size;
static	struct	pr_devdata	*cg9devdata;

static struct fbdesc {
	short           group;
	short           depth;
	int             allplanes;
}               fbdesc[CG9_NFBS] = {
	{ PIXPG_OVERLAY, 1, 0 },
	{ PIXPG_OVERLAY_ENABLE, 1, 0 },
	{ PIXPG_24BIT_COLOR, 32, 0x00ffffff }  };

Pixrect        *
cg9_make (fd, size, depth, attr)
    int             fd;
    struct pr_size  size;
    int             depth;
    struct fbgattr *attr;
{
    register Pixrect	*pr;
    struct cg9_data	*cg9d;
    struct pr_devdata	*dd;

    /*
     * Allocate/initialize "cg9" pixrect and get virtual address for it
     */
    cg9size = size;

    if ((pr = pr_makefromfd_2(fd, size, depth, &cg9devdata, &dd,
	sizeof (struct cg9_data), attr->fbtype.fb_size, 0, 0, 0)) != 0) {

	pr->pr_ops = &cg9_ops;
	cg9d = cg9_d (pr);
	_cg9_prd_init(cg9d, dd->fd, dd->va, attr);

	{
	    /*
	     * default is the monochrome overlay plane.  Note that neither
	     * overlay nor enable planes are initialized.
	     */
	    int             initplane;
	    int             video_on = FBVIDEO_ON;

	    initplane = PIX_GROUP (fbdesc[0].group) | fbdesc[0].allplanes;
	    (void) cg9_putattributes (pr, &initplane);
	    (void) ioctl (cg9d->fd, FBIOSVIDEO, &video_on);	/* turn it on */
	    cg9d->flags &= ~CG9_OVERLAY_CMAP;
	}
    }
    return pr;
}

/* ARGSUSED */
_cg9_prd_init (cg9d, fd, va, attr)
    struct cg9_data *cg9d;
    int             fd;
    caddr_t         va;
    struct fbgattr *attr;
{
    register int    i;
    int             pagesize = getpagesize ();

    cg9d->flags = CG9_PRIMARY | CG9_OVERLAY_CMAP;
    cg9d->fd = fd;
    cg9d->windowfd = cg9d->real_windowfd = -2;
    cg9d->cms.cms_size = cg9d->cms.cms_addr = 0;

    /*
     * Initialize all plane groups, we have three here.  All of them are
     * described by the "fbdesc" table.
     */
    for (i = 0; i < CG9_NFBS; i++) {
	cg9d->fb[i].group = fbdesc[i].group;
	cg9d->fb[i].depth = fbdesc[i].depth;
	cg9d->fb[i].mprp.mpr.md_linebytes =
	    mpr_linebytes (cg9size.x, fbdesc[i].depth);
	cg9d->fb[i].mprp.mpr.md_offset.x = 0;
	cg9d->fb[i].mprp.mpr.md_offset.y = 0;
	cg9d->fb[i].mprp.mpr.md_primary = 0;

	/*
	 * if it is more than 1 bit deep, it is possible to plane mask it
	 */
	cg9d->fb[i].mprp.mpr.md_flags = fbdesc[i].allplanes != 0
	    ? MP_DISPLAY | MP_PLANEMASK : MP_DISPLAY;
	cg9d->fb[i].mprp.planes = fbdesc[i].allplanes;
    }
    {
	int             ovlsize;
	u_char         *im_ptr = (u_char *) va;

	ovlsize = mpr_linebytes (cg9size.x, 1) * cg9size.y;
	ovlsize = ROUNDUP (ovlsize, pagesize);
	cg9d->fb[0].mprp.mpr.md_image = (short *) im_ptr;
	cg9d->fb[1].mprp.mpr.md_image = (short *) (im_ptr += ovlsize);
	cg9d->fb[2].mprp.mpr.md_image = (short *) (im_ptr += ovlsize);
	cg9d->ctrl_sp = (struct cg9_control_sp *)
	    (im_ptr+ROUNDUP(cg9d->fb[2].mprp.mpr.md_linebytes*cg9size.y,
	    pagesize));
    }
}

cg9_destroy (pr)		       /* ditto cg4_destroy */
    Pixrect        *pr;
{
    if (pr) {
	struct cg9_data *cg9d;

	if (cg9d = cg9_d (pr)) {
	    if (cg9d->flags & CG9_PRIMARY)
		pr_unmakefromfd (cg9d->fd, &cg9devdata);
	    free (cg9d);
	}
	free (pr);
    }
    return 0;
}

#ifndef lint
static	char sccsid[] = "@(#)cg4.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * Color memory frame buffer (cg4) create and destroy
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_make.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/memvar.h>
#include <pixrect/cg4var.h>

/* initial active frame buffer */
#ifndef	CG4_INITFB
#define	CG4_INITFB	2
#endif

/* device data list head */
static struct pr_devdata *cg4devdata;

/* pixrect create function */
Pixrect *
cg4_make(fd, size, depth)
	int fd;
	struct pr_size size;
	int depth;
{
	register int w, h;
	register Pixrect *pr;
	register struct cg4_data *cgd;
	struct pr_devdata *curdd;
	int pagesize, mmapbytes;
	caddr_t fb_addr;
	register struct cg4fb *fbp;

	static struct fbdesc {
		short	group;
		short	depth;
		int	allplanes;
	} fbdesc[CG4_NFBS] = {
		{ PIXPG_OVERLAY,	1,   0 },
		{ PIXPG_OVERLAY_ENABLE,	1,   0 },
		{ PIXPG_8BIT_COLOR,	8, 255 }
	};
	register struct fbdesc *descp;

	/* 
	 * compute total space to be mmaped 
	 */
	w = size.x;
	h = size.y;
	pagesize = getpagesize();
	mmapbytes = 0;
	for (descp = fbdesc; descp < &fbdesc[CG4_NFBS]; descp++)
		mmapbytes += 
			ROUNDUP(mpr_linebytes(w, descp->depth) * h, pagesize);

	/* allocate pixrect and map frame buffer */
	if ((pr = pr_makefromfd(fd, size, depth, &cg4devdata, &curdd,
		mmapbytes, sizeof(struct cg4_data), 0)) == 0)
		return 0;

	/* initialize pixrect data */
	pr->pr_ops = &cg4_ops;

	cgd = cg4_d(pr);
	cgd->flags = CG4_PRIMARY | CG4_OVERLAY_CMAP;
	cgd->planes = 0;
	cgd->fd = curdd->fd;

	fb_addr = (caddr_t) curdd->va;

	for (fbp = cgd->fb, descp = fbdesc; descp < &fbdesc[CG4_NFBS];
		fbp++, descp++) {
		fbp->group = descp->group;
		fbp->depth = descp->depth;
		fbp->mprp.mpr.md_linebytes = mpr_linebytes(w, descp->depth);
		fbp->mprp.mpr.md_image = (short *) fb_addr;
		fbp->mprp.mpr.md_offset.x = 0;
		fbp->mprp.mpr.md_offset.y = 0;
		fbp->mprp.mpr.md_primary = 0;
		fbp->mprp.mpr.md_flags = descp->allplanes != 0
			? MP_DISPLAY | MP_PLANEMASK
			: MP_DISPLAY;
		fbp->mprp.planes = descp->allplanes;

		fb_addr += ROUNDUP(fbp->mprp.mpr.md_linebytes * h, pagesize);
	}

	/* see if the frame buffer really has an overlay plane color map */
	{
		static int overlayplanes = PIX_GROUP(PIXPG_OVERLAY);
		u_char rgb[1];

		(void) cg4_putattributes(pr, &overlayplanes);
		if (cg4_getcolormap(pr, overlayplanes, 1, rgb, rgb, rgb) != 0)
			cgd->flags &= ~(CG4_OVERLAY_CMAP);
	}

	/* set up pixrect initial state */
	{
		int initplanes = 
			PIX_GROUP(fbdesc[CG4_INITFB].group) |
			fbdesc[CG4_INITFB].allplanes;

		(void) cg4_putattributes(pr, &initplanes);
	}

	return pr;
}


cg4_destroy(pr)
	Pixrect *pr;
{
	if (pr != 0) {
		register struct cg4_data *cgd;

		if ((cgd = cg4_d(pr)) != 0) {
			if (cgd->flags & CG4_PRIMARY)
				pr_unmakefromfd(cgd->fd, &cg4devdata);
			free(cgd);
		}
		free(pr);
	}
	return 0;
}

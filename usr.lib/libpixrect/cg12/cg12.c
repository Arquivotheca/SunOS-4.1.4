#ifndef lint
static char     sccsid[] = "@(#)cg12.c	1.1 94/10/31 SMI";
#endif

/* Copyright 1990, Sun Microsystems, Inc. */

#include <sys/types.h>			/* for fbio	 */
#include <sys/ioctl.h>			/* FBIO_*	 */
#include <sun/fbio.h>			/* FBIO_*	 */
#include <pixrect/cg12_var.h>		/* CG12*	 */
#include <pixrect/pr_impl_make.h>	/* attr		 */
#include <pixrect/pr_planegroups.h>	/* PIXPG	 */
#include <pixrect/pr_dblbuf.h>		/* PR_DBL*	 */
#include <pixrect/gp1var.h>		/* GP*		 */
#include <sun/gpio.h>			/* GP1IO_*	 */

int                 cg12_offscreen;
int                 cg12_width, cg12_height;
extern int          gp1_get_cmdver();

/*
 * Note: the order of frame buffers initialized here must match the
 * virtual address order set in the device driver (cgtwelve.c) and
 * used in the mpr.md_image values.
 */

static struct fbdesc
{
    short               group;
    short               depth;
    int                 allplanes;
}                   fbdesc[CG12_NFBS] =
{
    { PIXPG_OVERLAY, 1, 0 },		/* Wid Bit [7] of 0-7		 */
    { PIXPG_OVERLAY_ENABLE, 1, 0 },	/* Wid Bit [6] of 0-7		 */
    { PIXPG_24BIT_COLOR, 32, 0x00ffffff },
    { PIXPG_8BIT_COLOR, 8, 0xff },	/* Shared memory with 24-bit	 */
    { PIXPG_WID, 8, 0x3f },		/* Wid Bits [0-5] of 0-7	 */
    { PIXPG_ZBUF, 16, 0xffff }
};

static struct pr_devdata *cg12_devdata;

Pixrect            *
cg12_make(fd, size, depth, attr)
int                 fd;
struct pr_size      size;
int                 depth;
struct fbgattr     *attr;
{
    int                 i;
    Pixrect            *pr;
    struct pr_devdata  *dd;
    struct cg12_data   *cg12d;
    struct fbinfo       fbinfo;

    if ((pr = pr_makefromfd_2(fd, size, depth, &cg12_devdata, &dd,
		sizeof(struct cg12_data),
		attr->fbtype.fb_size + attr->sattr.dev_specific[GP_SHMEMSIZE],
		0, 0, 0)) != 0)
    {
	pr->pr_ops = &cg12_ops;
	cg12d = cg12_d(pr);

	/* do the gp (cg2) specific stuff first */

	/* cgpr_va not needed? */
	cg12d->gp_shmem = (char *) (dd->va + attr->fbtype.fb_size);
	cg12d->cgpr_fd = dd->fd;
	/* cgpr_planes not needed? */
	cg12d->cgpr_offset.x = 0;
	cg12d->cgpr_offset.y = 0;
	if (ioctl(fd, FBIOGINFO, &fbinfo) < 0 ||
	    ioctl(fd, GP1IO_GET_TRUMINORDEV, &cg12d->minordev) < 0 ||
	    ioctl(fd, GP1IO_GET_GBUFFER_STATE, &cg12d->gbufflag) < 0)
	{
	    cg12_destroy(pr);
	    return 0;
	}
	cg12d->cg2_index = fbinfo.fb_unit;
	cg12d->ioctl_fd = dd->fd;
	(void) gp1_get_cmdver(pr);	/* fills in ncmd, cmdver */
	cg12d->flags |= CG2D_GP;
	cg12d->linebytes = mpr_linebytes(size.x, 32);
	cg12d->fbtype = FBTYPE_SUNGP3;

	/* now the cg12 specific stuff */

	cg12d->fd = dd->fd;
	cg12d->cg12_flags = CG12_PRIMARY | CG12_OVERLAY_CMAP;
	cg12d->wid_dbl_info.dbl_wid.wa_type = 0;
	cg12d->wid_dbl_info.dbl_wid.wa_index = -1;
	cg12d->wid_dbl_info.dbl_wid.wa_count = 1;
	cg12d->wid_dbl_info.dbl_fore = PR_DBL_A;
	cg12d->wid_dbl_info.dbl_back = PR_DBL_B;
	cg12d->wid_dbl_info.dbl_read_state = PR_DBL_A;
	cg12d->wid_dbl_info.dbl_write_state = PR_DBL_BOTH;
	cg12d->windowfd = -1;

	/*
	 * Initialize all plane groups, we have six here.  All of them are
	 * described by the "fbdesc" table.
	 */

	for (i = 0; i < CG12_NFBS; i++)
	{
	    cg12d->fb[i].group = fbdesc[i].group;
	    cg12d->fb[i].depth = fbdesc[i].depth;
	    cg12d->fb[i].mprp.mpr.md_linebytes =
		mpr_linebytes(size.x, fbdesc[i].depth);
	    cg12d->fb[i].mprp.mpr.md_offset.x = 0;
	    cg12d->fb[i].mprp.mpr.md_offset.y = 0;
	    cg12d->fb[i].mprp.mpr.md_primary = 0;

	    /* if its more than 1 bit deep, it is possible to plane mask it */
	    cg12d->fb[i].mprp.mpr.md_flags = fbdesc[i].allplanes != 0
		? MP_DISPLAY | MP_PLANEMASK : MP_DISPLAY;
	    cg12d->fb[i].mprp.planes = fbdesc[i].allplanes;
	}

	/*
	 * egret a,b,c have physical memory to 1024x1024 with 1152x900 screen
	 * egret f has physical memory to 1280x1228 with 1280x1024 screen
	 */

	if (attr->fbtype.fb_width == 1280)
	{
	    cg12d->fb[0].mprp.mpr.md_image =
		(short *) (dd->va + CG12_VOFF_OVERLAY_HR);
	    cg12d->fb[1].mprp.mpr.md_image =
		(short *) (dd->va + CG12_VOFF_ENABLE_HR);
	    cg12d->fb[2].mprp.mpr.md_image =
		(short *) (dd->va + CG12_VOFF_COLOR24_HR);
	    cg12d->fb[3].mprp.mpr.md_image =
		(short *) (dd->va + CG12_VOFF_COLOR8_HR);
	    cg12d->fb[4].mprp.mpr.md_image =
		(short *) (dd->va + CG12_VOFF_WID_HR);
	    cg12d->fb[5].mprp.mpr.md_image =
		(short *) (dd->va + CG12_VOFF_ZBUF_HR);
	    cg12d->ctl_sp = (struct cg12_ctl_sp *) (dd->va + CG12_VOFF_CTL_HR);
	}
	else
	{
	    cg12d->fb[0].mprp.mpr.md_image =
		(short *) (dd->va + CG12_VOFF_OVERLAY);
	    cg12d->fb[1].mprp.mpr.md_image =
		(short *) (dd->va + CG12_VOFF_ENABLE);
	    cg12d->fb[2].mprp.mpr.md_image =
		(short *) (dd->va + CG12_VOFF_COLOR24);
	    cg12d->fb[3].mprp.mpr.md_image =
		(short *) (dd->va + CG12_VOFF_COLOR8);
	    cg12d->fb[4].mprp.mpr.md_image =
		(short *) (dd->va + CG12_VOFF_WID);
	    cg12d->fb[5].mprp.mpr.md_image =
		(short *) (dd->va + CG12_VOFF_ZBUF);
	    cg12d->ctl_sp = (struct cg12_ctl_sp *) (dd->va + CG12_VOFF_CTL);
	}
	cg12_width = attr->fbtype.fb_width;
	cg12_height = attr->fbtype.fb_height;
	cg12_offscreen = ((attr->fbtype.fb_width * attr->fbtype.fb_height +
	    CG12_DITHER_SIZE + CG12_SOLID_PAT_SIZE + CG12_HOLLOW_PAT_SIZE)
	    >> 3);

	{

	    /*
	     * Default is the monochrome overlay plane.  Note that neither
	     * overlay nor enable planes are initialized.
	     */
	    int                 initplane;
	    int                 video_on = FBVIDEO_ON;

	    /* pick a default, any default and change from dbx. */
	    i = 0;
	    initplane = PIX_GROUP(fbdesc[i].group) | fbdesc[i].allplanes;
	    (void) cg12_putattributes(pr, &initplane);
	    (void) ioctl(cg12d->fd, FBIOSVIDEO, &video_on);	/* turn it on */
	    cg12d->flags &= ~CG12_OVERLAY_CMAP;
	}
    }
    return pr;
}

cg12_destroy(pr)
Pixrect            *pr;
{
    if (pr)
    {
	struct cg12_data   *cg12d;

	if (cg12d = cg12_d(pr))
	{
	    if (cg12d->cg12_flags & CG12_PRIMARY)
		pr_unmakefromfd(cg12d->fd, &cg12_devdata);
	    free(cg12d);
	}
	free(pr);
    }
    return 0;
}

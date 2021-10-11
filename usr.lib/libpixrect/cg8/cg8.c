#ifndef lint
static char     sccsid[] = "@(#)cg8.c 1.1 10/31/94, SMI";
#endif

/*
 * Copyright 1988, Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_make.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/cg8var.h>
#include <sundev/cg8reg.h>

static struct pr_devdata *cg8devdata;
extern struct pixrectops cg8_pip_ops;

/*
    We think the board look like this:
    ------------------------------------------------------------
    DACBASE		: ramdac (32 bytes)
    DACBASE + pagesize	: p4 register (4 bytes)
    OVLBASE		: overlay plane (mpr_linebytes(w * 1) * h)
    OVLBASE + ovlsize	: enable plane (mpr_linebytes(w * 1) * h)
    OVLBASE + 2* ovlsize: framebuffer (mpr_linebytes(w * 32) * h)
    ------------------------------------------------------------
    The real board layout is defined in cg8reg.h.
    The translation is done by the driver mmap function.
*/

Pixrect        *
cg8_make (fd, size, depth)
    int             fd;
    struct pr_size  size;
    int             depth;
{
    static struct fbdesc {
	short           group;
	short           depth;
	int             allplanes;
    }               fbdesc[CG8_NFBS] = {
	{
	    PIXPG_OVERLAY, 1, 0
	},
	{
	    PIXPG_OVERLAY_ENABLE, 1, 0
	},
	{
	    PIXPG_24BIT_COLOR, 32, 0x00ffffff
	}
    };

    register struct cg8_data *cg8d;
    struct pr_devdata        *curdd;
    Pipio_Fb_Info            fb_info;      /* Driver description of frame buffers. */
    int                      i;		   /* Index: number of frame buffer now checking. */
    int                      mapsize;	   /* # of bytes to be mapped in */
    int	                     sun_cg8;      /* Return code from ioctl, if ~= 0 its an old style cg8. */
    register int             ovlsize;	   /* # of bytes per bitplane */
    register int             pagesize;	   /* # of bytes per page */
    int			     pip_present;  /* -> where ioctl will place pip present indication. */
    Pixrect                  *pr;          /* Pixrect for device being "made". */

    /*	There are 3 possible cg8 frame buffers which must be accommodated by this code:
     *    (1) The original p4 cg8 (IBIS) card.
     *    (2) The original sbus cg8 look-alike, which does not return the alpha
     *        channel as zeros.
     *    (3) The sbus TC/PIP which has 8-bit support, and optional a picture in
     *        a picture daughter board.
     *
     *  Use the PIPIO_G_FB_INFO ioctl to determine which of the 3 cases we have.
     */
    pagesize = getpagesize ();
    ovlsize = ROUNDUP (mpr_linebytes (size.x, 1) * size.y, pagesize);

    sun_cg8 = (ioctl(fd, PIPIO_G_FB_INFO, &fb_info) != 0);
    if ( sun_cg8 )
    {
	mapsize = ovlsize + ovlsize;
	mapsize += ROUNDUP (mpr_linebytes (size.x, 32) * size.y, pagesize);
    }
    else 
    {
	mapsize = fb_info.total_mmap_size;
    }

    if ((pr = pr_makefromfd_2 (fd, size, depth, &cg8devdata, &curdd,
	sizeof (struct cg8_data), mapsize, CG8_VADDR_FB, 2 * pagesize, CG8_VADDR_DAC)) == 0)
    {
	return (Pixrect *) NULL;
    }

    /*	Initialize the pixrect which was returned by the pr_makefromfd_2 call.
     *	Initialize the frame buffer information based on whether the frame buffer
     *	is a p4 or sbus style cg8. 
     */
    cg8d = cg8_d (pr);

    pr->pr_ops = &cg8_ops;

    cg8d->windowfd = cg8d->real_windowfd = -2;
    cg8d->cms.cms_size = cg8d->cms.cms_addr = 0;


    cg8d->flags = CG8_PRIMARY | CG8_OVERLAY_CMAP;
    cg8d->planes = 0;
    cg8d->fd = curdd->fd;
	
    if ( sun_cg8 )
    {
	cg8d->num_fbs = CG8_NFBS;
	for (i = 0; i < cg8d->num_fbs; i++) {
	    cg8d->fb[i].group = fbdesc[i].group;
	    cg8d->fb[i].depth = fbdesc[i].depth;
	    cg8d->fb[i].mprp.mpr.md_linebytes = mpr_linebytes (size.x, fbdesc[i].depth);
	    cg8d->fb[i].mprp.mpr.md_offset.x = 0;
	    cg8d->fb[i].mprp.mpr.md_offset.y = 0;
	    cg8d->fb[i].mprp.mpr.md_primary = 0;

	    /* If it is more than 1 bit deep, it is possible to plane mask it
	     */
	    cg8d->fb[i].mprp.mpr.md_flags = fbdesc[i].allplanes != 0
		? MP_DISPLAY | MP_PLANEMASK : MP_DISPLAY;
	    cg8d->fb[i].mprp.planes = fbdesc[i].allplanes;
	}
    }

    else
    {
	cg8d->num_fbs = fb_info.frame_buffer_count;
	for (i = 0; i < cg8d->num_fbs; i++) 
	{
	    cg8d->fb[i].group = fb_info.fb_descriptions[i].group;
	    cg8d->fb[i].depth = fb_info.fb_descriptions[i].depth;
	    cg8d->fb[i].mprp.mpr.md_linebytes = fb_info.fb_descriptions[i].linebytes;
	    cg8d->fb[i].mprp.mpr.md_offset.x = 0;
	    cg8d->fb[i].mprp.mpr.md_offset.y = 0;
	    cg8d->fb[i].mprp.mpr.md_primary = 0;

	    cg8d->fb[i].mprp.mpr.md_flags = fb_info.fb_descriptions[i].depth != 1 
		? MP_DISPLAY | MP_PLANEMASK : MP_DISPLAY;
	    if ( cg8d->fb[i].group == PIXPG_VIDEO_ENABLE ) 
	    {
		cg8d->flags |= CG8_PIP_PRESENT;
	    }
	    else if ( cg8d->fb[i].group == PIXPG_8BIT_COLOR )
	    {
		cg8d->flags |= CG8_EIGHT_BIT_PRESENT;
	    }
	    cg8d->fb[i].mprp.planes = fb_info.fb_descriptions[i].depth == 1 ? 0 :
		(fb_info.fb_descriptions[i].depth == 32 ? PIX_ALL_PLANES : 0xFF);
	}
    }

    /*  Assign the address pointers to the images which were mapped in
     *  by "pr_makefromfd_2". Again processing is different depending
     *  upon whether the cg8 is sun or not.
     */
    if ( sun_cg8 )
    {
	register u_char *im_ptr;

	im_ptr = (u_char *) curdd->va;
	cg8d->fb[0].mprp.mpr.md_image = (short *) im_ptr;
	cg8d->fb[1].mprp.mpr.md_image = (short *) (im_ptr += ovlsize);
	cg8d->fb[2].mprp.mpr.md_image = (short *) (im_ptr += ovlsize);
    }
    else
    {
	register u_char *im_ptr;

	im_ptr = (u_char *) curdd->va;

	for (i = 0; i < cg8d->num_fbs; i++)
	{
	    cg8d->fb[i].mprp.mpr.md_image = (short *) ((u_int)im_ptr + fb_info.fb_descriptions[i].mmap_offset);
	}
    }


    {
	/*
	 * default is the monochrome overlay plane.  Note that neither
	 * overlay nor enable planes are initialized.
	 */
	int             initplane;
	int             video_on = FBVIDEO_ON;

	initplane = PIX_GROUP (cg8d->fb[0].group) | cg8d->fb[0].mprp.planes;
	(void) cg8_putattributes (pr, &initplane);
	(void) ioctl (cg8d->fd, FBIOSVIDEO, &video_on);	/* turn it on */
	cg8d->flags &= ~CG8_OVERLAY_CMAP;
    }

    return pr;
}

cg8_destroy (pr)		       /* ditto cg4_destroy */
    Pixrect        *pr;
{
    if (pr) {
	struct cg8_data *cg8d;

	if (cg8d = cg8_d (pr)) {
	    if (cg8d->flags & CG8_PRIMARY)
		pr_unmakefromfd (cg8d->fd, &cg8devdata);
	    free (cg8d);
	}
	free (pr);
    }
    return 0;
}

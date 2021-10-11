#ifndef lint
static char	sccsid[] =	"@(#)cg12_colormap.c	1.1 94/10/31 SMI";
#endif

/* Copyright 1990 by Sun Microsystems, Inc. */

/*
 * cg12 colormap and attribute functions.
 *
 * cg12_{put,get}colormap can be called in two ways, through the macros
 * pr_{put,get}lut or directly through pr_{put,get}colormap.  If through
 * {put,get}lut, then PR_FORCE_UPDATE is embedded in the "index" argument.
 *
 * The force flag indicates that the caller wants to update the colormaps
 * in the new "true-color" mode rather than the traditional "indexed-color"
 * mode.
 *
 * putcolormap is also a kernel function, call by dtop_putcolormap.
 *
 * putattributes is also a kernel function.
 */

#include <sys/types.h>			/* for fbio.h			 */
#include <sys/ioctl.h>			/* fbio				 */
#include <sun/fbio.h>			/* FBIO*			 */
#include <pixrect/cg12_var.h>		/* CG12_*			 */
#include <pixrect/pr_planegroups.h>	/* PIXPG_*			 */
#include <pixrect/pr_dblbuf.h>		/* PR_DBL*			 */

/* all these includes just to get WINWIDGET defined */

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>	/* WINWIDGET			 */

#ifdef	KERNEL
#include "cgtwelve.h"

#if NCGTWELVE > 0
#define	ioctl	cgtwelveioctl
#else
#define ioctl
#endif	/* NCGTWELVE */

#endif	/* KERNEL */

cg12_putcolormap(pr, index, count, red, green, blue)
Pixrect            *pr;
int                 index,
		    count;
unsigned char      *red,
		   *green,
		   *blue;
{
    struct cg12_data   *cg12d;
    struct fbcmap       fbmap;
    int                 cc,
			i,
			plane;

    /*
     * Set "cg12d" to the cg12 private data structure.  If the plane is not
     * specified in the index, then use the currently active plane.
     */

    cg12d = cg12_d(pr);
    if (PIX_ATTRGROUP(index))
    {
	plane = PIX_ATTRGROUP(index) & PIX_GROUP_MASK;
	index &= PR_FORCE_UPDATE | PIX_ALL_PLANES;
    }
    else
	plane = cg12d->fb[cg12d->active].group;

    /*
     * When PR_FORCE_UPDATE is set, pass everything straight through to the
     * ioctl.  If PR_FORCE_UPDATE is not set, take "indexed" actions.
     */

    if (index & PR_FORCE_UPDATE || plane == PIXPG_8BIT_COLOR)
    {
#ifdef KERNEL
	cg12d->flags |= CG12_KERNEL_UPDATE;	/* bcopy vs. copyin */
#endif

	fbmap.index = index | PIX_GROUP(plane);
	fbmap.count = count;
	fbmap.red = red;
	fbmap.green = green;
	fbmap.blue = blue;
	return ioctl(cg12d->fd, FBIOPUTCMAP, (caddr_t) & fbmap, 0);
    }
    else	/* indexed */
    {
	if (plane == PIXPG_OVERLAY_ENABLE || plane == PIXPG_WID ||
	    plane == PIXPG_ZBUF || plane == PIXPG_24BIT_COLOR)
	    return 0;

	if (plane == PIXPG_OVERLAY)
	{
	    for (cc = i = 0; count && !cc; i++, index++, count--)
	    {

#ifdef KERNEL
		cg12d->flags |= CG12_KERNEL_UPDATE;	/* bcopy vs. copyin */
#endif

		/* Index 0 is mapped to 1.  All others mapped to 3. */
		fbmap.index = (index ? 3 : 1) | PIX_GROUP(plane);
		fbmap.count = 1;
		fbmap.red = red + i;
		fbmap.green = green + i;
		fbmap.blue = blue + i;
		cc = ioctl(cg12d->fd, FBIOPUTCMAP, (caddr_t) & fbmap, 0);
	    }
	    return cc;
	}
    }
    return PIX_ERR;
}

cg12_putattributes(pr, planesp)
Pixrect            *pr;
u_int              *planesp;		/* encoded group and plane mask */

{
    struct cg12_data   *cg12d;		/* private data */
    u_int               planes,
			group;
    int                 dont_set_planes;

    cg12d = cg12_d(pr);
    dont_set_planes = *planesp & PIX_DONT_SET_PLANES;
    planes = *planesp & PIX_ALL_PLANES;	/* the plane mask */
    group = PIX_ATTRGROUP(*planesp);	/* extract the group part */

    /*
     * User is trying to set the group to something else. We'll see if the
     * group is supported.  If does, do as he wishes.
     */

    if (group != PIXPG_CURRENT || group == PIXPG_INVALID)
    {
	int                 active = 0,
			    found = 0,
			    cmsize;

	for (; active < CG12_NFBS && !found; active++)
	    if (found =
		(group == cg12d->fb[active].group))
	    {
		cg12d->mprp = cg12d->fb[active].mprp;
		cg12d->planes = PIX_GROUP(group) |
		    (cg12d->mprp.planes & PIX_ALL_PLANES);
		cg12d->active = active;
		pr->pr_depth = cg12d->fb[active].depth;
		switch (group)
		{
		    default:
		    case PIXPG_8BIT_COLOR:
		    case PIXPG_24BIT_COLOR:
			cmsize = 256;
			break;

		    case PIXPG_OVERLAY:
			cmsize = 4;
			break;

		    case PIXPG_OVERLAY_ENABLE:
		    case PIXPG_WID:
		    case PIXPG_ZBUF:
			cmsize = 0;
			break;
		}
		(void) cg12_ioctl(pr, FBIOSCMSIZE, (caddr_t) & cmsize);
	    }
    }

    /* group is PIXPG_CURRNT here */
    if (!dont_set_planes)
    {
	cg12d->planes =
	    cg12d->mprp.planes =
		cg12d->fb[cg12d->active].mprp.planes =
		    (cg12d->planes & ~PIX_ALL_PLANES) | planes;
    }

    return 0;
}

cg12_set_state(pr)
struct pixrect     *pr;
{
    int                 i
#ifdef KERNEL
			;
#else
			, j;
#endif
    unsigned int        planes;
    struct cg12_data   *cg12d;
    static unsigned int dbl_mask[4] = {0xffffff, 0xf0f0f0, 0x0f0f0f, 0};
    extern int          cg12_width;


    cg12d = cg12_d(pr);
    planes = cg12d->planes & PIX_ALL_PLANES;
    switch (PIX_ATTRGROUP(cg12d->planes))
    {
        default:
        case PIXPG_8BIT_COLOR:
	    /* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	    /* leave WSTATUS bit set always to inform PROM to save the
	       context before displaying any character */
            cg12d->ctl_sp->eic.host_control = CG12_EIC_WSTATUS;
	    /* !!! */
            cg12d->ctl_sp->apu.hpage = (cg12_width == 1280)
		? CG12_HPAGE_8BIT_HR : CG12_HPAGE_8BIT;
            cg12d->ctl_sp->apu.haccess = CG12_HACCESS_8BIT;
            cg12d->ctl_sp->dpu.pln_sl_host = CG12_PLN_SL_8BIT;
            cg12d->ctl_sp->dpu.pln_rd_msk_host = CG12_PLN_RD_8BIT;
            i = planes & 0xff;
            cg12d->ctl_sp->dpu.pln_wr_msk_host = i << 16 | i << 8 | i;
            break;

        case PIXPG_24BIT_COLOR:
            cg12d->ctl_sp->apu.hpage = (cg12_width == 1280)
		? CG12_HPAGE_24BIT_HR : CG12_HPAGE_24BIT;
            cg12d->ctl_sp->apu.haccess = CG12_HACCESS_24BIT;
            cg12d->ctl_sp->dpu.pln_sl_host = CG12_PLN_SL_24BIT;
            cg12d->ctl_sp->dpu.pln_rd_msk_host = CG12_PLN_RD_24BIT;
#ifdef KERNEL
            cg12d->ctl_sp->dpu.pln_wr_msk_host = CG12_PLN_WR_24BIT;
#else
            if (cg12d->windowfd >= 0)
                ioctl(cg12d->windowfd, WINWIDGET, &cg12d->wid_dbl_info);

            i = (cg12d->wid_dbl_info.dbl_read_state == PR_DBL_A)
                ? cg12d->wid_dbl_info.dbl_fore
                : cg12d->wid_dbl_info.dbl_back;
                  
            switch (cg12d->wid_dbl_info.dbl_write_state)
            {     
                case PR_DBL_A:
                    j = cg12d->wid_dbl_info.dbl_fore;
                    break;
           
                case PR_DBL_B:
                    j = cg12d->wid_dbl_info.dbl_back;
                    break;
    
                case PR_DBL_NONE:
                    j = PR_DBL_NONE;
                    break;
                  
                case PR_DBL_BOTH:
                default:
                    j = PR_DBL_BOTH;
            }
            pr_dbl_set(pr, PR_DBL_READ, i, PR_DBL_WRITE, j, 0);
            i = ((cg12d->ctl_sp->eic.host_control & CG12_EIC_RES0) >> 27) |
                ((cg12d->ctl_sp->eic.host_control & CG12_EIC_RES1) >> 25);
            cg12d->ctl_sp->dpu.pln_wr_msk_host =
                planes & dbl_mask[i] & 0xffffff;
#endif
            break;
    
        case PIXPG_OVERLAY:
	    /* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	    /* leave WSTATUS bit set always to inform PROM to save the
	       context before displaying any character */
            cg12d->ctl_sp->eic.host_control = CG12_EIC_WSTATUS;
	    /* !!! */
            cg12d->ctl_sp->apu.hpage = (cg12_width == 1280)
		? CG12_HPAGE_OVERLAY_HR : CG12_HPAGE_OVERLAY;
            cg12d->ctl_sp->apu.haccess = CG12_HACCESS_OVERLAY;
            cg12d->ctl_sp->dpu.pln_sl_host = CG12_PLN_SL_OVERLAY;
            cg12d->ctl_sp->dpu.pln_wr_msk_host = CG12_PLN_WR_OVERLAY;
            cg12d->ctl_sp->dpu.pln_rd_msk_host = CG12_PLN_RD_OVERLAY;
            break;
           
        case PIXPG_OVERLAY_ENABLE:
	    /* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	    /* leave WSTATUS bit set always to inform PROM to save the
	       context before displaying any character */
            cg12d->ctl_sp->eic.host_control = CG12_EIC_WSTATUS;
	    /* !!! */
            cg12d->ctl_sp->apu.hpage = (cg12_width == 1280)
		? CG12_HPAGE_ENABLE_HR : CG12_HPAGE_ENABLE;
            cg12d->ctl_sp->apu.haccess = CG12_HACCESS_ENABLE;
            cg12d->ctl_sp->dpu.pln_sl_host = CG12_PLN_SL_ENABLE;
            cg12d->ctl_sp->dpu.pln_wr_msk_host = CG12_PLN_WR_ENABLE;
            cg12d->ctl_sp->dpu.pln_rd_msk_host = CG12_PLN_RD_ENABLE;
            break;
 
        case PIXPG_WID:
	    /* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	    /* leave WSTATUS bit set always to inform PROM to save the
	       context before displaying any character */
            cg12d->ctl_sp->eic.host_control = CG12_EIC_WSTATUS;
	    /* !!! */
            cg12d->ctl_sp->apu.hpage = (cg12_width == 1280)
		? CG12_HPAGE_WID_HR : CG12_HPAGE_WID;
            cg12d->ctl_sp->apu.haccess = CG12_HACCESS_WID;
            cg12d->ctl_sp->dpu.pln_sl_host = CG12_PLN_SL_WID;
            cg12d->ctl_sp->dpu.pln_rd_msk_host = CG12_PLN_RD_WID;
            cg12d->ctl_sp->dpu.pln_wr_msk_host = (planes & 0x3f) << 16;
            break;
           
        case PIXPG_ZBUF:
	    /* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	    /* leave WSTATUS bit set always to inform PROM to save the
	       context before displaying any character */
            cg12d->ctl_sp->eic.host_control = 0x24000000;
	    /* !!! */
            cg12d->ctl_sp->apu.hpage = (cg12_width == 1280)
		? CG12_HPAGE_ZBUF_HR : CG12_HPAGE_ZBUF;
            cg12d->ctl_sp->apu.haccess = CG12_HACCESS_ZBUF;
            cg12d->ctl_sp->dpu.pln_sl_host = CG12_PLN_SL_ZBUF;
            cg12d->ctl_sp->dpu.pln_wr_msk_host = CG12_PLN_WR_ZBUF;
            cg12d->ctl_sp->dpu.pln_rd_msk_host = CG12_PLN_RD_ZBUF;
            break;
    }
}

#ifndef	KERNEL	/* getcolormap and getattributes are not used by kernel */

cg12_getcolormap(pr, index, count, red, green, blue)
Pixrect            *pr;
int                 index,
		    count;
unsigned char      *red,
		   *green,
		   *blue;
{
    int                 cc,
			i,
			plane;
    struct cg12_data   *cg12d;
    struct fbcmap       fbmap;

    /*
     * Set "cg12d" to the cg12 private data structure.  If the plane is not
     * specified in the index, then use the currently active plane.
     */

    cg12d = cg12_d(pr);
    if (PIX_ATTRGROUP(index))
    {
	plane = PIX_ATTRGROUP(index) & PIX_GROUP_MASK;
	index &= PR_FORCE_UPDATE | PIX_ALL_PLANES;
    }
    else
	plane = cg12d->fb[cg12d->active].group;

    if (index & PR_FORCE_UPDATE || plane == PIXPG_24BIT_COLOR ||
	plane == PIXPG_8BIT_COLOR)
    {
	fbmap.index = index | PIX_GROUP(plane);
	fbmap.count = count;
	fbmap.red = red;
	fbmap.green = green;
	fbmap.blue = blue;

	return ioctl(cg12d->fd, FBIOGETCMAP, (caddr_t) & fbmap, 0);
    }
    else
    {
	if (plane == PIXPG_OVERLAY_ENABLE || plane == PIXPG_WID ||
	    plane == PIXPG_ZBUF)
	    return 0;

	if (plane == PIXPG_OVERLAY)
	{
	    for (cc = i = 0; count && !cc; i++, index++, count--)
	    {
		/* Index 0 is mapped to 1.  All others mapped to 3. */
		fbmap.index = (index ? 3 : 1) | PIX_GROUP(plane);
		fbmap.count = 1;
		fbmap.red = red + i;
		fbmap.green = green + i;
		fbmap.blue = blue + i;
		cc = ioctl(cg12d->fd, FBIOGETCMAP, (caddr_t) & fbmap, 0);
	    }
	    return cc;
	}
    }
    return PIX_ERR;
}

cg12_getattributes(pr, planesp)
Pixrect            *pr;
int                *planesp;

{
    if (planesp)
	*planesp = cg12_d(pr)->planes & PIX_ALL_PLANES;
    return 0;
}

#endif					/* !KERNEL */

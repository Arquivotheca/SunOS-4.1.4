#ifndef lint
static char	sccsid[] =	"@(#)cg9_colormap.c	1.1 94/10/31 SMI";
#endif

/* Copyright 1989 by Sun Microsystems, Inc. */

/*
 * cg9 colormap and attribute functions.
 *
 * cg9_{put,get}colormap can be called in two ways, through the macros
 * pr_{put,get}lut or directly through pr_{put,get}colormap.  If through
 * {put,get}lut, then PR_FORCE_UPDATE is embedded in the "index" argument.
 *
 * The force flag indicates that the caller wants to update the colormaps
 * in the new "true-color" mode rather than the traditional "indexed-color"
 * mode.  If the force flag is not set, then this code maps the requests
 * into an "emulated indexed-color" mode.
 *
 * putcolormap is also a kernel function, call by dtop_putcolormap.
 *
 * putattributes is also a kernel function.
 */

#include <sys/ioctl.h>			/* fbio				*/
#include <sun/fbio.h>			/* FBIO*			*/
#include <pixrect/pixrect.h>		/* lots of reasons		*/
#include <pixrect/pr_planegroups.h>	/* PIXPG_*			*/
#include <pixrect/cg9var.h>		/* CG9_*			*/

#ifdef	KERNEL
#include "cgnine.h"
#if NCGNINE > 0
#define	ioctl	cgnineioctl
#else
#define ioctl
#endif
#endif

cg9_putcolormap(pr, index, count, red, green, blue)
Pixrect		*pr;
int		index, count;
unsigned char	*red, *green, *blue;
{
    struct cg9_data	*cg9d;
    struct fbcmap	fbmap;
    int			cc,i,plane;

    /*
     * Set "cg9d" to the cg9 private data structure.  If the plane
     * is not specified in the index, then use the currently active
     * plane.
     */

    cg9d = cg9_d(pr);
    if (PIX_ATTRGROUP(index))
    {
	plane = PIX_ATTRGROUP(index) & PIX_GROUP_MASK;
	index &= PR_FORCE_UPDATE | PIX_ALL_PLANES;
    }
    else
	plane = cg9d->fb[cg9d->active].group;

    /*
     * When PR_FORCE_UPDATE is set, pass everything straight through to
     * the ioctl.  If PR_FORCE_UPDATE is not set, take "emulated index"
     * actions.
     */

    if (index & PR_FORCE_UPDATE || plane == PIXPG_24BIT_COLOR)
    {
#	ifdef KERNEL
	cg9d->flags |= CG9_KERNEL_UPDATE;	/* bcopy vs. copyin */
#	endif
        fbmap.index = index | PIX_GROUP(plane);
        fbmap.count = count;
        fbmap.red = red;
        fbmap.green = green;
        fbmap.blue = blue;
        return ioctl(cg9d->fd, FBIOPUTCMAP, (caddr_t) &fbmap, 0);
    }
    else	/* emulated index */
    {
	if (plane == PIXPG_OVERLAY_ENABLE) return 0;

	if (plane == PIXPG_OVERLAY)
	{
	    for (cc=i=0; count && !cc; i++,index++,count--)
	    {
#		ifdef KERNEL
		cg9d->flags |= CG9_KERNEL_UPDATE;	/* bcopy vs. copyin */
#		endif
		/* Index 0 is mapped to 1.  All others mapped to 3. */
		fbmap.index = (index ? 3 : 1) | PIX_GROUP(plane);
		fbmap.count = 1;
		fbmap.red = red+i;
		fbmap.green = green+i;
		fbmap.blue = blue+i;
		cc = ioctl(cg9d->fd, FBIOPUTCMAP, (caddr_t) &fbmap, 0);
	    }
	    return cc;
	}
    }
    return PIX_ERR;
}

#ifndef	KERNEL	/* getcolormap and getattributes are not used by kernel */

cg9_getcolormap(pr, index, count, red, green, blue)
Pixrect		*pr;
int		index, count;
unsigned char	*red, *green, *blue;
{
    int			cc,i,plane;
    struct cg9_data	*cg9d;
    struct fbcmap	fbmap;

    /*
     * Set "cg9d" to the cg9 private data structure.  If the plane
     * is not specified in the index, then use the currently active
     * plane.
     */

    cg9d = cg9_d(pr);
    if (PIX_ATTRGROUP(index))
    {
	plane = PIX_ATTRGROUP(index) & PIX_GROUP_MASK;
	index &= PR_FORCE_UPDATE | PIX_ALL_PLANES;
    }
    else
	plane = cg9d->fb[cg9d->active].group;
 
    if (index&PR_FORCE_UPDATE || plane == PIXPG_24BIT_COLOR)
    {
        fbmap.index = index | PIX_GROUP(plane);
        fbmap.count = count;
        fbmap.red = red;
        fbmap.green = green;
        fbmap.blue = blue;
 
        return ioctl(cg9d->fd, FBIOGETCMAP, (caddr_t) &fbmap, 0);
    }
    else
    {
	if (plane == PIXPG_OVERLAY_ENABLE) return 0;

	if (plane == PIXPG_OVERLAY)
	{
	    for (cc=i=0; count && !cc; i++,index++,count--)
	    {
		/* Index 0 is mapped to 1.  All others mapped to 3. */
		fbmap.index = (index ? 3 : 1) | PIX_GROUP(plane);
		fbmap.count = 1;
		fbmap.red = red+i;
		fbmap.green = green+i;
		fbmap.blue = blue+i;
		cc = ioctl(cg9d->fd, FBIOGETCMAP, (caddr_t) &fbmap, 0);
	    }
	    return cc;
	}
    }
    return PIX_ERR;
}

cg9_getattributes (pr, planesp)
    Pixrect        *pr;
    int            *planesp;

{
    if (planesp)
	*planesp = cg9_d (pr)->planes & PIX_ALL_PLANES;
    return 0;
}

#endif !KERNEL

cg9_putattributes (pr, planesp)
    Pixrect        *pr;
    u_int          *planesp;	       /* encoded group and plane mask */

{
    struct cg9_data *cg9d;	       /* private data */
    u_int           planes,
		    group;
    int             dont_set_planes;

    cg9d = cg9_d (pr);
    dont_set_planes = *planesp & PIX_DONT_SET_PLANES;
    planes = *planesp & PIX_ALL_PLANES;/* the plane mask */
    group = PIX_ATTRGROUP (*planesp);  /* extract the group part */

    /*
     * User is trying to set the group to something else. We'll see if the
     * group is supported.  If does, do as he wishes.
     */

    if (group != PIXPG_CURRENT || group == PIXPG_INVALID) {
	int             active = 0,
	                found = 0,
			cmsize;

	if (group == PIXPG_TRANSPARENT_OVERLAY)
	{
	    group = PIXPG_OVERLAY;
	    cg9d->flags |= CG9_COLOR_OVERLAY;
	}
	else cg9d->flags &= ~CG9_COLOR_OVERLAY;

	for (; active < CG9_NFBS && !found; active++)
	    if (found =
		(group == cg9d->fb[active].group)) {
		cg9d->mprp = cg9d->fb[active].mprp;
		/* hardware plane mask too? no, because of sunview? */
		cg9d->planes = PIX_GROUP (group) |
		    (cg9d->mprp.planes & PIX_ALL_PLANES);
		cg9d->active = active;
		pr->pr_depth = cg9d->fb[active].depth;
		switch (group)
		{
		    default:
		    case PIXPG_24BIT_COLOR:	cmsize = 256;	break;
		    case PIXPG_OVERLAY:		cmsize = 4;	break;
		    case PIXPG_OVERLAY_ENABLE:	cmsize = 0;
		}
		(void) cg9_ioctl(pr, FBIOSCMSIZE, (caddr_t) &cmsize);
	    }
    }

    /* group is PIXPG_CURRNT here */
    if (!dont_set_planes) {
	cg9d->planes =
	    cg9d->mprp.planes =
	    cg9d->fb[cg9d->active].mprp.planes =
	    PIX_GROUP (group) | planes;
    }

    return 0;
}

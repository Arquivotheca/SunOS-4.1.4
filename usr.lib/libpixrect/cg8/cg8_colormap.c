#ifndef lint
static char     sccsid[] = "@(#)cg8_colormap.c 1.1 of 94/10/31, SMI";
#endif

/* Copyright 1988 by Sun Microsystems, Inc. */

/* cg8 colormap and attribute functions */

/* see cg9_colormap.c for detailed comments */

#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/cg8var.h>

#ifdef KERNEL
#define ioctl cgeightioctl
#endif

/* ARGSUSED */
cg8_putcolormap(pr, index, count, red, green, blue)
    Pixrect        *pr;
    int             index,
                    count;
    unsigned char  *red,
                   *green,
                   *blue;
{
    struct cg8_data     *cg8d;
    struct fbcmap       fbmap;
    int                 cc,i,plane;
 
    /*
     * Set "cg8d" to the cg8 private data structure.  If the plane
     * is not specified in the index, then use the currently active
     * plane.
     */
 
    cg8d = cg8_d(pr);
    if (PIX_ATTRGROUP(index))
    {
        plane = PIX_ATTRGROUP(index) & PIX_GROUP_MASK;
        index &= PR_FORCE_UPDATE | PIX_ALL_PLANES;
    }
    else
        plane = cg8d->fb[cg8d->active].group;
 
    /*
     * When PR_FORCE_UPDATE is set, pass everything straight through to
     * the ioctl.  If PR_FORCE_UPDATE is not set, take "emulated index"
     * actions.
     */
 
    if (index & PR_FORCE_UPDATE || plane == PIXPG_24BIT_COLOR || plane == PIXPG_8BIT_COLOR)
    {
#       ifdef KERNEL
        cg8d->flags |= CG8_KERNEL_UPDATE;       /* bcopy vs. copyin */
#       endif
        fbmap.index = index | PIX_GROUP(plane);
        fbmap.count = count;
        fbmap.red = red;
        fbmap.green = green;
        fbmap.blue = blue;
        return ioctl(cg8d->fd, FBIOPUTCMAP, (caddr_t) &fbmap, 0);
    }
    else        /* emulated index */
    {
        if (plane == PIXPG_OVERLAY_ENABLE) return 0;
 
        if (plane == PIXPG_OVERLAY)
        {
            for (cc=i=0; count && !cc; i++,index++,count--)
            {
#               ifdef KERNEL
                cg8d->flags |= CG8_KERNEL_UPDATE;       /* bcopy vs. copyin */
#               endif
                /* Index 0 is mapped to 1.  All others mapped to 3. */
                fbmap.index = (index ? 3 : 1) | PIX_GROUP(plane);
                fbmap.count = 1;
                fbmap.red = red+i;
                fbmap.green = green+i;
                fbmap.blue = blue+i;
                cc = ioctl(cg8d->fd, FBIOPUTCMAP, (caddr_t) &fbmap, 0);
            }
            return cc;
        }
    }
    return PIX_ERR;
}

#ifndef KERNEL

cg8_getcolormap (pr, index, count, red, green, blue)
    Pixrect        *pr;
    int             index,
                    count;
    unsigned char  *red,
                   *green,
                   *blue;
{
    int                 cc,i,plane;
    struct cg8_data     *cg8d;
    struct fbcmap       fbmap;
        
    /*
     * Set "cg8d" to the cg8 private data structure.  If the plane
     * is not specified in the index, then use the currently active
     * plane.
     */ 
       
    cg8d = cg8_d(pr);
    if (PIX_ATTRGROUP(index))
    {   
        plane = PIX_ATTRGROUP(index) & PIX_GROUP_MASK;
        index &= PR_FORCE_UPDATE | PIX_ALL_PLANES;
    }   
    else
        plane = cg8d->fb[cg8d->active].group;
        
    if (index&PR_FORCE_UPDATE || plane == PIXPG_24BIT_COLOR ||
		plane == PIXPG_8BIT_COLOR)
    {   
        fbmap.index = index | PIX_GROUP(plane);
        fbmap.count = count;
        fbmap.red = red;
        fbmap.green = green;
        fbmap.blue = blue;
 
        return ioctl(cg8d->fd, FBIOGETCMAP, (caddr_t) &fbmap, 0);
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
                cc = ioctl(cg8d->fd, FBIOGETCMAP, (caddr_t) &fbmap, 0);
            }
            return cc;
        }
    }   
    return PIX_ERR;
}

#endif !KERNEL

cg8_putattributes (pr, planesp)
    Pixrect        *pr;
    u_int          *planesp;	       /* encoded group and plane mask */

{
    struct cg8_data *cg8d;	       /* private data */
    u_int           planes,
                    group;
    int             dont_set_planes;

    dont_set_planes = *planesp & PIX_DONT_SET_PLANES;
    planes = *planesp & PIX_ALL_PLANES;/* the plane mask */
    group = PIX_ATTRGROUP (*planesp);  /* extract the group part */

    cg8d = cg8_d (pr);

    /*
     * User is trying to set the group to something else. We'll see if the
     * group is supported.  If does, do as he wishes.
     */
    if (group != PIXPG_CURRENT || group == PIXPG_INVALID) {
	int             active = 0,
	                found = 0,
			cmsize;
	/* kernel should not access the color frame buffer */

	if (group == PIXPG_TRANSPARENT_OVERLAY)
	{
	    group = PIXPG_OVERLAY;
	    cg8d->flags |= CG8_COLOR_OVERLAY;
	}
	else cg8d->flags &= ~CG8_COLOR_OVERLAY;

	for (; active < cg8d->num_fbs && !found; active++)
	    if (found =
		(group == cg8d->fb[active].group)) {
		cg8d->mprp = cg8d->fb[active].mprp;
		cg8d->planes = PIX_GROUP (group) |
		    (cg8d->mprp.planes & PIX_ALL_PLANES);
		cg8d->active = active;
		pr->pr_depth = cg8d->fb[active].depth;
		switch (group)
		{
		    default:
           case PIXPG_8BIT_COLOR:
           case PIXPG_24BIT_COLOR:
               cmsize = 256;
               if ( cg8d->flags & CG8_PIP_PRESENT )cg8d->flags |= CG8_STOP_PIP;
               break;
           case PIXPG_OVERLAY:
               cmsize = 4;
               cg8d->flags &= ~CG8_STOP_PIP;
               break;
           case PIXPG_OVERLAY_ENABLE:
               cmsize = 0;
               cg8d->flags &= ~CG8_STOP_PIP;
               break;
           case PIXPG_VIDEO_ENABLE:
              cmsize = 0;
              if ( cg8d->flags & CG8_PIP_PRESENT )cg8d->flags |= CG8_STOP_PIP;
		      break;
		}
		(void) cg8_ioctl(pr, FBIOSCMSIZE, (caddr_t) &cmsize);
	    }
    }

    /* group is PIXPG_CURRNT here */
    if (!dont_set_planes) {
	cg8d->planes =
	    cg8d->mprp.planes =
	    cg8d->fb[cg8d->active].mprp.planes =
	    PIX_GROUP (group) | planes;
    }

    return 0;
}

#ifndef KERNEL
cg8_getattributes (pr, planesp)
    Pixrect        *pr;
    int            *planesp;

{
    if (planesp)
	*planesp = cg8_d (pr)->planes & PIX_ALL_PLANES;
    return 0;
}

#endif !KERNEL

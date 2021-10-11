#ifndef lint
static char        sccsid[] =      "@(#)gt_colormap.c	1.1 94/10/31 SMI";
#endif

/* Copyright 1989 by Sun Microsystems, Inc. */
/*
 * gt colormap and attribute functions.
 * putcolormap is also a kernel function, call by dtop_putcolormap.
 * putattributes is also a kernel function.
 */

#include <sys/ioctl.h>			
#include <pixrect/pixrect.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/gtvar.h>

#ifdef	KERNEL
#include "gt.h"
#if NGT > 0
#define	ioctl	gtioctl
#else
#define ioctl
#endif
#endif

gt_putcolormap(pr, index, count, red, green, blue)
Pixrect		*pr;
int		index, count;
unsigned char	*red, *green, *blue;
{
    struct gt_data	*gtd;
    struct fb_clut	fbclut;
    int			plane;
    
    gtd = (struct gt_data *) gt_d(pr);
    /* Extract the plane if necessary */


    if (PIX_ATTRGROUP(index)) {
	plane = PIX_ATTRGROUP(index) & PIX_GROUP_MASK;
	index &= PR_FORCE_UPDATE | PIX_ALL_PLANES;  
    }
    else
	plane = gtd->planes;
    
    if (PIX_ATTRGROUP(plane) == PIXPG_8BIT_COLOR)
	 fbclut.index = gtd->clut_id;
    else if (PIX_ATTRGROUP(plane) == PIXPG_24BIT_COLOR) {
	if (index & PR_FORCE_UPDATE)
	    fbclut.index = GT_CLUT_INDEX_24BIT;
	else
	    return(0);
    } else if ((PIX_ATTRGROUP(plane) == PIXPG_CURSOR) ||
	      (PIX_ATTRGROUP(plane) == PIXPG_CURSOR_ENABLE)){
	/* never post to cusor lut */
	return (0);
    }

    fbclut.flags = 0; 
#ifdef KERNEL
    fbclut.flags |= FB_KERNEL;		
#else
    /* If neither of these are set use the system default */
    if (index & PR_DONT_DEGAMMA)	
	fbclut.flags |= FB_NO_DEGAMMA;
    else if (index & PR_DEGAMMA) {
	fbclut.flags |= FB_DEGAMMA;
    }
#endif	
    fbclut.count  = count;
    fbclut.offset  = index & ~(PR_FORCE_UPDATE|PR_DONT_DEGAMMA|PR_DEGAMMA);
    fbclut.red	  = red;
    fbclut.green  = green;
    fbclut.blue	  = blue;
    return ioctl(gtd->fd, FB_CLUTPOST, (caddr_t) &fbclut, 0);

}

#ifndef	KERNEL	/* getcolormap and getattributes are not used by kernel */

gt_getcolormap(pr, index, count, red, green, blue)
Pixrect		*pr;
int		index, count;
unsigned char	*red, *green, *blue;
{
    struct gt_data	*gtd;
    struct fb_clut	fbclut;
    int			plane;
    
    gtd = (struct gt_data *)gt_d(pr);
    
    if (PIX_ATTRGROUP(index))
    {
	plane = PIX_ATTRGROUP(index) & PIX_GROUP_MASK;
	index &= PR_FORCE_UPDATE | PIX_ALL_PLANES;
    }
    else
	plane = gtd->planes;
    fbclut.flags = 0;    

    /* If neither of these are set use the system default */
    if (index & PR_DONT_DEGAMMA)	
	fbclut.flags |= FB_NO_DEGAMMA;
    else if (index & PR_DEGAMMA) {
	fbclut.flags |= FB_DEGAMMA;
    }

    if (PIX_ATTRGROUP(plane) == PIXPG_8BIT_COLOR)
	 fbclut.index = gtd->clut_id;
    else if (PIX_ATTRGROUP(plane) == PIXPG_24BIT_COLOR)
	fbclut.index = GT_CLUT_INDEX_24BIT;

     fbclut.count = count;
     fbclut.offset = index & ~(PR_DONT_DEGAMMA | PR_DEGAMMA | PR_FORCE_UPDATE);
     fbclut.red = red;
     fbclut.green = green;
     fbclut.blue = blue;

     return ioctl(gtd->fd, FB_CLUTREAD, (caddr_t) &fbclut, 0);	
}

gt_getattributes (pr, planesp)
    Pixrect        *pr;
    int            *planesp;

{
    if (planesp)
	*planesp = gt_d (pr)->planes & PIX_ALL_PLANES;
    return (0);

}
#endif !KERNEL

gt_putattributes (pr, planesp)
    Pixrect        *pr;
    u_int          *planesp;	       /* encoded group and plane mask */

{
    struct gt_data *gtd;	       
    u_int           planes,
		    group;
    int             dont_set_planes;
    static int	    mpg[] = { HKPP_MPG_WRITE_RGB, 0, 0,0, HKPP_MPG_WRITE_OV,
							HKPP_MPG_WRITE_Z};
    gtd = gt_d (pr);
    dont_set_planes = *planesp & PIX_DONT_SET_PLANES;
    planes = *planesp & PIX_ALL_PLANES;
    group = PIX_ATTRGROUP (*planesp);  /* extract the group part */

    if (group != PIXPG_CURRENT || group == PIXPG_INVALID) {

	int             found = 0;
	int             active = 0;
	for (; active < GT_NFBS && !found; active++)
	    if (found =
		(group == gtd->fb[active].group)) {
		gtd->mprp = gtd->fb[active].mprp;
		gtd->planes = PIX_GROUP (group) |
		    (gtd->mprp.planes & PIX_ALL_PLANES);
		gtd->active = active;
		pr->pr_depth = gtd->fb[active].depth;
		gtd->mpg_fcs = (gtd->mpg_fcs & HKPP_MPG_FCS_MASK); 
		gtd->mpg_fcs |= (mpg[active] & HKPP_MPG_WRITE_EN_MASK);
	    }
    }
    /* group is PIXPG_CURRENT here */
    if (!dont_set_planes) {
	gtd->planes =
	    gtd->mprp.planes =
	    gtd->fb[gtd->active].mprp.planes =
	    (gtd->planes & ~PIX_ALL_PLANES)| planes;
    }
    return (0);    
    
}

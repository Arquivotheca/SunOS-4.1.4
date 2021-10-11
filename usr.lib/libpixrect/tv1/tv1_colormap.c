#ifndef lint
static char Sccsid[] = "@(#)tv1_colormap.c	1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1988, Sun Microsystems, Inc.
 */

#ifndef KERNEL
#include <stdio.h>
#endif
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/pr_util.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/tv1var.h>

extern struct pixrectops tv1_vid_ops;
extern struct tv1_data* get_tv1_data();
void tv1_setEmuType();

/* ARGSUSED */
tv1_putcolormap (pr, index, count, red, green, blue)
    Pixrect        *pr;
    int             index,
                    count;
    unsigned char  *red,
                   *green,
                   *blue;
{
    int rtn = 0;

    if (get_tv1_data(pr)->active == -1) {
        rtn = pr_putcolormap(get_tv1_data(pr)->emufb, index, count, red, green, blue);
    }
    return (rtn);
}

#ifndef KERNEL

tv1_getcolormap (pr, index, count, red, green, blue)
    Pixrect        *pr;
    int             index,
                    count;
    unsigned char  *red,
                   *green,
                   *blue;

{
    int rtn = PIX_ERR;

    if (get_tv1_data(pr)->active == -1) {
        rtn = pr_getcolormap(get_tv1_data(pr)->emufb, index, count, red, green, blue);
    }
    return (rtn);
}

#endif !KERNEL

int tv1_nop()
{
    return 0;
}


tv1_putattributes (pr, planesp)
    Pixrect        *pr;
    u_int          *planesp;	       /* encoded group and plane mask */

{
    struct tv1_data *tv1d = get_tv1_data (pr);/* private data */
    int planes = *planesp & PIX_ALL_PLANES;
    int group = PIX_ATTRGROUP(*planesp);
    int dont_set_planes = *planesp & PIX_DONT_SET_PLANES;
    int rtn = 0;
    u_int          new_planesp;	 /* new encoded group and plane mask */
    if( !tv1d )
    {
#ifdef KERNEL	
	printf( "Kernel set_attributes could not get tv1_d\n" );
#else	
	fprintf( stderr, "set_attributes could not get tv1_d\n" );
#endif	
	return( -1 );
    }
    	
    /*
     * User is trying to set the group to something else. We'll see if the
     * group is supported.  If does, do as he wishes.
     */
    if (group != PIXPG_CURRENT || group == PIXPG_INVALID) {
	int active;

	for (active = TV1_NFBS - 1;
	     active >= 0 && group != tv1d->fb[active].group; active--)
		;
	tv1d->active = active;
	if (active >= 0) {
	    tv1d->mprp = tv1d->fb[active].mprp;
	    tv1d->planes = PIX_GROUP(group) | (tv1d->planes & PIX_ALL_PLANES);

	    /*  retrieve the depth, data, and ops vectors  */
	    switch( tv1d->fb[active].group )
	    {
	      case PIXPG_VIDEO:
		/*  Sadly, must special case this & not use setEmuType  */
		*(pr->pr_ops) = tv1_vid_ops;
#ifndef KERNEL /* no pr_video in kernel, so better not copy! */
		pr->pr_depth = tv1d->pr_video->pr_depth;
		pr->pr_data = tv1d->pr_video->pr_data;
#endif !KERNEL
		break;

	      case PIXPG_VIDEO_ENABLE:
		tv1_setEmuType( pr, tv1d->pr_video_enable );
		break;

	      default:
#ifdef KERNEL
		printf( "Kernel set invalid active plane group: %d\n",
		       tv1d->fb[active].group );
#else
		fprintf( stderr, "Invalid active plane group!!\n" );
#endif		
	    }
	}
    }

   /*
    *    We're emulating the bound device - set up ops, data, depth
    */
#ifdef KERNEL	/* Kernel doesn't have get attributes */
    new_planesp = *planesp;
#else
    if (dont_set_planes) {
	/* some of the bound devices (cgtwo) don't look at the
	 * "don't set planes" bit, so we do a get first.
         */
	pr_getattributes(tv1d->emufb, &new_planesp);
	new_planesp &= PIX_ALL_PLANES;
	new_planesp = PIX_GROUP(group) |
		      (new_planesp & PIX_ALL_PLANES) | PIX_DONT_SET_PLANES;
    } else {
	new_planesp = *planesp;
    }
#endif
    if (tv1d->active == -1 &&
	!((rtn = pr_putattributes(tv1d->emufb, &new_planesp))))
	tv1_setEmuType( pr, tv1d->emufb );

    else if (!dont_set_planes)
	tv1d->planes = 
	  tv1d->mprp.planes =
	    tv1d->fb[tv1d->active].mprp.planes = PIX_GROUP (group) | planes;

    return rtn;
}

#ifndef KERNEL
tv1_getattributes (pr, planesp)
    Pixrect        *pr;
    unsigned int   *planesp;

{
    if (get_tv1_data(pr)->active == -1)
	return (pr_getattributes(get_tv1_data(pr)->emufb, planesp));
    else if (planesp)
	*planesp = get_tv1_data(pr)->planes & PIX_ALL_PLANES;
    return 0;
}

#endif !KERNEL

/*  set up a tv1 pixrect to emulate another type  */
void
tv1_setEmuType( prTV, prEmu )
    Pixrect* prTV;	/*  Should be the tv1 pixrect to alter  */
    Pixrect* prEmu;	/*  Pixrect to emulate  */
{
    extern int tv1_ioctl();

    prTV->pr_depth = prEmu->pr_depth;
    prTV->pr_data = prEmu->pr_data;
    *(prTV->pr_ops) = *(prEmu->pr_ops);


    prTV->pr_ops->pro_putattributes = tv1_putattributes;
    prTV->pr_ops->pro_nop = tv1_ioctl;
#ifndef KERNEL
    prTV->pr_ops->pro_getattributes = tv1_getattributes;
    prTV->pr_ops->pro_region = tv1_region;
    prTV->pr_ops->pro_destroy = tv1_destroy;
#endif !KERNEL
}


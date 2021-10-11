
#ifndef lint
static char Sccsid[] = "@(#)tv1_ops.c	1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1988, Sun Microsystems, Inc.
 */

/* This file contains the ops vector for the SunVideo enable plane */

#include <sys/types.h>
#include <pixrect/pixrect.h>

extern int  tv1_nop();
	    tv1_putattributes();
	    tv1_putcolormap();
	    tv1_ioctl();
	    sup_rop(),
	    sup_nop(),
	    sup_putattributes(),
	    sup_putcolormap();

#ifndef KERNEL
extern int  tv1_destroy(),
	    tv1_getattributes(),
	    tv1_getcolormap(),
	    sup_stencil(),
	    sup_batchrop(),
	    sup_nop(),
	    sup_destroy(),
	    sup_get(),
	    sup_put(),
	    sup_vector(),
	    sup_getcolormap(),
	    sup_getattributes();

extern struct pixrect	*tv1_region(),
			*sup_region();

#endif

struct pixrectops tv1_vid_ops = 
{
    	tv1_nop,	/* pro_rop (these get filled in later) */
#ifndef KERNEL
	tv1_nop,	/* pro_stencil */
	tv1_nop,	/* pro_batchrop */
	tv1_ioctl,	/* pro_ioctl */
	tv1_destroy,	/* pro_destroy */
	tv1_nop,	/* pro_get */
	tv1_nop,	/* pro_put */
	tv1_nop,	/* pro_vector */
	tv1_region,
#endif !KERNEL
	tv1_putcolormap,	/* pro_putcolormap */
#ifndef KERNEL
	tv1_getcolormap,	/* pro_getcolormap */
#endif !KERNEL
	tv1_putattributes,
#ifndef KERNEL
	tv1_getattributes,
#endif !KERNEL
#ifdef KERNEL
	tv1_ioctl,	/* pro_ioctl */
#endif KERNEL
};

struct pixrectops tv1_en_ops =
{
    sup_rop,
#ifndef KERNEL
    sup_stencil,
    sup_batchrop,
    sup_nop,
    sup_destroy,
    sup_get,
    sup_put,
    sup_vector,
    sup_region,
#endif !KERNEL
    sup_putcolormap,
#ifndef KERNEL
    sup_getcolormap,
#endif !KERNEL
    sup_putattributes,
#ifndef KERNEL
    sup_getattributes,
#endif
#ifdef KERNEL
    sup_nop,
#endif KERNEL
};

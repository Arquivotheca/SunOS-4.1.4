
#ifndef lint
static char Sccsid[] = "@(#)tv1_ioctl.c	1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1988, Sun Microsystems, Inc.
 */


#include <sys/types.h>
#include <sun/fbio.h>
#include <sun/tvio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_make.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/tv1var.h>
#include <sys/errno.h>

extern struct tv1_data *get_tv1_data();

tv1_ioctl(pr, cmd, data)
    struct pixrect *pr;
    int cmd;
    int *data;
{
    switch(cmd) {
	case FBIOGPLNGRP:
	    return(tv1_get_planegroup(pr, data));
	    
	case FBIOAVAILPLNGRP:
	    return(tv1_available_plane_groups(pr, data));
	    
	default:
	    return(pr_ioctl(get_tv1_data(pr)->emufb, cmd, data));
    }
    
}

tv1_get_planegroup(pr, data)
    struct pixrect *pr;
    int *data;
{
    if (get_tv1_data(pr)->active != -1) {
	*data = PIX_ATTRGROUP(get_tv1_data(pr)->planes);
    } else {
	*data = pr_get_plane_group(get_tv1_data(pr)->emufb);
    }
    return(0);
}

tv1_available_plane_groups(pr, data)
    struct pixrect *pr;
    int *data;
{
    int gpsize;

    int maxgroups = 32, i;
    char groups[256];

    /* get vector for underlying fb, then add tv1 groups */
    gpsize = pr_available_plane_groups(get_tv1_data(pr)->emufb,
						  maxgroups, groups);
    if (PIXPG_VIDEO < maxgroups) {
	groups[PIXPG_VIDEO] = 1;
    }
    if (PIXPG_VIDEO_ENABLE < maxgroups)
	groups[PIXPG_VIDEO_ENABLE] = 1;
    if (PIXPG_VIDEO_ENABLE + 1 > gpsize) {
	gpsize = PIXPG_VIDEO_ENABLE + 1;
    }
    if (gpsize > maxgroups) {
	gpsize = maxgroups;
    }
    *data = 0;
    for (i=0; i < gpsize; i++) {
	if (groups[i]) {
	    *data |= MAKEPLNGRP(i);
	}
    }
    return(0);
}

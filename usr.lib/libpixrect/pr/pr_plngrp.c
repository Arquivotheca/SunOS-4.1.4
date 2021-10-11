#ifndef lint
static char sccsid[] = "@(#)pr_plngrp.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * Plane groups functions
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_planegroups.h>

#define MAX_PLANE_GROUPS 32

/*
 * Fill in vector of available plane groups
 */
pr_available_plane_groups(pr, maxgroups, groups)
	register Pixrect *pr;
	register int maxgroups;
	char *groups;
{
    /* clear out group vector */
    unsigned int    plnbitvec = 0;

#ifdef KERNEL
    bzero((caddr_t) groups, (u_int) maxgroups);
#else
    bzero(groups, (u_int) maxgroups);
#endif

    if (pr_ioctl(pr, FBIOAVAILPLNGRP, &plnbitvec) < 0) {
	if (pr->pr_depth == 8)
	    plnbitvec = MAKEPLNGRP(PIXPG_8BIT_COLOR);
	else if (pr->pr_depth == 1)
	    plnbitvec = MAKEPLNGRP(PIXPG_MONO);
	else
	    plnbitvec = 0;
    }

    if (plnbitvec) {
	unsigned int    bitmask;
	int             max = 0;

	/*
	 * bitmasking is always kludgy, but I cannot imagine the day of
	 * having more than 31 planegroups.
	 */
	if (maxgroups >= MAX_PLANE_GROUPS)
	    maxgroups = MAX_PLANE_GROUPS;
	bitmask = 1 << (maxgroups - 1);
	while (maxgroups-- > 0) {
	    /* a logical expression */
	    groups[maxgroups] = ((plnbitvec & bitmask) != 0);
	    if (!max && groups[maxgroups])
		max = maxgroups + 1;
	    bitmask >>= 1;
	}
	return max;
    }
    return 0;
}


/*
 * Get current plane group
 */
pr_get_plane_group(pr)
	register Pixrect *pr;
{
	int plngrp = 0;
	/* 
	 * If it's a cg4, grub group out of private data.
	 * This is ugly but it works in the kernel.
	 */
	if (!pr_ioctl(pr, FBIOGPLNGRP, &plngrp))
	    return plngrp;

	/* not a cg4, guess group from depth */
	switch (pr->pr_depth) {
	case 32:
	case 24:
		return PIXPG_24BIT_COLOR;

	case 8: 
		return PIXPG_8BIT_COLOR;

	case 1:
		return PIXPG_MONO;

	/* we don't know what's going on */
	default:
		return PIXPG_CURRENT;
	}
}

/*
 * Set current plane group
 */
/*ARGSUSED*/
void
pr_set_plane_group(pr, group)
	Pixrect *pr;
	int group;
{
    /*
     * This is a no-op unless the pixrect is a cg4 and we are setting the
     * group to something other than the current one.
     */
    char            gp[PIXPG_INVALID + 1];

    if (pr_available_plane_groups(pr, PIXPG_INVALID, gp) > 0 &&
	(gp[PIXPG_OVERLAY] || gp[PIXPG_VIDEO_ENABLE]) ||
	 gp[PIXPG_CURSOR_ENABLE]) {
	if ((group &= PIX_GROUP_MASK) != PIXPG_CURRENT &&
	    group != PIXPG_INVALID) {

	    int             planes;

	    planes = PIX_GROUP(group) | PIX_DONT_SET_PLANES;
	    pr_putattributes(pr, &planes);
	}
    }
}

/*
 * Set plane group and plane mask
 */
/* ARGSUSED */
void
pr_set_planes(pr, group, planes)
    Pixrect        *pr;
    int             planes;
    int             group;
{
    char            gp[PIXPG_INVALID + 1];
    if (pr_available_plane_groups(pr, PIXPG_INVALID, gp) > 0 &&
	(gp[PIXPG_OVERLAY] || gp[PIXPG_VIDEO_ENABLE] ) ||
	gp[PIXPG_CURSOR_ENABLE])
	planes = planes & PIX_ALL_PLANES | PIX_GROUP(group & PIX_GROUP_MASK); 

    pr_putattributes(pr, &planes);
}

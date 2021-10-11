#ifndef lint
static char sccsid[] = "@(#)demo_rubber_band.c  1.1 94/10/31 SMI";
#endif

/*
 ***********************************************************************
 *
 * demo_rubber_band.c
 *
 *	@(#)demo_rubber_band.c 1.4 90/10/23 08:35:18
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 *	Demo host program for Hawk shared memory simulation.
 *
 *	This module contains the routines to simulate rubber band lines
 *	using pixrect to the overlay planes of a CG9 frame buffer.
 *	This does not work on any other frame buffer and is a bit of
 *	a hack.
 *
 * 24-May-90 Scott R. Nelson	  Initial version.
 * 16-Jul-90 Scott R. Nelson	  Add override for non-CG9 machines.
 * 19-Jul-90 Scott R. Nelson	  Added comments.
 *
 ***********************************************************************
 */

#include <stdio.h>
#include <sys/types.h>
#include <pixrect/pixrect_hs.h>
#include "demo.h"



/*
 *==============================================================
 *
 * Procedure Hierarchy:
 *
 *	init_overlay
 *	    pr_open
 *	draw_box
 *	    hawk_stopped
 *	    pr_set_plane_group
 *	    pr_vector
 *
 *==============================================================
 */



static struct pixrect *screen;		/* Access to the screen */



/*
 *--------------------------------------------------------------
 *
 * init_overlay
 *
 *	Open the dialbox and prepare to use the dials.
 *
 *--------------------------------------------------------------
 */

void
init_overlay()
{
#if 0
    screen = pr_open("/dev/cgnine0");
    if (screen == NULL) {
	fprintf(stderr,
	    "Unable to open /dev/cgnine0.  No rubber-band overlays for you!\n");
	use_cg9_overlay = 0;
    }
#endif
    use_cg9_overlay = 0;

} /* End of init_overlay */



/*
 *--------------------------------------------------------------
 *
 * draw_box
 *
 *	Draw a box into the overlay planes.  If "on" is 1, turn the
 *	box on, else erase it.
 *
 *--------------------------------------------------------------
 */

void
draw_box(x, y, w, h, on)
    int x, y;				/* Upper left corner */
    int w, h;				/* Width and height */
    int on;				/* Flag to draw or erase */
{
    int set_clear;
    int x2, y2;				/* Lower right corner */

    if (!use_cg9_overlay)
	return;				/* Don't rubber-band */

    if (!hawk_stopped())
	return;				/* We can't draw when Hawk is drawing */

    x2 = x + w - 1;
    y2 = y + h - 1;

    if (on)
	set_clear = PIX_SET;
    else
	set_clear = PIX_CLR;

    /* Draw the lines */
    pr_set_plane_group(screen, PIXPG_OVERLAY);
    pr_vector(screen, x,  y,  x,  y2, PIX_CLR, 1);
    pr_vector(screen, x2, y,  x2, y2, PIX_CLR, 1);
    pr_vector(screen, x,  y,  x2, y,  PIX_CLR, 1);
    pr_vector(screen, x,  y2, x2, y2, PIX_CLR, 1);

    /* Enable the lines to be seen */
    pr_set_plane_group(screen, PIXPG_OVERLAY_ENABLE);
    pr_vector(screen, x,  y,  x,  y2, set_clear, 1);
    pr_vector(screen, x2, y,  x2, y2, set_clear, 1);
    pr_vector(screen, x,  y,  x2, y,  set_clear, 1);
    pr_vector(screen, x,  y2, x2, y2, set_clear, 1);

    /* Put the plane group back how it was */
    pr_set_plane_group(screen, PIXPG_24BIT_COLOR);
} /* End of draw_box */

/* End of demo_rubber_band.c */

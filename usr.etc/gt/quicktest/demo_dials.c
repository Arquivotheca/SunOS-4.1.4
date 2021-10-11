#ifndef lint
static char sccsid[] = "@(#)demo_dials.c  1.1 94/10/31 SMI";
#endif

/*
 ***********************************************************************
 *
 * demo_dials.c
 *
 *	@(#)demo_dials.c 1.6 90/09/27 10:43:49
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 *	Demo host program for Hawk shared memory simulation.
 *
 *	This module containts the routines to read from the dialbox.
 *
 *	Data from the control dials is as follows:
 *	First byte is 0x7B
 *	Second byte is the dial number 0-7
 *	Third and fourth bytes are zero
 *	Second 32-bit word is movement where 64 = one degree.
 *	Third 32-bit word is system seconds
 *	Fourth 32-bit word is system microseconds (in 1/100th second)
 *
 * 22-May-90 Scott R. Nelson	  Initial version.
 * 19-Jul-90 Scott R. Nelson	  Added relative mode for rotations and
 *				  added comments.
 *
 ***********************************************************************
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include "demo.h"



/*
 *==============================================================
 *
 * Procedure Hierarchy:
 *
 *	init_dials
 *	    open
 *	get_dial
 *	    read
 *	constrain_input
 *	    (*dial_ptr->dial_constraints)(dial_ptr)
 *	constrain_window_x
 *	constrain_window_y
 *	constrain_window_width
 *	constrain_window_height
 *
 *==============================================================
 */



/*
 *--------------------------------------------------------------
 *
 * init_dials
 *
 *	Open the dialbox and prepare to use the dials.
 *
 *--------------------------------------------------------------
 */

void
init_dials()
{
    if (!use_dials)
	return;				/* Don't use the dials */

    db_fd = open("/dev/dialbox", O_RDWR);
    if (db_fd < 0) {
	perror("Open failed for /dev/dialbox:");
	exit(1);
    }
} /* End of init_dials */



/*
 *--------------------------------------------------------------
 *
 * get_dial
 *
 *	Get dial input information.  The information specifies which
 *	dial it is for and contains relative movement information.
 *	This routine then uses the values in the dial data structure
 *	to get the desired effect.  For example, dial input for a
 *	rotation matrix wants a value between 0.0 and 2.0 * PI.
 *	Dial input for moving a window needs to stay on the screen
 *	and is affected by things such as window height and width.
 *
 *--------------------------------------------------------------
 */

static int foo;
void
get_dial()
{
    int buf[4];				/* Input buffer */
    int dial_num;			/* Number of current dial */
    dial_data *dial_ptr;		/* Current dial record */

    if (!use_dials)
	return;				/* Don't use the dials */

    if (read(db_fd, buf, 16) == 16) {
	if ((buf[0] & 0xff000000) != 0x7B000000) {
	    fprintf(stderr, "Error reading dial (0x%.8X)\n", buf[0]);
	    return;
	}

	/* Get the dial number */
	dial_num = (buf[0] >> 16) & 0xff;

	if (cur_dials[dial_num] == NULL)
	    return;			/* Nothing to do here */

	dial_ptr = cur_dials[dial_num];

	/* Add the relative dial movement */
	if (dial_ptr->flag & RELATIVE)
	    dial_ptr->current_abs = buf[1];
	else
	    dial_ptr->current_abs += buf[1];

	constrain_input(dial_ptr);

	(*dial_ptr->attr_update)();
#if 0
fprintf(stderr, "%d%c%c\r",dial_num,buf[1]<0 ? '-' : '+',(foo++ & 0x3f) + '@');
#endif
    }
    else {
	fprintf(stderr, "Error reading dial (short data)\n");
	return;
    }

} /* End of get_dial */



/*
 *--------------------------------------------------------------
 *
 * constrain_input
 *
 *	Assuming that the current_abs value has been updated, check
 *	for any limits or wrap conditions and update all numbers
 *	accordingly.
 *
 *--------------------------------------------------------------
 */

void
constrain_input(dial_ptr)
    dial_data *dial_ptr;		/* Pointer to dial input structure */
{
    /* Check for passing either lower or upper limits */
    if (dial_ptr->current_abs < dial_ptr->min_abs) {
	if (dial_ptr->flag & CLAMP)
	    dial_ptr->current_abs = dial_ptr->min_abs;
	else				/* Wrap around */
	    dial_ptr->current_abs += dial_ptr->max_abs - dial_ptr->min_abs;
    }
    else if (dial_ptr->current_abs > dial_ptr->max_abs) {
	if (dial_ptr->flag & CLAMP)
	    dial_ptr->current_abs = dial_ptr->max_abs;
	else				/* Wrap around */
	    dial_ptr->current_abs -= dial_ptr->max_abs - dial_ptr->min_abs;
    }

    /* Create the actual value to use */
    dial_ptr->current_value = (float) (dial_ptr->current_abs) *
	dial_ptr->mult_factor;

    /* Handle any constraints relative to other dials */
    if (dial_ptr->dial_constraints != NULL)
	(*dial_ptr->dial_constraints) (dial_ptr);
} /* End of constrain_input */



/*
 *--------------------------------------------------------------
 *
 * constrain_window_x
 *
 *--------------------------------------------------------------
 */

void
constrain_window_x(dial_ptr)
    dial_data *dial_ptr;		/* Pointer to dial input structure */
{
    if ((int) (dle_ptr->window_x.current_value +
	dle_ptr->window_width.current_value) >= screen_x) {
	/* Decrease window_x */
	dle_ptr->window_x.current_value =
	    (float) screen_x - dle_ptr->window_width.current_value;
	dle_ptr->window_x.current_abs =
	    (int) (dle_ptr->window_x.current_value /
	    dle_ptr->window_x.mult_factor);
    }
} /* End of constrain_window_x */



/*
 *--------------------------------------------------------------
 *
 * constrain_window_y
 *
 *--------------------------------------------------------------
 */

void
constrain_window_y(dial_ptr)
    dial_data *dial_ptr;		/* Pointer to dial input structure */
{
    if ((int) (dle_ptr->window_y.current_value +
	dle_ptr->window_height.current_value) >= screen_y) {
	/* Decrease window_y */
	dle_ptr->window_y.current_value =
	    (float) screen_y - dle_ptr->window_height.current_value;
	dle_ptr->window_y.current_abs =
	    (int) (dle_ptr->window_y.current_value /
	    dle_ptr->window_y.mult_factor);
    }
} /* End of constrain_window_y */



/*
 *--------------------------------------------------------------
 *
 * constrain_window_width
 *
 *--------------------------------------------------------------
 */

void
constrain_window_width(dial_ptr)
    dial_data *dial_ptr;		/* Pointer to dial input structure */
{
    if ((int) (dle_ptr->window_x.current_value +
	dle_ptr->window_width.current_value) >= screen_x) {
	/* Decrease width */
	dle_ptr->window_width.current_value =
	    (float) screen_x - dle_ptr->window_x.current_value;
	dle_ptr->window_width.current_abs =
	    (int) (dle_ptr->window_width.current_value /
	    dle_ptr->window_width.mult_factor);
    }
} /* End of constrain_window_width */



/*
 *--------------------------------------------------------------
 *
 * constrain_window_height
 *
 *--------------------------------------------------------------
 */

void
constrain_window_height(dial_ptr)
    dial_data *dial_ptr;		/* Pointer to dial input structure */
{
    if ((int) (dle_ptr->window_y.current_value +
	dle_ptr->window_height.current_value) >= screen_y) {
	/* Decrease height */
	dle_ptr->window_height.current_value =
	    (float) screen_y - dle_ptr->window_y.current_value;
	dle_ptr->window_height.current_abs =
	    (int) (dle_ptr->window_height.current_value /
	    dle_ptr->window_height.mult_factor);
    }
} /* End of constrain_window_height */

/* End of demo_dials.c */

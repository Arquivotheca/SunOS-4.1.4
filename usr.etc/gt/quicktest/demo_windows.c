#ifndef lint
static char sccsid[] = "@(#)demo_windows.c  1.1 94/10/31 SMI";
#endif

/*
 ***********************************************************************
 *
 * demo_windows.c
 *
 *	@(#)demo_windows.c 1.6 91/01/29 13:48:20
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 *	Demo host program for Hawk shared memory simulation.
 *
 *	This module contains the user interface code involved in modifying
 *	windows.
 *
 * 27-Sep-90 Scott R. Nelson	  Split off from demo_user_interf.c
 * 11-Oct-90 Scott R. Nelson	  Changes to window bounds instruction.
 * 29-Jan-91 Kevin C. Rushforth	  Fixed matrices for HIS 1.5
 *
 ***********************************************************************
 */

#include <stdio.h>
#include <curses.h>
#include "demo.h"



/*
 *==============================================================
 *
 * Procedure Hierarchy:
 *
 *	get_window
 *	    draw_box
 *	    draw_one_frame
 *	display_window
 *	    display_main_screen
 *	update_window
 *	fix_window_aspect
 *
 *==============================================================
 */



/*
 *--------------------------------------------------------------
 *
 * get_window
 *
 *	Get the window coordinates and size.
 *	Return when the user is done and types 'q' or ESC or CTRL-D.
 *
 *--------------------------------------------------------------
 */

void
get_window()
{
    Hk_window_boundary *wb;		/* Pointer to window info to get */
    dial_data *x, *y, *w, *h;		/* Structures to update */

    continuous_draw = 0;		/* Can't keep updating for this */

    wb = &(dle_ptr->p_dl_data->window_bounds);

    x = &dle_ptr->window_x;
    y = &dle_ptr->window_y;
    w = &dle_ptr->window_width;
    h = &dle_ptr->window_height;

    switch (input_ch) {
    case 'x':
	x->current_abs -= x->key_step;
	constrain_input(x);
	(*x->attr_update)();
	break;
    case 'X':
	x->current_abs += x->key_step;
	constrain_input(x);
	(*x->attr_update)();
	break;

    case 'y':
	y->current_abs -= y->key_step;
	constrain_input(y);
	(*y->attr_update)();
	break;
    case 'Y':
	y->current_abs += y->key_step;
	constrain_input(y);
	(*y->attr_update)();
	break;

    case 'w':
	w->current_abs -= w->key_step;
	constrain_input(w);
	(*w->attr_update)();
	break;
    case 'W':
	w->current_abs += w->key_step;
	constrain_input(w);
	(*w->attr_update)();
	break;

    case 'h':
	h->current_abs -= h->key_step;
	constrain_input(h);
	(*h->attr_update)();
	break;
    case 'H':
	h->current_abs += h->key_step;
	constrain_input(h);
	(*h->attr_update)();
	break;

    case 'F':
    case 'f':
	/* Go to full screen.  Rely on constraints to give reasonable values */
	x->current_abs -= x->key_step * screen_x;
	constrain_input(x);
	(*x->attr_update)();

	y->current_abs -= y->key_step * screen_y;
	constrain_input(y);
	(*y->attr_update)();

	w->current_abs += w->key_step * screen_x;
	constrain_input(w);
	(*w->attr_update)();

	h->current_abs += h->key_step * screen_y;
	constrain_input(h);
	(*h->attr_update)();

	break;

    case 'd':
	draw_one_frame();
	break;

    case 'q':
    case 'Q':
    case CTRL_D:
    case ESC:
	/* @@@@@ This should put the old window back to background first */
	draw_box(wb->xleft, wb->ytop, wb->width, wb->height, 0);
	pop_menu();
	return;				/* Don't draw box */

    case CTRL_C:
	done_with_program = 1;
	break;

    case CTRL_L:
	clear();
	refresh();
	(*display_current_screen[menu_sp])();
	break;

    default:
	break;
    } /* End of switch */

} /* End of get_window */



static Menu window_menu[] = {
    {"X, x", "Move right, left"},
    {"Y, y", "Move down, up"},
    {"W, w", "Make wider, narrower"},
    {"H, h", "Make taller, shorter"},
    {"f", "Make the window full screen"},
    {"d", "Draw current object once"},
    {"q", "return to main menu"},
    {"", ""},
};

static Menu window_dial_menu[] = {
    {"", "        Dials:"},
    {"", "( x_left ) ( y_top  )"},
    {"", "( width  ) ( height )"},
    {"", "(        ) (        )"},
    {"", "(        ) (        )"},
    {"", ""},
};



/*
 *--------------------------------------------------------------
 *
 * display_window
 *
 *	Display the menu that allows the windows to be moved.
 *
 *--------------------------------------------------------------
 */

void
display_window()
{
    Hk_window_boundary *wb;		/* Pointer to window info to display */

    wb = &(dle_ptr->p_dl_data->window_bounds);

    display_main_screen(window_menu, window_dial_menu, 5, 9);

    standout();
    move(5, 4);
    printw("Window:");
    standend();
    move(6, 7);
    printw("Position: %d, %d", wb->xleft, wb->ytop);
    move(7, 7);
    printw("Size: %d, %d", wb->width, wb->height);
    move(LINES - 1, 0);			/* Leave cursor at the bottom */
    REFRESH;
} /* End of display_window */



/*
 *--------------------------------------------------------------
 *
 * update_window
 *
 *--------------------------------------------------------------
 */

void
update_window()
{
    Hk_window_boundary *wb;		/* Pointer to window info to get */

    wb = &(dle_ptr->p_dl_data->window_bounds);

    continuous_draw = 0;		/* Can't keep updating here */

    /* Make the box go away */
    draw_box(wb->xleft, wb->ytop, wb->width, wb->height, 0);

    dle_ptr->p_dl_data->window_bounds.xleft = dle_ptr->window_x.current_value;
    dle_ptr->p_dl_data->window_bounds.ytop = dle_ptr->window_y.current_value;
    dle_ptr->p_dl_data->window_bounds.width = dle_ptr->window_width.current_value;
    dle_ptr->p_dl_data->window_bounds.height = dle_ptr->window_height.current_value;
    fix_window_aspect();

    /* Display the box */
    draw_box(wb->xleft, wb->ytop, wb->width, wb->height, 1);
} /* End of update_window */



/*
 *--------------------------------------------------------------
 *
 * fix_window_aspect
 *
 *	Adjust the X or Y scale to compensate for a non-square window.
 *	-1.0 to 1.0 should always be in the narrowest dimension.
 *
 *--------------------------------------------------------------
 */

void
fix_window_aspect()
{
    Hk_window_boundary *wb;
    float ratio;

    identity(dle_ptr->p_dl_data->view.vt);
    identity(dle_ptr->p_dl_data->left_view.vt);
    identity(dle_ptr->p_dl_data->right_view.vt);

    wb = &(dle_ptr->p_dl_data->window_bounds);
    if (wb->height > wb->width) {
	ratio = (float) wb->width / (float) wb->height;
	dle_ptr->p_dl_data->view.vt[HKM_1_1] = ratio;
	dle_ptr->p_dl_data->left_view.vt[HKM_1_1] = ratio;
	dle_ptr->p_dl_data->right_view.vt[HKM_1_1] = ratio;
    }
    else {
	ratio = (float) wb->height / (float) wb->width;
	dle_ptr->p_dl_data->view.vt[HKM_0_0] = ratio;
	dle_ptr->p_dl_data->left_view.vt[HKM_0_0] = ratio;
	dle_ptr->p_dl_data->right_view.vt[HKM_0_0] = ratio;
    }

    dle_ptr->p_dl_data->view.vt[HKM_2_3] =
        -(dle_ptr->perspective.current_value);
    dle_ptr->p_dl_data->view.vt[HKM_2_2] = 0.2;

    dle_ptr->p_dl_data->left_view.vt[HKM_2_3] =
        -(dle_ptr->perspective.current_value);
    dle_ptr->p_dl_data->left_view.vt[HKM_2_2] = 0.2;
    rot_y(-3.0, dle_ptr->mat.matrix);
    concat_mat(dle_ptr->p_dl_data->left_view.vt, dle_ptr->mat.matrix,
        dle_ptr->p_dl_data->left_view.vt);

    dle_ptr->p_dl_data->right_view.vt[HKM_2_3] =
        -(dle_ptr->perspective.current_value);
    dle_ptr->p_dl_data->right_view.vt[HKM_2_2] = 0.2;
    rot_y(3.0, dle_ptr->mat.matrix);
    concat_mat(dle_ptr->p_dl_data->right_view.vt, dle_ptr->mat.matrix,
        dle_ptr->p_dl_data->right_view.vt);

    dle_ptr->p_dl_data->tr_flags |= TRF_PREPARE_WINDOW;
} /* End of fix_window_aspect */

/* End of demo_windows.c */

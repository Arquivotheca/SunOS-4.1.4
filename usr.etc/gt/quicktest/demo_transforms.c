#ifndef lint
static char sccsid[] = "@(#)demo_transforms.c  1.1 94/10/31 SMI";
#endif

/*
 ***********************************************************************
 *
 * demo_transforms.c
 *
 *	@(#)demo_transforms.c 1.7 91/04/18 16:16:03
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 *	Demo host program for Hawk shared memory simulation.
 *
 *	This module contains user interface code involved with modifying
 *	transformation matrices.
 *
 * 27-Sep-90 Scott R. Nelson	  Split off from demo_user_interf.c
 *  4-Oct-90 Scott R. Nelson	  Added auto_rotate flag.
 * 29-Jan-91 Kevin C. Rushforth	  Fixed matrices for HIS 1.5
 * 18-Apr-91 Kevin C. Rushforth	  Added routine to print matrices.
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
 *	get_transforms
 *	    push_menu
 *	    draw_one_frame
 *	    pop_menu
 *	display_transforms
 *	    display_main_screen
 *	get_rot
 *	    constrain_input
 *	    draw_one_frame
 *	    pop_menu
 *	display_rot
 *	    display_main_screen
 *	update_x_rot
 *	update_y_rot
 *	update_z_rot
 *	get_scale
 *	    constrain_input
 *	    draw_one_frame
 *	    pop_menu
 *	display_scale
 *	    display_main_screen
 *	update_scale
 *	get_trans
 *	    constrain_input
 *	    draw_one_frame
 *	    pop_menu
 *	display_trans
 *	    display_main_screen
 *	update_trans
 *	get_persp
 *	    constrain_input
 *	    draw_one_frame
 *	    pop_menu
 *	display_persp
 *	    display_main_screen
 *	update_persp
 *	get_axis
 *	    constrain_input
 *	    draw_one_frame
 *	    pop_menu
 *	display_axis
 *	    display_main_screen
 *	update_x_rot_axis
 *	update_y_rot_axis
 *	update_z_rot_axis
 *	update_rot_speed
 *
 *==============================================================
 */



static void print_matrices();
static void dump_matrices();



static Menu main_dial_menu[] = {
    {"", "        Dials:"},
    {"", "( x_rot  ) (x_trans )"},
    {"", "( y_rot  ) (y_trans )"},
    {"", "( z_rot  ) (z_trans )"},
    {"", "( scale  ) ( persp  )"},
    {"", ""},
};



/*
 *--------------------------------------------------------------
 *
 * get_transforms
 *
 *	Get rotations, translations, scale and perspective from
 *	the keyboard.
 *
 *--------------------------------------------------------------
 */

void
get_transforms()
{
    switch (input_ch) {

    case 'r':
	push_menu(display_rot, get_rot);
	break;

    case 's':
	push_menu(display_scale, get_scale);
	break;

    case 't':
	push_menu(display_trans, get_trans);
	break;

    case 'p':
	push_menu(display_persp, get_persp);
	break;

    case 'R':
	reset_scale();
	reset_persp();
	reset_trans();
	reset_rot();
	break;

    case 'P':
	print_matrices();
	break;

    case 'D':
	continuous_draw = 1;
	break;

    case 'd':
	if (!continuous_draw)
	    draw_one_frame();
	continuous_draw = 0;
	break;

    case 'q':
    case 'Q':
    case CTRL_D:
    case ESC:
	pop_menu();
	break;

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
} /* End of get_transforms */



static Menu transform_menu[] = {
    {"r", "Rotate"},
    {"t", "Translate"},
    {"s", "Scale"},
    {"p", "Perspective"},
    {"R", "Reset all matrices"},
    {"P", "Print matrices"},
    {"d, D", "Draw current object once, continuously"},
    {"q", "return to main menu"},
    {"", ""},
};



/*
 *--------------------------------------------------------------
 *
 * display_transforms
 *
 *--------------------------------------------------------------
 */

void
display_transforms()
{
    standout();
    move(5, 4);
    printw("Transformations:");
    standend();
    display_main_screen(transform_menu, main_dial_menu, 5, 6);
    move(LINES - 1, 0);			/* Leave cursor at the bottom */
    REFRESH;
} /* End of display_transforms */



/*
 *--------------------------------------------------------------
 *
 * print_matrices
 *
 *--------------------------------------------------------------
 */

static void
print_matrices()
{
    clear();
    REFRESH;
    noraw();
    dump_matrices();
    raw();
    wait_for_space();
} /* End of print_matrices */



/*
 *--------------------------------------------------------------
 *
 * dump_matrices
 *
 *--------------------------------------------------------------
 */

static void
dump_matrices()
{
    Hk_dmatrix m;
    dl_data *d;
    int i;
    int indx;

    d = dle_ptr->p_dl_data;

    printf("/* Current View */\n");
    printf("set_attribute hk_view\n");
    printf(".fword %g, %g, %g, %g\n",
	d->view.xcenter, d->view.ycenter, d->view.xsize, d->view.ysize);
    printf(".matrix_pair <\\\n");
    for (i = 0; i < 4; i++) {
	indx = i * 4;
	printf("< %.12lf, %.12lf, %.12lf, %.12lf >",
	    d->view.vt[indx], d->view.vt[indx+1],
	    d->view.vt[indx+2], d->view.vt[indx+3]);
	if (i == 3)
	    printf(" >\n");
	else
	    printf(" ,\\\n");
    }


    printf("/* Current Modeling Transform */\n");

    identity(m);
    concat_mat(d->global_matrix.matrix, m, m);
    concat_mat(d->rot_axis_matrix.matrix, m, m);
    concat_mat(d->spin_matrix.matrix, m, m);
    concat_mat(d->trans_matrix.matrix, m, m);
    concat_mat(d->rot_matrix.matrix, m, m);
    concat_mat(d->scale_matrix.matrix, m, m);

    printf("set_attribute hk_load_gmt\n.matrix <\\\n");
    for (i = 0; i < 4; i++) {
	indx = i * 4;
	printf("< %.12lf, %.12lf, %.12lf, %.12lf >",
	    m[indx], m[indx+1], m[indx+2], m[indx+3]);
	if (i == 3)
	    printf(" >\n");
	else
	    printf(" ,\\\n");
    }

} /* End of dump_matrices */



/*
 *--------------------------------------------------------------
 *
 * get_rot
 *
 *	Get the X, Y, Z rotation from the keyboard
 *
 *--------------------------------------------------------------
 */

void
get_rot()
{
    dial_data *x, *y, *z;		/* Structures to update */

    x = &dle_ptr->x_rot;
    y = &dle_ptr->y_rot;
    z = &dle_ptr->z_rot;

    switch (input_ch) {
    case 'x':
	if (x->flag & RELATIVE)
	    x->current_abs = -x->key_step;
	else
	    x->current_abs -= x->key_step;
	constrain_input(x);
	(*x->attr_update)();
	break;

    case 'y':
	if (y->flag & RELATIVE)
	    y->current_abs = -y->key_step;
	else
	    y->current_abs -= y->key_step;
	constrain_input(y);
	(*y->attr_update)();
	break;

    case 'z':
	if (z->flag & RELATIVE)
	    z->current_abs = -z->key_step;
	else
	    z->current_abs -= z->key_step;
	constrain_input(z);
	(*z->attr_update)();
	break;

    case 'X':
	if (x->flag & RELATIVE)
	    x->current_abs = x->key_step;
	else
	    x->current_abs += x->key_step;
	constrain_input(x);
	(*x->attr_update)();
	break;

    case 'Y':
	if (y->flag & RELATIVE)
	    y->current_abs = y->key_step;
	else
	    y->current_abs += y->key_step;
	constrain_input(y);
	(*y->attr_update)();
	break;

    case 'Z':
	if (z->flag & RELATIVE)
	    z->current_abs = z->key_step;
	else
	    z->current_abs += z->key_step;
	constrain_input(z);
	(*z->attr_update)();
	break;

    case 'R':
    case 'r':
	reset_rot();
	break;

    case '+':
	x->key_step = (int) ((float) (x->key_step) * 1.25);
	y->key_step = (int) ((float) (y->key_step) * 1.25);
	z->key_step = (int) ((float) (z->key_step) * 1.25);
	break;
    case '-':
	x->key_step = (int) ((float) (x->key_step) * 0.8);
	y->key_step = (int) ((float) (y->key_step) * 0.8);
	z->key_step = (int) ((float) (z->key_step) * 0.8);
	break;

    case 'D':
	continuous_draw = 1;
	break;

    case 'd':
	if (!continuous_draw)
	    draw_one_frame();
	continuous_draw = 0;
	break;

    case 'q':
    case 'Q':
    case CTRL_D:
    case ESC:
	pop_menu();
	return;

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

} /* End of get_rot */



static Menu xyz_menu[] = {
    {"X, x", "Increase, decrease X by step"},
    {"Y, y", "Increase, decrease Y by step"},
    {"Z, z", "Increase, decrease Z by step"},
    {"r", "Reset XYZ to 0.0"},
    {"+, -", "Increase, decrease current step size"},
    {"d, D", "Draw current object once, continuously"},
    {"", ""},
};



/*
 *--------------------------------------------------------------
 *
 * display_rot
 *
 *	Display the menu that allows rotation values to be input.
 *
 *--------------------------------------------------------------
 */

void
display_rot()
{
    display_main_screen(xyz_menu, main_dial_menu, 5, 11);

    standout();
    move(5, 4);
    printw("Rotate:");
    standend();
    move(6, 7);
    printw("X: %g ", dle_ptr->x_rot.current_value);
    move(7, 7);
    printw("Y: %g ", dle_ptr->y_rot.current_value);
    move(8, 7);
    printw("Z: %g ", dle_ptr->z_rot.current_value);
    move(9, 7);
    printw("step: %g ", (float) dle_ptr->x_rot.key_step *
	(float) dle_ptr->x_rot.mult_factor);
    move(LINES - 1, 0);			/* Leave cursor at the bottom */
    REFRESH;
} /* End of display_rot */



/*
 *--------------------------------------------------------------
 *
 * update_x_rot
 *
 *--------------------------------------------------------------
 */

void
update_x_rot()
{
    rot_x(dle_ptr->x_rot.current_value, dle_ptr->mat.matrix);
    concat_mat(dle_ptr->p_dl_data->rot_matrix.matrix, dle_ptr->mat.matrix,
	dle_ptr->p_dl_data->rot_matrix.matrix);
} /* End of update_x_rot */



/*
 *--------------------------------------------------------------
 *
 * update_y_rot
 *
 *--------------------------------------------------------------
 */

void
update_y_rot()
{
    rot_y(dle_ptr->y_rot.current_value, dle_ptr->mat.matrix);
    concat_mat(dle_ptr->p_dl_data->rot_matrix.matrix, dle_ptr->mat.matrix,
	dle_ptr->p_dl_data->rot_matrix.matrix);
} /* End of update_y_rot */



/*
 *--------------------------------------------------------------
 *
 * update_z_rot
 *
 *--------------------------------------------------------------
 */

void
update_z_rot()
{
    rot_z(dle_ptr->z_rot.current_value, dle_ptr->mat.matrix);
    concat_mat(dle_ptr->p_dl_data->rot_matrix.matrix, dle_ptr->mat.matrix,
	dle_ptr->p_dl_data->rot_matrix.matrix);
} /* End of update_z_rot */



/*
 *--------------------------------------------------------------
 *
 * get_scale
 *
 *	Get the scale from the keyboard
 *
 *--------------------------------------------------------------
 */

void
get_scale()
{
    dial_data *s;

    s = &dle_ptr->scale;

    switch (input_ch) {
    case 'x':
    case 's':
	s->current_abs -= s->key_step;
	constrain_input(s);
	(*s->attr_update)();
	break;
    case 'X':
    case 'S':
	s->current_abs += s->key_step;
	constrain_input(s);
	(*s->attr_update)();
	break;
    case '+':
	s->key_step = (int) ((float) (s->key_step) * 1.25);
	break;
    case '-':
	s->key_step = (int) ((float) (s->key_step) * 0.8);
	break;

    case 'D':
	continuous_draw = 1;
	break;

    case 'd':
	if (!continuous_draw)
	    draw_one_frame();
	continuous_draw = 0;
	break;

    case 'R':
    case 'r':
	reset_scale();
	break;

    case 'q':
    case 'Q':
    case CTRL_D:
    case ESC:
	pop_menu();
	return;

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

} /* End of get_scale */



static Menu scale_menu[] = {
    {"S, s", "Increase, decrease by step"},
    {"+, -", "Increase, decrease current step size"},
    {"R", "Reset scale to 1.0"},
    {"d, D", "Draw current object once, continuously"},
    {"q", "return to main menu"},
    {"", ""},
};



/*
 *--------------------------------------------------------------
 *
 * display_scale
 *
 *	Display the menu that allows the scale to be input.
 *
 *--------------------------------------------------------------
 */

void
display_scale()
{
    display_main_screen(scale_menu, main_dial_menu, 5, 9);

    standout();
    move(5, 4);
    printw("Scale:");
    standend();
    move(6, 7);
    printw("Value: %g ", dle_ptr->scale.current_value);
    move(7, 7);
    printw("step: %g ", (float) dle_ptr->scale.key_step *
	(float) dle_ptr->scale.mult_factor);
    move(LINES - 1, 0);			/* Leave cursor at the bottom */
    REFRESH;
} /* End of display_scale */



/*
 *--------------------------------------------------------------
 *
 * update_scale
 *
 *--------------------------------------------------------------
 */

void
update_scale()
{
    scale_xyz(dle_ptr->scale.current_value, dle_ptr->scale.current_value,
	dle_ptr->scale.current_value, dle_ptr->p_dl_data->scale_matrix.matrix);
    dle_ptr->p_dl_data->scale_matrix.scale = dle_ptr->scale.current_value;
} /* End of update_scale */



/*
 *--------------------------------------------------------------
 *
 * get_trans
 *
 *	Get the X, Y, Z transation from the keyboard
 *
 *--------------------------------------------------------------
 */

void
get_trans()
{
    dial_data *x, *y, *z;		/* Structures to update */

    x = &dle_ptr->x_trans;
    y = &dle_ptr->y_trans;
    z = &dle_ptr->z_trans;

    switch (input_ch) {
    case 'x':
	if (x->flag & RELATIVE)
	    x->current_abs = -x->key_step;
	else
	    x->current_abs -= x->key_step;
	constrain_input(x);
	(*x->attr_update)();
	break;

    case 'y':
	if (y->flag & RELATIVE)
	    y->current_abs = -y->key_step;
	else
	    y->current_abs -= y->key_step;
	constrain_input(y);
	(*y->attr_update)();
	break;

    case 'z':
	if (z->flag & RELATIVE)
	    z->current_abs = -z->key_step;
	else
	    z->current_abs -= z->key_step;
	constrain_input(z);
	(*z->attr_update)();
	break;

    case 'X':
	if (x->flag & RELATIVE)
	    x->current_abs = x->key_step;
	else
	    x->current_abs += x->key_step;
	constrain_input(x);
	(*x->attr_update)();
	break;

    case 'Y':
	if (y->flag & RELATIVE)
	    y->current_abs = y->key_step;
	else
	    y->current_abs += y->key_step;
	constrain_input(y);
	(*y->attr_update)();
	break;

    case 'Z':
	if (z->flag & RELATIVE)
	    z->current_abs = z->key_step;
	else
	    z->current_abs += z->key_step;
	constrain_input(z);
	(*z->attr_update)();
	break;

    case 'R':
    case 'r':
	reset_trans();
	break;

    case '+':
	x->key_step = (int) ((float) (x->key_step) * 1.25);
	y->key_step = (int) ((float) (y->key_step) * 1.25);
	z->key_step = (int) ((float) (z->key_step) * 1.25);
	break;
    case '-':
	x->key_step = (int) ((float) (x->key_step) * 0.8);
	y->key_step = (int) ((float) (y->key_step) * 0.8);
	z->key_step = (int) ((float) (z->key_step) * 0.8);
	break;

    case 'D':
	continuous_draw = 1;
	break;

    case 'd':
	if (!continuous_draw)
	    draw_one_frame();
	continuous_draw = 0;
	break;

    case 'q':
    case 'Q':
    case CTRL_D:
    case ESC:
	pop_menu();
	return;

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

} /* End of get_trans */



/*
 *--------------------------------------------------------------
 *
 * display_trans
 *
 *	Display the menu that allows translation values to be input.
 *
 *--------------------------------------------------------------
 */

void
display_trans()
{
    display_main_screen(xyz_menu, main_dial_menu, 5, 11);

    standout();
    move(5, 4);
    printw("Translate:");
    standend();
    move(6, 7);
    printw("X: %g ", dle_ptr->x_trans.current_value);
    move(7, 7);
    printw("Y: %g ", dle_ptr->y_trans.current_value);
    move(8, 7);
    printw("Z: %g ", dle_ptr->z_trans.current_value);
    move(9, 7);
    printw("step: %g ", (float) dle_ptr->x_trans.key_step *
	(float) dle_ptr->x_trans.mult_factor);
    move(LINES - 1, 0);			/* Leave cursor at the bottom */
    REFRESH;
} /* End of display_trans */



/*
 *--------------------------------------------------------------
 *
 * update_trans
 *
 *--------------------------------------------------------------
 */

void
update_trans()
{
    translate(dle_ptr->x_trans.current_value, dle_ptr->y_trans.current_value,
	dle_ptr->z_trans.current_value, dle_ptr->p_dl_data->trans_matrix.matrix);
} /* End of update_trans */



/*
 *--------------------------------------------------------------
 *
 * get_persp
 *
 *	Get the perspective from the keyboard
 *
 *--------------------------------------------------------------
 */

void
get_persp()
{
    dial_data *p;

    p = &dle_ptr->perspective;

    switch (input_ch) {
    case 'x':
    case 'p':
	p->current_abs -= p->key_step;
	constrain_input(p);
	(*p->attr_update)();
	break;
    case 'X':
    case 'P':
	p->current_abs += p->key_step;
	constrain_input(p);
	(*p->attr_update)();
	break;
    case '+':
	p->key_step = (int) ((float) (p->key_step) * 1.25);
	break;
    case '-':
	p->key_step = (int) ((float) (p->key_step) * 0.8);
	break;

    case 'D':
	continuous_draw = 1;
	break;

    case 'd':
	if (!continuous_draw)
	    draw_one_frame();
	continuous_draw = 0;
	break;

    case 'R':
    case 'r':
	reset_persp();
	break;

    case 'q':
    case 'Q':
    case CTRL_D:
    case ESC:
	pop_menu();
	return;

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

} /* End of get_persp */



static Menu persp_menu[] = {
    {"P, p", "Increase, decrease by step"},
    {"+, -", "Increase, decrease current step size"},
    {"R", "Reset perspective to 0.0"},
    {"d, D", "Draw current object once, continuously"},
    {"q", "return to main menu"},
    {"", ""},
};



/*
 *--------------------------------------------------------------
 *
 * display_persp
 *
 *	Display the menu that allows perspective to be input.
 *
 *--------------------------------------------------------------
 */

void
display_persp()
{
    display_main_screen(persp_menu, main_dial_menu, 5, 9);

    standout();
    move(5, 4);
    printw("Perspective:");
    standend();
    move(6, 7);
    printw("Value: %g ", dle_ptr->perspective.current_value);
    move(7, 7);
    printw("step: %g ", (float) dle_ptr->perspective.key_step *
	(float) dle_ptr->perspective.mult_factor);
    move(LINES - 1, 0);			/* Leave cursor at the bottom */
    REFRESH;
} /* End of display_persp */



/*
 *--------------------------------------------------------------
 *
 * update_persp
 *
 *	Same as fix_window_aspect, but without the TRF_PREPARE_WINDOW
 *
 *--------------------------------------------------------------
 */

void
update_persp()
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
} /* End of update_persp */



/*
 *--------------------------------------------------------------
 *
 * get_axis
 *
 *	Get rotation axis and speed values.  Return when the user is
 *	done and types 'q' or ESC or CTRL-D.
 *
 *--------------------------------------------------------------
 */

void
get_axis()
{
    dial_data *x, *y, *z, *s;		/* Structures to update */

    x = &dle_ptr->x_rot_axis;
    y = &dle_ptr->y_rot_axis;
    z = &dle_ptr->z_rot_axis;
    s = &dle_ptr->rot_speed;

    switch (input_ch) {
    case 'x':
	x->current_abs = -x->key_step;	/* Relative */
	constrain_input(x);
	(*x->attr_update)();
	break;

    case 'y':
	y->current_abs = -y->key_step;	/* Relative */
	constrain_input(y);
	(*y->attr_update)();
	break;

    case 'z':
	z->current_abs = -z->key_step;	/* Relative */
	constrain_input(z);
	(*z->attr_update)();
	break;

    case 'X':
	x->current_abs = x->key_step;	/* Relative */
	constrain_input(x);
	(*x->attr_update)();
	break;

    case 'Y':
	y->current_abs = y->key_step;	/* Relative */
	constrain_input(y);
	(*y->attr_update)();
	break;

    case 'Z':
	z->current_abs = z->key_step;	/* Relative */
	constrain_input(z);
	(*z->attr_update)();
	break;

    case 's':
	s->current_abs -= s->key_step;	/* Absolute */
	constrain_input(s);
	(*s->attr_update)();
	auto_rotate = 1;
	break;

    case 'S':
	s->current_abs += s->key_step;	/* Absolute */
	constrain_input(s);
	(*s->attr_update)();
	auto_rotate = 1;
	break;

    case 'R':
    case 'r':
	reset_axis();
	break;

    case 'A':				/* Turn auto-rotation on and off */
    case 'a':
	cycle_forward(auto_rotate, TRUE);
	break;

    case '+':
	x->key_step = (int) ((float) (x->key_step) * 1.25);
	y->key_step = (int) ((float) (y->key_step) * 1.25);
	z->key_step = (int) ((float) (z->key_step) * 1.25);
	s->key_step = (int) ((float) (s->key_step) * 1.25);
	break;
    case '-':
	x->key_step = (int) ((float) (x->key_step) * 0.8);
	y->key_step = (int) ((float) (y->key_step) * 0.8);
	z->key_step = (int) ((float) (z->key_step) * 0.8);
	s->key_step = (int) ((float) (s->key_step) * 0.8);
	break;

    case 'D':
	continuous_draw = 1;
	break;

    case 'd':
	if (!continuous_draw)
	    draw_one_frame();
	continuous_draw = 0;
	break;

    case 'q':
    case 'Q':
    case CTRL_D:
    case ESC:
	pop_menu();
	return;

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

} /* End of get_axis */



static Menu axis_menu[] = {
    {"X, x", "Increase, decrease X by step"},
    {"Y, y", "Increase, decrease Y by step"},
    {"Z, z", "Increase, decrease Z by step"},
    {"S, s", "Increase, decrease speed"},
    {"A, a", "Turn on, off auto-rotate"},
    {"R", "Reset axis and speed to 0"},
    {"+, -", "Increase, decrease current step sizes"},
    {"d, D", "Draw current object once, continuously"},
    {"q", "Return to main menu"},
    {"", ""},
};

static Menu axis_dial_menu[] = {
    {"", "        Dials:"},
    {"", "( x_rot  ) (        )"},
    {"", "( y_rot  ) (        )"},
    {"", "( z_rot  ) (        )"},
    {"", "( speed  ) (        )"},
    {"", ""},
};



/*
 *--------------------------------------------------------------
 *
 * display_axis
 *
 *	Display the menu that allows the rotation axis to be moved.
 *
 *--------------------------------------------------------------
 */

void
display_axis()
{
    display_main_screen(axis_menu, axis_dial_menu, 5, 12);

    standout();
    move(5, 4);
    printw("Rotation axis:");
    standend();
    move(6, 7);
    printw("X: %g ", dle_ptr->x_rot_axis.current_value);
    move(7, 7);
    printw("Y: %g ", dle_ptr->y_rot_axis.current_value);
    move(8, 7);
    printw("Z: %g ", dle_ptr->z_rot_axis.current_value);
    move(9, 7);
    printw("Speed: %g ", dle_ptr->rot_speed.current_value);
    move(10, 7);
    printw("Auto-rotate: %s", (auto_rotate == FALSE) ? "off" : "on");
    move(11, 7);
    printw("step: %g ", (float) dle_ptr->x_rot_axis.key_step *
	(float) dle_ptr->x_trans.mult_factor);
    move(LINES - 1, 0);			/* Leave cursor at the bottom */
    REFRESH;
} /* End of display_axis */



/*
 *--------------------------------------------------------------
 *
 * update_x_rot_axis
 *
 *--------------------------------------------------------------
 */

void
update_x_rot_axis()
{
    rot_x(dle_ptr->x_rot_axis.current_value, dle_ptr->mat.matrix);
    concat_mat(dle_ptr->p_dl_data->rot_axis_matrix.matrix, dle_ptr->mat.matrix,
	dle_ptr->p_dl_data->rot_axis_matrix.matrix);
} /* End of update_x_rot_axis */



/*
 *--------------------------------------------------------------
 *
 * update_y_rot_axis
 *
 *--------------------------------------------------------------
 */

void
update_y_rot_axis()
{
    rot_y(dle_ptr->y_rot_axis.current_value, dle_ptr->mat.matrix);
    concat_mat(dle_ptr->p_dl_data->rot_axis_matrix.matrix, dle_ptr->mat.matrix,
	dle_ptr->p_dl_data->rot_axis_matrix.matrix);
} /* End of update_y_rot_axis */



/*
 *--------------------------------------------------------------
 *
 * update_z_rot_axis
 *
 *--------------------------------------------------------------
 */

void
update_z_rot_axis()
{
    rot_z(dle_ptr->z_rot_axis.current_value, dle_ptr->mat.matrix);
    concat_mat(dle_ptr->p_dl_data->rot_axis_matrix.matrix, dle_ptr->mat.matrix,
	dle_ptr->p_dl_data->rot_axis_matrix.matrix);
} /* End of update_z_rot_axis */



/*
 *--------------------------------------------------------------
 *
 * update_rot_speed
 *
 *--------------------------------------------------------------
 */

void
update_rot_speed()
{
    auto_rotate = 1;			/* Turn on auto-rotation */
} /* End of update_rot_speed */



/*
 *--------------------------------------------------------------
 *
 * reset_scale
 *
 *	Reset the current scale matrix to 1.0
 *
 *--------------------------------------------------------------
 */

void
reset_scale()
{
    identity(dle_ptr->p_dl_data->scale_matrix.matrix);
    dle_ptr->p_dl_data->scale_matrix.scale = 1.0;
    dle_ptr->scale.current_value = 1.0;
    dle_ptr->scale.current_abs = DIAL_ONE_ROTATION * 2;
} /* End of reset_scale */



/*
 *--------------------------------------------------------------
 *
 * reset_persp
 *
 *	Reset the current perspective matrix to 0.0
 *
 *--------------------------------------------------------------
 */

void
reset_persp()
{
    identity(dle_ptr->p_dl_data->global_matrix.matrix);
    dle_ptr->p_dl_data->global_matrix.scale = 1.0;
    dle_ptr->p_dl_data->view.vt[HKM_2_2] = 0.2;
    dle_ptr->p_dl_data->view.vt[HKM_2_3] = 0.0;
    dle_ptr->perspective.current_value = 0.0;
    dle_ptr->perspective.current_abs = 0;
} /* End of reset_persp */



/*
 *--------------------------------------------------------------
 *
 * reset_trans
 *
 *	Reset the current translation matrix to 0.0
 *
 *--------------------------------------------------------------
 */

void
reset_trans()
{
    identity(dle_ptr->p_dl_data->trans_matrix.matrix);
    dle_ptr->p_dl_data->trans_matrix.scale = 1.0;
    dle_ptr->x_trans.current_value = 0.0;
    dle_ptr->x_trans.current_abs = 0;
    dle_ptr->y_trans.current_value = 0.0;
    dle_ptr->y_trans.current_abs = 0;
    dle_ptr->z_trans.current_value = 0.0;
    dle_ptr->z_trans.current_abs = 0;
} /* End of reset_trans */



/*
 *--------------------------------------------------------------
 *
 * reset_rot
 *
 *	Reset the current rotation matrix to 0.0
 *
 *--------------------------------------------------------------
 */

void
reset_rot()
{
    identity(dle_ptr->p_dl_data->rot_matrix.matrix);
    dle_ptr->p_dl_data->rot_matrix.scale = 1.0;
    dle_ptr->x_rot.current_value = 0.0;
    dle_ptr->x_rot.current_abs = 0.0;
    dle_ptr->y_rot.current_value = 0.0;
    dle_ptr->y_rot.current_abs = 0.0;
    dle_ptr->z_rot.current_value = 0.0;
    dle_ptr->z_rot.current_abs = 0.0;
} /* End of reset_rot */



/*
 *--------------------------------------------------------------
 *
 * reset_axis
 *
 *	Reset the current spin and rotation axis matrix to 0.0
 *
 *--------------------------------------------------------------
 */

void
reset_axis()
{
    identity(dle_ptr->p_dl_data->rot_axis_matrix.matrix);
    dle_ptr->p_dl_data->rot_axis_matrix.scale = 1.0;
    identity(dle_ptr->p_dl_data->spin_matrix.matrix);
    dle_ptr->p_dl_data->spin_matrix.scale = 1.0;
    dle_ptr->x_rot_axis.current_value = 0.0;
    dle_ptr->x_rot_axis.current_abs = 0.0;
    dle_ptr->y_rot_axis.current_value = 0.0;
    dle_ptr->y_rot_axis.current_abs = 0.0;
    dle_ptr->z_rot_axis.current_value = 0.0;
    dle_ptr->z_rot_axis.current_abs = 0.0;
    dle_ptr->rot_speed.current_value = 0.0;
    dle_ptr->rot_speed.current_abs = 0.0;
} /* End of reset_axis */

/* End of demo_transforms.c */

#ifndef lint
static char sccsid[] = "@(#)demo_colors.c  1.1 94/10/31 SMI";
#endif

/*
 ***********************************************************************
 *
 * demo_colors.c
 *
 *	@(#)demo_colors.c 1.8 91/04/09 16:06:51
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 *	Demo host program for Hawk shared memory simulation.
 *
 *	This module contains the user interface code involved in setting
 *	colors and surface properties.
 *
 * 27-Sep-90 Scott R. Nelson	  Split off from demo_user_interf.c
 *  4-Oct-90 Scott R. Nelson	  Added ability to cycle attributes in
 *				  both directions.
 * 15-Jan-91 Scott R. Nelson	  Changes to make HIS 1.5 work.
 *  9-Apr-91 John M. Perry	  Store backgound color in DGA page.
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
 *	get_bg_color
 *	    draw_one_frame
 *	display_bg_color
 *	update_bg_color
 *	get_line_color
 *	    draw_one_frame
 *	display_line_color
 *	update_obj_color
 *	get_surf_props
 *	display_surf_prop
 *	update_surf_prop
 *	get_depth_cue
 *	display_depth_cue
 *	update_depth_cue
 *
 *==============================================================
 */



/*
 *--------------------------------------------------------------
 *
 * get_bg_color
 *
 *	Get the background color from the keyboard.  Return when the
 *	user is done and types 'q' or ESC or CTRL-D.
 *
 *--------------------------------------------------------------
 */

void
get_bg_color()
{
    dial_data *r, *g, *b;		/* Structures to update */
    int color_change;			/* Color change flag */
    unsigned bg_color;			/* Packed background color */

    r = &dle_ptr->bg_r;
    g = &dle_ptr->bg_g;
    b = &dle_ptr->bg_b;
    color_change = 0;

    switch (input_ch) {
    case 'r':
	r->current_abs -= r->key_step;
	constrain_input(r);
	(*r->attr_update)();
	color_change = 1;
	break;

    case 'g':
	g->current_abs -= g->key_step;
	constrain_input(g);
	(*g->attr_update)();
	color_change = 1;
	break;

    case 'b':
	b->current_abs -= b->key_step;
	constrain_input(b);
	(*b->attr_update)();
	color_change = 1;
	break;

    case 'R':
	r->current_abs += r->key_step;
	constrain_input(r);
	(*r->attr_update)();
	color_change = 1;
	break;

    case 'G':
	g->current_abs += g->key_step;
	constrain_input(g);
	(*g->attr_update)();
	color_change = 1;
	break;

    case 'B':
	b->current_abs += b->key_step;
	constrain_input(b);
	(*b->attr_update)();
	color_change = 1;
	break;

    case 'F':				/* Cycle back thru fast clear sets */
	cycle_back(dle_ptr->p_dl_data->fast_clear_set, 4);
	update_bg_color();
	break;

    case 'f':				/* Cycle forward thru fast clear sets */
	cycle_forward(dle_ptr->p_dl_data->fast_clear_set, 4);
	update_bg_color();
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

} /* End of get_bg_color */



static Menu bg_menu[] = {
    {"R, r", "Increase, decrease R by 1"},
    {"G, g", "Increase, decrease G by 1"},
    {"B, b", "Increase, decrease B by 1"},
    {"F, f", "Turn on, off fast clear"},
    {"d, D", "Draw current object once, continuously"},
    {"q", "return to main menu"},
    {"", ""},
};

static Menu color_dial_menu[] = {
    {"", "        Dials:"},
    {"", "(  red   ) (x_trans )"},
    {"", "( green  ) (y_trans )"},
    {"", "(  blue  ) (z_trans )"},
    {"", "( scale  ) ( persp  )"},
    {"", ""},
};



/*
 *--------------------------------------------------------------
 *
 * display_bg_color
 *
 *	Display the menu that allows background color values to be input.
 *
 *--------------------------------------------------------------
 */

void
display_bg_color()
{
    display_main_screen(bg_menu, color_dial_menu, 5, 11);

    standout();
    move(5, 4);
    printw("Background color:");
    standend();
    move(6, 7);
    printw("R: %g ", dle_ptr->p_dl_data->bg_color.r);
    move(7, 7);
    printw("G: %g ", dle_ptr->p_dl_data->bg_color.g);
    move(8, 7);
    printw("B: %g ", dle_ptr->p_dl_data->bg_color.b);
    move(9, 7);
    if (dle_ptr->p_dl_data->fast_clear_set < 4)
	printw("Fast clear: %d ", dle_ptr->p_dl_data->fast_clear_set);
    else
	printw("Fast clear: off ");
    move(LINES - 1, 0);			/* Leave cursor at the bottom */
    REFRESH;
} /* End of display_bg_color */



/*
 *--------------------------------------------------------------
 *
 * update_bg_color
 *
 *--------------------------------------------------------------
 */

void
update_bg_color()
{
    int fcs;				/* Fast clear set to use */
    unsigned bg_color;			/* Packed background color */

    fcs = dle_ptr->p_dl_data->fast_clear_set;
    dle_ptr->p_dl_data->fcbg_color.fast_clear_set = fcs;
    dle_ptr->p_dl_data->display_a.fast_clear_set = fcs;
    dle_ptr->p_dl_data->display_b.fast_clear_set = fcs;

    dle_ptr->p_dl_data->bg_color.r = dle_ptr->bg_r.current_value;
    dle_ptr->p_dl_data->bg_color.g = dle_ptr->bg_g.current_value;
    dle_ptr->p_dl_data->bg_color.b = dle_ptr->bg_b.current_value;

    /* Check for the need to update the fast clear background color */
    if (dle_ptr->p_dl_data->fast_clear_set < 4) {
	/* Set the color */
        bg_color =
	       ((int) (dle_ptr->bg_r.current_value * 255.9) & 0xff)
            | (((int) (dle_ptr->bg_g.current_value * 255.9) & 0xff) << 8)
            | (((int) (dle_ptr->bg_b.current_value * 255.9) & 0xff) << 16);
        dle_ptr->p_dl_data->fcbg_color.color = bg_color;
	/* Cause update */
	dle_ptr->p_dl_data->tr_flags |= TRF_SET_FCBG | TRF_PREPARE_WINDOW;

	if (dga) {
		dga->wx_dbuf.device.hawk.gt_fc_col.c_32[0] = bg_color;
		dga->wx_dbuf.device.hawk.gt_fc_col.c_32[1] = bg_color;
	}
    }

} /* End of update_bg_color */



/*
 *--------------------------------------------------------------
 *
 * get_line_color
 *
 *	Get the line (object) color from the keyboard
 *
 *--------------------------------------------------------------
 */

void
get_line_color()
{
    dial_data *r, *g, *b;		/* Structures to update */

    r = &dle_ptr->line_r;
    g = &dle_ptr->line_g;
    b = &dle_ptr->line_b;

    switch (input_ch) {
    case 'r':
	r->current_abs -= r->key_step;
	constrain_input(r);
	(*r->attr_update)();
	break;

    case 'g':
	g->current_abs -= g->key_step;
	constrain_input(g);
	(*g->attr_update)();
	break;

    case 'b':
	b->current_abs -= b->key_step;
	constrain_input(b);
	(*b->attr_update)();
	break;

    case 'R':
	r->current_abs += r->key_step;
	constrain_input(r);
	(*r->attr_update)();
	break;

    case 'G':
	g->current_abs += g->key_step;
	constrain_input(g);
	(*g->attr_update)();
	break;

    case 'B':
	b->current_abs += b->key_step;
	constrain_input(b);
	(*b->attr_update)();
	break;

    case 'T':
    case 't':
	if (dle_ptr->p_dl_data->antialias) {
	    dle_ptr->p_dl_data->antialias = FALSE;
	    dle_ptr->p_dl_data->z_buffer_update = HK_Z_UPDATE_ALL;
	}
	else {
	    dle_ptr->p_dl_data->antialias = TRUE;
	    dle_ptr->p_dl_data->z_buffer_update = HK_Z_UPDATE_NONE;
	}
	break;

    case 'A':
	dle_ptr->p_dl_data->antialias = TRUE;
	dle_ptr->p_dl_data->z_buffer_update = HK_Z_UPDATE_NONE;
	break;

    case 'a':
	dle_ptr->p_dl_data->antialias = FALSE;
	dle_ptr->p_dl_data->z_buffer_update = HK_Z_UPDATE_ALL;
	break;

    case 'O':
/*	dle_ptr->p_dl_data->blend_op.rgb_blend_prog = HK_BLEND_AA_ARBITRARY;*/
/*	dle_ptr->p_dl_data->blend_op.alpha_blend_prog = HK_BLEND_AA_ARBITRARY;*/
	break;

    case 'o':
/*	dle_ptr->p_dl_data->blend_op.rgb_blend_prog = HK_BLEND_AA_CONSTANT;*/
/*	dle_ptr->p_dl_data->blend_op.alpha_blend_prog = HK_BLEND_AA_CONSTANT;*/
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

} /* End of get_line_color */



static Menu line_menu[] = {
    {"A, a", "Enable, disable antialiasing"},
    {"T, t", "Toggle antialiasing"},
    {"R, r", "Increase, decrease R by 1"},
    {"G, g", "Increase, decrease G by 1"},
    {"B, b", "Increase, decrease B by 1"},
/*    {"o, O", "Set AA blend-op to constant, arbitrary"},*/
    {"d, D", "Draw current object once, continuously"},
    {"q", "return to main menu"},
    {"", ""},
};



/*
 *--------------------------------------------------------------
 *
 * display_line_color
 *
 *	Display the menu that allows line color values to be input.
 *
 *--------------------------------------------------------------
 */

void
display_line_color()
{
    display_main_screen(line_menu, color_dial_menu, 5, 12);

    standout();
    move(5, 4);
    printw("Line attributes:");
    standend();
    move(6, 7);
    printw("R: %g ", dle_ptr->p_dl_data->object_color.r);
    move(7, 7);
    printw("G: %g ", dle_ptr->p_dl_data->object_color.g);
    move(8, 7);
    printw("B: %g ", dle_ptr->p_dl_data->object_color.b);
    move(9, 7);
    printw("Antialiasing: %s ",
	(dle_ptr->p_dl_data->antialias == FALSE) ? "off" : "on");
#if 0
    move(10, 7);
    if (dle_ptr->p_dl_data->blend_op.rgb_blend_prog == HK_BLEND_AA_CONSTANT)
	printw("Blend program: constant");
    else if (dle_ptr->p_dl_data->blend_op.rgb_blend_prog == HK_BLEND_AA_ARBITRARY)
	printw("Blend program: arbitrary");
    else
	printw("Blend program: unknown");
#endif
    move(LINES - 1, 0);			/* Leave cursor at the bottom */
    REFRESH;
} /* End of display_line_color */



/*
 *--------------------------------------------------------------
 *
 * update_obj_color
 *
 *--------------------------------------------------------------
 */

void
update_obj_color()
{
    float average;

    dle_ptr->p_dl_data->object_color.r = dle_ptr->line_r.current_value;
    dle_ptr->p_dl_data->object_color.g = dle_ptr->line_g.current_value;
    dle_ptr->p_dl_data->object_color.b = dle_ptr->line_b.current_value;

    average = (dle_ptr->line_r.current_value + dle_ptr->line_g.current_value
	+ dle_ptr->line_b.current_value) / 3.0;

    dle_ptr->p_dl_data->left_color.b = average;
    dle_ptr->p_dl_data->right_color.r = average;
} /* End of update_obj_color */



/*
 *--------------------------------------------------------------
 *
 * get_surf_props
 *
 *	Get the surface properties from the keyboard
 *
 *--------------------------------------------------------------
 */

void
get_surf_props()
{
    dial_data *a, *d, *s, *t;		/* Structures to update */
    dial_data *r, *g, *b, *e;

    a = &dle_ptr->ambient;
    d = &dle_ptr->diffuse;
    s = &dle_ptr->specular;
    t = &dle_ptr->transparency;
    r = &dle_ptr->spec_r;
    g = &dle_ptr->spec_g;
    b = &dle_ptr->spec_b;
    e = &dle_ptr->spec_exp;

    /* Currently one color (line color) is used for everything */

    switch (input_ch) {

	/* Ambient coefficient */
    case 'A':
	a->current_abs += a->key_step;
	constrain_input(a);
	(*a->attr_update)();
	break;

    case 'a':
	a->current_abs -= a->key_step;
	constrain_input(a);
	(*a->attr_update)();
	break;

	/* Diffuse coefficient */
    case 'D':
	d->current_abs += d->key_step;
	constrain_input(d);
	(*d->attr_update)();
	break;

    case 'd':
	d->current_abs -= d->key_step;
	constrain_input(d);
	(*d->attr_update)();
	break;

	/* Specular coefficient */
    case 'S':
	s->current_abs += s->key_step;
	constrain_input(s);
	(*s->attr_update)();
	break;

    case 's':
	s->current_abs -= s->key_step;
	constrain_input(s);
	(*s->attr_update)();
	break;

	/* Transparency coefficient */
    case 'T':
	t->current_abs += t->key_step;
	constrain_input(t);
	(*t->attr_update)();
	break;

    case 't':
	t->current_abs -= t->key_step;
	constrain_input(t);
	(*t->attr_update)();
	break;

	/* Specular red */
    case 'I':
	r->current_abs += r->key_step;
	constrain_input(r);
	(*r->attr_update)();
	break;

    case 'i':
	r->current_abs -= r->key_step;
	constrain_input(r);
	(*r->attr_update)();
	break;

	/* Specular green */
    case 'J':
	g->current_abs += g->key_step;
	constrain_input(g);
	(*g->attr_update)();
	break;

    case 'j':
	g->current_abs -= g->key_step;
	constrain_input(g);
	(*g->attr_update)();
	break;

	/* Specular blue */
    case 'K':
	b->current_abs += b->key_step;
	constrain_input(b);
	(*b->attr_update)();
	break;

    case 'k':
	b->current_abs -= b->key_step;
	constrain_input(b);
	(*b->attr_update)();
	break;

	/* Specular exponent */
    case 'X':
	e->current_abs += e->key_step;
	constrain_input(e);
	(*e->attr_update)();
	break;

    case 'x':
	e->current_abs -= e->key_step;
	constrain_input(e);
	(*e->attr_update)();
	break;

	/* Flat/gouraud shading */
    case 'G':
	cycle_back(dle_ptr->p_dl_data->shading_method, HK_SHADING_GOURAUD);
	break;
    case 'g':
	cycle_forward(dle_ptr->p_dl_data->shading_method, HK_SHADING_GOURAUD);
	break;

	/* Lighting degree */
    case 'L':
	cycle_back(dle_ptr->p_dl_data->lighting_degree, HK_SPECULAR_LIGHTING);
	break;
    case 'l':
	cycle_forward(dle_ptr->p_dl_data->lighting_degree, HK_SPECULAR_LIGHTING);
	break;

	/* Face culling */
    case 'C':
	cycle_back(dle_ptr->p_dl_data->culling_mode, HK_CULL_FRONTFACE);
	break;
    case 'c':
	cycle_forward(dle_ptr->p_dl_data->culling_mode, HK_CULL_FRONTFACE);
	break;

	/* Transparency quality */
    case 'Q':
	cycle_forward(dle_ptr->p_dl_data->trans_qual, HK_ALPHA_BLEND);
	break;

	/* Edges */
    case 'E':
	cycle_back(dle_ptr->p_dl_data->edge, HK_EDGE_ALL);
	break;
    case 'e':
	cycle_forward(dle_ptr->p_dl_data->edge, HK_EDGE_ALL);
	break;

    case 'W':
	continuous_draw = 1;
	break;

    case 'w':
	if (!continuous_draw)
	    draw_one_frame();
	continuous_draw = 0;
	break;

    case 'q':
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

} /* End of get_surf_props */



static Menu surf_prop_menu[] = {
    {"A, a", "Ambient reflection coefficient"},
    {"D, d", "Diffuse reflection coefficient"},
    {"S, s", "Specular reflection coefficient"},
    {"T, t", "Transparency coefficient"},
    {"I, i", "Red specular color"},
    {"J, j", "Green specular color"},
    {"K, k", "Blue specular color"},
    {"X, x", "Specular exponent"},
    {"g", "Toggle gouraud/flat shading"},
    {"l", "Cycle through lighting degrees"},
    {"c", "Cycle through face culling values"},
    {"Q", "Toggle transparency quality"},
    {"e", "Toggle edges"},
    {"w, W", "Draw current object once, continuously (NOTE!)"},
    {"q", "return to main menu"},
    {"", ""},
};

static Menu surf_prop_dial_menu[] = {
    {"", "        Dials:"},
    {"", "( ambient) (spec red)"},
    {"", "( diffuse) (spec grn)"},
    {"", "(specular) (spec blu)"},
    {"", "( transp ) (spec exp)"},
    {"", ""},
};



/*
 *--------------------------------------------------------------
 *
 * display_surf_prop
 *
 *	Display the menu that allows surface properties to be changed.
 *
 *--------------------------------------------------------------
 */

void
display_surf_prop()
{
    display_main_screen(surf_prop_menu, surf_prop_dial_menu, 5, 13);

    standout();
    move(5, 4);
    printw("Surface properties:");
    standend();
    move(6, 7);
    printw("Ambient: %g ",
	dle_ptr->p_dl_data->mat_prop.ambient_reflection_coefficient);
    move(7, 7);
    printw("Diffuse: %g ",
	dle_ptr->p_dl_data->mat_prop.diffuse_reflection_coefficient);
    move(8, 7);
    printw("Specular: %g ",
	dle_ptr->p_dl_data->mat_prop.specular_reflection_coefficient);
    move(9, 7);
    printw("Transparent: %g ",
	dle_ptr->p_dl_data->transp_degree);

    move(6, 30);
    printw("Spec. red: %g ",
	dle_ptr->p_dl_data->spec_color.r);
    move(7, 30);
    printw("Spec. green: %g ",
	dle_ptr->p_dl_data->spec_color.g);
    move(8, 30);
    printw("Spec. blue: %g ",
	dle_ptr->p_dl_data->spec_color.b);
    move(9, 30);
    printw("Spec. exp.: %g ",
	dle_ptr->p_dl_data->mat_prop.specular_exponent);

    move(10, 7);
    if (dle_ptr->p_dl_data->shading_method == HK_SHADING_GOURAUD)
	printw("Shading method: gouraud");
    else if (dle_ptr->p_dl_data->shading_method == HK_SHADING_FLAT)
	printw("Shading method: flat");
    else
	printw("Shading method: unknown");

    move(11, 7);
    if (dle_ptr->p_dl_data->lighting_degree == HK_NO_LIGHTING)
	printw("Lighting degree: no lighting");
    else if (dle_ptr->p_dl_data->lighting_degree == HK_AMBIENT_LIGHTING)
	printw("Lighting degree: ambient");
    else if (dle_ptr->p_dl_data->lighting_degree == HK_DIFFUSE_LIGHTING)
	printw("Lighting degree: diffuse");
    else if (dle_ptr->p_dl_data->lighting_degree == HK_SPECULAR_LIGHTING)
	printw("Lighting degree: specular");
    else
	printw("Lighting degree: unknown");

    move(12, 7);
    if (dle_ptr->p_dl_data->culling_mode == HK_CULL_NONE)
	printw("Face culling mode: none");
    else if (dle_ptr->p_dl_data->culling_mode == HK_CULL_BACKFACE)
	printw("Face culling mode: backface");
    else if (dle_ptr->p_dl_data->culling_mode == HK_CULL_FRONTFACE)
	printw("Face culling mode: frontface");
    else
	printw("Face culling mode: unknown");

    move(10, 40);
    if (dle_ptr->p_dl_data->trans_qual == HK_ALPHA_BLEND)
	printw("Transparency quality: alpha blend");
    else if (dle_ptr->p_dl_data->trans_qual == HK_SCREEN_DOOR)
	printw("Transparency quality: screen door");
    else
	printw("Transparency quality: unknown");

    move(11, 40);
    if (dle_ptr->p_dl_data->edge == HK_EDGE_NONE)
	printw("Edge highlighting: none");
    else if (dle_ptr->p_dl_data->edge == HK_EDGE_SELECTED)
	printw("Edge highlighting: selected");
    else if (dle_ptr->p_dl_data->edge == HK_EDGE_ALL)
	printw("Edge highlighting: all");
    else
	printw("Edge highlighting: unknown");

    move(LINES - 1, 0);			/* Leave cursor at the bottom */
    REFRESH;
} /* End of display_surf_prop */



/*
 *--------------------------------------------------------------
 *
 * update_surf_prop
 *
 *--------------------------------------------------------------
 */

void
update_surf_prop()
{
    dle_ptr->p_dl_data->mat_prop.ambient_reflection_coefficient =
	dle_ptr->ambient.current_value;
    dle_ptr->p_dl_data->mat_prop.diffuse_reflection_coefficient =
	dle_ptr->diffuse.current_value;
    dle_ptr->p_dl_data->mat_prop.specular_reflection_coefficient =
	dle_ptr->specular.current_value;
    dle_ptr->p_dl_data->transp_degree = dle_ptr->transparency.current_value;
    dle_ptr->p_dl_data->spec_color.r = dle_ptr->spec_r.current_value;
    dle_ptr->p_dl_data->spec_color.g = dle_ptr->spec_g.current_value;
    dle_ptr->p_dl_data->spec_color.b = dle_ptr->spec_b.current_value;
    dle_ptr->p_dl_data->mat_prop.specular_exponent =
	dle_ptr->spec_exp.current_value;
} /* End of update_surf_prop */



/*
 *--------------------------------------------------------------
 *
 * get_depth_cue
 *
 *	Get depth-cue parameters.  Return when the user is
 *	done and types 'q' or ESC or CTRL-D.
 *
 *--------------------------------------------------------------
 */

void
get_depth_cue()
{
    dial_data *f, *b, *m, *n;		/* Structures to update */

    f = &dle_ptr->dc_front;
    b = &dle_ptr->dc_back;
    m = &dle_ptr->dc_max;
    n = &dle_ptr->dc_min;

    switch (input_ch) {
    case 'O':
	dle_ptr->p_dl_data->depth_cue = TRUE;
	break;

    case 'o':
	dle_ptr->p_dl_data->depth_cue = FALSE;
	break;

    case 'T':
    case 't':
	dle_ptr->p_dl_data->depth_cue = !(dle_ptr->p_dl_data->depth_cue);
	break;

    case 'F':
	f->current_abs += f->key_step;
	constrain_input(f);
	(*f->attr_update)();
	break;

    case 'f':
	f->current_abs -= f->key_step;
	constrain_input(f);
	(*f->attr_update)();
	break;

    case 'B':
	b->current_abs += b->key_step;
	constrain_input(b);
	(*b->attr_update)();
	break;

    case 'b':
	b->current_abs -= b->key_step;
	constrain_input(b);
	(*b->attr_update)();
	break;

    case 'M':
	m->current_abs += m->key_step;
	constrain_input(m);
	(*m->attr_update)();
	break;

    case 'm':
	m->current_abs -= m->key_step;
	constrain_input(m);
	(*m->attr_update)();
	break;

    case 'N':
	n->current_abs += n->key_step;
	constrain_input(n);
	(*n->attr_update)();
	break;

    case 'n':
	n->current_abs -= n->key_step;
	constrain_input(n);
	(*n->attr_update)();
	break;

    case '+':
	f->key_step = (int) ((float) (f->key_step) * 1.25);
	b->key_step = (int) ((float) (b->key_step) * 1.25);
	m->key_step = (int) ((float) (m->key_step) * 1.25);
	n->key_step = (int) ((float) (n->key_step) * 1.25);
	break;
    case '-':
	f->key_step = (int) ((float) (f->key_step) * 0.8);
	b->key_step = (int) ((float) (b->key_step) * 0.8);
	m->key_step = (int) ((float) (m->key_step) * 0.8);
	n->key_step = (int) ((float) (n->key_step) * 0.8);
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

} /* End of get_depth_cue */

static Menu depth_cue_menu[] = {
    {"O, o", "Turn depth-cuing on, off"},
    {"T, t", "Toggle depth-cuing"},
    {"F, f", "Move front plane forward, back"},
    {"B, b", "Move back plane back, forward"},
    {"M, m", "Increase, decrease maximum intensity"},
    {"N, n", "Increase, decrease minimum intensity"},
    {"d, D", "Draw current object once, continuously"},
    {"q", "return to main menu"},
    {"", ""},
};

static Menu depth_cue_dial_menu[] = {
    {"", "        Dials:"},
    {"", "( front  ) (  back  )"},
    {"", "(  max   ) (  min   )"},
    {"", "(        ) (        )"},
    {"", "(        ) (        )"},
    {"", ""},
};



/*
 *--------------------------------------------------------------
 *
 * display_depth_cue
 *
 *	Display the menu that allows the depth-cue parameters to be
 *	changed.
 *
 *--------------------------------------------------------------
 */

void
display_depth_cue()
{

    display_main_screen(depth_cue_menu, depth_cue_dial_menu, 5, 12);

    standout();
    move(5, 4);
    printw("Depth-cue parameters:");
    standend();
    move(6, 7);
    printw("Depth-cue: %s ",
	(dle_ptr->p_dl_data->depth_cue == FALSE) ? "off" : "on");
    move(7, 7);
    printw("Front: %g ", dle_ptr->dc_front.current_value);
    move(8, 7);
    printw("Back: %g ", dle_ptr->dc_back.current_value);
    move(9, 7);
    printw("Max: %g ", dle_ptr->dc_max.current_value);
    move(10, 7);
    printw("Min: %g ", dle_ptr->dc_min.current_value);
    move(LINES - 1, 0);			/* Leave cursor at the bottom */
    REFRESH;
} /* End of display_depth_cue */



/*
 *--------------------------------------------------------------
 *
 * update_depth_cue
 *
 *--------------------------------------------------------------
 */

void
update_depth_cue()
{
    dle_ptr->p_dl_data->dc_param.z_front = dle_ptr->dc_front.current_value;
    dle_ptr->p_dl_data->dc_param.z_back = dle_ptr->dc_back.current_value;
    dle_ptr->p_dl_data->dc_param.scale_front = dle_ptr->dc_max.current_value;
    dle_ptr->p_dl_data->dc_param.scale_back = dle_ptr->dc_min.current_value;
} /* End of update_depth_cue */

/* End of demo_colors.c */

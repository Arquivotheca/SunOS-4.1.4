#ifndef lint
static char sccsid[] = "@(#)demo_obj_control.c  1.1 94/10/31 SMI";
#endif

/*
 ***********************************************************************
 *
 * demo_obj_control.c
 *
 *	@(#)demo_obj_control.c 1.9 91/05/09 12:53:40
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 *	Demo host program for Hawk shared memory simulation.
 *
 *	This module contains the user interface code for the main control
 *	menu and for getting in new objects.
 *
 * 27-Sep-90 Scott R. Nelson	  Split off from demo_user_Interf.c
 *  4-Oct-90 Scott R. Nelson	  Added new keyboard toggle menu.
 * 15-Jan-91 Scott R. Nelson	  Changes to make HIS 1.5 work.
 *
 ***********************************************************************
 */

#include <stdio.h>
#include <curses.h>
#include "demo.h"

#include "demo_sv.h"

/*
 *==============================================================
 *
 * Procedure Hierarchy:
 *
 *	get_main_loop
 *	    display_help_menu
 *	    draw_one_frame
 *	    select_object
 *	    get_object
 *	display_main_menu
 *	get_object
 *	    build_dl
 *	    sleep
 *	display_object
 *	get_filename
 *	display_filename
 *	get_keyboard_toggle
 *	display_keyboard_toggle
 *
 *==============================================================
 */



static char file_name[256];		/* Name of .hdl file */
static int file_name_len;		/* Length of above name */

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
 * get_main_loop
 *
 *	This is the top level keyboard command loop
 *
 *--------------------------------------------------------------
 */

void
get_main_loop()
{
    switch (input_ch) {
	case 'h':			/* Help */
	case '?':
	display_help_menu();
	break;

    case 'D':
	continuous_draw = 1;
	break;

    case 'd':
	if (!continuous_draw)
	    draw_one_frame();
	continuous_draw = 0;
	break;

    case 'A':
    case 'a':
	push_menu(display_axis, get_axis);
	cur_dials[0] = &dle_ptr->x_rot_axis;
	cur_dials[1] = &dle_ptr->y_rot_axis;
	cur_dials[2] = &dle_ptr->z_rot_axis;
	cur_dials[3] = &dle_ptr->rot_speed;
	cur_dials[4] = NULL;
	cur_dials[5] = NULL;
	cur_dials[6] = NULL;
	cur_dials[7] = NULL;
	break;

    case 'B':
    case 'b':
	push_menu(display_bg_color, get_bg_color);
	cur_dials[0] = &dle_ptr->bg_r;
	cur_dials[1] = &dle_ptr->bg_g;
	cur_dials[2] = &dle_ptr->bg_b;
	break;

    case 'C':
    case 'c':
	push_menu(display_depth_cue, get_depth_cue);
	cur_dials[0] = &dle_ptr->dc_front;
	cur_dials[1] = &dle_ptr->dc_max;
	cur_dials[2] = NULL;
	cur_dials[3] = NULL;
	cur_dials[4] = &dle_ptr->dc_back;
	cur_dials[5] = &dle_ptr->dc_min;
	cur_dials[6] = NULL;
	cur_dials[7] = NULL;
	break;

    case 'K':
    case 'k':
	push_menu(display_keyboard_toggle, get_keyboard_toggle);
	/* Leave the dials with main transformations */
	break;

    case 'L':
    case 'l':
	push_menu(display_line_color, get_line_color);
	cur_dials[0] = &dle_ptr->line_r;
	cur_dials[1] = &dle_ptr->line_g;
	cur_dials[2] = &dle_ptr->line_b;
	break;

    case 'S':
    case 's':
	push_menu(display_surf_prop, get_surf_props);
	cur_dials[0] = &dle_ptr->ambient;
	cur_dials[1] = &dle_ptr->diffuse;
	cur_dials[2] = &dle_ptr->specular;
	cur_dials[3] = &dle_ptr->transparency;
	cur_dials[4] = &dle_ptr->spec_r;
	cur_dials[5] = &dle_ptr->spec_g;
	cur_dials[6] = &dle_ptr->spec_b;
	cur_dials[7] = &dle_ptr->spec_exp;
	break;

    case 'T':
    case 't':
	push_menu(display_transforms, get_transforms);
	break;

    case 'W':
    case 'w':
	if( sv_win.wid || dga ) {
        	fprintf(stderr,"\rIn %s use the mouse to control window position and size", dga ? "OpenWindows" : "SunView");
		sleep(2);
		fprintf(stderr,"\r                                                                  \r");
	} else {
	    push_menu(display_window, get_window);
	    cur_dials[0] = &dle_ptr->window_x;
	    cur_dials[1] = &dle_ptr->window_width;
	    cur_dials[2] = NULL;
	    cur_dials[3] = NULL;
	    cur_dials[4] = &dle_ptr->window_y;
	    cur_dials[5] = &dle_ptr->window_height;
	    cur_dials[6] = NULL;
	    cur_dials[7] = NULL;
	}
	break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
	select_object(input_ch - '0');
	break;

    case 'R':
	reset_scale();
	reset_persp();
	reset_trans();
	reset_rot();
	reset_axis();
	break;

    case 'o':
	push_menu(display_object, get_object);
	break;

    case CTRL_C:
    case CTRL_D:
	done_with_program = 1;
	break;

    case CTRL_L:
	clear();
	refresh();
	(*display_current_screen[menu_sp])();
	break;
 
    case 'z':
	dle_ptr->p_dl_data->tr_flags &= ~TRF_STEREO;
	break;
     
    case 'Z': 
	dle_ptr->p_dl_data->tr_flags |= TRF_STEREO; 
	break;

    default:
	break;
    }

} /* End of get_main_loop */



static Menu main_menu[] = {
    {"0-9", "Select object"},
    {"a", "Axis of rotation"},
    {"b", "Background color"},
    {"c", "Depth-Cue parameters"},
    {"d, D", "Draw current object once, continuously"},
    {"k", "Keyboard toggle mode"},
    {"l", "Line attributes"},
    {"o", "Get a new .hdl object"},
    {"s", "Surface properties"},
    {"t", "Transformations"},
    {"w", "Window position"},
    {"Z, z", "Turn stereo mode on, off"},
    {"R", "Reset all transformation settings for current object"},
    {"h, ?", "Print out help menu"},
    {"CTRL-D", "exit"},
    {"", ""},
};



/*
 *--------------------------------------------------------------
 *
 * display_main_menu
 *
 *	Display the main menu
 *
 *--------------------------------------------------------------
 */

void
display_main_menu()
{
    display_main_screen(main_menu, main_dial_menu, 3, 5);
    REFRESH;
} /* End of display_main_menu */



/*
 *--------------------------------------------------------------
 *
 * get_object
 *
 *	Get an object from a .hdl file specified by the user.
 *	This can only be added to an object that was not previously
 *	defined.  Do as much checking as possible to make sure this
 *	will work.
 *
 *--------------------------------------------------------------
 */

void
get_object()
{
    int i;

    switch (input_ch) {

    case 'f':
	file_name_len = 0;
	file_name[file_name_len] = 0;
	push_menu(display_filename, get_filename);
	break;

    case 'l':
	clear();
	REFRESH;
	noraw();
	system("ls *.hdl");
	raw();
	wait_for_space();
	break;

    case 'L':
	clear();
	REFRESH;
	noraw();
	system("ls");
	raw();
	wait_for_space();
	break;

    case 'n':
	/* Select object number... */
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
	select_object(dl);
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
} /* End of get_object */

static Menu object_menu[] = {
    {"f", "File name"},
    {"l", "List directory (.hdl files only)"},
    {"L", "List directory (all files)"},
/*    {"n", "Select object number"},*/
    {"q", "return to main menu"},
    {"", ""},
};



/*
 *--------------------------------------------------------------
 *
 * display_object
 *
 *	Display the menu that allows a new object to be selected
 *
 *--------------------------------------------------------------
 */

void
display_object()
{
    int i;				/* Iterative counter */

    display_main_screen(object_menu, NULL, 5, 7);

    move(5, 4);
    printw("Available objects (highlighted): ");
    for (i = 0; i < 10; i++) {
	printw(" ");
	if (dle[i].dl_begin == NULL)
	    standout();
	printw("%d", i);
	if (dle[i].dl_begin == NULL)
	    standend();
    }
    move(LINES - 1, 0);			/* Leave cursor at the bottom */
    REFRESH;
} /* End of display_object */



/*
 *--------------------------------------------------------------
 *
 * get_filename
 *
 *	Get a file name from the keyboard and open the file.
 *
 *--------------------------------------------------------------
 */

void
get_filename()
{
    int i;

    switch (input_ch) {
    case BS:
    case DEL:
	/* Back up one character */
	if (file_name_len > 0) {
	    file_name_len -= 1;
	    file_name[file_name_len] = 0;
	}
	break;

    case CTRL_U:
    case CTRL_W:
	/* Erase the whole line */
	file_name_len = 0;
	file_name[file_name_len] = 0;
	break;

    case CR:
    case NL:
	/* End of string, done. */
	printw("\nOpening %s   ", file_name);
	REFRESH;

	i = 0;
	while ((dle[i].dl_begin != NULL) && (i < OBJ_MAX))
	    i++;
	if (i < OBJ_MAX) {
	    if (build_dl(file_name, i) == 1) {
		printw("\n%s opened successfully as object %d\n", file_name, i);
		select_object(i);
	    }
	    else {
		printw("Can't create %s.", file_name);
	    }
	}
	else {
	    printw("Maximum of %d objects, restart to get more.", OBJ_MAX);
	}
	REFRESH;

	wait_for_space();
	pop_menu();
	break;

    default:
	/* Get one more character in */
	if ((input_ch > 0x1f) && (input_ch < 0x7f)) {
	    file_name[file_name_len] = input_ch;
	    file_name_len += 1;
	    file_name[file_name_len] = 0;
	}
	break;
    }
} /* End of get_filename */



/*
 *--------------------------------------------------------------
 *
 * display_filename
 *
 *	Display the input file name.  This must leave the cursor at the
 *	end of the line to let the user know where he is at.
 *
 *--------------------------------------------------------------
 */

void
display_filename()
{
    move(1, 0);
    clrtobot();

    move(2, 0);
    printw("Input file name: %s", file_name);

    REFRESH;
} /* End of display_filename */



/*
 *--------------------------------------------------------------
 *
 * get_keyboard_toggle
 *
 *	Use the keyboard to toggle all parameters that don't have
 *	incremental numeric values.
 *
 *--------------------------------------------------------------
 */

void
get_keyboard_toggle()
{

    switch (input_ch) {

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
	select_object(input_ch - '0');
	break;

    case 'A':				/* Turn on, off antialiasing */
    case 'a':
	if (dle_ptr->p_dl_data->antialias) {
	    dle_ptr->p_dl_data->antialias = FALSE;
	    dle_ptr->p_dl_data->z_buffer_update = HK_Z_UPDATE_ALL;
	}
	else {
	    dle_ptr->p_dl_data->antialias = TRUE;
	    dle_ptr->p_dl_data->z_buffer_update = HK_Z_UPDATE_NONE;
	}
	break;

    case 'B':				/* Toggle between aa blend-ops */
    case 'b':
#if 0
	if (dle_ptr->p_dl_data->blend_op.rgb_blend_prog ==
	    HK_BLEND_AA_CONSTANT) {
	    dle_ptr->p_dl_data->blend_op.rgb_blend_prog = HK_BLEND_AA_ARBITRARY;
	    dle_ptr->p_dl_data->blend_op.alpha_blend_prog =
		HK_BLEND_AA_ARBITRARY;
	}
	else {
	    dle_ptr->p_dl_data->blend_op.rgb_blend_prog = HK_BLEND_AA_CONSTANT;
	    dle_ptr->p_dl_data->blend_op.alpha_blend_prog =
		HK_BLEND_AA_CONSTANT;
	}
#endif
	break;

    case 'C':				/* Cycle through face culling values */
	cycle_back(dle_ptr->p_dl_data->culling_mode, HK_CULL_FRONTFACE);
	break;
    case 'c':
	cycle_forward(dle_ptr->p_dl_data->culling_mode, HK_CULL_FRONTFACE);
	break;

    case 'D':				/* Turn depth-cue on, off */
    case 'd':
	dle_ptr->p_dl_data->depth_cue = !(dle_ptr->p_dl_data->depth_cue);
	break;

    case 'E':
	cycle_back(dle_ptr->p_dl_data->edge, HK_EDGE_ALL);
	break;
    case 'e':
	cycle_forward(dle_ptr->p_dl_data->edge, HK_EDGE_ALL);
	break;

    case 'F':				/* Cycle back thru fast clear sets */
	cycle_back(dle_ptr->p_dl_data->fast_clear_set, 4);
	update_bg_color();
	break;

    case 'f':				/* Cycle forward thru fast clear sets */
	cycle_forward(dle_ptr->p_dl_data->fast_clear_set, 4);
	update_bg_color();
	break;

    case 'G':
	cycle_back(dle_ptr->p_dl_data->shading_method, HK_SHADING_GOURAUD);
	break;
    case 'g':
	cycle_forward(dle_ptr->p_dl_data->shading_method, HK_SHADING_GOURAUD);
	break;

    case 'L':
	cycle_back(dle_ptr->p_dl_data->lighting_degree, HK_SPECULAR_LIGHTING);
	break;
    case 'l':
	cycle_forward(dle_ptr->p_dl_data->lighting_degree, HK_SPECULAR_LIGHTING);
	break;

    case 'R':				/* Turn auto-rotation on and off */
    case 'r':
	cycle_forward(auto_rotate, TRUE);
	break;

    case 'T':				/* Toggle transparency quality */
    case 't':
	cycle_forward(dle_ptr->p_dl_data->trans_qual, HK_ALPHA_BLEND);
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
    case 'Q':
    case CTRL_D:
    case ESC:
	pop_menu();
	select_object(dl);
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
} /* End of get_keyboard_toggle */



static Menu keyboard_toggle_menu[] = {
    {"0-9", "Select object"},
    {"a", "Turn antialiasing of lines on and off"},
/*    {"b", "Toggle blend-op between constant and arbitrary"},*/
    {"c", "Cycle through face culling values"},
    {"d", "Turn depth-cue on, off"},
    {"e", "Toggle edge highlighting"},
    {"f", "Turn fast clear on, off"},
    {"g", "Toggle gouraud/flat shading"},
    {"l", "Cycle through lighting degrees"},
    {"r", "Turn auto-rotation on and off"},
    {"t", "Toggle transparency quality"},
    {"q", "Return to main menu"},
    {"", ""},
};



/*
 *--------------------------------------------------------------
 *
 * display_keyboard_toggle
 *
 *	Display the menu that allows attributes to be toggled.
 *
 *--------------------------------------------------------------
 */

void
display_keyboard_toggle()
{
    int i;				/* Iterative counter */

    display_main_screen(keyboard_toggle_menu, main_dial_menu, 5, 16);

    move(5, 4);
    printw("Current modes: ");

    move(6,7);
    printw("Antialiasing: %s ",
	(dle_ptr->p_dl_data->antialias == FALSE) ? "off" : "on");

    move(7,7);
#if 0
    if (dle_ptr->p_dl_data->blend_op.rgb_blend_prog == HK_BLEND_AA_CONSTANT)
	printw("Blend program: constant ");
    else if (dle_ptr->p_dl_data->blend_op.rgb_blend_prog ==
	HK_BLEND_AA_ARBITRARY)
	printw("Blend program: arbitrary ");
    else
#endif
	printw("Blend program: unknown ");

    move(8,7);
    if (dle_ptr->p_dl_data->culling_mode == HK_CULL_NONE)
	printw("Face culling mode: none ");
    else if (dle_ptr->p_dl_data->culling_mode == HK_CULL_BACKFACE)
	printw("Face culling mode: backface ");
    else if (dle_ptr->p_dl_data->culling_mode == HK_CULL_FRONTFACE)
	printw("Face culling mode: frontface ");
    else
	printw("Face culling mode: unknown ");

    move(9,7);
    printw("Depth-cue: %s ",
	(dle_ptr->p_dl_data->depth_cue == FALSE) ? "off" : "on");

    move(10,7);
    if (dle_ptr->p_dl_data->edge == HK_EDGE_NONE)
	printw("Edge highlighting: none ");
    else if (dle_ptr->p_dl_data->edge == HK_EDGE_SELECTED)
	printw("Edge highlighting: selected ");
    else if (dle_ptr->p_dl_data->edge == HK_EDGE_ALL)
	printw("Edge highlighting: all ");
    else
	printw("Edge highlighting: unknown ");

    move(11,7);
    if (dle_ptr->p_dl_data->fast_clear_set < 4)
	printw("Fast clear: %d ", dle_ptr->p_dl_data->fast_clear_set);
    else
	printw("Fast clear: off ");

    move(12,7);
    if (dle_ptr->p_dl_data->shading_method == HK_SHADING_GOURAUD)
	printw("Shading method: gouraud ");
    else if (dle_ptr->p_dl_data->shading_method == HK_SHADING_FLAT)
	printw("Shading method: flat ");
    else
	printw("Shading method: unknown ");

    move(13,7);
    if (dle_ptr->p_dl_data->lighting_degree == HK_NO_LIGHTING)
	printw("Lighting degree: no lighting ");
    else if (dle_ptr->p_dl_data->lighting_degree == HK_AMBIENT_LIGHTING)
	printw("Lighting degree: ambient ");
    else if (dle_ptr->p_dl_data->lighting_degree == HK_DIFFUSE_LIGHTING)
	printw("Lighting degree: diffuse ");
    else if (dle_ptr->p_dl_data->lighting_degree == HK_SPECULAR_LIGHTING)
	printw("Lighting degree: specular ");
    else
	printw("Lighting degree: unknown ");

    move(14,7);
    printw("Auto-rotate: %s ", (auto_rotate == FALSE) ? "off" : "on");

    move(15,7);
    if (dle_ptr->p_dl_data->trans_qual == HK_ALPHA_BLEND)
	printw("Transparency quality: alpha blend ");
    else if (dle_ptr->p_dl_data->trans_qual == HK_SCREEN_DOOR)
	printw("Transparency quality: screen door ");
    else
	printw("Transparency quality: unknown ");

    move(LINES - 1, 0);			/* Leave cursor at the bottom */
    REFRESH;
} /* End of display_keyboard_toggle */

/* End of demo_obj_control.c */

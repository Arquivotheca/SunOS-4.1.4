#ifndef lint
static char sccsid[] = "@(#)demo_menus.c  1.1 94/10/31 SMI";
#endif

/*
 ***********************************************************************
 *
 * demo_menus.c
 *
 *	@(#)demo_menus.c 1.18 91/05/09 12:53:34
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 *	Demo host program for Hawk shared memory simulation.
 *
 *	This module contains the user interface code.  The output is
 *	tuned for an 80 x 24 CRT.
 *
 *	display_* routines put the right stuff up on the screen.
 *
 *  3-Apr-90 Scott R. Nelson	  Initial version.
 *  9-May-90 Scott R. Nelson	  Integrated this module with the other code.
 * 24-May-90 Scott R. Nelson	  Initial version split off from
 *				  demo_user_interf.c.
 * 19-Jul-90 Scott R. Nelson	  Tuning of code and more comments.
 * 23-Jul-90 Scott R. Nelson	  Added rotation axis, sub-menu for
 *				  transforms.
 * 21-Sep-90 Scott R. Nelson	  Wording changes.
 * 27-Sep-90 Scott R. Nelson	  Split out main menu routines into
 *				  separate files.
 * 22-Feb-91 John M. Perry	  Change name to GT Object Demo
 * 22-Apr-91 Christopher Klein	  Change name to GT Verification Tool
 *
 ***********************************************************************
 */

#include <stdio.h>
#include <math.h>
#include <curses.h>
#include "demo.h"



/*
 *==============================================================
 *
 * Procedure Hierarchy:
 *
 *	display_main_screen
 *	    display_menu
 *	    display_data
 *	display_help_menu
 *	display_menu
 *	display_data
 *	display_exit_message
 *	wait_for_space
 *
 *==============================================================
 */



static Menu main_menu[] = {
    {"0-9", "Select object"},
    {"o", "Get a new .hdl object"},
    {"t", "Transformations"},
    {"a", "Axis of rotation"},
    {"w", "Window position"},
    {"l", "Line attributes"},
    {"c", "Depth-Cue parameters"},
    {"b", "Background color"},
    { "s", "Surface properties"},
    {"d, D", "Draw current object once, continuously"},
    {"R", "Reset all settings for current object"},
    {"h, ?", "Print out help menu"},
    {"CTRL-D", "exit"},
    {"", ""},
};



/*
 *--------------------------------------------------------------
 *
 * display_main_screen
 *
 *	Update the text on the user control window.
 *
 *--------------------------------------------------------------
 */

void
display_main_screen(m, d, x, y)
    Menu *m;				/* Which menu to use */
    Menu *d;				/* Which dial menu to use */
    int x, y;				/* Position */
{
    erase();

    move(0, 25);
    printw("GT Verification Tool %s", DEMO_VERSION);

    move(2, 0);
    printw("Object = %d", dl);

    move(1, 0);
    printw(dle_ptr->name);

    move(2, 13);
    printw("Translate = < %g, %g, %g >", dle_ptr->x_trans.current_value,
	dle_ptr->y_trans.current_value, dle_ptr->z_trans.current_value);

    move(3, 13);
    printw("Scale = %g", dle_ptr->scale.current_value);

    move(3, 40);
    printw("Perspective = %g", dle_ptr->perspective.current_value);

    if (m != NULL)
	display_menu(m, x, y);

    if (d != NULL)
	display_data(d, 55, 5);

    move(LINES - 1, 0);			/* Leave cursor at the bottom */
} /* End of display_main_screen */



/*
 *--------------------------------------------------------------
 *
 * display_help_menu
 *
 *	Display the help menu and wait for the user to hit a key before
 *	going back to draw the regular text.
 *
 *--------------------------------------------------------------
 */

void
display_help_menu()
{
    erase();
    move(0, 0);
    printw("A few hints for using the demo program:\n\n");

    printw("All commands are single character and take effect immediately.  The\n");
    printw("menu shows you which commands are currently valid by highlighting\n");
    printw("them, followed by a short description of the function.\n\n");

    printw("ESC will always exit back a level from the current menu.\n");
    printw("Use CTRL_D to exit the entire program (from the top level).  New\n");
    printw("objects may be included using the \"o\" menu followed by the \"f\"\n");
    printw("function.  Errors made in typing the file name currently require a\n");
    printw("retry.  This will be fixed.\n\n");

    printw("It is recommended that you first position a new object using \"t,r\"\n");
    printw("so that the top of the object is up.  Then go to the \"a\" menu, set\n");
    printw("the speed and adjust the axis of rotation as desired.  Then use the\n");
    printw("\"l\" menu to set the color and turn on antialiasing, the \"b\" menu to get\n");
    printw("fast clear, the \"w\" menu to move or resize the window.  Note that\n");
    printw("window changes currently stop the automatic motion.  Use \"D\" from\n");
    printw("the top menu to get continuous motion.\n\n");

    printw("At this point experiment with changing the object attributes.  If\n");
    printw("the control dials don't always work as desired, switch back to\n");
    printw("object 0 then back to your desired object.  This may help.\n");

    wait_for_space();
} /* End of display_help_menu */



/*
 *--------------------------------------------------------------
 *
 * display_menu
 *
 *	Display the specified menu, highlighting the characters
 *	that perform some action.
 *
 *--------------------------------------------------------------
 */

void
display_menu(m, x, y)
    Menu *m;				/* The menu to display */
    int x, y;				/* Starting screen position */
{
    int max_offset;			/* Maximum offset of highlight part */
    int offset;				/* Offset of current highlight part */
    Menu *mptr;				/* Temporary menu pointer */

    /* Determine spacing */
    max_offset = 0;
    mptr = m;
    while (mptr->text_string[0] != 0) {
	offset = strlen(mptr->highlight);
	if (offset > max_offset)
	    max_offset = offset;
	mptr++;
    }
    max_offset += 2;

    /* Display it all */
    mptr = m;
    while (mptr->text_string[0] != 0) {
	standout();
	move(y, x);
	printw(mptr->highlight);
	standend();
	move(y, x + strlen(mptr->highlight));
	printw(":");
	move(y, x + max_offset);
	printw(mptr->text_string);

	mptr++;
	y++;
    };
    move(LINES - 1, 0);			/* Leave cursor at the bottom */

} /* End of display_menu */



/*
 *--------------------------------------------------------------
 *
 * display_exit_message
 *
 *	Display the message indicating that the program is exiting.
 *
 *--------------------------------------------------------------
 */

void
display_exit_message()
{
    standout();
    move(22, 0);
    printw("Exiting GT Verification Tool");
    standend();
} /* End of display_exit_message */



/*
 *--------------------------------------------------------------
 *
 * display_data
 *
 *	Display the specified data at the specified screen location.
 *
 *--------------------------------------------------------------
 */

void
display_data(m, x, y)
    Menu *m;				/* The menu to display */
    int x, y;				/* Starting screen position */
{
    Menu *mptr;				/* Temporary menu pointer */

    /* Display it all */
    mptr = m;
    while (mptr->text_string[0] != 0) {
	move(y, x);
	printw(mptr->text_string);

	mptr++;
	y++;
    };
    move(LINES - 1, 0);			/* Leave cursor at the bottom */

} /* End of display_data */



/*
 *--------------------------------------------------------------
 *
 * wait_for_space
 *
 *	Put out a message and wait for the user to press the
 *	space bar before continuing.
 *
 *--------------------------------------------------------------
 */

void
wait_for_space()
{
    int ch;				/* Input characters */

    standout();
    printw("\tHit space to continue");
    standend();
    REFRESH;
    if (input_file == NULL) do {
	ch = getch();
    } while (ch != ' ');
    clear();
} /* End of wait_for_space */

/* End of demo_menus.c */

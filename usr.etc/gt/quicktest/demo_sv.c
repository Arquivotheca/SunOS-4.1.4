#ifndef lint
static char sccsid[] = "@(#)demo_sv.c  1.1 94/10/31 SMI";
#endif

/*
 ***********************************************************************
 *
 * demo_sv.c
 *
 *	@(#)demo_sv.c 1.4 91/05/13 14:19:02
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 *	Demo host program for Hawk shared memory simulation.
 *
 *	Subroutines needed for Demo to run under SunView.
 *      The main reason for putting routines here is to separate
 *      the from Xlib.h which also has a typedef "Window"
 *
 * 13-Apr-91 Chris Klein          Initial file.
 * 09-May-91 Chris Klein          Added Auto-cycle buttons, help, quit
 *                                and individual image buttons.
 *
 ***********************************************************************
 */

#include <math.h>
#include <sys/time.h>

#include "demo_sv.h"

#include <suntool/walkmenu.h>
#include <suntool/window.h>
#include <suntool/textsw.h>

/*
 *==============================================================
 *
 * Procedure Hierarchy:
 *
 *      set_sv_wb
 *      sv_repaint
 *          update_bg_color
 *      canvas_event_proc
 *          set_sv_wb
 *      cr_sv_frame
 *      sv_win_show
 *
 *==============================================================
 */
 

/*
 *==============================================================
 *
 * Procedure set_sv_wb
 *      Sets Hawk window bounds from the SunView Rects for the frame
 *      and canvas.  The canvases Rect is relative to the frame so it's
 *      absolute coords have to be calculated.
 *
 *==============================================================
 */

void set_sv_wb()
{
	Rect crect, frect;

	/* If we get called without the wb_ptr being set,  return  */
	if( ! sv_win.cur_wb_ptr ) return;
	
	memcpy(&frect, window_get( sv_win.frame, WIN_RECT ), sizeof(Rect));
	memcpy(&crect, window_get( sv_win.canvas, WIN_RECT ), sizeof(Rect));

	sv_win.cur_wb_ptr->xleft = frect.r_left + (frect.r_width - crect.r_width) / 2;
	sv_win.cur_wb_ptr->ytop  = frect.r_top + frect.r_height - crect.r_height - 5;

	sv_win.cur_wb_ptr->width = crect.r_width;
	sv_win.cur_wb_ptr->height = crect.r_height;

	return;
}

/*
 *==============================================================
 *
 * Procedure sv_repaint
 *      Called whenever the canvas needs a repaint.  Sets the window bounds
 *      and then updates the background color.
 *
 *==============================================================
 */


static
sv_repaint(c, pw, ra)
Canvas c;
Pixwin *pw;
Rectlist *ra;
{
	set_sv_wb();
	update_bg_color();
   	return;
}

/*
 *==============================================================
 *
 * Procedure canvas_event_proc
 *      Captures the resize and repaint event and sets the window bounds.
 *
 *==============================================================
 */


static
canvas_event_proc(win, event)
Window win;
Event  *event;
{
	if (event_action(event) == WIN_RESIZE || event_action(event) == WIN_REPAINT) {
		set_sv_wb(); 
  	 }
   	return;
}

#ifdef QUICKTEST
/*
 *==============================================================
 *
 * Procedure sv_quit
 *      Quits the SunView Progam when under QUICKTEST
 *
 *==============================================================
 */


static
sv_quit(win, event)
Window win;
Event  *event;
{
	done_with_program = 1;
}

/*
 *==============================================================
 *
 * Procedure sv_help
 *      Provides user with help message under QUICKTEST
 *
 *==============================================================
 */

char *helpmsg = "\n\
\n\
	Sparcstation 2 GT Quicktest Tool\n\
\n\
	GT Quicktest demonstrates the GT accelerator by displaying\n\
	wireframe and polygon models within a window. It loads\n\
	several models and displays them, either singly or in sequence.\n\
\n\
	The following buttons are available:\n\
\n\
	Image Display\n\
\n\
	Auto Cycle	Displays each of the three objects for a few seconds\n\
			at a time.\n\
\n\
	Sun Logo	Displays a Sun Logo consisting of purple\n\
			depth-cued polygons. The logo slowly rotates in\n\
			a light blue background and appears to fade\n\
			into a background fog.\n\
\n\
	X29		Displays an X29 (complete with transparent cockpit)\n\
			rotating on a light purple background. The body\n\
			is a light grey, the tail cone black and the\n\
			wings/stabilizer either red or blue.\n\
\n\
	F15		Displays a wireframe F15 composed of anti-aliased\n\
			vectors rotating very slowly against a black\n\
			background.\n\
\n\
	Help		Displays this text.\n\
\n\
	Quit 		Exits GT Quicktest\n\
\n\
\n\
";

static
sv_help(win, event)
Window win;
Event  *event;
{
	static Frame help_frame;
	static Textsw helptxt;
	
	if( help_frame == (Frame) NULL ) {
		help_frame = window_create( sv_win.frame, FRAME,
			FRAME_SHOW_LABEL, TRUE, 0);
		helptxt = window_create( help_frame, TEXTSW, 0 );
		textsw_insert(helptxt, helpmsg, strlen(helpmsg) );
	}
	window_set( help_frame, WIN_SHOW, TRUE, 0);

   	return;
}

/*
 *==============================================================
 *
 * Procedure sv_image_display
 *      Display image based on users panel selection
 *
 *==============================================================
 */


static sv_image_display( item, value, event )
Panel_item item;
int value;
Event *event;
{
        if( value == 0 )  /* Auto Cycle   */
                Autocycle = 499;
        else {
		Autocycle = 0;
		select_object( value );
	}
	return;
}
#endif QUICKTEST

/*
 *==============================================================
 *
 * Procedure cr_sv_frame
 *      Creates the frame and canvas.  Disables "Quit" from frame menu.
 *      Set the global canvas pixwin pointer and sets notifyall so
 *      a moved window will generate a WIN_REPAINT
 *
 *==============================================================
 */

void cr_sv_frame()
{
	Menu menu;
	Menu_item mi;
	
    	sv_win.frame = (Frame) window_create( NULL, FRAME,
    		FRAME_LABEL,  "GT QuickTest",
   		0);
   		
   	menu = window_get( sv_win.frame, WIN_MENU );
   	mi = menu_find( menu, MENU_STRING, "Quit", 0);
   	menu_set( mi, MENU_INACTIVE, TRUE, 0);
   	
#ifdef QUICKTEST
        sv_win.panel = window_create( sv_win.frame, PANEL, 0);
        panel_create_item(sv_win.panel, PANEL_BUTTON,
                PANEL_NOTIFY_PROC, sv_quit,
                PANEL_LABEL_IMAGE, panel_button_image(sv_win.panel," QUIT ",0,0), 0);
        panel_create_item(sv_win.panel, PANEL_BUTTON,
                PANEL_NOTIFY_PROC, sv_help,
                PANEL_LABEL_IMAGE, panel_button_image(sv_win.panel," HELP ",0,0), 0);
        panel_create_item(sv_win.panel, PANEL_CHOICE,
                PANEL_LABEL_STRING, "Image Display",
                PANEL_CHOICE_STRINGS, "Auto Cycle", "Sun Logo", "X29", "F15", 0,
                PANEL_NOTIFY_PROC, sv_image_display,
                0);

#endif
    	sv_win.canvas = (Canvas) window_create( sv_win.frame, CANVAS, 
    		CANVAS_REPAINT_PROC,  sv_repaint,
     		WIN_HEIGHT,   DEF_WIN_HEIGHT,
    		WIN_WIDTH,    DEF_WIN_WIDTH,
    		WIN_EVENT_PROC,  canvas_event_proc,
		CANVAS_COLOR24, TRUE,
#ifdef QUICKTEST
		WIN_BELOW, sv_win.panel,
		WIN_X, 0, WIN_Y, 27,
#endif
    		0 );
	window_fit( sv_win.canvas );

    	sv_win.pw = (Pixwin *) canvas_pixwin(sv_win.canvas);

        window_fit(sv_win.frame);

        win_setnotifyall(sv_win.pw->pw_windowfd, TRUE);

	return;
}

/*
 *==============================================================
 *
 * Procedure sv_win_show
 *      This procedure is stricly here so the conflicting "Window" declarations
 *      don't get hit.
 *
 *==============================================================
 */


sv_win_show()
{
	/*  Put Sunview Window up    */
	window_set( sv_win.frame, WIN_SHOW, TRUE, 0);
}

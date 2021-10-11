#ifndef lint
static char sccsid[] = "@(#)demo_x.c  1.1 94/10/31 SMI";
#endif

#ifdef XVIEW

/*
 ***********************************************************************
 *
 * demo_x.c
 *
 *	@(#)demo_x.c 1.5 91/05/30 14:51:04
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 *	Subroutines needed for Demo to support Sun XView window.
 *
 * 16-May-91 John M. Perry 	Initial file.
 * 17-May-91 John M. Perry 	Add XView pannel control buttons.
 * 30-May-91 John M. Perry 	Cleanup Quit/Destroy handling.
 *
 ***********************************************************************
 */

#include <xview/xview.h>
#include <xview/textsw.h>
#include <xview/panel.h>

static Frame  qt_frame;
static Frame  qt_help_frame;

extern done_with_program; 

/*
 *==============================================================
 *
 * Procedure Hierarchy:
 *
 *	qt_destroy_func
 *	qt_quit
 * 	qt_help
 *          xv_create
 *	    textsw_insert
 *	    win_insert
 *	qt_image_select
 *           select_object( value );
 *      cr_x_frame
 *	    xv_init
 *	    xv_create
 * 	    xv_set
 *	    XMapWindow
 *	    XSync
 *	    xv_get
 *
 *==============================================================
 */



/*
 *==============================================================
 *
 * Procedure qt_destroy_func 
 *      Intercepts "quit" from frame menu and terminates QUICKTEST
 *
 *==============================================================
 */

Notify_value
qt_destroy_func(client, status)
Notify_client client;
Destroy_status status;
{
	if (status == DESTROY_CLEANUP) 
	    done_with_program = 1;

	return(NOTIFY_DONE);
}



/*
 *==============================================================
 *
 * Procedure qt_quit
 *      Quits QUICKTEST from canvas menu
 *
 *==============================================================
 */


static
qt_quit(win, event)
Xv_Window win;
Event  *event;
{
        done_with_program = 1;
}



#ifdef QUICKTEST

/*
 *==============================================================
 *
 * Procedure qt_help
 *      displays Quick Test help window. 
 *
 *==============================================================
 */

qt_help(canvas_pw, event)
	Xv_Window canvas_pw;
	Event 	  *event;
{
	Textsw qt_help_text;
	extern char *helpmsg;

	if ( !qt_help_frame ) {
	    qt_help_frame = xv_create( qt_frame, FRAME, 
                                       FRAME_LABEL, "GT QuickTest Help",
				       NULL );
	    qt_help_text = xv_create( qt_help_frame, TEXTSW, NULL );
	    textsw_insert( qt_help_text, helpmsg, strlen(helpmsg) );
	}

	xv_set(qt_help_frame, XV_SHOW, 	TRUE, NULL);
	XFlush(xv_get(qt_help_frame, XV_DISPLAY));
}



/*
 *==============================================================
 *
 * Procedure qt_image_select
 *      Select image based on users panel selection
 *
 *==============================================================
 */
 
 
static 
qt_image_select( item, value, event )
	Panel_item item;
	int value;
	Event *event;
{
	extern Autocycle;

        if( value == 0 )  /* Auto Cycle   */
                Autocycle = 499;
        else {
                Autocycle = 0;
                select_object( value );
        }
}

#endif QUICKTEST

/*
 *==============================================================
 *
 * Procedure x_event_proc
 *      handles mouse events, brings up menu.
 *
 *==============================================================
 */


void
x_event_proc(canvas_pw, event)
Xv_Window canvas_pw;
Event *event;
{
        if (event_action(event) == ACTION_MENU && event_is_down(event)) {
            Menu menu = (Menu)xv_get(canvas_pw, WIN_CLIENT_DATA);
            menu_show(menu, canvas_pw, event, NULL);
        }
}


/*
 *==============================================================
 *
 * Procedure cr_x_window
 *      Creates the frame and canvas. 
 *
 *==============================================================
 */

Window
cr_x_window(nm, argc, argv, width, height)
char *nm;
int argc;
char **argv;
int width, height;
{
	Canvas canvas;
	Menu menu;
	Panel panel;
	Window x_win;
	Icon icon;

	qt_help_frame = NULL;

	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL);

	qt_frame = (Frame)xv_create ( NULL, FRAME,
                                      FRAME_LABEL,   nm,
                                      NULL );

	notify_interpose_destroy_func(qt_frame, qt_destroy_func);

#ifdef QUICKTEST

	panel = (Panel)xv_create( qt_frame, PANEL, 
				  XV_WIDTH, width,
				  XV_HEIGHT, 30,
				  NULL);

	xv_create( panel, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Quit",
        	   PANEL_NOTIFY_PROC,  qt_quit,
		   NULL );

	xv_create( panel, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "HELP",
		   PANEL_NOTIFY_PROC, qt_help,
		   NULL );

	xv_create( panel, PANEL_CHOICE,
		   PANEL_LABEL_STRING, "Image Display",
		   PANEL_CHOICE_STRINGS, 
			"Auto Cycle", "Sun Logo", "X29", "F15", NULL,
		   PANEL_NOTIFY_PROC, qt_image_select,
		   NULL );

#endif QUICKTEST


        canvas = (Canvas)xv_create(qt_frame, CANVAS, 
				   OPENWIN_AUTO_CLEAR, FALSE,
				   CANVAS_RETAINED,    FALSE,
				   XV_X,	  0,
                                   WIN_WIDTH,     width,
                                   WIN_HEIGHT,    height,
				   WIN_DEPTH, 24,
				   XV_VISUAL_CLASS, TrueColor,
#ifdef QUICKTEST
				   WIN_BELOW, panel,
#endif QUICKTEST
				   NULL);

	window_fit( qt_frame );


        menu = (Menu)xv_create(NULL, MENU,
                               MENU_TITLE_ITEM, nm,
                               MENU_ACTION_ITEM, "quit", qt_quit,
                               NULL);

	icon = xv_create(qt_frame, ICON, XV_LABEL, nm, NULL);

	xv_set(qt_frame, 
		FRAME_ICON, 	icon,
		XV_SHOW, 	TRUE, 
		NULL);

	XFlush(xv_get(qt_frame, XV_DISPLAY));

        xv_set( canvas_paint_window(canvas),
                WIN_EVENT_PROC,         x_event_proc,
                WIN_CLIENT_DATA,        menu,
                NULL );


	x_win = xv_get(canvas_paint_window(canvas), XV_XID);

	return(x_win);
}

#endif XVIEW

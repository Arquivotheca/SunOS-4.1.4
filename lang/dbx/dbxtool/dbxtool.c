#ifndef lint
static	char sccsid[] = "@(#)dbxtool.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

static char release_version[] = "@(#)RELEASE dbxtool 4.1-FCS 94/10/31";

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <suntool/sunview.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/textsw.h>
#include <suntool/ttysw.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_ioctl.h>
#include <suntool/wmgr.h>
#include <signal.h>
#include <ctype.h>
#include <syscall.h>

#include "defs.h"

#include "typedefs.h"
#include "buttons.h"
#include "cmd.h"
#include "dbxenv.h"
#include "dbxtool.h"
#include "disp.h"
#include "pipe.h"
#include "src.h"
#include "status.h"

/* Handle for the window-frame surrounding the whole tool */
private	Frame	base_frame;

public	Rect	tool_rect;		/* Current size and positon of tool */
private Boolean	_istool;		/* Is the tool up yet? */
private	Boolean	mysigwinch = true;	/* Sigwinch caused by dbxtool? */
public	Boolean	debugpipe;		/* Print pipe operators */
private	short	icon_data[] = {
#include <images/dbxtool.icon>
};
mpr_static(icon_image, TOOL_ICONWIDTH, TOOL_ICONHEIGHT, 1, icon_data);
private struct	icon	icon = {
	TOOL_ICONWIDTH,
	TOOL_ICONHEIGHT,
	NULL,
		0,
		0,
		TOOL_ICONWIDTH,
		TOOL_ICONHEIGHT,
	&icon_image,
		0,
		0,
		0,
		0,
	NULL,
	NULL,
	ICON_BKGRDCLR
};


Pixfontp pw_pfsysopen();
char	*mktemp();
char	*rindex();

private Notify_value base_frame_event();

/*
 * Bring up the interface for dbxtool.
 * At this point the .dbxinit file has been processed
 * so the size of the subwindows and the font have been 
 * determined.
 */
main(argc, argv)
int	argc;
char	*argv[];
{
	Pixfontp pixfont;
	char	*name;
	Rectp	rp;

	if (argc >= 2) {
		if (argv[1][0] == '-' and argv[1][1] == 'd') {
			debugpipe = true;
		}
	}
	name = argv[0];
#if DEBUG
	{ char name_space[ 80 ];
		strcpy( name_space, "TEST version of " );
		strcat( name_space, name );
		name = name_space;
	}
#endif

	/*
	 * Create and customize tool
	 */
	pixfont = pw_pfsysopen();
	set_toolfont(pixfont);
 
	argc--;
	argv++;
	my_frame_create( &argc, argv, name, &icon );

	/*
	 * Create and customize subwindows
	 */
	create_status_sw(base_frame, pixfont);
	create_src_sw(base_frame, pixfont);
	create_buttons_sw(base_frame, pixfont);

	/*
	 * IF we do */
	new_buttonsfont( get_toolfont( ) );
	/*	new_buttonsfont( get_toolfont( ) );
	 * here, then the rest of the create_* routines could
	 * use the default layouts?
	 */

	create_cmd_sw(base_frame, pixfont);
	create_display_sw(base_frame, pixfont);
	pw_pfsysclose();

	get_screen_dimensions();
	create_pipe(argc, argv);
	remember_curdir();
	wait_initdone();
	notify_set_input_func(get_cmd_sw_h(), pipe_input, get_pipein());
	new_buttonsfont(get_toolfont());
	change_size(true);
	rp = (Rectp) window_get( base_frame, WIN_RECT );
	tool_rect = *rp;

	_istool = true;

	if (not window_get( base_frame, FRAME_CLOSED ) ) {
		/* The window is open */
		dbx_display_source(get_curfile(), get_curline());
	} else {
		/*
		 * This "else" clause fixes an obscure bug that resulted
		 * in an empty frame when dbxtool had been started iconic.
		 */
		stretch( 0, -10 );
		stretch( 0,  10 );
	}


	(void) notify_interpose_event_func(base_frame, base_frame_event,
		NOTIFY_SAFE);

	window_main_loop( base_frame );

	dbx_cleanup();

	exit(0);
	/* NOTREACHED */
}

private Notify_value base_frame_event(client, event, arg, when)
Panel     client;
Eventp    event;
Notify_arg arg;
Notify_event_type when;
{
	Notify_value r;
	 
	if (event_id(event) == WIN_RESIZE)
		base_frame_resize();
	r = notify_next_event_func(client, event, arg, when);
	return(r);
}
										  

/*
 * Create the frame (outermost window) for dbxtool
 */
private	my_frame_create(argcp, argv, name, iconp)
int	*argcp;
char	**argv;
char	*name;
struct   icon *iconp;
{
	base_frame = window_create( (Window)NULL, FRAME,
		FRAME_LABEL,		name,
		FRAME_ICON,		iconp,
		FRAME_ARGC_PTR_ARGV,	argcp, argv,
		0);
	if ( base_frame == nil ) {
		fprintf(stderr, "Could not create the tool\n");
		exit(1);
	}
}

/*
 * Set the mysigwinch flag when a change in size is caused
 * by dbxtool instead of the user.
 */
public	set_mysigwinch(val)
Boolean	val;
{
	mysigwinch = val;
}

public 	Boolean	get_mysigwinch()
{
	return(mysigwinch);
}

/*
 * Return the handle for the base frame
 */
public	Frame	get_base_frame()
{
	return( base_frame ); 
} 

/*
 * Return the rect for the base frame
 */
public  Rect    * get_base_rect()
{
	return (Rect *) window_get( get_base_frame(), WIN_RECT );
}

/*
 * Return whether the tool is up yet or not.
 */
public	Boolean	istool()
{
	return(_istool);
}

/*
 * Find out if a given line is currently being displayed in a subwindow.
 * If so, fill in the rect it occupies.  'line' starts at one, but 'top'
 * and 'bot' start at zero.  This routine returns -1 if the subwindow
 * does not contain the line in question, or the screen line number if it
 * does.  The returned screen line number starts at one.
 */
public int text_contains_line(textsw, line, rect)
Textsw	textsw;
int	line;
Rectp	rect;
{
	int	status_or_slnum;
	int	top, bot;

	textsw_file_lines_visible( textsw, &top, &bot );
	--line;		/* make it be zero-indexed */

	if( top <= line &&  line <= bot ) {
		status_or_slnum = line - top +1;
	} else {
		status_or_slnum = -1;
		rect_construct(rect, -1, -1, -1, -1);
	}
	return(status_or_slnum);
}

/*
 * Position a the given 'line' at 'top_margin' lines from
 * the top of the window.  (The first 'line' is one; but the
 * first 'TEXTSW_FIRST_LINE' is zero.)
 */
public text_display(textsw, line, top_margin)
Textsw	textsw;
int	line;
int	top_margin;
{
	int	line_start;

	if (top_margin < 0 || 
	    top_margin > textsw_screen_line_count(textsw)) {
		top_margin = 0;
	}
	line_start = line - top_margin -1;
	window_set(textsw, TEXTSW_FIRST_LINE, line_start, 0 );
}

/*
 * Get the filename associated with a textsw.
 * Return true or false according to whether the
 * filename was retrieved or not.
 */
public Boolean text_getfilename(textsw, buf)
Textsw	textsw;
char	*buf;
{
	int	no_file;

	buf[0] = '\0';
	no_file = textsw_append_file_name(textsw, buf);
	if (no_file) {
		return(false);
	}
	return(true);
}

#define w_getrect( w ) (Rect *)window_get( (w), WIN_RECT )
public prsws()
{
	Rect   *rectp ;
	Frame   base_frame;
 
	base_frame = get_base_frame();
 
	rectp = w_getrect( base_frame );
	fprintf(stderr, "tool base frame\ttop %d\theight %d\twidth %d\n",
		rectp->r_top, rectp->r_height, rectp->r_width );
 
	rectp = w_getrect( get_statusw_h() );
	fprintf(stderr, "status\ttop %d\theight %d\n",
		rectp->r_top, rectp->r_height);
 
	rectp = w_getrect( get_srctext() );
	fprintf(stderr, "src\ttop %d\theight %d\n",
		rectp->r_top, rectp->r_height);
 
	rectp = w_getrect( get_buttonw_h() );
	fprintf(stderr, "buttons\ttop %d\theight %d\n",
		rectp->r_top, rectp->r_height);
 
	rectp = w_getrect( get_cmd_sw_h() );
	fprintf(stderr, "cmd\ttop %d\theight %d\n",
		rectp->r_top, rectp->r_height);
 
	rectp = w_getrect( get_disptext() );
	fprintf(stderr, "display\ttop %d\theight %d\n",
		rectp->r_top, rectp->r_height);
 
	fprintf(stderr, "\n");
}

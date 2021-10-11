#ifndef lint
static	char sccsid[] = "@(#)status.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/textsw.h>
#include <suntool/ttysw.h>
#include <suntool/panel.h>
#include <sys/dir.h>
#include <sys/param.h>

#include "defs.h"

#include "typedefs.h"
#include "cmd.h"
#include "dbxlib.h"
#include "dbxtool.h"
#include "src.h"
#include "status.h"
#include "pipe.h"

#define STATUS_LINES	2		/* Default # of lines in status sw */

#define set_show_item(i, b)	panel_set((i), PANEL_SHOW_ITEM, (b), 0)
#define set_label_item(i, s)    panel_set((i), PANEL_LABEL_STRING, (s), 0)
#define get_show_item(item)	(int) panel_get((item), PANEL_SHOW_ITEM)
#define get_label_item(item)	(char *) panel_get((item), PANEL_LABEL_STRING)
#define ishidden(item)		(get_show_item((item)) == FALSE)
#define isvisible(item)		(get_show_item((item)) == TRUE)
#define hide(i)			set_show_item((i), FALSE);
#define visible(i)		set_show_item((i), TRUE);
#define make_label(x, y, str)		panel_create_item(status, \
					PANEL_MESSAGE, \
					PANEL_LABEL_X,		PANEL_CU(x), \
					PANEL_LABEL_Y,		(y), \
					PANEL_SHOW_ITEM,	FALSE, \
					PANEL_LABEL_BOLD,	FALSE, \
					PANEL_LABEL_STRING,	(str), \
					0)
#define make_bold_label(x, y, str)	panel_create_item(status, \
					PANEL_MESSAGE, \
					PANEL_LABEL_X,		PANEL_CU(x), \
					PANEL_LABEL_Y,		(y), \
					PANEL_SHOW_ITEM,	FALSE, \
					PANEL_LABEL_BOLD,	TRUE, \
					PANEL_LABEL_STRING,	(str), \
					0)

/*
 * States for line1 of status subwindow
 */
typedef enum	line1	{
	CLEAR1,				/* Empty */
	AWAITING,			/* "Awaiting Execution" */
	FUNC_ONLY,			/* "Stopped in Func: " */
	FULL_BP,			/* "Stopped in File: Func: Line: " */
	TRACING,			/* "Tracing in File: Func: Line: " */
} Line1;

/*
 * Statues for line2 of status subwindow
 */
typedef enum	line2	{
	CLEAR2,				/* Empty */
	NO_SOURCE,			/* "No Source Displayed" */
	SRC_DISPLAYED			/* "Source Displayed: file: lines: " */
} Line2;

#define MIN_FILE_WIDTH		14	/* Min size of "File" field */
#define STOPPED_FUNC_WIDTH	14	/* Size of "Func" field */
#define STOPPED_LINE_WIDTH	9	/* Size of "Line" field */

private	Panel	status;			/* Panel subwindow for status */
private Pixfontp status_font;		/* Font used in status subwindow */
private Line1	line1 = CLEAR1;		/* State of line1 */
private Line2	line2 = CLEAR2;		/* State of line2 */
private	Notify_value status_event();
private char curdir[MAXPATHLEN+1];

/*
 * Various hard coded messages that appear in the status subwindow.
 */
private	char	stopped_file_str[]    = "Stopped in File: ";
private char	tracing_file_str[]    = "Tracing in File: ";
private	char	stopped_func_str[]    = "Func: ";
private	char	stopped_line_str[]    = "Line:  ";
private char	stopped_in_func_str[] = "Stopped in Func: ";
private	char	src_displayed_str[]   = "File Displayed:  ";
private	char	lines_displayed_str[] = "Lines: ";
private char	awaiting_str[]        = "Awaiting Execution";
private char	nosource_str[]        = "No Source Displayed";

/*
 * All the possible components of the status subwindow.
 */
private Panel_item awaiting;		/* "Awaiting Execution" */
private Panel_item nosource;		/* "No Source Displayed" */
private Panel_item stopped_in_func_msg;	/* "Stopped in Func: " */
private Panel_item stopped_in_func;	/* Stopped in func name */
private	Panel_item stopped_file_msg;	/* "Stopped in File: " */
private	Panel_item tracing_file_msg;	/* "Tracing in File: " */
private	Panel_item stopped_file;	/* Stopped in file name */
private	Panel_item stopped_func_msg;	/* "Func: " for stopped in */
private	Panel_item stopped_func;	/* Stopped in func name */
private	Panel_item stopped_line_msg;	/* "Line: " for stopped in */
private	Panel_item stopped_line;	/* Stopped in line number */
private	Panel_item src_displayed_msg;	/* "File Displayed:  " */
private	Panel_item src_displayed;	/* File displayed file name */
private	Panel_item lines_displayed_msg;	/* "Lines: " for source displayed */
private	Panel_item lines_displayed;	/* Displayed line numbers */

/*
 * Create the status subwindow.
 */
public	create_status_sw(base_frame, pixfont)
Frame	base_frame;
Pixfontp pixfont;
{
	/* window_fit_height won't do it.  */
	status = (Panel) window_create( base_frame, PANEL,
		PANEL_HEIGHT,		ATTR_ROW(STATUS_LINES),
		0);
	if (status == nil) {
		fprintf(stderr, "Could not create the status subwindow\n");
		exit(1);
	}
	notify_interpose_event_func(status, status_event, NOTIFY_SAFE);
	status_font = pixfont;
	create_status_options();
}

/*
 * Create the status options.
 * Leave as much room as possible for the file name portion.
 */
private create_status_options()
{
	Rect	rect;
	int	width;
	int	file_width;
	int	n;
	int	x;
	int	y;

	rect.r_width = (int) window_get(status, WIN_WIDTH);
	rect.r_height = (int) window_get(status, WIN_HEIGHT);
	width = rect.r_width/font_width(status_font);
	n = strlen(stopped_file_str) + strlen(stopped_func_str) +
	    STOPPED_FUNC_WIDTH + strlen(stopped_line_str) + STOPPED_LINE_WIDTH;
	file_width = width - n;
	if (file_width < MIN_FILE_WIDTH) {
		file_width = MIN_FILE_WIDTH;
	}

	window_set(status, PANEL_FONT, status_font, 0);
	awaiting = make_label(0, 0, awaiting_str);
	nosource = make_label(0, font_height(status_font),  nosource_str);

	/*
	 * Layout of line one: "Stopped in: Func: func"
	 */
	x = 0;
	stopped_in_func_msg = make_label(x, 0, stopped_in_func_str);
	x += strlen(stopped_in_func_str);
	stopped_in_func = make_bold_label(x, 0, "");

	/*
	 * Layout line one:  "Stopped in File: file.c  Func: func Line: xxx"
	 * or                "Tracing in File: file.c  Func: func Line: xxx"
	 */
	x = 0;
	stopped_file_msg = make_label(x, 0, stopped_file_str);
	tracing_file_msg = make_label(x, 0, tracing_file_str);
	x += strlen(stopped_file_str);
	stopped_file = make_bold_label(x, 0, "");
	x += file_width;
	stopped_func_msg = make_label(x, 0, stopped_func_str);
	x += strlen(stopped_func_str);
	stopped_func = make_bold_label(x, 0, "");
	x += STOPPED_FUNC_WIDTH;
	stopped_line_msg = make_label(x, 0, stopped_line_str);
	x += strlen(stopped_line_str);
	stopped_line = make_bold_label(x, 0, "");
	
	/*
	 * Layout line two: "Source Displayed: File: file.c Lines xxx-yyy"
	 */
	x = 0;
	y = font_height(status_font);
	src_displayed_msg = make_label(x, y, src_displayed_str);
	x += strlen(src_displayed_str);
	src_displayed = make_bold_label(x, y, "");
	x += file_width + strlen(stopped_func_str) + STOPPED_FUNC_WIDTH;
	lines_displayed_msg = make_label(x, y, lines_displayed_str);
	x += strlen(lines_displayed_str);
	lines_displayed = make_bold_label(x, y, "");
}

/*
 * Display the current status.
 * Display two pieces of status -
 *      1) the current breakpoint (if the program is active), and
 *      2) the current source being displayed.
 */
status_display()
{
	do_line1();
	do_line2();
}

/*
 * Set up line one of the status subwindow.
 */
private do_line1()
{
	char	*brkfile;
	char	*brkfunc;
	char	buf[100];
	int	brkline;

	brkline = get_brkline();
	brkfunc = get_brkfunc();
	brkfile = shortname(get_brkfile());
	if (get_isstopped()) {
		if (brkline <= 0 or brkfile == nil) {
			reset_line1(FUNC_ONLY);
			turn_on(stopped_in_func_msg);
			turn_on_value(stopped_in_func, brkfunc);
		} else {
			reset_line1(FULL_BP);
			turn_on(stopped_file_msg);
			turn_on_value(stopped_file, brkfile);
			turn_on(stopped_func_msg);
			turn_on_value(stopped_func, brkfunc);
			turn_on(stopped_line_msg);
			sprintf(buf, "%d", brkline);
			turn_on_value(stopped_line, buf);
		}
	} else if (get_istracing()) {
		reset_line1(TRACING);
		turn_on(tracing_file_msg);
		turn_on_value(stopped_file, brkfile);
		turn_on(stopped_func_msg);
		turn_on_value(stopped_func, brkfunc);
		turn_on(stopped_line_msg);
		sprintf(buf, "%d", brkline);
		turn_on_value(stopped_line, buf);
	} else {
		reset_line1(AWAITING);
		turn_on(awaiting);
	}
}

/*
 * Check if line one is entering a new state.
 */
private	reset_line1(newstate)
Line1	newstate;
{
	if (newstate != line1) {
		clear_line1(line1);
		line1 = newstate;
	}
}

/*
 * Clear all the items associated with a given state of line one.
 */
clear_line1(state)
Line1	state;
{
	switch (state) {
	case CLEAR1:
		break;

	case AWAITING:
		hide(awaiting);
		break;

	case FUNC_ONLY:
		hide(stopped_in_func_msg);
		hide(stopped_in_func);
		break;
	
	case FULL_BP:
		hide(stopped_file_msg);
		hide(stopped_file);
		hide(stopped_func_msg);
		hide(stopped_func);
		hide(stopped_line_msg);
		hide(stopped_line);
		break;

	case TRACING:
		hide(tracing_file_msg);
		hide(stopped_file);
		hide(stopped_func_msg);
		hide(stopped_func);
		hide(stopped_line_msg);
		hide(stopped_line);
		break;
	}
}

/*
 * Set up line two of the status subwindow
 */
private	do_line2()
{
	Textsw	srctext;
	char	textfile[MAXNAMLEN];
	int	top;
	int	bot;
	char	buf[100];

	srctext = get_srctext();
	textfile[0] = '\0';
	textsw_view_line_info(srctext, &top, &bot);
	if (text_getfilename(srctext, textfile) == true and
	    top != -1 and bot != -1) {
		reset_line2(SRC_DISPLAYED);
		turn_on(src_displayed_msg);
		turn_on_value(src_displayed, shortname(textfile));
		turn_on(lines_displayed_msg);
		sprintf(buf, "%d-%d", top, bot);
		turn_on_value(lines_displayed, buf);
	} else {
		reset_line2(NO_SOURCE);
		turn_on(nosource);
	}
}

/*
 * Check if line two is entering a new state.
 */
private	reset_line2(newstate)
Line2	newstate;
{
	if (newstate != line2) {
		clear_line2(line2);
		line2 = newstate;
	}
}

/*
 * Clear all the items associated with a given state of line two.
 */
private	clear_line2(state)
Line2	state;
{
	switch (state) {
	case CLEAR2:
		break;
	
	case NO_SOURCE:
		hide(nosource);
		break;

	case SRC_DISPLAYED:
		hide(src_displayed_msg);
		hide(src_displayed);
		hide(lines_displayed_msg);
		hide(lines_displayed);
		break;
	}
}

/*
 * Make an item visible.
 */
private	turn_on(item)
Panel_item item;
{
	if (ishidden(item)) {
		visible(item);
	}
}

/*
 * Make a message item visible and give it a new value.
 */
private turn_on_value(item, value)
Panel_item item;
char	*value;
{
	char 	*oldstr;
	
	if (isvisible(item)) {
		oldstr = get_label_item(item);
		if (strcmp(oldstr, value) == 0) {
			return;
		}
		hide(item);
	}
	set_label_item(item, value);
	visible(item);
}

/*
 * The font has changed.  Must change the font of each of 
 * the items.  The easiest way to do this is to free all existing
 * items and create new ones to replace them.
 */
public	new_statusfont(font)
Pixfontp font;
{
	Panel_item item;

	clear_line1(line1);
	clear_line2(line2);
	status_font = font;
	panel_each_item(status, item)
		panel_free(item);
	panel_end_each;
	create_status_options();
}

/*
 * ARGSUSED
 */
private	Notify_value status_event(client, event, arg, when)
Panel	client;
Eventp	event;
Notify_arg arg;
Notify_event_type when;
{
	Notify_value r;

        r = notify_next_event_func(client, event, arg, when);
	if (event_id(event) == WIN_RESIZE) {
		status_resize();
	}
	return(r);
}

/*
 * Resize routine for the status subwindow.
 */
private	status_resize()
{
	new_statusfont(status_font);
	status_display();
}

/*
 * Return the number of lines in the status subwindow.
 */
public	get_nstatuslines()
{
	return(STATUS_LINES);
}

/*
 * Return the height of the status subwindow in pixels.
 */
public	status_height()
{
	return(STATUS_LINES * font_height(status_font));
}

/*
 * Return the minimun height of the status subwindow.
 */
public	get_statusmin()
{
	return(STATUS_LINES * font_height(status_font));
}

/*
 * Get the handle for the status subwindow.
 */
public get_statusw_h()
{
	return  (int)status ;
}

public remember_curdir()
{

	char *end;

	end = curdir+strlen(getwd(curdir));
	*end++='/';
	*end='\0';
}

/* abbreviate an absolute file name by omitting cwd */
public String shortname(name)
char *name;
{
	/* abbreviate an absolute file name by omitting cwd */
	char *c;
	int d;
	if(name && name[0] == '/' && (d = substr(name,curdir))) {
		c = name+d;
	} else {
		c= name;
	}
	return c;
}

private substr(path,cwd)
char *path,*cwd;
{
	register char *p, *d;

	for(p=path,d=cwd; *p != '\0' && *d != '\0' && *p == *d; p++, d++ );
	if(*p == '\0') /* ran out of path ?! */
		return 0;
	/* if we did not match all of cwd backup to the last / */
	if( *d != '\0' ) {
		while(p != path && *p != '/')
			p--;
		if(p != path)
			p++;
	}
	return (p-path);
}

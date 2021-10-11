#ifndef lint
static	char sccsid[] = "@(#)buttons.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/selection.h>
#include <suntool/textsw.h>
#include <suntool/ttysw.h>

#undef ord
#include "defs.h"
#include "pipeout.h"

#include "typedefs.h"
#include "buttons.h"
#include "cmd.h"
#include "dbxenv.h"
#include "dbxlib.h"
#include "dbxtool.h"
#include "selection.h"
#include "menu.h"

#define BUTTON_VSPACE	3		/* Vertical space between buttons */
#define BUTTON_HSPACE	2		/* Horizontal space between buttons */
#define BUTTON_VBORDER	3		/* # pixels between button and border */
#define BUTTON_EXTRA_HEIGHT	4	/* Diff between button/font heights */
#define MINLEN		4		/* Min size button, in chars */
#define MAXLEN		10		/* Max size button, in chars */

typedef	struct	Button	*Button;

struct	Button	{
	char	*text;			/* Text of the button */
	Strfunc	sel_inter;		/* Selection interpretation routine */
	char	*show_text;		/* Text that is shown */
	Pixrectp image;			/* Button image */
	Panel_item handle;		/* Button handle */
	Button	next;			/* Linked list */
};

/*
 * The default buttons.
 */
private	struct	Button	def_buttons[] = {
	{ "print",	sel_expand },
	{ "print *",	sel_expand },
	{ "next",	sel_ignore },
	{ "step",	sel_ignore },
	{ "stop at",	sel_lineno },
	{ "cont",	sel_ignore },
	{ "stop in",	sel_expand },
	{ "clear",	sel_lineno },
	{ "where",	sel_ignore },
	{ "up",		sel_ignore },
	{ "down",	sel_ignore },
	{ "run",	sel_ignore },
	{ nil,		nil }
};

private	Button	button_hdr;		/* List of buttons */
private Button	*bpatch = &button_hdr;	/* Place to add a new button */

private	Panel	buttons;		/* Buttons panel subwin struct */
private int	button_sw_height;	/* Height of the buttons subwindow */
private	int	button_width;		/* Width of a button */
private	int	button_height;		/* Height of a button */
private int	nbuttons;		/* Number of buttons */
private	int	maxlen;			/* Max length of text in a button */
private Pixfontp button_font;		/* Button font */
private Boolean	buttons_displayed;	/* Have the buttons been displayed? */

private	int	button_cmd();
private Notify_value button_event();

/*
 * Create the buttons subwindow
 */
public	create_buttons_sw(base_frame, pixfont)
Frame	base_frame;
Pixfontp pixfont;
{
	/* Height will be calculated by window_fit_height */
	buttons = window_create( base_frame, PANEL,
		PANEL_SHOW_MENU, FALSE,
		PANEL_PAINT, PANEL_NONE,
		0);
	if (buttons == nil) {
		fprintf(stderr, "Could not create the buttons subwindow\n");
		exit(1);
	}
	(void)notify_interpose_event_func(buttons, button_event, NOTIFY_SAFE);
	button_font = pixfont;
	buttons_default();
	menu_default();
}

/*
 * Initialize the command buttons to their default values.
 */
private	buttons_default()
{
	Button	bp;

	button_hdr = def_buttons;
	nbuttons++;
	bpatch = &def_buttons[0].next;
	for (bp = &def_buttons[1]; bp->text != nil; bp++) {
		*bpatch = bp;
		bpatch = &bp->next;
		nbuttons++;
	}
	button_hdr = &def_buttons[0];

	/*
	 * Dynamically allocate the strings for the default
	 * buttons so the deallocator doesn't need to know 
	 * the difference.
	 */
	for (bp = def_buttons; bp->text != nil; bp++) {
		bp->text = strdup(bp->text);
	}
}

/*
 * Add a button to the list.
 */
private	add_button(str, func)
char	*str;
Strfunc	func;
{
	Button	bp;
	int	oldheight;

	oldheight = buttons_height();
	bp = new(Button);
	bp->text = strdup(str);
	bp->sel_inter = func;
	bp->next = nil;
	*bpatch = bp;
	bpatch = &bp->next;
	nbuttons++;
	if (buttons_displayed) {
		show_button(bp);
		redraw_buttons(oldheight);
	}
}

/*
 * Redraw the buttons subwindow.  A button has been added or deleted.
 * If the height of the subwindow has changed, do lots of work,
 * otherwise, just redraw this subwindow.
 */
private redraw_buttons(oldheight)
int	oldheight;
{
	if (buttons_displayed) {
		if (oldheight != buttons_height()) {
			change_size(false);
		} else {
			button_resize();
		}
	}
}

/*
 * Make all the buttons.  Make a pass over all the buttons to 
 * find the longest string and make all the buttons the same width.
 */
private	make_buttons()
{
	Button	bp;
	int	len;

	maxlen = 0;
	for (bp = button_hdr; bp != nil; bp = bp->next) {
		len = strlen(bp->text);
		if (len > maxlen) {
			maxlen = len;
		}
	}
	if (maxlen < MINLEN) {
		maxlen = MINLEN;
	} else if (maxlen > MAXLEN) {
		maxlen = MAXLEN;
	}
	window_set(buttons, PANEL_FONT, button_font, 0);
	for (bp = button_hdr; bp != nil; bp = bp->next) {
		show_button(bp);
	}
	button_width = button_hdr->image->pr_size.x;
	button_height = button_hdr->image->pr_size.y;
	buttons_displayed = true;
	window_fit_height( buttons );
}

/*
 * Show a button.  Create an image.
 */
private show_button(bp)
Button	bp;
{
	if (bp->show_text == nil) {
		bp->show_text = strdup(bp->text);
		if (strlen(bp->show_text) > maxlen) {
			bp->show_text[maxlen - 1] = '*';
			bp->show_text[maxlen] = '\0';
		}
	}
	bp->image = panel_button_image(buttons, bp->show_text, maxlen, 
	    button_font);
	bp->handle = panel_create_item(buttons, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, bp->image, 
		PANEL_NOTIFY_PROC, button_cmd,
		PANEL_SHOW_ITEM,   (bp->text[0] != '\0'),
		0);
}

/*
 * A command button has been picked.
 * Search the list of buttons for a matching handle and process
 * the command.
 * ARGSUSED
 */
private	button_cmd(handle)
Panel_item handle;
{
	Button	bp;

	for (bp = button_hdr; bp != nil; bp = bp->next) {
		if (handle == bp->handle) {
			interpret_selection(bp->sel_inter, bp->text);
			break;
		}
	}
}

/*
 * Interpret the selection.
 * Build a command and pass it to dbx.
 */
public interpret_selection(func, text)
Strfunc	func;
char	*text;
{
	char	*sel;
	char	buf[512];

	sel = func();
	if (sel != nil) {
		if (func == sel_command) {
			sprintf(buf, "%s", sel);
		} else {
			sprintf(buf, "%s %s\n", text, sel);
		}
	} else {
		strcpy(buf, "\n");
	}
	ttysw_cmd(get_cmd_sw_h(), buf, strlen(buf));
}

/*
 * Notification routine for events coming into the buttons subwindow.
 * Look for the RESIZE event to lay out the buttons again.
 * ARGSUSED
 */
private Notify_value button_event(client, event, arg, when)
Panel  	  client;
Eventp 	  event;
Notify_arg arg;
Notify_event_type when;
{ 
	Notify_value r;

	if (event_id(event) == MS_RIGHT) {
		menu_show(get_menu(), get_buttonw_h(), event, 0);
		r = NOTIFY_DONE;
	} else {
		r = notify_next_event_func(client, event, arg, when); 
		if (event_id(event) == WIN_RESIZE) {
			button_resize();
		}
	}
	return(r);
}

/*
 * The size of the buttons subwindow has changed.
 * Layout the buttons again.
 * Center the buttons in the subwindow, and use as many rows as 
 * necessary.
 * 
 */
public	button_resize()
{
	Button	bp;
	int	left;
	int	top;
	int	buttons_perrow;
	int	width;
	int	row_width;
	int	remaining;
	int	i;
	int	buttons_this_row;

	width = (int) get_toolwidth();

	buttons_perrow = buttons_per_row(nbuttons);
	i = buttons_perrow;	/* Force start of new row */
	top = -(button_height + BUTTON_VSPACE - BUTTON_VBORDER);
	remaining = nbuttons;
	for (bp = button_hdr; bp != nil; bp = bp->next) {
		if (++i >= buttons_perrow) {
			if (remaining < buttons_perrow) {
				buttons_this_row = remaining;
			} else {
				buttons_this_row = buttons_perrow;
			}
			row_width = buttons_this_row * button_width + 
				(buttons_this_row - 1) * BUTTON_HSPACE;
			left = (width - row_width)/2;
			top += button_height + BUTTON_VSPACE;
			i = 0;
		}
		panel_set(bp->handle, 
			PANEL_LABEL_Y, top,
			PANEL_LABEL_X, left,
			PANEL_PAINT, PANEL_NONE,
			0);
		left += button_width + BUTTON_HSPACE;
		remaining--;
	}
	panel_paint(buttons, PANEL_CLEAR);
}

/*
 * A user defined button.
 * If it is the first button clear the default ones.
 */
public	dbx_new_button(seltype, str)
Seltype	seltype;
char	*str;
{
	Strfunc	func;

	func = sel_func(seltype);
	add_button(str, func);
}

/*
 * Delete an existing button.
 * Find the button in the linked list of buttons and remove it.
 * Repaint the subwindow and adjust the back patch pointer.
 */
public	dbx_unbutton(str)
char	*str;
{
	Button	bp;
	Button	next;
	int	oldheight;

	oldheight = buttons_height();
	bpatch = &button_hdr;
	for (bp = button_hdr; bp != nil; bp = next) {
		next = bp->next;
		if (streq(bp->text, str)) {
			nbuttons--;
			*bpatch = next;
			if (bp->handle != nil) {
				panel_free(bp->handle);
			}
			dispose(bp->text);
			if (bp->show_text != nil) {
				dispose(bp->show_text);
			}
			dispose(bp);
		} else {
			bpatch = &bp->next;
		}
	}
	for (bp = button_hdr; bp != nil; bp = bp->next) {
		bpatch = &bp->next;
	}
	redraw_buttons(oldheight);
}

/*
 * Return the handle for the buttons subwindow
 */
public	Panel get_buttonw_h()
{
	return( buttons );
}

/*
 * Taking the width of the tool into account, 
 * return the desired number of buttons per row of the subwindow.  
 */
private buttons_per_row(max_per_row)
int max_per_row;
{
	int	fakewidth;
	register int	per_row;

	fakewidth = get_toolwidth() - (2 * TOOL_BORDERWIDTH) + BUTTON_HSPACE;
	per_row = fakewidth / (button_width + BUTTON_HSPACE);
	if (per_row > max_per_row) {
		per_row = max_per_row;
	}
	if (per_row <= 0) {
		per_row = 1;
	}
	return(per_row);
}

/*
 * Taking the width of the tool into account, 
 * return the desired height of the buttons subwindow.  
 */
public	buttons_height()
{
	Frame	base_frame;
	Rectp	rp;
	int	buttons_perrow;
	int	maxheight;
	int	rows;
	int	height;

	if (nbuttons == 0) {
		return(button_sw_height);
	}
	buttons_perrow = buttons_per_row(nbuttons);
	rows = nbuttons/buttons_perrow;
	if ((nbuttons % buttons_perrow) != 0) {
		rows++;
	}
	button_sw_height = rows * (button_height + BUTTON_VSPACE) - 
	    BUTTON_VSPACE + 2*BUTTON_VBORDER;
	base_frame = get_base_frame();
	rp = get_screenrect();
	maxheight = rp->r_height -
		(tool_stripeheight(base_frame)+2) - 
		get_statusmin() -
		tool_subwindowspacing(base_frame) - 
		get_srcmin() -
		tool_subwindowspacing(base_frame) - 
		/* button subwindow lives here */
		tool_subwindowspacing(base_frame) - 
		get_cmdmin() -
		tool_subwindowspacing(base_frame) - 
		get_dispmin() -
		tool_borderwidth(base_frame);
	if (button_sw_height > maxheight) {
		button_sw_height = maxheight;
	}
	return(button_sw_height);
}

/*
 * The font has changed.  Must remove all the buttons and create new
 * ones.
 */
public	new_buttonsfont(font)
Pixfontp font;
{
	Panel_item item;

	button_font = font;
	panel_each_item(buttons, item)
		panel_free(item);
	panel_end_each;
	make_buttons();
}


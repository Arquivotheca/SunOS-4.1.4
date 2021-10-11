#ifndef lint
static	char sccsid[] = "@(#)src.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <suntool/sunview.h>
#include <suntool/textsw.h>
#include <suntool/ttysw.h>
#include <suntool/walkmenu.h>
#include <suntool/alert.h>

#include <sys/param.h>
#include <sys/file.h>

#include "defs.h"

#include "typedefs.h"
#include "bp.h"
#include "cmd.h"
#include "dbxtool.h"
#include "src.h"

#define	NSRCLINES	20		/* Default # of src lines */
#define MINSRC		 1		/* Min # of src lines */
#ifndef public
#define SRC_OFFSET	32		/* Offset to start of text in pixels */
#endif

private	Textsw	srctext;		/* Handle for src text subwindow */
private	int	nsrclines = NSRCLINES;	/* Number of lines in src subwindow */
private	Boolean	readonly = true;	/* State of src window */
private Menu	menu;			/* Menu */
private Menu_item enable_item;		/* Menu item for enable */
private Menu_item disable_item;		/* Menu item for disable */

private	char	enable_str[] = "Enable Edit";
private	char	disable_str[] = "Disable Edit";
private	char	save_str[] = "Save Changes";
private	char	ignore_str[] = "Ignore Changes";

private	int	src_notify();
private	void	enable_edit();
private	void	disable_save();
private	void	disable_ignore();
private	Boolean	maybe_save_changes();

/*
 * Create the src subwindow
 */
public	create_src_sw( base_frame, pixfont)
Frame	base_frame;
Pixfontp pixfont;
{
	int	height;
	Textsw_status	status;
	Menu	disable_menu;

	height = get_nsrclines() * font_height(pixfont);
	srctext = window_create( base_frame, TEXTSW,
	    TEXTSW_STATUS,		&status,
	    WIN_HEIGHT,			height,
	    TEXTSW_LEFT_MARGIN,		SRC_OFFSET,
	    TEXTSW_NOTIFY_LEVEL,	TEXTSW_NOTIFY_STANDARD |
					TEXTSW_NOTIFY_PAINT    |
					TEXTSW_NOTIFY_REPAINT  |
					TEXTSW_NOTIFY_SCROLL,
	    TEXTSW_NOTIFY_PROC,		src_notify,
	    TEXTSW_BROWSING,		TRUE,
	    TEXTSW_DISABLE_CD,		TRUE,
	    0);
	if (srctext == nil || status != TEXTSW_STATUS_OKAY) {
		fprintf(stderr, "Could not create the source window\n");
		exit(1);
	}
	menu = (Menu) window_get(srctext, TEXTSW_MENU);
	enable_item = menu_create_item(
	    MENU_ACTION_ITEM, enable_str, enable_edit,
	    0);
	disable_menu = menu_create(
	    MENU_ACTION_ITEM, save_str, disable_save,
	    MENU_ACTION_ITEM, ignore_str, disable_ignore,
	    0);
	disable_item = menu_create_item(
	    MENU_PULLRIGHT_ITEM, disable_str, disable_menu,
	    0);

	menu_set(menu, 
	    MENU_APPEND_ITEM, enable_item,
	    0);
}

/*
 * Handle notifications from the src subwindow for events like
 * scrolling and repainting.  The annotations (stop signs, arrows)
 * must be redrawn then.
 */
private	src_notify(textsw, args)
Textsw          textsw;
Attr_avlist	args;
{
	Boolean		pass_through;
	Boolean		redraw;
	Attr_avlist	save_args = args;

	pass_through = false;
	redraw = false;
	for (; *args; args = attr_next(args)) {
	    switch ((Textsw_action)*args) {
		case TEXTSW_ACTION_SCROLLED:
		case TEXTSW_ACTION_PAINTED:
			redraw = true;
			break;
		default:
			pass_through = true;
			break;
	    }
	}
	if (pass_through) {
		textsw_default_notify(textsw, save_args);
	}
	if (redraw) {
		dbx_redraw(textsw);
	}
}



/*
 * Set the number of lines in the src subwindow.
 */
public	dbx_setsrclines(n, reformat)
int	n;
Boolean	reformat;
{
	Pixfontp pixfont;
	int	fheight;
	int	extra;

	if (n < MINSRC) {
		n = MINSRC;
	}
	if (n != nsrclines) {
		nsrclines = n;
		if (istool() and reformat) {
			pixfont = text_getfont(srctext);
			fheight = font_height(pixfont);
			extra = tool_too_tall();
			if (extra > 0) {
				nsrclines -= (extra + fheight - 1)/fheight;
			}
			change_size(false);
		}
	}
}

/*
 * Return the text subwindow handle for the src subwindow.
 */
public	Textsw	get_srctext()
{
	return(srctext);
}

/*
 * Return the height of the src subwindow in pixels
 */
public	src_height()
{
	Pixfontp pixfont;

	pixfont = text_getfont(srctext);
	return(nsrclines * font_height(pixfont));
}

/*
 * Return the number of lines in the src subwindow.
 */
public	get_nsrclines()
{
	return(nsrclines);
}

/*
 * Return the minimun height of the src subwindow.
 */
public	get_srcmin()
{
	Pixfontp pixfont;

	pixfont = text_getfont(srctext);
	return(MINSRC * font_height(pixfont));
}

/*
 * Notify proc for the "Enable Editing" menu item in the source window.
 */
private void enable_edit()
{
	char	filename[MAXPATHLEN];
	char	message[MAXPATHLEN*2];
	int	result;

	filename[0] = 0;
	if (textsw_append_file_name(srctext, filename) == 0) {
		if (access(filename, W_OK) != 0) {
			(void)sprintf(message,
				      "The file %s can not be written",
				      filename);
			result = alert_prompt(srctext, (Event*)NULL,
					      ALERT_MESSAGE_STRINGS,
					      message,
					      "Do you still want to edit it?",
					      0,
					      ALERT_BUTTON_YES, "No, cancel editing",
					      ALERT_BUTTON_NO, "Yes",
					      0);
			if (result == ALERT_YES) {
				return;
			}
		}
	}

	menu_set(menu,
	    MENU_REMOVE_ITEM, enable_item,
	    MENU_APPEND_ITEM, disable_item,
	    0);

	window_set(srctext,
	    TEXTSW_BROWSING, false,
	    TEXTSW_INSERTION_POINT, textsw_get(srctext, TEXTSW_FIRST),
	    0);

	readonly = false;
	remove_glyphs_from_file();
}

/*
 * Disable editing and save any changes.
 */
private void disable_save()
{
	if( window_get( srctext, TEXTSW_MODIFIED ) ) {

		if (textsw_save(srctext, 0, 0) != 0) {
			return;
		}
	}
	insert_disable_item();
	dbx_redraw(srctext);
}

/*
 * Disable editing and ignore any changes.
 */
private void disable_ignore()
{
	int	lineno;
	char	file[100];

	if( window_get( srctext, TEXTSW_MODIFIED ) ) {
		file[0] = '\0';
		lineno = (int) window_get(srctext, TEXTSW_FIRST_LINE);
		textsw_append_file_name(srctext, file);
		window_set(srctext,
		   TEXTSW_FILE, 	file,
		   TEXTSW_FIRST_LINE,	lineno,
		   0);
	}
	insert_disable_item();
	dbx_redraw(srctext);
}

/*
 * Put the disable editing item into the menu
 * and make the src window editable.
 */
private insert_disable_item()
{
	if (readonly == false) {
		menu_set(menu,
		    MENU_REMOVE_ITEM, disable_item,
		    MENU_APPEND_ITEM, enable_item,
		    0);

		window_set(srctext,
		    TEXTSW_BROWSING, true,
		    0);

		readonly = true;
	}
}

/*
 * Load a new file into the source window.
 * If the current file has any changes, prompt the user
 * to see if the changes should be saved.
 */
public Textsw_status dbx_load_src_file(file, lineno)
char	*file;
int	lineno;
{
	Textsw_status	status;

	maybe_save_changes(srctext);
	remove_glyphs_from_file();
	insert_disable_item();
	window_set(srctext,
	   TEXTSW_STATUS,	 &status,
	   TEXTSW_FILE, 	file,
	   TEXTSW_FIRST_LINE,	lineno,
	   0);
	return(status);
}

/*
 * See if the current file has any changes in it.
 * If so, then prompt the user to see if they should
 * be saved.
 * When we are all said and done, the src window will
 * be readonly again.
 */
private Boolean maybe_save_changes(srctext)
Textsw	srctext;
{
	Boolean	answer;
	char	message[100];
	char	file[100];

	answer = false;
	if( window_get( srctext, TEXTSW_MODIFIED ) ) {
		file[0] = '\0';
		textsw_append_file_name(srctext, file);
		sprintf(message, "Do you wish to save your changes to file %s",
		    file);
		answer = (Boolean) confirm_yes(message);
		if (answer == true) {
			textsw_save(srctext, 0, 0);
		}
	}
	return(false);
}

/*
 * Return whether the source window is currently editable.
 */
public Boolean src_is_editable()
{
	if (readonly) {
		return(false);
	} else {
		return(true);
	}
}

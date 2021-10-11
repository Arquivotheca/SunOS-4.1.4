#ifndef lint
static	char sccsid[] = "@(#)cmd.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/tty.h>
#include <suntool/ttysw.h>
#include <suntool/textsw.h>

#include "defs.h"

#include "typedefs.h"
#include "cmd.h"
#include "dbxlib.h"
#include "dbxtool.h"

#define NCMDLINES	12		/* Default # of cmd lines */
#define MINCMD		 1		/* Min # of cmd lines */

private	Cmdsw	cmd_sw;		/* Handle for cmd subwindow */
private	int	ncmdlines = NCMDLINES;	/* # of lines in the cmd subwindow */

/*
 * Create the cmd subwindow.
 */
public	create_cmd_sw( base_frame, pixfont)
Frame	 base_frame;
Pixfontp pixfont;
{
	int		height;
	char		gfx_win_name[50];

	height = get_ncmdlines() * font_height(pixfont);
	/*
	 * We must preserve the value of the gfx window environment variable
	 * in order to be able to debug gfx programs.  Since ttysw_create
	 * changes this setting, we save it here then reset it later on.
	 */
	we_getgfxwindow(gfx_win_name);

	/*
	 * Create the dbx command subwindow.
	 * We must set both the textsw-font and the ttysw-font (the window
	 * font).  Currently, each command subwindow is implemented as two
	 * windows -- a textsw and a ttysw.  We must make sure the fonts
	 * do not get out of sync.
	 * Don't fork until we create the dbx (see "create_pipe").
	 */
	cmd_sw = (Cmdsw) window_create( base_frame, TERM,
				WIN_HEIGHT,		height,
				WIN_FONT, 		pixfont, 
				TTY_ARGV,               TTY_ARGV_DO_NOT_FORK,
				0 ); 

	if (cmd_sw == nil) { 
		fprintf(stderr, "Could not create the command window\n");
		exit(1); 
	}

	we_setgfxwindow(gfx_win_name);
}

/*
 * Set the number of lines in the cmd subwindow.
 */
public	dbx_setcmdlines(n, reformat)
int	n;
Boolean reformat;
{
	Pixfontp pixfont;
        int     fheight;
	int     extra;

	if (n < MINCMD) {
		n = MINCMD;
	}
	if (n != ncmdlines) {
		ncmdlines = n;
		if (istool() and reformat) {
			pixfont = text_getfont(cmd_sw);
			fheight = font_height(pixfont);
			extra = tool_too_tall();
			if (extra > 0) {
				ncmdlines -= (extra + fheight - 1)/fheight;
			}
			change_size(false);
		}
	}
}

/*
 * Return the height of the cmd subwindow in pixels
 */
public	cmd_height()
{
	Pixfontp pixfont;

	pixfont = text_getfont(cmd_sw);
	return(ncmdlines * font_height(pixfont));
}

/*
 * Return the number of lines in the cmd subwindow.
 */
public	get_ncmdlines()
{
	return(ncmdlines);
}

/*
 * Return the minimum height of the cmd subwindow.
 */
public	get_cmdmin()
{
	Pixfontp pixfont;

	pixfont = text_getfont(cmd_sw);
	return(MINCMD * font_height(pixfont));
}

/*
 * Return the handle for the cmd subwindow.
 */
public	Textsw	get_cmd_sw_h()
{
	return cmd_sw;
}



/*
 * This is gross, will not work next release, requires access to
 * ttysw_impl.h, and is otherwise unfortunate.  It is to be hoped that
 * PSV will provide better hooks into the layout of tool subwindows.
 *
 * The problem here is that a command subwindow "contains" both a tty
 * subwindow (whose handle you have), and a text sw (whose handle you
 * don't have).  Setting the rect for the cmdsw does not also set the
 * corresponding textsw rect.  We do that here, in a very nasty way.
 *
 * cmd_sw is the handle for the command sw, and is actually a pointer
 * to a Ttysw.
 */
#define Cmdsw Cmdsw_impl	/* who cares? */
#define FILE  Textsw		/* a blatant lie */

#include "ttysw_impl.h"

cmd_set_textswrect ( rp ) Rectp rp; {
  Ttysw * cmd_tty;
  Textsw cmd_text;
  Rect rcopy;
	Pixfontp pixfont;
	cmd_tty = (Ttysw *) cmd_sw;

	cmd_text = (Textsw) cmd_tty->ttysw_hist;

	/* Must make a copy because window_set wrecks our rects */
	rcopy = *rp;
	pixfont = text_getfont(cmd_sw);
	window_set( cmd_text, WIN_RECT, &rcopy,
		TEXTSW_FONT, pixfont,
		WIN_FONT, pixfont,
		0 );
}

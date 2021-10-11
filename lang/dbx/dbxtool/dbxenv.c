#ifndef lint
static	char sccsid[] = "@(#)dbxenv.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <suntool/sunview.h>
#include <suntool/fullscreen.h>
#include <suntool/scrollbar.h>
#include <suntool/textsw.h>
#include <suntool/ttysw.h>
#include <suntool/panel.h>

#include "defs.h"

#include "typedefs.h"
#include "buttons.h"
#include "cmd.h"
#include "disp.h"
#include "dbxenv.h"
#include "dbxtool.h"
#include "src.h"
#include "status.h"

#define	WIDTH		80		/* Default width in chars */
#define MINWIDTH 	10		/* Min width in chars */

/*
 * This file contains a bunch of routines to process the
 * 'toolenv' command.
 */

private	char	fontfile[256];		/* Name of the current font */
private Pixfontp tool_font;		/* Current font */
private	int	tool_width = WIDTH;	/* Width of tool in chars */
private int	extra_width = 
    SCROLL_DEFAULT_WIDTH + SRC_OFFSET +
    2 * TOOL_BORDERWIDTH;		/* Additional width in pixels */
private Rect	screen_rect;		/* Dimenisions of physical screen */
private int	maxwidth;		/* Max width of tool in pixels */

char	*getenv();

/*
 * Allow the user to set the width of the tool.
 * If the tool is up, dynamically change the size of the tool.
 */
public	dbx_setwidth(width, reformat)
int	width;
Boolean	reformat;
{
	Frame	base_frame;
	Rectp	rectp;

	if (width < MINWIDTH) {
		width = MINWIDTH;
	}
	width *= font_width(tool_font);
	if (width > maxwidth) {
		width = maxwidth;
	}
	if (width != tool_width) {
		tool_width = width/font_width(tool_font);
		if (reformat) {
			base_frame = get_base_frame( );

			rectp = (Rectp) window_get( base_frame, WIN_RECT );
			rectp->r_width = width + extra_width;
			window_set( base_frame, WIN_RECT, rectp, 0 );
		}
	}
}

/*
 * Allow the user to change the font.
 * For the command window, we must change both the textsw-font
 * and the ttysw-font (the window font).  See `cmd.c`create_cmd_sw.
 */
public	dbx_setfont(file)
char	*file;
{
	Pixfontp pixfont;
	Textsw	srctext;
	Cmdsw	cmd_sw;
	Textsw	disptext;

	pixfont = pf_open(file);
	if (pixfont != NULL) {
		srctext = get_srctext();
		new_statusfont(pixfont);
		window_set(srctext, TEXTSW_FONT, pixfont, 0);
		new_buttonsfont(pixfont);
		cmd_sw = get_cmd_sw_h();
		window_set(cmd_sw, TEXTSW_FONT, pixfont,
				   WIN_FONT, pixfont,
				   0);
		disptext = get_disptext();
		window_set(disptext, TEXTSW_FONT, pixfont, 0);
		strcpy(fontfile, file);
		set_toolfont(pixfont);
		change_size(false);
	}
}

/*
 * Keep track of the font for the tool.
 */
public	set_toolfont(pixfont)
Pixfontp pixfont;
{
	tool_font = pixfont;
	new_textfont(pixfont);
}

/*
 * Return the tool's font.
 */
public Pixfontp get_toolfont()
{
	return(tool_font);
}

enum	sw_indices {
	STATUS,
	SRC,
	BUTTON,
	CMD,
	DISPLAY,
};
#define N_SWS	(ord(DISPLAY) + 1)

typedef int (*pfri)();                  /* pointer to function returning int */
struct	Subwindow {
	pfri    handle_func;            /* Func to get handle for the window */
	pfri    height_func;            /* Func to get the current height */
	pfri    min_func;               /* Func to get min height */
	pfri    set_nlines;             /* Func to set the # of lines in sw */
	Boolean min_h;			/* Are we at the min height? */
	Rect	rect;			/* New rect */
	int 	oldheight;
};
typedef	struct	Subwindow *Subwindow;
private	struct	Subwindow sw_info[N_SWS] = {
	{ (pfri)get_statusw_h, status_height, status_height, nil },
	{ (pfri)get_srctext, src_height, get_srcmin, dbx_setsrclines },
	{ (pfri)get_buttonw_h, buttons_height, buttons_height, nil },
	{ (pfri)get_cmd_sw_h, cmd_height, get_cmdmin, dbx_setcmdlines },
	{ (pfri)get_disptext, disp_height, get_dispmin, dbx_setdisplines },
};

/*
 * Change the size of the tool/subwindows.
 * Use the values of the "size" variables (width, srclines, etc.)
 * to lay out the tool and the subwindows.
 *
 * Here, we calculate both the location and height of each subwindow
 * (actually, the height is figured in each sw's height_func).
 * Eventually, we'd like the locations to be figured out by the window
 * system, via the WIN_BELOW attribute (specified by default during create).
 *
 * If your .dbxinit has a lot of button/unbutton commands in it, this can
 * get called a lot, before the "firsttime".  So if firsttime and istool()
 * are both false, do nothing.
 */
public change_size(firsttime) 
Boolean	firsttime;
{
	Frame	base_frame;
	Subwindow swp;
	Rect	toolrect;
	Rect	oldtoolrect;
	Rectp	rp;
	int	minus;
	int	extra;
	int	top;
	int	sw_offset;
	int	width;
	int	height;

	if( ! firsttime  &&  ! istool() ) return;

	base_frame = get_base_frame();
	width = get_toolwidth();

	top = 0;
	sw_offset = tool_sw_iconic_offset( (Tool *)base_frame );

	for (swp = sw_info; swp < &sw_info[N_SWS]; swp++) {

		rp = (Rectp) window_get( swp->handle_func(), WIN_RECT );
		swp->rect = *rp;

		swp->rect.r_top = top + sw_offset;
		swp->rect.r_width = WIN_EXTEND_TO_EDGE;
		swp->rect.r_height = swp->height_func();
		top += swp->rect.r_height +
			tool_subwindowspacing( base_frame );

		if (firsttime) {
		    my_setrect( swp->handle_func(), WIN_RECT, &swp->rect, 0);

		    if( swp == &sw_info[ (int)CMD ] ) {
			cmd_set_textswrect( &swp->rect );
		    }
		}
	}

	width += 2*tool_borderwidth( base_frame );
	height = top + tool_borderwidth( base_frame )
		     - tool_subwindowspacing( base_frame );

	/*
	 * The base frame's dimensions are the outer ones; we must
	 * include the stripe height and its goofy fudge factor.
	 */
	height += 2 + tool_stripeheight( base_frame );

	rp = (Rectp) window_get( base_frame, WIN_RECT );
	oldtoolrect = *rp;

	toolrect = oldtoolrect;
	toolrect.r_width = width;
	toolrect.r_height = height;
	my_setrect( base_frame, WIN_RECT, &toolrect, 0 );
	make_changes( base_frame, &toolrect, &oldtoolrect);
}

/*
 * The tool has been stretched.  Any change in width is applied to all
 * of the subwindows (automatically, via WIN_EXTEND_TO_EDGE).  Any change
 * in height is applied to the 'src', 'cmd', and 'disp' subwindows.
 * This routine is (was) called only from tool_layoutsubwindows.
 */
public	stretch(wdiff, hdiff)
int	wdiff;
int	hdiff;
{
	register Frame		base_frame;
	register Subwindow	swp;
	Rect			toolrect;
	Rect			oldtoolrect;
	Rectp			rp;
	int			top;
	register int		ix;
	register int		height;
	register int		width;
	int			button_diff;
	int			min_h;
	int			remainder;
	int			nsws;
	int			persw;
	int			nlines;
	int			nchars;

	if( ! istool() ) return;

	if( wdiff == 0  &&  hdiff == 0 ) return;

	base_frame = get_base_frame();
	oldtoolrect = tool_rect;
	rp = (Rectp) window_get( base_frame, WIN_RECT );
	toolrect = *rp;
	/*
	 * Get the current rects for each of the subwindows.
	 */
	for (swp = sw_info; swp < &sw_info[N_SWS]; swp++) {
		rp = (Rectp) window_get( swp->handle_func(), WIN_RECT );
		swp->rect = *rp;

		/*
		 * Handle sw boundary manager changes that the user
		 * invoked.
		 */
		if ((wdiff == 0) && (hdiff == 0)) {
			height = swp->rect.r_height;
			if (height != swp->oldheight) {
				ix = swp - sw_info;
				switch (ix) {
				case ord(SRC):
				case ord(CMD): 
				case ord(DISPLAY): 
					nlines= height / font_height(tool_font);
					swp->set_nlines(nlines, false);
					break;
				}
			}
		}
	}

	/*
	 * Change the width of the tool.  This should propagate
	 * to the subwindows due to their WIN_EXTEND_TO_EDGE attributes.
	 */
	if (wdiff != 0) {
		width = toolrect.r_width;
		nchars = (width - extra_width) / font_width(tool_font);
		dbx_setwidth(nchars, false);
	}

	/*
	 * Special case:  see if the change in width allows
	 * the height of the buttons subwindow to also change.
	 */
	swp = &sw_info[ord(BUTTON)];
	height = swp->height_func();
	if (height != swp->rect.r_height) {
		button_diff = height - swp->rect.r_height;
		toolrect.r_height += button_diff;
		swp->rect.r_height = height;
		for (swp++; swp < &sw_info[N_SWS]; swp++) {
			swp->rect.r_top += button_diff;
		}
	}


	top = 0;
	nsws = 3;
	persw = hdiff / nsws;
	remainder = hdiff % nsws;
	hdiff = 0;

	while (nsws > 0 and persw != 0) {
		for (swp = sw_info; swp < &sw_info[N_SWS]; swp++) {
			swp->rect.r_top = top;
			height = swp->rect.r_height;
			if (swp->min_h == false) {
				ix = swp - sw_info;
				switch (ix) {
				case ord(SRC):
				case ord(CMD): 
				case ord(DISPLAY): 
					height += persw;
					nlines= height/font_height(tool_font);
					swp->set_nlines(nlines, false);
					break;
				}
				min_h = swp->min_func();
				if (height < min_h) {
					hdiff -= min_h - height;
					height = min_h;
					swp->min_h = true;
					nsws--;
				}
			}
			swp->rect.r_height = height;
			top += height + tool_subwindowspacing( base_frame );
		}
		hdiff += remainder;
		if( nsws > 0 ) {
		    persw = hdiff / nsws;
		    remainder = hdiff % nsws;
		}
		hdiff = 0;
		wdiff = 0;
		/*
		top = tool_stripeheight( base_frame ) + 2;
		*/
		top = 0;
	}
	for (swp = sw_info; swp < &sw_info[N_SWS]; swp++) {
		swp->min_h = false;
	}
	make_changes( base_frame, &toolrect, &oldtoolrect);
	set_mysigwinch(false);
}

public void base_frame_resize()
        {
        Rect    rect, *wrect;
        int dh, dw;

        wrect = (Rect *)get_base_rect( );
        rect = *wrect;
        dw = rect.r_width - tool_rect.r_width;
        dh = rect.r_height - tool_rect.r_height;
        stretch (dw, dh);
        }

/*
 * The sw_info array now contains new rects for each of the subwindows.
 * Go thru and do some error checking and then make all the changes.
 */
private	make_changes( base_frame, toolrectp, oldtoolrectp)
Frame	base_frame;
Rectp	toolrectp;
Rectp	oldtoolrectp;
{
	Subwindow swp;
	int	minus;
	int	extra;
	private Boolean recurse = false;

	/*
	 * Check if the tool is too wide for the screen
	 * Shift it left to try to fit it in; if it still doesn't
	 * fit, make it narrower.
	 */
	if (rect_right(toolrectp) > rect_right(&screen_rect)) {
		extra = rect_right(toolrectp) - rect_right(&screen_rect);
		toolrectp->r_left -= extra;
		if (toolrectp->r_left < 0) {
			toolrectp->r_left = 0;
			/*
			 * We need not alter the widths of the subwindows
			 * because they're WIN_EXTEND_TO_EDGE
			 */
		}
	}
	
	/*
	 * Check if the tool is too tall
	 */
	if (rect_bottom(toolrectp) > rect_bottom(&screen_rect)) {
		extra = rect_bottom(toolrectp) - rect_bottom(&screen_rect);
		toolrectp->r_top -= extra;
		if (toolrectp->r_top < 0) {
			if (recurse == false) {
				/*
				 * In very strange circumstances we could
				 * get called recursively from stretch().
				 * If that happens, then thems the breaks.
				 */
				recurse = true;
				minus = toolrectp->r_top; 
				toolrectp->r_top = 0;
				stretch(0, minus);
				swp = &sw_info[ord(DISPLAY)];
				toolrectp->r_height = rect_bottom(&swp->rect);
			}
                } 
	}

	/*
	 * If the tool has changed sizes, change it.
	 */
	if (not rect_equal(oldtoolrectp, toolrectp)) {
		my_setrect( base_frame, WIN_RECT, toolrectp, 0 );
		set_mysigwinch(true);
		tool_rect = *toolrectp;
	}

	/*
	 * Change the size for each of the subwindows.
	 */
	for (swp = sw_info; swp < &sw_info[N_SWS]; swp++) {
		swp->rect.r_width = WIN_EXTEND_TO_EDGE ;
		if( swp == &sw_info[ (int)CMD ] ) {
			cmd_set_textswrect( &swp->rect );
		}
		swp->oldheight = swp->rect.r_height;
		my_setrect( swp->handle_func(), WIN_RECT, &swp->rect, 0 );
		set_mysigwinch(true);
	}
	recurse = false;
}

/*
 * Return the height of the tool.
 */
public	get_toolheight()
{
	Frame	base_frame;
	int	height;
	int	fheight;
	int	stripeheight;

	fheight = font_height(tool_font);
	if (istool()) {
		base_frame = get_base_frame();
		stripeheight = tool_stripeheight( base_frame );
	} else {
		stripeheight = fheight;
	}
	height = stripeheight + 2 +		/* This is the stripe height */
		 get_nstatuslines() * fheight +
		 TOOL_SUBWINDOWSPACING +
		 get_nsrclines() * fheight +
	         TOOL_SUBWINDOWSPACING +
		 buttons_height() +
		 TOOL_SUBWINDOWSPACING +
		 get_ncmdlines() * fheight +
		 TOOL_SUBWINDOWSPACING +
		 get_ndisplines() * fheight +
		 TOOL_SUBWINDOWSPACING;
	return(height);
}

/*
 * Return the width of the tool.
 */
public	get_toolwidth()
{
	int	width;
	
	width =	get_toolwidth_chars() * font_width(tool_font) +
		extra_width;
	return(width);
}

/*
 * Get the dimensions of the physical screen.
 */
public	get_screen_dimensions()
{
	Frame   base_frame;
	Rectp   rp;

	base_frame = get_base_frame();
	rp = (Rectp) window_get( base_frame, WIN_SCREEN_RECT );

	screen_rect = *rp;      /* copy the rect struct */

	maxwidth = screen_rect.r_width - extra_width;
}

/*
 * Return the tool width in characters.
 */
public	get_toolwidth_chars()
{
	return(tool_width);
}

/*
 * See if the current values of the environment variables
 * cause the tool to be taller than the screen.  If so, return
 * the number of pixels too large.
 */
public	tool_too_tall()
{
	int	height;

	height = get_toolheight();
	if (height > screen_rect.r_height) {
		return(height - screen_rect.r_height);
	}
	return(0);
}

/*
 * See if the current values of the environment variables
 * cause the tool to be fatter than the screen.  If so, return
 * the number of pixels too fat.
 */
public	tool_too_fat()
{
	int	width;

	width = get_toolwidth();
	if (width > screen_rect.r_width) {
		return(width - screen_rect.r_width);
	}
	return(0);
}
	
public	Rectp	get_screenrect()
{
	return(&screen_rect);
}

/*
 * Print the values of the dbxtool environment variables.
 */
public	dbx_prtoolenv()
{
	Cmdsw	cmd;
	char	*file;
	char	buf[100];

	if (fontfile[0] == '\0') {
		file = getenv("DEFAULT_FONT");
		if (file != nil) {
			strcpy(fontfile, file);
		} else {
			strcpy(fontfile, "<builtin>");
		}
	}
	cmd = get_cmd_sw_h();
	sprintf(buf, "font\t\t%s\n", fontfile);
	ttysw_output(cmd, buf, strlen(buf));
	sprintf(buf, "width\t\t%d\n", tool_width);
	ttysw_output(cmd, buf, strlen(buf));
	sprintf(buf, "srclines\t%d\n", get_nsrclines());
	ttysw_output(cmd, buf, strlen(buf));
	sprintf(buf, "cmdlines\t%d\n", get_ncmdlines());
	ttysw_output(cmd, buf, strlen(buf));
	sprintf(buf, "displines\t%d\n", get_ndisplines());
	ttysw_output(cmd, buf, strlen(buf));
	sprintf(buf, "topmargin\t%d\n", get_topmargin());
	ttysw_output(cmd, buf, strlen(buf));
	sprintf(buf, "botmargin\t%d\n", get_botmargin());
	ttysw_output(cmd, buf, strlen(buf));
}


/*
 * In 3.4, window_set(...WIN_RECT...) wrecks the rect you're
 * trying to set!  Copy it and set (and wreck) that one.
 */
my_setrect ( wh, wr, rp, z )
	Window wh;
	Rectp rp;
{
  Rect rcopy;

	rcopy = *rp;
	window_set( wh, wr, &rcopy, 0 );
}

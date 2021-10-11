#ifndef lint
static	char sccsid[] = "@(#)disperse.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <suntool/tool_hs.h>
#include <suntool/textsw.h>
#include <suntool/scrollbar.h>

#include "defs.h"

#include "typedefs.h"
#include "dbxtool.h"
#include "buttons.h"
#include "cmd.h"
#include "disp.h"
#include "src.h"
#include "status.h"

/*
 * Enumeratation to give each subwindow an index.
 * The subwindows are listed in the order to receive
 * changes. The `buttons' subwindow is handled specially.
 * The `status` subwindow does not change height.
 */
enum	subwindow	{
	SRC,
	CMD,
	DISP,
};
typedef enum	subwindow	Subwindow;
typedef	int	(*intfunc)();

#define NTXTSW		(ord(DISP) + 1)	/* Num of text subwindows */

/*
 * A struct of routines to set and inquire about the height
 * of a subwindow.
 */
struct	swinfo	{
	intfunc	get_height;		/* Get the height with this routine */
	intfunc	set_height;		/* Set the height with this routine */
	intfunc	get_min;		/* Get the min height with this one */
};
struct	swinfo	swinfo[NTXTSW] = {
	{ src_height,    dbx_setsrclines,    get_srcmin },
	{ cmd_height,    dbx_setcmdlines,    get_cmdmin },
	{ disp_height,   dbx_setdisplines,   get_dispmin },
};
private	int	sw_height[NTXTSW];

/*
 * The user has stretched the tool.  We need to disperse
 * the change in size here.  Disperse as equally as possible
 * across three of the text subwindows.
 */
disperse_change(oldrect)
Rectp	oldrect;
{
	Toolp	tool;
	Textsw	srctext;
	Rect	newrect;
	Pixfontp pixfont;
	int	diff;
	int	nlines;
	int	width;
	int	tool_height;
	int	but_height;
	int	i;

	tool = get_tool();
	win_getrect(tool->tl_windowfd, &newrect);

	/*
	 * Get the existing heights of the subwindows.
	 */
	diff = newrect.r_height - oldrect->r_height;
	for (i = 0; i < NTXTSW; i++) {
		sw_height[i] = swinfo[i].get_height();
	}
	but_height = buttons_height();

	/*
	 * Change the width.
	 */
	srctext = get_srctext();
	pixfont = (Pixfontp) textsw_get(srctext, TEXTSW_FONT);
	width = (int) textsw_get(srctext, TEXTSW_LEFT_MARGIN);
	width += newrect.r_width - SCROLL_DEFAULT_WIDTH - 2*TOOL_BORDERWIDTH;
	width = width/font_width(pixfont);
	dbx_setwidth(width, false);

	/*
	 * See if the change in width has caused the buttons
	 * subwindow to change height.  If so, this change
	 * must also be accounted for.
	 */
	diff += but_height - buttons_height();

	/*
	 * Disperse the height.
	 */
	nlines = diff/font_height(pixfont);
	if (nlines > 0) {
		stretch_tool(nlines, pixfont);
	} else if (nlines < 0) {
		shrink_tool(-nlines, pixfont);
	}
	tool_height = get_toolheight();
	if (tool_height != newrect.r_height) {
		newrect.r_height = tool_height;
		win_setrect(tool->tl_windowfd, &newrect);
		set_mysigwinch(true);
	}
}

/*
 * Divide the new height into units of the height of
 * the font.  Give an equal number of lines to each
 * text subwindow.
 */
stretch_tool(nlines, pixfont)
int	nlines;
Pixfontp pixfont;
{
	int	incr;
	int	remainder;
	int	sw_lines;
	int	i;

	incr = nlines / NTXTSW;
	remainder = nlines % NTXTSW;
	for (i = 0; i < NTXTSW; i++) {
		sw_lines = sw_height[i] / font_height(pixfont);
		sw_lines += incr;
		if (i < remainder) {
			sw_lines++;
		}
		swinfo[i].set_height(sw_lines, false);
	}
}

/*
 * The tool has gotten shorter.
 * Find out how many subwindows can shrink,
 * and spread the shrinkage across them.
 * Loop until everything settles down because
 * a subwindow may not be able to absorb all 
 * of its share of the shrinkage before hitting
 * its minimum.
 */
shrink_tool(nlines, pixfont)
int	nlines;
Pixfontp pixfont;
{
	int	n_sws;
	int	incr;
	int	height;
	int	remainder;
	int	minimum;
	int	sw_lines;
	int	font_h;	
	int	min_lines;
	int	i;

	font_h = font_height(pixfont);
	while (nlines > 0) {
		n_sws = ncan_shrink();
		if (n_sws == 0) {
			break;		/* should not happen */
		}
		incr = nlines / n_sws;
		remainder = nlines % n_sws;
		nlines = 0;
		for (i = 0; i < NTXTSW; i++) {
			minimum = swinfo[i].get_min();
			height = swinfo[i].get_height();
			if (minimum == height) {
				continue;
			}
			sw_lines = sw_height[i] / font_h;
			sw_lines -= incr;
			if (i < remainder) {
				sw_lines--;
			}
			min_lines = minimum / font_h;
			if (sw_lines < min_lines) {
				nlines += min_lines - sw_lines;
				sw_lines = min_lines;
			}
			swinfo[i].set_height(sw_lines, false);
		}
	}
}

/*
 * Find out how many subwindows can shrink
 */
ncan_shrink()
{
	int	minimum;
	int	height;
	int	i, n;

	n = 0;
	for (i = 0; i < NTXTSW; i++) {
		minimum = swinfo[i].get_min();
		height = swinfo[i].get_height();
		if (height > minimum) {
			n++;
		}
	}
	return(n);
}

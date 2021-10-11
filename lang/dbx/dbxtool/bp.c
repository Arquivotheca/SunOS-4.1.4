#ifndef lint
static	char sccsid[] = "@(#)bp.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */


#include <suntool/sunview.h>
#include <suntool/textsw.h>
#include <suntool/ttysw.h>
#include <suntool/panel.h>

#include <signal.h>
#include <sys/dir.h>
#include <ctype.h>

#include "defs.h"

#include "typedefs.h"
#include "bp.h"
#include "dbxlib.h"
#include "dbxtool.h"
#include "decorations.h"
#include "disp.h"
#include "src.h"
#include "status.h"
#include "pipe.h"

extern Textsw_mark textsw_add_glyph_on_line();

#define TOPMARGIN	3
#define BOTMARGIN	3

#ifndef public
enum Glyph_type	{ 
	GLYPH_UNKNOWN, 
	STOPSIGN, 
	HOLLOW_ARROW, 
	SOLID_ARROW,
	HOLLOW_ARROW_STOPSIGN, 
	SOLID_ARROW_STOPSIGN 
};

typedef enum Glyph_type	Glyph_type;

#define N_GLYPHS	(ord(SOLID_ARROW_STOPSIGN) + 1)
#endif

#define is_stoparrow(type)	((type) == SOLID_ARROW_STOPSIGN || \
			         (type) == HOLLOW_ARROW_STOPSIGN)
#define is_arrow(type)		((type) == SOLID_ARROW || \
			         (type) == HOLLOW_ARROW)
#define add_stop(type)		((type) == HOLLOW_ARROW? HOLLOW_ARROW_STOPSIGN:\
				 (type) == SOLID_ARROW? SOLID_ARROW_STOPSIGN: \
				 (type))
#define sub_stop(type)		((type) == HOLLOW_ARROW_STOPSIGN? HOLLOW_ARROW:\
				 (type) == SOLID_ARROW_STOPSIGN? SOLID_ARROW: \
				 (type))
#define sub_arrow(type)		((type) == HOLLOW_ARROW_STOPSIGN? STOPSIGN:\
				 (type) == SOLID_ARROW_STOPSIGN? STOPSIGN: \
				 (type))

typedef	struct	Brkpt	*Brkpt;
struct	Brkpt	{
	Glyph_type 	type;		/* Type of glyph */
	Brkpt		next;		/* Linked list */
	char		*filename;	/* File containing breakpoint */
	int		lineno;		/* Line number within filename of bp */
	Glyph_type	oldtype;	/* Previous type of glyph */
	Textsw_mark	glyph;		/* Glyph handle from textsw */
	int		count;		/* # of bp's at this filename/lineno */
};


private int	topmargin = TOPMARGIN;	/* Margin from top of src sw */
private int	botmargin = BOTMARGIN;	/* Margin from bot of src sw */
private Brkpt	bphdr;			/* Break point list */	
private Decorations decor;		/* Set of decorations for the font */
private Boolean redraw;		 	/* Has dbx_redraw() been called */

private	Brkpt	new_bp();
private Brkpt	find_bp();

/*
 * Display the source associated with a given location
 * in a file.  This may require changing source files.
 */
public dbx_display_source(source, srcline)
char	*source;
int	srcline;
{
	Textsw	srctext;
	Rect	rect;
	int	nlines;
	int	lineno;
	int	screenline;

	srctext = get_srctext();
	if (source[0] == '\0') {
		remove_glyphs_from_file();
		textsw_reset(srctext, 0, 0);
		status_display();
		return;
	}
	if (srcline <= 0) {
		srcline = 1;
	}
	redraw = false;
	if (file_in_src(source)) {
		screenline = text_contains_line(srctext, srcline, &rect);
		if (rect.r_left == -1) {
			text_display(srctext, srcline, topmargin);
		} else {
			nlines = textsw_screen_line_count(srctext);
			if (screenline < topmargin) {
				textsw_scroll_lines(srctext,
				    screenline - topmargin);
			} else if (screenline > nlines - botmargin) {
				textsw_scroll_lines(srctext,
				    (botmargin - nlines) + screenline + 1);
			}
		}
	} else {
		lineno = (srcline <= topmargin)? 0: srcline - 1 - topmargin;
		if (dbx_load_src_file(source, lineno) != TEXTSW_STATUS_OKAY) {
			return;
		}
	}
	if (not redraw) {
		dbx_redraw();
	}
}

/*
 * Assign a given glyph to a breakpoint.
 */
private bp_glyph(bp)
Brkpt	bp;
{
	Textsw	srctext;
	Pixrectp pr;

	if (src_is_editable()) {
		return;
	}
	if (bp->oldtype != bp->type) {
		srctext = get_srctext();
		remove_glyph(bp);
		bp->oldtype = bp->type;
		pr = glyphs[ord(bp->type)];
		bp->glyph = textsw_add_glyph_on_line(srctext,
		    bp->lineno, pr, PIX_SRC^PIX_DST, -pr->pr_size.x, 0, true);
	}
}

/*
 * Remove a glyph associated with a breakpoint.
 */
private remove_glyph(bp)
Brkpt	bp;
{
	Textsw	srctext;

	if (bp->glyph != nil) {
		srctext = get_srctext();
		textsw_remove_glyph(srctext, bp->glyph, true);
		bp->glyph = nil;
		bp->oldtype = GLYPH_UNKNOWN;
	}
}

/*
 * The src subwindow has been changed, either scrolled or redrawn.
 * Hilight the stopped at line or any breakpoints that may be
 * shown.
 */
public	dbx_redraw()
{
	redraw = true;
	status_display();
	assign_arrows();
	draw_glyphs();
}

/*
 * Draw all the glyphs
 */
private draw_glyphs()
{
	Brkpt	bp;

	for (bp = bphdr; bp != nil; bp = bp->next) {
		draw_glyph(bp);
	}
}

/*
 * Walk thru the list of breakpoints and see if the arrow is on the 
 * same line as a breakpoint.
 */
private assign_arrows()
{
	Brkpt	bp;
	char	*brkfile;
	char	*hollowfile;
	int	brkline;
	int	hollowline;

	if (not get_isstopped()) {
		return;
	}
	remove_arrows();
	brkfile = get_brkfile();
	brkline = get_brkline();
	hollowfile = get_hollowfile();
	hollowline = get_hollowline();
	assign_arrow1(brkfile, brkline, SOLID_ARROW);
	if (brkfile == nil or brkline != hollowline or
	   (hollowfile != nil and not streq(brkfile, hollowfile))) {
		assign_arrow1(hollowfile, hollowline, HOLLOW_ARROW);
	}
}

/*
 * Assign the arrow to a line.
 * If there is a breakpoint on the same line make it a 
 * stop arrow, otherwise use a regular arrow.
 */
private	assign_arrow1(filename, lineno, type)
char	*filename;
int	lineno;
Glyph_type	type;
{
	Brkpt	bp;

	if (filename == nil || lineno <= 0) {
		return;
	}
	bp = find_bp(filename, lineno);
	if (bp != nil) {
		bp->type = add_stop(type);
		bp->count++;
	} else {
		bp = new_bp(filename, lineno, type);
		bp->next = bphdr;
		bphdr = bp;
	}
}

/*
 * Remove any arrows.
 * If an arrow is found by itself, then remove it from the list.
 * If some type of stoparrow is found, convert it to just a stop.
 */
private	remove_arrows()
{
	Brkpt	bp;
	Brkpt	next;
	Brkpt	*bpatch;

	bpatch = &bphdr;
	for (bp = bphdr; bp != nil; bp = next) {
		next = bp->next;
		switch(bp->type) {
		case SOLID_ARROW:
		case HOLLOW_ARROW:
			remove_glyph(bp);
			*bpatch = next;
			dispose(bp->filename);
			dispose(bp);
			break;
		case SOLID_ARROW_STOPSIGN:
		case HOLLOW_ARROW_STOPSIGN:
			bp->type = sub_arrow(bp->type);
			bp->count--;
			bpatch = &bp->next;
			break;
		default:
			bpatch = &bp->next;
			break;
		}
	}
}

/*
 * Try to find a breakpoint in the given file at the given line number.
 */
private Brkpt	find_bp(file, line)
char	*file;
int	line;
{
	Brkpt	bp;

	for (bp = bphdr; bp != nil; bp = bp->next) {
		if (streq(file, bp->filename) and line == bp->lineno) {
			return(bp);
		}
	}
	return(nil);
}

/*
 * Put up an individual glyph.
 * See if the filename of the glyph matches the filename currently in the
 * textsw.  If so set the glyph.
 */
private draw_glyph(bp)
Brkpt	bp;
{
	Textsw	srctext;
	char	srcfile[MAXNAMLEN];

	srctext = get_srctext();
	if (bp->filename != nil and
	    text_getfilename(srctext, srcfile) == true and 
	    streq(srcfile, bp->filename)) {
		bp_glyph(bp);
	}
}

/*
 * A break point has been set.
 * Add it to the list.
 * If it is on the same line as the arrow, draw a stoparrow, otherwise, just
 * a regular stop sign.
 */
public	dbx_setbreakpoint(filename, lineno)
char	*filename;
int	lineno;
{
	Textsw	srctext;
	Brkpt	bp;
	Brkpt	*bpatch;

	filename = findsource(filename);
	bpatch = &bphdr;
	for (bp = bphdr; bp != nil; bp = bp->next) {
		if (streq(bp->filename, filename) and bp->lineno == lineno) {
			bp->count++;
			if (is_arrow(bp->type)) {
				bp->type = add_stop(bp->type);
			}
			return;
		}
		bpatch = &bp->next;
	}
	bp = new_bp(filename, lineno, STOPSIGN);
	*bpatch = bp;

	srctext = get_srctext();
	dbx_redraw(srctext);
}

/*
 * Create an initialize a new breakpoint struct
 */
private Brkpt new_bp(filename, lineno, type)
char	*filename;
int	lineno;
Glyph_type type;
{
	Brkpt	bp;

	bp = new(Brkpt);
	bp->type = type;
	bp->filename = strdup(filename);
	bp->lineno = lineno;
	bp->oldtype = GLYPH_UNKNOWN;
	bp->count = 1;
	bp->glyph = nil;
	bp->next = nil;
	return(bp);
}

/*
 * A breakpoint has been deleted.
 * Remove it from the list and, if it is visible in the src subwindow,
 * draw an arrow or white space by the line.
 */
public	dbx_delbreakpoint(filename, lineno)
char	*filename;
int	lineno;
{
	Brkpt	bp;
	Brkpt	*bpatch;

	filename = findsource(filename);
	bpatch = &bphdr;
	for (bp = bphdr; bp != nil; bp = bp->next) {
		if (streq(filename, bp->filename) && lineno == bp->lineno) {
			bp->count--;
			if (bp->count == 0) {
				remove_glyph(bp);
				*bpatch = bp->next;
				dispose(bp->filename);
				dispose(bp);
			} else if (bp->count == 1 and is_stoparrow(bp->type)) {
				bp->type = sub_stop(bp->type);
			}
			dbx_redraw();
			break;
		}
		bpatch = &bp->next;
	}
}


/*
 * Dbx is ready to resume execution.
 * Take the arrow down, and if it was over a stop sign, restore 
 * the stop sign.
 */
public	dbx_resume()
{
	remove_arrows();
	draw_glyphs();
}

/*
 * Starting another debugging session.
 * Remove all the breakpoints and glyphs.
 * Clear the src, and disp windows, and redraw the status subwindow.
 */
public	dbx_reinit(delete_bps)
Boolean	delete_bps;
{
	Textsw	srctext;
	Textsw	disptext;
	Brkpt	bp;
	Brkpt	next;

	remove_glyphs_from_file();
	if (delete_bps) {
		for (bp = bphdr; bp != nil; bp = next) {
			next = bp->next;
			dispose(bp->filename);
			dispose(bp);
		}
		bphdr = nil;
	}
	
	srctext = get_srctext();
	textsw_reset(srctext, 0, 0);
	disptext = get_disptext();
	textsw_reset(disptext, 0, 0);
	status_display();
}

/*
 * Remove all the glyphs from the current textsw file.
 */
public remove_glyphs_from_file()
{
	Textsw	srctext;
	Brkpt	bp;
	char	file[MAXNAMLEN];

	srctext = get_srctext();
	if (text_getfilename(srctext, file) == false) {
		return;
	}
	for (bp = bphdr; bp != nil; bp = bp->next) {
		if (streq(file, bp->filename)) {
			remove_glyph(bp);
		}
	}
}

/*
 * Is the given filename the one being shown in the src
 * subwindow?
 */
public Boolean	file_in_src(file)
char	*file;
{
	Textsw	srctext;
	char	textfile[MAXNAMLEN];

	if (file == nil) {
		return(false);
	}
	srctext = get_srctext();
	if (text_getfilename(srctext, textfile) == false or
	    not streq(textfile, file)) {
		return(false);
	}
	return(true);
}

/*
 * Place the given line of the given file in the 'topmargin'
 * slot and emphasize it for one second.
 */
public dbx_emphasize(file, lineno)
char 	*file;
Lineno	lineno;
{
	Textsw	srctext;
	Pixwinp	pixwin;
	Rect	rect;

	dbx_display_source(file, lineno);
	srctext = get_srctext();
	(void) text_contains_line(srctext, lineno, &rect);

	/* below sunview level -- naughty! */
	pixwin = (Pixwinp) textsw_get(srctext, TEXTSW_PIXWIN);
	pw_writebackground(pixwin, rect.r_left, rect.r_top,
	    rect.r_width, rect.r_height, PIX_NOT(PIX_SRC) ^ PIX_DST);
	sleep(1);
	pw_writebackground(pixwin, rect.r_left, rect.r_top,
	    rect.r_width, rect.r_height, PIX_NOT(PIX_SRC) ^ PIX_DST);
}

/*
 * Given a font, get the set of decorations for it.
 */
public	new_textfont(font)
Pixfontp font;
{
	decor = get_decorations(font);
}

/*
 * Set the bottom margin.
 * The current breakpoint will not appear in the last 'botmargin' lines
 * of the src subwindow.
 */
public	dbx_setbotmargin(n)
int	n;
{
	if (n > 0) {
		botmargin = n;
	}
}

/*
 * Set the top margin.
 * The current breakpoint will not appear in the top 'topmargin' lines
 * of the src subwindow.
 */
public	dbx_settopmargin(n)
int	n;
{
	if (n > 0) {
		topmargin = n;
	}
}

/*
 * Return the bottom margin.
 */
public	get_botmargin()
{
	return(botmargin);
}

/*
 * Return the top margin.
 */
public	get_topmargin()
{
	return(topmargin);
}


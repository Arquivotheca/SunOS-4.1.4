#ifndef lint
static	char sccsid[] = "@(#)disp.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <suntool/sunview.h>
#include <suntool/textsw.h>
#include <suntool/ttysw.h>
#include <sys/file.h>

#include "defs.h"

#include "typedefs.h"
#include "dbxlib.h"
#include "disp.h"

#define	NDISPLINES	3		/* Default # of display lines */
#define	MINDISP 	1		/* Min # of display lines */

private	Textsw	disptext;		/* Handle for display text subwin */
private	int	ndisplines = NDISPLINES;/* # of lines in display subwindow */

char	*mktemp();

/*
 * Create the display subwindow.
 * Must create the backing file here.
 * ARGSUSED
 */
public	create_display_sw(base_frame, pixfont)
Frame	base_frame;
Pixfontp pixfont;
{
	Textsw_status	status;

	disptext = (Textsw) window_create( base_frame, TEXTSW,
		TEXTSW_STATUS,		&status,
		TEXTSW_READ_ONLY,	TRUE,
		TEXTSW_DISABLE_CD,	TRUE,
		TEXTSW_DISABLE_LOAD,	TRUE,
		0);
	if (disptext == nil || status != TEXTSW_STATUS_OKAY) {
		fprintf(stderr, "Could not create the display window\n");
		exit(1);
	}
}

/*
 * Set the number of lines in the display subwindow.
 */
public	dbx_setdisplines(n, reformat)
int	n;
Boolean	reformat;
{
	Pixfontp pixfont;
	int	fheight;
	int	extra;

	if (n < MINDISP) {
		n = MINDISP;
	}
	if (n != ndisplines) {
		ndisplines = n;
		if (istool() and reformat) {
			pixfont = text_getfont(disptext);
			fheight = font_height(pixfont);
			extra = tool_too_tall();
			if (extra > 0) {
				ndisplines -= (extra + fheight - 1)/fheight;
			}
			change_size(false);
		}
	}
}


/*
 * Put something in the display subwindow.
 * Position it at the same lineno number as before.
 */
public	dbx_display(file)
char	*file;
{
	int		lineno;
	int		dummy;
	Textsw_status	status;

	/* The textsw interface is slightly inconsistent in its use of line
	 * numbers: textsw_view_line_info uses 1 as the origin, whereas the
	 * TEXTSW_FIRST_LINE attribute uses 0 as the origin, thus the
	 * subtraction in the call to window_set.
	 */
	textsw_view_line_info(disptext, &lineno, &dummy);
	window_set(disptext,
		   TEXTSW_STATUS, &status,
		   TEXTSW_FILE, file,
		   TEXTSW_FIRST_LINE, (lineno > 0) ? lineno-1 : 0,
		   0);
}

/*
 * Return the text subwindow handle for the display subwindow.
 */
public	Textsw	get_disptext()
{
	return(disptext);
}
/*
 * Return the height of the display subwindow in pixels
 */
public	disp_height()
{
	Pixfontp pixfont;

	pixfont = text_getfont(disptext);
	return(ndisplines * font_height(pixfont));
}

/*
 * Return the number of lines in the display subwindow.
 */
public	get_ndisplines()
{
	return(ndisplines);
}

/*
 * Return the minimum height of the display subwindow.
 */
public	get_dispmin()
{
	Pixfontp pixfont;

	pixfont = text_getfont(disptext);
	return(MINDISP * font_height(pixfont));
}

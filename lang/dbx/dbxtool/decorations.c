#ifndef lint
static	char sccsid[] = "@(#)decorations.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <suntool/sunview.h>
#include <suntool/ttysw.h>

#include "defs.h"

#include "typedefs.h"
#include "bp.h"
#include "decorations.h"

#define GLYPH_HEIGHT	16
#define GLYPH_WIDTH	16

#define N_DECORATIONS	4	/* Number of point sizes for decorations */

#ifndef public
/*
 * Structure of the three decorations for each of several differenct
 * pixel heights. 
 */
struct	decorations {
	int	height;
	Pixrectp hollow;
	Pixrectp solid;
	Pixrectp stopsign;
};
typedef struct	decorations *Decorations;

struct	pixrect	*glyphs[N_GLYPHS];

#endif

/*
 * The hollow arrows for the lines in the call stack
 */
private	short	hollow_data16[] = {
#include "hollow.16"
};
private	short	hollow_data14[] = {
#include "hollow.14"
};
private	short	hollow_data12[] = {
#include "hollow.12"
};
private	short	hollow_data10[] = {
#include "hollow.10"
};
mpr_static(hollow16, GLYPH_WIDTH, GLYPH_HEIGHT, 1, hollow_data16);
mpr_static(hollow14, GLYPH_WIDTH, GLYPH_HEIGHT, 1, hollow_data14);
mpr_static(hollow12, GLYPH_WIDTH, GLYPH_HEIGHT, 1, hollow_data12);
mpr_static(hollow10, GLYPH_WIDTH, GLYPH_HEIGHT, 1, hollow_data10);

/*
 * The solid arrows for the current line
 */
private	short	solid_data16[] = {
#include "solid.16"
};
private	short	solid_data14[] = {
#include "solid.14"
};
private	short	solid_data12[] = {
#include "solid.12"
};
private	short	solid_data10[] = {
#include "solid.10"
};
mpr_static(solid16, GLYPH_WIDTH, GLYPH_HEIGHT, 1, solid_data16);
mpr_static(solid14, GLYPH_WIDTH, GLYPH_HEIGHT, 1, solid_data14);
mpr_static(solid12, GLYPH_WIDTH, GLYPH_HEIGHT, 1, solid_data12);
mpr_static(solid10, GLYPH_WIDTH, GLYPH_HEIGHT, 1, solid_data10);

/*
 * The stop sign pixrects for lines with breakpoints
 */
private	short	stop_data16[] = { 
#include "stopsign.16"
};
private	short	stop_data14[] = { 
#include "stopsign.14"
};
private	short	stop_data12[] = { 
#include "stopsign.12"
};
private	short	stop_data10[] = { 
#include "stopsign.10"
};
mpr_static(stopsign16, GLYPH_WIDTH, GLYPH_HEIGHT, 1, stop_data16);
mpr_static(stopsign14, GLYPH_WIDTH, GLYPH_HEIGHT, 1, stop_data14);
mpr_static(stopsign12, GLYPH_WIDTH, GLYPH_HEIGHT, 1, stop_data12);
mpr_static(stopsign10, GLYPH_WIDTH, GLYPH_HEIGHT, 1, stop_data10);

/*
 * Array of decorations.  The array is sorted from tallest to shortest.
 */
struct	decorations	decorations[N_DECORATIONS] = {
	{ 16, &hollow16, &solid16, &stopsign16 },
	{ 14, &hollow14, &solid14, &stopsign14 },
	{ 12, &hollow12, &solid12, &stopsign12 },
	{ 10, &hollow10, &solid10, &stopsign10 },
};

private Pixrectp combine_glyphs();

/*
 * Given a font, return the set of decorations that matches
 * the height most closely.
 */
public	Decorations get_decorations(font)
Pixfontp font;
{
	Decorations dp;
	int	i;
	int	fheight;

	fheight = font_height(font);
	for (i = 0; i < N_DECORATIONS; i++) {
		dp = &decorations[i];
		if (fheight >= dp->height) {
			break;
		}
	}
	build_glyphs(dp);
	return(dp);
}

/*
 * Now that the decorations have been chosen build the glyphs that
 * will actually be displayed.
 */
private build_glyphs(dp)
Decorations dp;
{
	glyphs[ord(STOPSIGN)] = combine_glyphs(dp, nil, dp->stopsign);
	glyphs[ord(HOLLOW_ARROW)] = combine_glyphs(dp, nil, dp->hollow);
	glyphs[ord(SOLID_ARROW)] = combine_glyphs(dp, nil, dp->solid);
	glyphs[ord(HOLLOW_ARROW_STOPSIGN)] = combine_glyphs(dp, dp->hollow, 
	    dp->stopsign);
	glyphs[ord(SOLID_ARROW_STOPSIGN)] = combine_glyphs(dp, dp->solid, 
	    dp->stopsign);
}

/*
 * Take two glyphs and produce one larger one with the
 * two originals side by side.
 */
private Pixrectp combine_glyphs(dp, left, right)
Decorations dp;
Pixrectp left;
Pixrectp right;
{
	Pixrectp pr;

	pr = mem_create(2*GLYPH_WIDTH, GLYPH_HEIGHT, 1);
	if (left != nil) {
		pr_rop(pr, 0, 0, GLYPH_WIDTH, GLYPH_HEIGHT, PIX_SRC,
		    left, 0, 0);
	}
	if (right != nil) {
		pr_rop(pr, GLYPH_WIDTH, 0, GLYPH_WIDTH, GLYPH_HEIGHT, PIX_SRC,
		    right, 0, 0);
	}
	return(pr);
}

#ifndef lint
static char sccsid[] = "@(#)pr_reverse.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1983, 1987 by Sun Microsystems, Inc.
 */

/*
 * Two operations on operations, reversesrc and reversedst, are provided
 * for adjusting the operation code to take into account video reversing
 * of either the source or the destination.  These are implemented by
 * table lookup.  This process can be iterated, as in
 *	pr_reversedst[pr_reversesrc[op]].
 *
 * Note that the "op" that these tables reverses is shifted down one bit
 * from the "op" described in the header file.  In other words, it's not
 * what you expect, so look again.
 *
 * The PIX_OP_REVERSE{SRC,DST} macros in pr_impl_util.h should generally
 * be used instead of these tables.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>

char pr_reversedst[16] = {	/* NB: reverses both input and output */
	15, 13, 14, 12,
	 7,  5,  6,  4,
	11,  9, 10,  8,
	 3,  1,  2,  0
};

char pr_reversesrc[16] = {	/* transpose of the 4x4 identity */
	0,  4,  8,  12,
	1,  5,  9,  13,
	2,  6, 10,  14,
	3,  7, 11,  15
};


/*
 * Reverse video colormap utilities.
 */

static u_char black = 0, white = 255;

pr_blackonwhite(pr, min, max)
	Pixrect *pr;
	int min, max;
{
	(void) pr_putcolormap(pr, max, 1, &black, &black, &black);
	(void) pr_putcolormap(pr, min, 1, &white, &white, &white);
	return 0;
}

pr_whiteonblack(pr, min, max)
	Pixrect *pr;
	int min, max;
{
	(void) pr_putcolormap(pr, max, 1, &white, &white, &white);
	(void) pr_putcolormap(pr, min, 1, &black, &black, &black);
	return 0;
}

pr_reversevideo(pr, min, max)
	Pixrect *pr;
	int min, max;
{
	u_char rgb[2][3];

	(void) pr_getcolormap(pr, min, 1, &rgb[0][0], &rgb[0][1], &rgb[0][2]);
	(void) pr_getcolormap(pr, max, 1, &rgb[1][0], &rgb[1][1], &rgb[1][2]);
	(void) pr_putcolormap(pr, max, 1, &rgb[0][0], &rgb[0][1], &rgb[0][2]);
	(void) pr_putcolormap(pr, min, 1, &rgb[1][0], &rgb[1][1], &rgb[1][2]);
	return 0;
}

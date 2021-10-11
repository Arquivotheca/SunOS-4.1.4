| "@(#)s1152.pp.h 1.1 94/10/31 SMI"

| Copyright 1985 by Sun Microsystems, Inc.

/* Screen Size dependent Constants for GP Paint Processor.
*/

#define XMax		1151
#define YMax		899
#define SWidth		1152	/* Screen width */
#define SHeight		900	/* Screen height. */
#define SWWidth		144	/* Screen word (plane) width = sizeof(short) * SWidth/8 * sizeof(short) = 2 * SWidth / 16. */
#define SWPixels	64800	/* Pixels per plane = SHeight * SWWidth = 900 * 9. */
#define IncXY		1153	/* Screen width + 1, for incrementing x and y. */
#define IncXDecY	-1151	/* +1 - Screen Width, for increment x and decrementing y. */

| "@(#)s1024.pp.h 1.1 94/10/31 SMI"

| Copyright 1985 by Sun Microsystems, Inc.

/* Screen Size dependent Constants for GP Paint Processor.
    Width 1024.
*/

#define XMax		1023
#define YMax		1023
#define SWidth		1024	/* Screen width */
#define SHeight		1024	/* Screen height. */
#define SWWidth		128	/* Screen word (plane) width = sizeof(short) * SWidth/8 * sizeof(short) = 2 * SWidth / 16. */
#define SWPixels	65536	/* Pixels per plane = SHeight * SWWidth = 900 * 9. */
#define IncXY		1025	/* Screen width + 1, for incrementing x and y. */
#define IncXDecY	-1023	/* +1 - Screen Width, for increment x and decrementing y. */

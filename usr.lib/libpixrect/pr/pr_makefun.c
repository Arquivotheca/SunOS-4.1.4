/* @(#)pr_makefun.c 1.1 94/10/31 SMI */

/*
 * Copyright 1986-1989 by Sun Microsystems, Inc.
 */

/*
 * Pixrect make operation vector.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <sun/fbio.h>

Pixrect *bw2_make();
Pixrect *cg2_make();
Pixrect *cg3_make();
Pixrect *cg4_make();
Pixrect *cg6_make();
Pixrect *cg8_make();
Pixrect *cg9_make();
Pixrect *gp1_make();
Pixrect *tv1_make();
Pixrect *gt_make();


Pixrect *(*pr_makefun[FBTYPE_LASTPLUSONE])() = {
	0,			/* FBTYPE_SUN1BW */
	0,			/* FBTYPE_SUN1COLOR */
	bw2_make,		/* FBTYPE_SUN2BW */
	cg2_make,		/* FBTYPE_SUN2COLOR */
	gp1_make,		/* FBTYPE_SUN2GP */
	0,			/* FBTYPE_SUN5COLOR */
	cg3_make,		/* FBTYPE_SUN3COLOR */
	cg8_make, 		/* FBTYPE_MEMCOLOR == 7*/
	cg4_make,		/* FBTYPE_SUN4COLOR == 8 */
	0,			/* FBTYPE_NOTSUN1 == 9 */
	0,			/* FBTYPE_NOTSUN2 == 10 */
	0,			/* FBTYPE_NOTSUN3 == 11 */
	cg6_make,		/* FBTYPE_SUNFAST_COLOR == 12 */
	cg9_make,		/* FBTYPE_SUNROP_COLOR == 13 */
	tv1_make,		/* FBTYPE_SUNFB_VIDEO == 14 */
	0,			/* 15 */
	0,			/* 16 */
	0,			/* 17 */
	gt_make,		/* FBTYPE_SUNGT == 18 */
	0,			/* 19 */
};

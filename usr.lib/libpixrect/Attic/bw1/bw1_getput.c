#ifndef lint
static char sccsid[] = "@(#)bw1_getput.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1983, 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun1 Black-and-White get and put single pixel
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/bw1reg.h>
#include <pixrect/bw1var.h>

bw1_get(pr, x, y)
	register Pixrect *pr;
	register int x, y;
{
	register struct bw1pr *prd;
	register struct bw1fb *fb;
	register short *by;
	register short pumpprimer;

 	if (x < 0 || x >= pr->pr_size.x || 
		y < 0 || y >= pr->pr_size.y)
		return PIX_ERR;

	prd = bw1_d(pr);
	fb = prd->bwpr_va;

	bw1_touch(*bw1_xsrc(fb, x + prd->bwpr_offset.x));
	by = bw1_ysrc(fb, y + prd->bwpr_offset.y);
	pumpprimer = *by;
	if (prd->bwpr_flags & BW_REVERSEVIDEO) {
		if (*by >= 0)
			return 1;
	}
	else if (*by < 0)
		return 1;
	return 0;
}

bw1_put(pr, x, y, value)
	register Pixrect *pr;
	register int x, y;
	int value;
{
	register struct bw1pr *prd;
	register struct bw1fb *fb;

 	if (x < 0 || x >= pr->pr_size.x || 
		y < 0 || y >= pr->pr_size.y)
		return PIX_ERR;

	prd = bw1_d(pr);
	fb = prd->bwpr_va;

	bw1_setmask(fb, 0);
	bw1_setwidth(fb, 1);
	bw1_setfunction(fb, 
		prd->bwpr_flags & BW_REVERSEVIDEO ? ~BW_SRC:BW_SRC);
	bw1_touch(*bw1_xdst(fb, x + prd->bwpr_offset.x));
	*bw1_ydst(fb, y + prd->bwpr_offset.y) = 
		value & 1 ? ~0 : 0;

	return 0;
}

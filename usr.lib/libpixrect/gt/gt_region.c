#ifndef lint
static char     sccsid[] =      "@(#)gt_region.c	1.1 94/10/31 SMI";
#endif
 
/* Copyright 1990 - 1991 Sun Microsystems, Inc. */


#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <gt/gtvar.h>

Pixrect *
gt_region(pr, x, y, w, h)
	Pixrect *pr;
	int x, y, w, h;
{
	register Pixrect *newpr;
	struct pr_subregion srcreg;
	static struct pr_prpos nulpos = {0, { 0, 0 } };
	struct gt_data *gtpr;
	struct  gt_fb	*fbp;

	/* allocate gt pixrect */
	newpr = alloctype(Pixrect);
	if (newpr == 0) 
		return (0);

	gtpr = alloctype(struct gt_data);
	if (gtpr == 0) {
		free((char *)newpr);
		return (0);
	}
	*gtpr = *gt_d(pr);

	/* compute region offset, size */
	srcreg.pr = pr;
	srcreg.pos.x = x;
	srcreg.pos.y = y;
	srcreg.size.x = w;
	srcreg.size.y = h;

	pr_clip(&srcreg, &nulpos);

	*newpr = *pr;			    
	newpr->pr_size = srcreg.size;	    
	newpr->pr_data = (char*) gtpr;	    

	gtpr->mprp.mpr.md_offset.x += srcreg.pos.x;
	gtpr->mprp.mpr.md_offset.y += srcreg.pos.y;
	gtpr->mprp.mpr.md_primary = 0;
	
	/* Update this offset to all the fbs */
	for (fbp = &gtpr->fb[0];  fbp < &gtpr->fb[GT_NFBS];  fbp++) {
	    fbp->mprp.mpr.md_offset = gtpr->mprp.mpr.md_offset;
	    fbp->mprp.mpr.md_primary = 0;
	}
	return newpr;
}

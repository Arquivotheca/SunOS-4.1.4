#ifndef lint
static char Sccsid[] = "@(#)tv1_region.c	1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1988, Sun Microsystems, Inc.
 */


#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/tv1var.h>

extern char *calloc();
Pixrect *
tv1_region(pr, x, y, w, h)
	Pixrect	*pr;
	int x, y, w, h;
{
	struct pr_subregion srcreg;
	static struct pr_prpos nulpos = {
		0, { 0, 0 }
	};
	register struct cg4fb *fbp;
	register Pixrect *newpr;
	register struct tv1_data *tvd;
	register struct pixrectops *ops;

	extern struct tv1_data *get_tv1_data();

	if ((newpr = alloctype(Pixrect)) == 0)
		return 0;

	if ((tvd = alloctype(struct tv1_data)) == 0) {
		free((char *)newpr);
		return 0;
	}
	if ((ops = alloctype(struct pixrectops)) == 0) {
	    free((char *)tvd);
	    free((char *)newpr);
	    return(0);
	}

	srcreg.pr = pr;
	srcreg.pos.x = x;
	srcreg.pos.y = y;
	srcreg.size.x = w;
	srcreg.size.y = h;

	pr_clip(&srcreg, &nulpos);

	/* copy old pixrect; set size, data pointer */
	*newpr = *pr;
	newpr->pr_size = srcreg.size;
	newpr->pr_data = (caddr_t) tvd;

	/* copy old pixrect data */
	*tvd = *get_tv1_data(pr);
	/* insert pointer to real data by putting it on the list */
	tv1_save_data(tvd, newpr);

	/* copy ops */
	newpr->pr_ops = ops;
	*newpr->pr_ops = *pr->pr_ops;

	/* this is a secondary pixrect */
	tvd->flags &= ~(CG4_PRIMARY);

	/* fix up all the offsets */
	{
	    
	}
	/* create emulated subregion */
	tvd->emufb = pr_region(get_tv1_data(pr)->emufb, x, y, w, h);
	/* create enable plane subregion */

	tvd->pr_video_enable = pr_region(get_tv1_data(pr)->pr_video_enable, x, y, w, h);
	if (tvd->active == -1) {
	    newpr->pr_data = tvd->emufb->pr_data;
	} else {
	     newpr->pr_data = tvd->pr_video_enable->pr_data;
	}
	return newpr;
}

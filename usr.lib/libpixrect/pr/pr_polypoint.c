#ifndef lint
static char sccsid [] = "@(#)pr_polypoint.c	1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */
 
/*
 * Draw a list of points on the screen
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/gp1var.h>
#include <pixrect/cg12_var.h>
#include <pixrect/gtvar.h>

pr_polypoint(pr, dx, dy, n, ptlist, op)
	Pixrect *pr;
	int dx, dy, n;
	register struct pr_pos *ptlist;
	int op;
{
	extern int mem_rop(), mem_polypoint();
	extern int cg2_rop(), cg2_polypoint();
	extern int gp1_rop();
	extern int cg6_rop(), cg6_polypoint();

	static struct funcs {
		int (*rop)(), (*polypoint)();
	} functab[] = {
		{ mem_rop, mem_polypoint },
		{ cg2_rop, cg2_polypoint },
		{ cg6_rop, cg6_polypoint },
		{ cg9_rop, mem_polypoint },
		{ 0, 0 }
	};

	register int (*ropfunc)();
	register struct funcs *p;

	ropfunc = pr->pr_ops->pro_rop;

	if (ropfunc == gp1_rop) {
	    if (gp1_d(pr)->fbtype == FBTYPE_SUN2COLOR){

		return cg2_polypoint(pr, dx, dy, n, ptlist, op);

	    } else if (gp1_d(pr)->fbtype == FBTYPE_SUNGP3) {
		Pixrect dmempr;
		
		CG12_PR_TO_MEM(pr,dmempr);
		return mem_polypoint(pr, dx, dy, n, ptlist, op);

	    } else if (gp1_d(pr)->fbtype == FBTYPE_SUNROP_COLOR) {
		Pixrect         mempr;

		mempr = *pr;
		mempr.pr_data = (caddr_t) &gp1_d(pr)->cgpr.cg9pr;
		return mem_polypoint(mempr, dx, dy, n, ptlist, op);

	    }
	} else if (ropfunc == gt_rop) {
	    Pixrect	    mempr;

	    gt_pr_to_mem(pr, mempr, op);
            return mem_polypoint(&mempr, dx, dy, n, ptlist, op);
	}

	for (p = functab; p->rop != 0; p++)
		if (p->rop == ropfunc)
			return (*p->polypoint)(pr, dx, dy, n, ptlist, op);

	return PIX_ERR;
}

#ifndef lint
static char sccsid[] = "@(#)pr_curve.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include "chain.h"

pr_curve(pr, pos, curve, n, op, color)
Pixrect *pr;
struct pr_pos pos;
struct pr_curve curve[];
int n, op, color;
{
	extern int mem_rop(), mem_curve();
	extern int bw1_rop(), bw1_curve();
	extern int cg1_rop(), cg1_curve();
	extern int cg2_rop(), cg2_curve();
	extern int gp1_rop(), gp1_curve();

	static struct funcs {
		int (*rop)(), (*curve)();
	} functab[] = {
		{ mem_rop, mem_curve },
		{ cg2_rop, cg2_curve },
		{ gp1_rop, gp1_curve },
		{ cg1_rop, cg1_curve },
		{ bw1_rop, bw1_curve },
		{ 0, 0 }
	};

	register int (*ropfunc)();
	register struct funcs *p;

	ropfunc = pr->pr_ops->pro_rop;

	for (p = functab; p->rop != 0; p++)
		if (p->rop == ropfunc)
			return (*p->curve)(pr, pos, curve, n, op, color);

	return PIX_ERR;
}

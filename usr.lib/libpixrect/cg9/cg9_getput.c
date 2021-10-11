#ifndef lint
static char     sccsid[] =      "@(#)cg9_getput.c	1.1 94/10/31 SMI";
#endif

/* Copyright 1989 Sun Microsystems, Inc. */

#include <pixrect/cg9var.h>
#include <pixrect/mem32_var.h>
#include <pixrect/pr_planegroups.h>	/* PIXPG* */

cg9_get(pr, x, y)
Pixrect            *pr;
int                 x,
                    y;
{
    if (cg9_d(pr)->windowfd>=-1 && strcmp(cg9_d(pr)->cms.cms_name,"monochrome"))
    {
	Pixrect             mem32_pr;
	struct mprp32_data  mem32_pr_data;

	CG9_PR_TO_MEM32(pr, mem32_pr, mem32_pr_data);
	return mem32_get_index(pr, mem_get(pr, x, y));
    }

    return mem_get(pr, x, y);
}

cg9_put(pr, x, y, value)
Pixrect            *pr;
int                 x,
                    y,
                    value;
{
    if (cg9_d(pr)->windowfd>=-1 && strcmp(cg9_d(pr)->cms.cms_name,"monochrome"))
    {
	Pixrect             mem32_pr;
	struct mprp32_data  mem32_pr_data;

	CG9_PR_TO_MEM32(pr, mem32_pr, mem32_pr_data);
	return mem_put(pr, x, y, mem32_get_true(pr, value,
		mprp32_d(pr)->mprp.planes & PIX_ALL_PLANES));
    }

    return mem_put(pr, x, y, value);
}

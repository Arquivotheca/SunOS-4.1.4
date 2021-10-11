#ifndef	lint
static char         sccsid[] = "@(#)cg9_vector.c	1.1 94/10/31 SMI";
#endif

/* Copyright 1989 Sun Microsystems, Inc. */

#include <pixrect/pr_planegroups.h>	/* PIX_ALL_PLANES       */
#include <pixrect/cg9var.h>
#include <pixrect/mem32_var.h>

cg9_vector(pr, x0, y0, x1, y1, op, color)
Pixrect            *pr;
int                 x0,
                    y0,
                    x1,
                    y1,
                    op,
                    color;
{
    int                 cc,
                        original_planes;
    Pixrect             mem32_pr;
    struct mprp32_data  mem32_pr_data;
    struct cg9_control_sp *cg9_ctrl;

    cg9_ctrl = cg9_d(pr)->ctrl_sp;
    original_planes = cg9_ctrl->plane_mask;
    cg9_ctrl->plane_mask = PIX_ALL_PLANES;
    CG9_PR_TO_MEM32(pr, mem32_pr, mem32_pr_data);
    cc = mem32_vector(pr, x0, y0, x1, y1, op, color);
    cg9_ctrl->plane_mask = original_planes;
    return cc;
}

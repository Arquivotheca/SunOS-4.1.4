#ifndef lint
static char sccsid[] = "@(#)gp1_rop.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1988 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>
#include <pixrect/gp1var.h>
#include <pixrect/gp1cmds.h>
#include <pixrect/cg9var.h>
#include <pixrect/cg12_var.h>

#ifdef SHLIB
#define GP1_ROP _gp1_rop
#else
#define GP1_ROP gp1_rop
#endif

#define SWITCH_OPS(fb, ops)                                     \
    if (iscg2 = ((fb) == FBTYPE_SUN2COLOR)) {                   \
        (ops) = gp1_ops;                                        \
    }                                                           \
    else if ((fb) == FBTYPE_SUNROP_COLOR)                       \
	(ops) = cg9_ops;                                        \
    else                                                        \
	return PIX_ERR

#define PR_TO_CG(pr,cg,op_vec)                                  \
	if ((pr)->pr_ops->pro_rop == gp1_rop) {                 \
	    (cg) = *(pr);                                       \
            (cg).pr_ops = (op_vec);                             \
	    (cg).pr_data = (caddr_t) & gp1_d((pr))->cgpr;       \
	    (pr) = &(cg);                                       \
	}

GP1_ROP(dpr, dx, dy, dw, dh, op, spr, sx, sy)
    Pixrect        *dpr;
    int             dx,
                    dy,
                    dw,
                    dh;
    int             op;
    Pixrect        *spr;
    int             sx,
                    sy;
{
    struct pixrectops ops;
    Pixrect         dcgpr;
    Pixrect         scgpr;
    struct fbgattr  fbattr;
    int             iscg2;

    /* cg12 is not a standalone fb plus gp. */

    if (dpr->pr_ops == &cg12_ops || (spr && spr->pr_ops == &cg12_ops))
	return cg12_rop(dpr, dx, dy, dw, dh, op, spr, sx, sy);

    /* cannot use gp1_d(pr)->fbtype yet. */
    if (!pr_ioctl(dpr, FBIOGATTR, &fbattr) &&
	fbattr.fbtype.fb_type == FBTYPE_SUN2GP) {
	if (gp1_d(dpr)->fbtype == FBTYPE_SUNROP_COLOR)
	    gp1_sync(gp1_d(dpr)->gp_shmem, gp1_d(dpr)->ioctl_fd);
	SWITCH_OPS(gp1_d(dpr)->fbtype, ops);
	PR_TO_CG(dpr, dcgpr, &ops);
    }

    if (spr && !pr_ioctl(spr, FBIOGATTR, &fbattr) &&
	fbattr.fbtype.fb_type == FBTYPE_SUN2GP) {
	if (gp1_d(spr)->fbtype == FBTYPE_SUNROP_COLOR)
	    gp1_sync(gp1_d(spr)->gp_shmem, gp1_d(spr)->ioctl_fd);
	SWITCH_OPS(gp1_d(spr)->fbtype, ops);
	PR_TO_CG(spr, scgpr, &ops);
    }

    if (iscg2)
	return gp1_cg2_rop(dpr, dx, dy, dw, dh, op, spr, sx, sy);
    else
	return pr_rop(dpr, dx, dy, dw, dh, op, spr, sx, sy);
}

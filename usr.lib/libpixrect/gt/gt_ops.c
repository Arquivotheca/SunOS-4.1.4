#ifndef lint
static char     sccsid[] =      "@(#)gt_ops.c	1.1 94/10/31 SMI";
#endif
 
/*
 *	Copyright 1990-1991 by Sun Microsystems, Inc.
 */

#include  <sys/types.h>
#include  <pixrect/pixrect.h>
#include  <gt/gtvar.h>

struct pixrectops gtp_ops = {
	gt_rop,
	gt_stencil,
	gt_batchrop,
	gt_ioctl,
	gt_destroy,
	gt_get,
	gt_put,
	gt_vector,
	gt_region,
	gt_putcolormap,
	gt_getcolormap,
	gt_putattributes,
	gt_getattributes
};

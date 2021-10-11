/* @(#)cg12_ops.c 1.1 94/10/31 SMI */

/* Copyright 1990 by Sun Microsystems,  Inc. */

#include <pixrect/cg12_var.h>

struct pixrectops cg12_ops

#if !defined(SHLIB_SA)
= {
	gp1_rop,
	cg12_stencil,
	cg12_batchrop,
	cg12_ioctl,
	cg12_destroy,
	cg12_get,
	cg12_put,
	cg12_vector,
	cg12_region,
	cg12_putcolormap,
	cg12_getcolormap,
	cg12_putattributes,
	cg12_getattributes
}
#endif
;

/* @(#)cg1_ops.c 1.1 94/10/31 SMI */

/*
 * Copyright 1987 by Sun Microsystems,  Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/cg1var.h>

struct pixrectops cg1_ops = {
	cg1_rop,
	cg1_stencil,
	cg1_batchrop,
	0,
	cg1_destroy,
	cg1_get,
	cg1_put,
	cg1_vector,
	cg1_region,
	cg1_putcolormap,
	cg1_getcolormap,
	cg1_putattributes,
	cg1_getattributes
};

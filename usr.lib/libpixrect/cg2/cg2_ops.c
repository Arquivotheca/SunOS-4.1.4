/* @(#)cg2_ops.c 1.1 94/10/31 SMI */

/*
 * Copyright 1987 by Sun Microsystems,  Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/cg2var.h>

struct pixrectops cg2_ops = {
	cg2_rop,
	cg2_stencil,
	cg2_batchrop,
	0,
	cg2_destroy,
	cg2_get,
	cg2_put,
	cg2_vector,
	cg2_region,
	cg2_putcolormap,
	cg2_getcolormap,
	cg2_putattributes,
	cg2_getattributes
};

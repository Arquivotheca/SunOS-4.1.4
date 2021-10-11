/* @(#)cg3_ops.c 1.1 94/10/31 SMI */

/*
 * Copyright 1989 by Sun Microsystems,  Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/cg3var.h>

/* pixrect ops vector */
struct pixrectops cg3_ops = {
	mem_rop,
	mem_stencil,
	mem_batchrop,
	0,
	cg3_destroy,
	mem_get,
	mem_put,
	mem_vector,
	cg3_region,
	cg3_putcolormap,
	cg3_getcolormap,
	mem_putattributes,
	mem_getattributes
};

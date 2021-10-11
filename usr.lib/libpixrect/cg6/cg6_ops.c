/* @(#)cg6_ops.c 1.1 94/10/31 SMI */

/*
 * Copyright 1988-1989, Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/cg6var.h>

struct pixrectops cg6_ops = {
	cg6_rop,
	cg6_stencil,
	cg6_batchrop,
	0,
	cg6_destroy,
	cg6_get,
	cg6_put,
	cg6_vector,
	cg6_region,
	cg6_putcolormap,
	cg6_getcolormap,
	mem_putattributes,
	mem_getattributes
};

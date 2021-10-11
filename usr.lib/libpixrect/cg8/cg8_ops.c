/* @(#)cg8_ops.c of 94/10/31, SMI */

/*
 * Copyright 1988 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <pixrect/cg8var.h>

struct pixrectops cg8_ops = {
	cg8_rop,
	mem_stencil,
	mem_batchrop,
	cg8_ioctl,
	cg8_destroy,
	cg8_get,
	cg8_put,
	cg8_vector,
	cg8_region,
	cg8_putcolormap,
	cg8_getcolormap,
	cg8_putattributes,
	cg8_getattributes
};

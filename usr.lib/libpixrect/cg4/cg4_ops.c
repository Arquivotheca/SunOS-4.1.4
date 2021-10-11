/* @(#)cg4_ops.c 1.1 94/10/31 SMI */

/*
 * Copyright 1987 by Sun Microsystems,  Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/cg4var.h>
#include <pixrect/memvar.h>

/* pixrect ops vector */
struct pixrectops cg4_ops = {
	mem_rop,
	mem_stencil,
	mem_batchrop,
	cg4_ioctl,
	cg4_destroy,
	mem_get,
	mem_put,
	mem_vector,
	cg4_region,
	cg4_putcolormap,
	cg4_getcolormap,
	cg4_putattributes,
	cg4_getattributes
};

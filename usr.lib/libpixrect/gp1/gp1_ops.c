/* @(#)gp1_ops.c 1.1 94/10/31 SMI */

/*
 * Copyright 1987 by Sun Microsystems,  Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/cg2var.h>
#include <pixrect/gp1var.h>

struct pixrectops gp1_ops = {
	gp1_rop,
	gp1_stencil,
	gp1_batchrop,
	gp1_ioctl,
	gp1_destroy,
	gp1_get,
	gp1_put,
	gp1_vector,
	gp1_region,
	gp1_putcolormap,
	gp1_getcolormap,
	gp1_putattributes,
	gp1_getattributes
};

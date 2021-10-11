/* @(#)bw1_ops.c 1.1 94/10/31 SMI */

/*
 * Copyright 1987 by Sun Microsystems,  Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/bw1var.h>
#include <pixrect/memvar.h>

struct pixrectops bw1_ops = {
	bw1_rop, 
	mem_stencil,
	bw1_batchrop,
	0, 
	bw1_destroy, 
	bw1_get, 
	bw1_put, 
	bw1_vector,
	bw1_region, 
	bw1_putcolormap, 
	bw1_getcolormap,
	bw1_putattributes, 
	bw1_getattributes
};

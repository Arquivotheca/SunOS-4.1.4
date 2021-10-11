/* @(#)mem32_ops.c 1.1 94/10/31 SMI */

/* Copyright 1990 by Sun Microsystems,  Inc. */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/mem32_var.h>

struct pixrectops mem32_ops

#if !defined(SHLIB_SA)
= {
	mem32_rop,
	mem_stencil,
	mem_batchrop,
	0,
	mem_destroy,
	mem_get,
	mem_put,
	mem_vector,
	mem_region,
	mem_putcolormap,
	mem32_getcolormap,
	mem_putattributes,
	mem_getattributes
}
#endif
;

#ifndef lint
static char sccsid[] = "@(#)cg9_ops.c	1.1 94/10/31 SMI";
#endif

/*
 *	Copyright 1988 by Sun Microsystems, Inc.
 */

#include  <sys/types.h>
#include  <pixrect/pixrect.h>
#include  <pixrect/cg9var.h>

struct pixrectops cg9_ops = {
	cg9_rop,
	mem_stencil,
	mem_batchrop,
	cg9_ioctl,
	cg9_destroy,
	cg9_get,
	cg9_put,
	cg9_vector,
	cg9_region,
	cg9_putcolormap,
	cg9_getcolormap,
	cg9_putattributes,
	cg9_getattributes
};



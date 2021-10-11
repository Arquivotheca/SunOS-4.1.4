#ifndef lint
static	char sccsid[] = "@(#)cg2_standalone.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Sun2 Color pixrect create for standalone applications.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>

struct	cg2pr cg2prstruct =
    { 0, 0, 255, 256, 0, 0, 0 };
struct	pixrect cg2pixrectstruct =
    { &cg2_ops, { CG2_WIDTH, CG2_HEIGHT }, CG2_DEPTH, (caddr_t)&cg2prstruct };

struct	pixrectops cg2_ops = {
	cg2_rop,
	cg2_putcolormap,
};

struct	pixrect *
cg2_create(size, depth)
	struct pr_size size;
	int depth;
{
	return (&cg2pixrectstruct);
}

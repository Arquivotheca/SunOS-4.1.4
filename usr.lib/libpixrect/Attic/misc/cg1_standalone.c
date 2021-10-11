#ifndef lint
static	char sccsid[] = "@(#)cg1_standalone.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Sun1 Color pixrect create for standalone applications.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/cg1reg.h>
#include <pixrect/cg1var.h>

struct	cg1pr cg1prstruct =
    { 0, 0, 255, 256, 0, 0, 0 };
struct	pixrect cg1pixrectstruct =
    { &cg1_ops, { CG1_WIDTH, CG1_HEIGHT }, CG1_DEPTH, (caddr_t)&cg1prstruct };

struct	pixrectops cg1_ops = {
	cg1_rop,
	cg1_putcolormap,
};

struct	pixrect *
cg1_create(size, depth)
	struct pr_size size;
	int depth;
{
	return (&cg1pixrectstruct);
}

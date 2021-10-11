#ifndef lint
static char sccsid[] = "@(#)mem_kern.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1983, 1987 by Sun Microsystems, Inc.
 */

/*
 * Memory pixrect (non)creation in kernel
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>

extern	int	mem_rop();
extern	int	mem_putcolormap();
extern	int	mem_putattributes();

struct	pixrectops mem_ops = {
	mem_rop,
	mem_putcolormap,
	mem_putattributes,
#	ifdef _PR_IOCTL_KERNEL_DEFINED
	0
#	endif
};

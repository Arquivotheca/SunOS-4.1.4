#ifndef lint
static char sccsid[] = "@(#)cg4_ioctl.c	1.1 94/10/31 SMI";
#endif

/*
 *	Copyright 1988 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/ioccom.h>
#include <sun/fbio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/cg4var.h>

#ifdef KERNEL
#define ERROR ENOTTY
#else
#define ERROR PIX_ERR
#endif

int
cg4_ioctl (pr, cmd, data)
    Pixrect        *pr;
    int             cmd;
    caddr_t         data;
{
    static int      cmsize;

    switch (cmd) {
    case FBIOGPLNGRP:
	*(int *) data = PIX_ATTRGROUP(cg4_d(pr)->planes);
	break;
    case FBIOAVAILPLNGRP:
	{
	    static int      cg4groups =
	    MAKEPLNGRP(PIXPG_OVERLAY) |
	    MAKEPLNGRP(PIXPG_OVERLAY_ENABLE) |
	    MAKEPLNGRP(PIXPG_8BIT_COLOR);

	    *(int *) data = cg4groups;
	    break;
	}
    case FBIOGCMSIZE:
	if (cmsize < 0)
	    return -1;
	*(int *) data = cmsize;
	break;
    case FBIOSCMSIZE:
	cmsize = *(int *) data;
	break;
    default:
	return ERROR;
    }
    return 0;
}

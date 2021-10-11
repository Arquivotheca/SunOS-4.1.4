#ifndef lint
static char sccsid[] = "@(#)gp1_ioctl.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1989 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/cg2var.h>
#include <pixrect/gp1var.h>
#include <pixrect/gp1reg.h>
#include <pixrect/gp1cmds.h>
#include <pixrect/pr_planegroups.h>

static struct fbgattr gp1_attr = {
    FBTYPE_SUN2GP, 0,
    { FBTYPE_SUN2GP, 900, 1152, 8, 256, 0, },
    { 0, -1, { 0, 0, 0, 0, 0, 0, 0, 0, }, },
    { -1, -1, -1, -1},
};

gp1_ioctl(pr, cmd, data, flag)
    Pixrect        *pr;
    int             cmd;
    char           *data;
    int             flag;
{
    int fbtype = gp1_d(pr)->fbtype;
    Pixrect cgpr;

    switch (cmd) {
    case FBIOGATTR:
	*(struct fbgattr *) data = gp1_attr;
	break;
    case FBIOGTYPE:
	*(struct fbtype *) data = gp1_attr.fbtype;
	break;
    case GP1IO_SATTR:
	if (data)
		gp1_attr = *(struct fbgattr *) data;
	break;
    case FBIOGPLNGRP:
	if (fbtype == FBTYPE_SUNROP_COLOR) {
	    cgpr = *pr;
	    cg9_d(&cgpr) = &gp1_d(pr)->cgpr.cg9pr;
	    return cg9_ioctl(&cgpr, cmd, data, flag);
        }
	*(int *)data = PIXPG_8BIT_COLOR;
	break;
    
    case GP1IO_SCMAP:
	if (fbtype == FBTYPE_SUNROP_COLOR)
	{
	    int             i;
	    unsigned int   *gp_blk;
	    unsigned char   r[CG9_CMAP_SIZE],
	                    g[CG9_CMAP_SIZE],
	                    b[CG9_CMAP_SIZE];
	    struct	colormapseg	cms;
	    struct	cms_map		cmap;
	    union fbunit buffer;

	    if (fbtype != FBTYPE_SUNROP_COLOR)
		break;

	    cgpr = *pr;
	    cg9_d(&cgpr) = &gp1_d(pr)->cgpr.cg9pr;

	    if (cg9_d(&cgpr)->windowfd == -2)
		break;

	    GP1_PRD_SYNC(gp1_d(pr), return PIX_ERR);

	    cms = cg9_d(&cgpr)->cms;
	    cms.cms_addr = 0;
	    cmap.cm_red = r;
	    cmap.cm_green = g;
	    cmap.cm_blue = b;
	    mem32_win_getcms(cg9_d(&cgpr)->windowfd, &cms, &cmap);

	    gp_blk = (unsigned int *) ((char *) gp1_d (pr)->gp_shmem +
		     (GP2_SHMEM_SIZE - 1024 * (gp1_d (pr)->cg2_index + 1)));

	    for (i = 0; i < cg9_d (&cgpr)->cms.cms_size; i++) {
	        buffer.channel.R = r[i];
		buffer.channel.G = g[i];
		buffer.channel.B = b[i];
		GP1_PUT_I(gp_blk, buffer.packed);
	      }
	}
	break;

    case FBIOAVAILPLNGRP:
	if (gp1_d(pr)->fbtype != FBTYPE_SUNROP_COLOR) {
	    /* see fbio.h */
	    *(int *) data = MAKEPLNGRP(PIXPG_8BIT_COLOR);
	    break;
	}
	cgpr = *pr;
	cg9_d(&cgpr) = &gp1_d(pr)->cgpr.cg9pr;
	return cg9_ioctl(&cgpr, cmd, data, flag);
    case FBIODBLSINFO:
	gp1_sync(gp1_d(pr)->gp_shmem, gp1_d(pr)->ioctl_fd);
	/* fall thru */
    default:
	cgpr = *pr;
	cg9_d(&cgpr) = &gp1_d(pr)->cgpr.cg9pr;
	return cg9_ioctl(&cgpr, cmd, data, flag);
    }
    return 0;
}

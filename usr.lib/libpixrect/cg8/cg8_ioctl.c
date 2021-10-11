#ifndef lint
static char     sccsid[] = "@(#)cg8_ioctl.c	1.1 94/10/31 SMI";

#endif

/*
 * Copyright 1989 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/ioccom.h>
#include <sun/fbio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/cg8var.h>
#include <sunwindow/cms.h>	       /* struct colormapseg */

#ifdef KERNEL
#define ERROR ENOTTY
#define ioctl cgeightioctl
#else
#define ERROR PIX_ERR
#endif

static int      cmsize = -1;

int
cg8_ioctl(pr, cmd, data)
    Pixrect        *pr;
    int             cmd;
    caddr_t         data;
{
    static int          save_window_fd;

    switch (cmd)
    {
	case FBIOGPLNGRP:
	    *(int *) data = PIX_ATTRGROUP(cg8_d(pr)->planes);
	    break;

	case FBIOAVAILPLNGRP:
	    {
#ifndef PRE_PIP
		struct cg8_data *cg8d;
		int    fbi;
		int    group_mask = 0;

		cg8d = cg8_d(pr);
		for (fbi = 0; fbi < cg8d->num_fbs; fbi++)
		{
		    group_mask |= MAKEPLNGRP(cg8d->fb[fbi].group);
		}
		*(int *) data = group_mask;
#else PRE_PIP
		{
		    static int      cg4groups =
		    MAKEPLNGRP(PIXPG_OVERLAY) |
		    MAKEPLNGRP(PIXPG_OVERLAY_ENABLE) |
		    MAKEPLNGRP(PIXPG_24BIT_COLOR);

		    *(int *) data = cg4groups;
		}
#endif !PRE_PIP
		break;
	    }

    case FBIO_FULLSCREEN_ELIMINATION_GROUPS:
        {
			struct cg8_data *cg8d;
			char            *elim_groups = (char *)data;

		    cg8d = cg8_d(pr);
			bzero(elim_groups, PIXPG_LAST_PLUS_ONE);
			if ( cg8d->flags & CG8_EIGHT_BIT_PRESENT )
			{
			    elim_groups[PIXPG_24BIT_COLOR] = 1;
			}
			else
			{
				elim_groups[PIXPG_8BIT_COLOR] = 1;
			}
		}
		break;

	case FBIOGCMSIZE:
	    if (cmsize < 0)
		return -1;
	    *(int *) data = cmsize;
	    break;

	case FBIOSCMSIZE:
	    cmsize = *(int *) data;
	    break;

	case FBIOSCMS:
	    cg8_d(pr)->cms = *(struct colormapseg *) data;
	    break;

	case FBIOSWINFD:
            if (cg8_d(pr)->windowfd == -2 && cg8_d(pr)->real_windowfd != -2)
                break;
            cg8_d(pr)->windowfd = *(int *) data;
            /* fall into RWINFD if winfd changed by pw_putcolormap */

        case FBIOSRWINFD:
            cg8_d(pr)->real_windowfd = *(int *) data;
            break;

	case FBIOSAVWINFD:
	    if (cg8_d(pr)->windowfd >= -1)
	    {
		save_window_fd = cg8_d(pr)->windowfd; 
		cg8_d(pr)->windowfd = -2; 
	    }
	    break; 

	case FBIORESWINFD:
	    cg8_d(pr)->windowfd = save_window_fd; 
	    break; 
						 
	default:
	    return ERROR;
    }
    return 0;
}

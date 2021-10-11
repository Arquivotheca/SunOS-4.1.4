#ifndef lint
static char sccsid[] = "@(#)cg9_ioctl.c	1.1 94/10/31 SMI";
#endif

/* Copyright 1988 by Sun Microsystems, Inc. */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/ioccom.h>
#include <sun/fbio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/cg9var.h>
#include <sunwindow/cms.h>		/* struct colormapseg */

#ifdef KERNEL
#include "cgnine.h"
#define ERROR ENOTTY

#if  NCGNINE > 0
#define ioctl cgnineioctl
#else NCGNINE > 0
#define ioctl
#endif NCGNINE > 0

#else KERNEL
#define ERROR PIX_ERR
#endif KERNEL

static int cg9_current_display = FBDBL_BOTH;
static int cmsize = -1;
#define DSP_MASK(m) ((m) & FBDBL_BOTH)

int
cg9_ioctl (pr, cmd, data)
    Pixrect        *pr;
    int             cmd;
    caddr_t         data;
{
    struct	cg9_data	*prd = cg9_d(pr);
    static	int		save_window_fd;

    switch (cmd)
    {
	case FBIOGPLNGRP:
	    *(int *) data = PIX_ATTRGROUP(cg9_d(pr)->planes);
	    break;

	case FBIOAVAILPLNGRP:
	{
	    static int      cg9groups =
	    MAKEPLNGRP (PIXPG_OVERLAY) |
	    MAKEPLNGRP (PIXPG_OVERLAY_ENABLE) |
	    MAKEPLNGRP (PIXPG_24BIT_COLOR);

	    *(int *) data = cg9groups;
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

        case FBIOSCMS:
            cg9_d(pr)->cms = *(struct colormapseg *) data;
            break;
 
        case FBIOSWINFD:
	    if (cg9_d(pr)->windowfd == -2 && cg9_d(pr)->real_windowfd != -2)
		break;
	    cg9_d(pr)->windowfd = *(int *) data;
	    /* fall into RWINFD if winfd changed by pw_putcolormap */

        case FBIOSRWINFD:
            cg9_d(pr)->real_windowfd = *(int *) data;
            break;

	case FBIOSAVWINFD:
	    if (cg9_d(pr)->windowfd >= -1)
	    {
		save_window_fd = cg9_d(pr)->windowfd;
		cg9_d(pr)->windowfd = -2;
	    }
	    break;

	case FBIORESWINFD:
	    cg9_d(pr)->windowfd = save_window_fd;
	    break;
 
	case FBIODBLGINFO:
	{
	    struct fbdblinfo *dblinfo = (struct fbdblinfo *) data;
	    static int      result[4] = {FBDBL_A | FBDBL_B, FBDBL_B, FBDBL_A, 0};
	    dblinfo->dbl_devstate |= FBDBL_AVAIL;
	    if (!(prd->ctrl_sp->dbl_reg & CG9_ENB_12))
		cg9_current_display = dblinfo->dbl_display =
		    dblinfo->dbl_read = dblinfo->dbl_write = FBDBL_BOTH;
	    else {
		dblinfo->dbl_display = cg9_current_display;
		dblinfo->dbl_read = prd->ctrl_sp->dbl_reg & CG9_READ_B
		    ? FBDBL_B : FBDBL_A;
		dblinfo->dbl_write = result[(prd->ctrl_sp->dbl_reg &
				  (CG9_NO_WRITE_A | CG9_NO_WRITE_B)) >> 28];
		dblinfo->dbl_depth = 16;
	    }
	    break;
	}

	case FBIODBLSINFO:
	{
	    struct fbdblinfo *dblinfo = (struct fbdblinfo *) data;

	    /*
	     * the registers are write only, the pre-loading of the software
	     * copy does not make sense.  This is in hope of hw modifications
	     * to make them readable.
	     */
	    int             hw_dsp = prd->ctrl_sp->dbl_disp,
	                    hw_reg = prd->ctrl_sp->dbl_reg;

#ifdef KERNEL
	    if (servicing_interrupt())
		return 0;
	    (void) gp1_sync_from_fd(prd->fd);
#endif
	    /*
	     * Quick checks on erroneous requests
	     */
	    if ((dblinfo->dbl_display & FBDBL_NONE) ||
		(dblinfo->dbl_read & FBDBL_NONE) ||
		DSP_MASK(dblinfo->dbl_display) == FBDBL_BOTH ||
		DSP_MASK(dblinfo->dbl_read) == FBDBL_BOTH)
		return PIX_ERR;

	    if (dblinfo->dbl_display) {
		if (dblinfo->dbl_display & FBDBL_A)
		    hw_dsp &= ~CG9_DISPLAY_B;
		else if (dblinfo->dbl_display & FBDBL_B)
		    hw_dsp |= CG9_DISPLAY_B;
	    }

	    if (dblinfo->dbl_read) {
		if (dblinfo->dbl_read & FBDBL_A)
		    hw_reg &= ~CG9_READ_B;
		else if (dblinfo->dbl_read & FBDBL_B)
		    hw_reg |= CG9_READ_B;
		prd->ctrl_sp->dbl_reg = hw_reg;
	    }


	    if (dblinfo->dbl_write) {
		if (dblinfo->dbl_write & FBDBL_NONE) {
		    hw_reg &= ~CG9_ENB_12;
		    hw_reg |= CG9_NO_WRITE_A | CG9_NO_WRITE_B;
		}
		else if (DSP_MASK(dblinfo->dbl_write) == FBDBL_BOTH) {
		    hw_reg &= ~CG9_ENB_12;
		    hw_reg &= ~(CG9_NO_WRITE_A | CG9_NO_WRITE_B);
		}
		else if (dblinfo->dbl_write & FBDBL_A) {
		    hw_reg |= CG9_ENB_12 | CG9_NO_WRITE_B;
		    hw_reg &= ~CG9_NO_WRITE_A;
		}
		else if (dblinfo->dbl_write & FBDBL_B) {
		    hw_reg |= CG9_ENB_12 | CG9_NO_WRITE_A;
		    hw_reg &= ~CG9_NO_WRITE_B;
		}
		prd->ctrl_sp->dbl_reg = hw_reg;
	    }

	    if (DSP_MASK(dblinfo->dbl_display) &&
		(cg9_current_display != DSP_MASK(dblinfo->dbl_display))) {

		cg9_current_display = DSP_MASK(dblinfo->dbl_display);
		if (!(dblinfo->dbl_devstate & FBDBL_DONT_BLOCK)) {
		    int             dblwait;

		    dblwait = (hw_dsp & CG9_DISPLAY_B) ? 2 : 1;
#ifdef KERNEL
		    if (ioctl(prd->fd, FBIOVERTICAL, (caddr_t) &dblwait, 0) < 0)
			return PIX_ERR;
#else
		    if (ioctl(prd->fd, FBIOVERTICAL, &dblwait) < 0)
			return PIX_ERR;
#endif
		}
		else
		    prd->ctrl_sp->dbl_disp = hw_dsp;
	    }
	    break;
	}

	default:
	    return ERROR;
    }
    return 0;
}

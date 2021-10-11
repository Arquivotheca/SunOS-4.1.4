#ifndef lint
static char         sccsid[] = "@(#)cg12_ioctl.c	1.1 94/10/31 SMI";
#endif

/* Copyright 1990 by Sun Microsystems, Inc. */

#include <sys/types.h>			/* for fbio.h	*/
#include <sys/errno.h>			/* ERR* 	*/
#include <sys/ioccom.h>			/* ioctl defs	*/
#include <sun/fbio.h>			/* FBIO*	*/
#include <sun/gpio.h>			/* GP1IO_INFO*	*/
#include <pixrect/cg12_var.h>		/* cg12*	*/
#include <pixrect/pr_planegroups.h>	/* PIXPG*	*/
#include <pixrect/pr_dblbuf.h>		/* PR_DBL*	*/
#include <pixrect/gp1var.h>		/* GP1IO_SCMAP	*/
#include <pixrect/gp1cmds.h>		/* GP1_*	*/
#include <pixrect/gp1reg.h>		/* GP1_STATIC*	*/

/* all these includes just to get WINWIDGET defined */

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>	/* WINWIDGET	*/

static int          cmsize = -1;
#ifndef	KERNEL
static int          shared_8 = -1;
static int          shared_24 = -1;
static int          a_or_b = -1;
#endif

#define DSP_MASK(m) ((m) & FBDBL_BOTH)

int
cg12_ioctl(pr, cmd, data)
Pixrect            *pr;
int                 cmd;
caddr_t             data;
{
    struct cg12_data   *prd = cg12_d(pr);

    switch (cmd)
    {
	/*
	 * these first few cases are both in user and kernel space for
	 * plane groups and attributes.
	 */

	case FBIOGPLNGRP:
	    *(int *) data = PIX_ATTRGROUP(prd->planes);
	    break;

	case FBIOAVAILPLNGRP:
	    {
		static int          cg12groups =
		    MAKEPLNGRP(PIXPG_OVERLAY) |
		    MAKEPLNGRP(PIXPG_OVERLAY_ENABLE) |
		    MAKEPLNGRP(PIXPG_24BIT_COLOR) |
		    MAKEPLNGRP(PIXPG_8BIT_COLOR) |
		    MAKEPLNGRP(PIXPG_WID);
		    /* | MAKEPLNGRP(PIXPG_ZBUF); */

		*(int *) data = cg12groups;
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

	/*
	 * double buffering will have different kernel and user versions
	 * of DLBGINFO and DBLSINFO.  the kernel will never really be
	 * setting any double buffer attributes so the "get" can fake
	 * static values and the "set" can simply return.
	 */

#ifdef	KERNEL
	case FBIODBLGINFO:
	    {
		struct fbdblinfo   *dblinfo = (struct fbdblinfo *) data;

		dblinfo->dbl_devstate |= FBDBL_AVAIL;
		if (pr->pr_depth == 32)
		    dblinfo->dbl_devstate |= FBDBL_AVAIL_PG;
		dblinfo->dbl_display = FBDBL_A;
		dblinfo->dbl_read = FBDBL_A;
		dblinfo->dbl_write = FBDBL_A | FBDBL_B;
		dblinfo->dbl_depth = 16;
		dblinfo->dbl_wid = 1;
	    }

	case FBIODBLSINFO:
	    /* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	    /* leave WSTATUS bit set always to inform PROM to save the
	       context before displaying any character */
	    prd->ctl_sp->eic.host_control = CG12_EIC_WSTATUS;
	    prd->ctl_sp->apu.res2 = 0;
	    /* !!! */
	    break;
#endif	/* KERNEL */

	/* the remainder of the cases are not necessary in the kernel */

#ifndef	KERNEL
	case FBIOSWINFD:
	case FBIOSRWINFD:
	    prd->windowfd = *(int *) data;
	    break;

	case FBIODBLGINFO:
	    {
		unsigned char       i;
		struct fbdblinfo   *dblinfo = (struct fbdblinfo *) data;
		struct fb_wid_item  wid_item;
		struct fb_wid_list  wid_list;

		dblinfo->dbl_devstate |= FBDBL_AVAIL;

		if (prd->windowfd >= 0)
		    ioctl(prd->windowfd, WINWIDGET, &prd->wid_dbl_info);

		wid_item.wi_type = 0;
		wid_item.wi_index = prd->wid_dbl_info.dbl_wid.wa_index;
		wid_list.wl_count = 1;
		wid_list.wl_list = &wid_item;
		i = (ioctl(prd->fd, FBIO_WID_GET, (char *) &wid_list, 0) < 0)
		    ? 0 : wid_list.wl_list->wi_values[0] & 0x3;

		dblinfo->dbl_display = (i == 3) ? FBDBL_B : FBDBL_A;

		i = (prd->wid_dbl_info.dbl_read_state == PR_DBL_A)
			? prd->wid_dbl_info.dbl_fore
			: prd->wid_dbl_info.dbl_back;
		dblinfo->dbl_read = (i == PR_DBL_A) ? FBDBL_A : FBDBL_B;

		switch (prd->wid_dbl_info.dbl_write_state)
		{
		    case PR_DBL_A:
			dblinfo->dbl_write =
			    (prd->wid_dbl_info.dbl_fore==PR_DBL_A)
				? FBDBL_A : FBDBL_B;
			dblinfo->dbl_devstate |= FBDBL_AVAIL_PG;
			break;

		    case PR_DBL_B:
			dblinfo->dbl_write =
			    (prd->wid_dbl_info.dbl_back==PR_DBL_A)
				? FBDBL_A : FBDBL_B;
			dblinfo->dbl_devstate |= FBDBL_AVAIL_PG;
			break;

		    case PR_DBL_NONE:
			dblinfo->dbl_write = 0;
			break;

		    case PR_DBL_BOTH:
		    default:
			dblinfo->dbl_write = FBDBL_A | FBDBL_B;
		}

		dblinfo->dbl_depth = 16;
		dblinfo->dbl_wid = 1;
	    }
	    break;

	case FBIODBLSINFO:
	    {
		struct fbdblinfo   *dblinfo = (struct fbdblinfo *) data;
		unsigned int        planes;
		unsigned char       display;

		/* Quick check for erroneous requests */

		if ((dblinfo->dbl_display & FBDBL_NONE) ||
		    (dblinfo->dbl_read & FBDBL_NONE) ||
		    DSP_MASK(dblinfo->dbl_display) == FBDBL_BOTH ||
		    DSP_MASK(dblinfo->dbl_read) == FBDBL_BOTH)
		    return PIX_ERR;

		if (prd->windowfd >= 0)
		    ioctl(prd->windowfd, WINWIDGET, &prd->wid_dbl_info);

		if (PIX_ATTRGROUP(prd->planes) != PIXPG_24BIT_COLOR ||
		    (prd->wid_dbl_info.dbl_wid.wa_index == shared_8) ||
		    (prd->wid_dbl_info.dbl_wid.wa_index == shared_24))
		{
		    /* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
		    /* leave WSTATUS bit set always to inform PROM to save the
		       context before displaying any character */
		    prd->ctl_sp->eic.host_control = CG12_EIC_WSTATUS;
		    prd->ctl_sp->apu.res2 = 0;
		    /* !!! */
		    return 0;
		}

		/* 0xff indicates no display update so no need to block later */
		display = 0xff;

		if (dblinfo->dbl_display)
		{
		    if (dblinfo->dbl_display & FBDBL_A)
			display = 2;
		    else if (dblinfo->dbl_display & FBDBL_B)
			display = 3;
		}

		if (dblinfo->dbl_read)
		{
		    if (dblinfo->dbl_read & FBDBL_A)
		    {
			/* !!! see cgtwelve.c EIC-COMMENTS before changing !!!*/
			prd->ctl_sp->eic.host_control &= ~CG12_EIC_READB;
			prd->ctl_sp->apu.res2 = 0;
			/* !!! */
			prd->wid_dbl_info.dbl_read_state = PR_DBL_A;
		    }
		    else if (dblinfo->dbl_read & FBDBL_B)
		    {
			/* !!! see cgtwelve.c EIC-COMMENTS before changing !!!*/
			prd->ctl_sp->eic.host_control |= CG12_EIC_READB;
			prd->ctl_sp->apu.res2 = 0;
			/* !!! */
			prd->wid_dbl_info.dbl_read_state = PR_DBL_B;
		    }
		}

		if (dblinfo->dbl_write)
		{
		    if (dblinfo->dbl_write & FBDBL_NONE)
		    {
			/* !!! see cgtwelve.c EIC-COMMENTS before changing !!!*/
			prd->ctl_sp->eic.host_control &= ~CG12_EIC_DBL;
			prd->ctl_sp->apu.res2 = 0;
			/* !!! */
			/* !!! see cgtwelve.c EIC-COMMENTS before changing !!!*/
			prd->ctl_sp->eic.host_control |=
			    CG12_EIC_RES0 | CG12_EIC_RES1;
			prd->ctl_sp->apu.res2 = 0;
			/* !!! */
			prd->wid_dbl_info.dbl_write_state = PR_DBL_NONE;
			planes = prd->planes & PIX_ALL_PLANES;
			display = 0;
		    }
		    else if (DSP_MASK(dblinfo->dbl_write) == FBDBL_BOTH)
		    {
			/* !!! see cgtwelve.c EIC-COMMENTS before changing !!!*/
			prd->ctl_sp->eic.host_control &= ~CG12_EIC_DBL;
			prd->ctl_sp->apu.res2 = 0;
			/* !!! */
			/* !!! see cgtwelve.c EIC-COMMENTS before changing !!!*/
			prd->ctl_sp->eic.host_control &=
			    ~(CG12_EIC_RES0 | CG12_EIC_RES1);
			prd->ctl_sp->apu.res2 = 0;
			/* !!! */
			prd->wid_dbl_info.dbl_write_state = PR_DBL_BOTH;
			planes = prd->planes & PIX_ALL_PLANES;
			display = 0;
		    }
		    else if (dblinfo->dbl_write & FBDBL_A)
		    {
			/* !!! see cgtwelve.c EIC-COMMENTS before changing !!!*/
			prd->ctl_sp->eic.host_control |=
			    CG12_EIC_DBL | CG12_EIC_RES1;
			prd->ctl_sp->apu.res2 = 0;
			/* !!! */
			/* !!! see cgtwelve.c EIC-COMMENTS before changing !!!*/
			prd->ctl_sp->eic.host_control &= ~CG12_EIC_RES0;
			prd->ctl_sp->apu.res2 = 0;
			/* !!! */
			prd->wid_dbl_info.dbl_write_state = PR_DBL_A;
			planes = 0xf0f0f0 & (prd->planes & PIX_ALL_PLANES);
		    }
		    else if (dblinfo->dbl_write & FBDBL_B)
		    {
			/* !!! see cgtwelve.c EIC-COMMENTS before changing !!!*/
			prd->ctl_sp->eic.host_control |=
			    CG12_EIC_DBL | CG12_EIC_RES0;
			prd->ctl_sp->apu.res2 = 0;
			/* !!! */
			/* !!! see cgtwelve.c EIC-COMMENTS before changing !!!*/
			prd->ctl_sp->eic.host_control &= ~CG12_EIC_RES1;
			prd->ctl_sp->apu.res2 = 0;
			/* !!! */
			prd->wid_dbl_info.dbl_write_state = PR_DBL_B;
			planes = 0x0f0f0f & ((prd->planes & PIX_ALL_PLANES)>>4);
		    }
		    prd->ctl_sp->dpu.pln_wr_msk_host = planes;
		}

		if (display != 0xff)
		{
		    struct fb_wid_item  wid_item;
		    struct fb_wid_list  wid_list;

		    if (a_or_b == -1)
			ioctl(prd->fd, FBIO_DEVID, &a_or_b, 0);

		    wid_item.wi_type = 0;
		    wid_item.wi_index = prd->wid_dbl_info.dbl_wid.wa_index;
		    wid_item.wi_attrs = 1; /* 1 for ANY display update */
		    wid_item.wi_values[0] = display;
		    wid_item.wi_attrs |= (1<<CG12_WID_ALT_CMAP);
		    wid_item.wi_values[CG12_WID_ALT_CMAP] = 0;
		    wid_item.wi_attrs |= (1<<CG12_WID_ENABLE_2);
		    wid_item.wi_values[CG12_WID_ENABLE_2] = (a_or_b==1) ? 8 : 0;
		    wid_list.wl_flags = dblinfo->dbl_devstate&FBDBL_DONT_BLOCK;
		    wid_list.wl_count = 1;
		    wid_list.wl_list = &wid_item;
		    gp1_sync(prd->gp_shmem, prd->ioctl_fd);
		    if (ioctl(prd->fd, FBIO_WID_PUT, (char *) &wid_list, 0) < 0)
			return PIX_ERR;
		}
	    }
	    break;

	case FBIO_WID_ALLOC:
	    prd->wid_dbl_info.dbl_wid = *(struct fb_wid_alloc *) data;

	    if (a_or_b == -1)
		ioctl(prd->fd, FBIO_DEVID, &a_or_b, 0);

	    if (prd->wid_dbl_info.dbl_wid.wa_index == -1 ||
		((prd->wid_dbl_info.dbl_wid.wa_type == FB_WID_DBL_24) &&
		 ((prd->wid_dbl_info.dbl_wid.wa_index == shared_8) ||
		  (prd->wid_dbl_info.dbl_wid.wa_index == shared_24))))
	    {
		ioctl(prd->fd, FBIO_WID_ALLOC, &prd->wid_dbl_info.dbl_wid);
		*(struct fb_wid_alloc *) data = prd->wid_dbl_info.dbl_wid;

		switch (prd->wid_dbl_info.dbl_wid.wa_type)
		{
		    case FB_WID_SHARED_8:
			if (shared_8 != prd->wid_dbl_info.dbl_wid.wa_index)
			{
			    struct fb_wid_item  wid_item;
			    struct fb_wid_list  wid_list;

			    shared_8 = prd->wid_dbl_info.dbl_wid.wa_index;
			    wid_item.wi_type = 0;
			    wid_item.wi_index = shared_8;
			    wid_item.wi_attrs = 1; /* 1 for ALL displays */
			    wid_item.wi_values[0] = 1;
			    wid_item.wi_attrs |= (1<<CG12_WID_ALT_CMAP);
			    wid_item.wi_values[CG12_WID_ALT_CMAP] = 0;
			    wid_item.wi_attrs |= (1<<CG12_WID_ENABLE_2);
			    wid_item.wi_values[CG12_WID_ENABLE_2] =
				(a_or_b == 1) ? 8 : 0;
			    wid_list.wl_flags = 0;
			    wid_list.wl_count = 1;
			    wid_list.wl_list = &wid_item;
			    if (ioctl(prd->fd, FBIO_WID_PUT, &wid_list) < 0)
				return PIX_ERR;
			}
			break;

		    case FB_WID_SHARED_24:
			if (shared_24 != prd->wid_dbl_info.dbl_wid.wa_index)
			{
			    struct fb_wid_item  wid_item;
			    struct fb_wid_list  wid_list;

			    shared_24 = prd->wid_dbl_info.dbl_wid.wa_index;
			    wid_item.wi_type = 0;
			    wid_item.wi_index = shared_24;
			    wid_item.wi_attrs = 1; /* 1 for ALL displays */
			    wid_item.wi_values[0] = 0;
			    wid_item.wi_attrs |= (1<<CG12_WID_ALT_CMAP);
			    wid_item.wi_values[CG12_WID_ALT_CMAP] = 0;
			    wid_item.wi_attrs |= (1<<CG12_WID_ENABLE_2);
			    wid_item.wi_values[CG12_WID_ENABLE_2] =
				(a_or_b == 1) ? 8 : 0;
			    wid_list.wl_flags = 0;
			    wid_list.wl_count = 1;
			    wid_list.wl_list = &wid_item;
			    if (ioctl(prd->fd, FBIO_WID_PUT, &wid_list) < 0)
				return PIX_ERR;
			}
		}
	    }
	    break;

	case FBIO_FULLSCREEN_ELIMINATION_GROUPS:
	    {
		unsigned char  win_avail[PIXPG_LAST_PLUS_ONE];
		unsigned char *elim_groups = (unsigned char *) data;

		if (ioctl(prd->windowfd, WINGETAVAILPLANEGROUPS, win_avail) < 0)
		    break;
		bzero((char *) elim_groups, PIXPG_LAST_PLUS_ONE);
		if (win_avail[PIXPG_8BIT_COLOR] && win_avail[PIXPG_24BIT_COLOR])
		    elim_groups[PIXPG_8BIT_COLOR] = 1;
		elim_groups[PIXPG_ZBUF] = 1;
	    }
	    break;

	case FBIO_WID_DBL_SET:
	    {
		int                 i, offset;
		char                blocks[GP1_STATIC_BLOCKS];
		short              *shmem_ptr;
		unsigned int        bitvec, mask;
		struct static_block_info binfo;
		int	valid_flag = *(int *)data;

		/* Proceed only if valid_flag == 1 */
		if (!valid_flag) {
		    return 0;
		}
		
		/* can only double buffer in the 24-bit (32) plane group */

		if (pr->pr_depth != 32)
		    return 0;

		/* find the correct write mask for this pr */

		if (prd->windowfd >= 0)
		    ioctl(prd->windowfd, WINWIDGET, &prd->wid_dbl_info);
		switch (prd->wid_dbl_info.dbl_write_state)
		{
		    case PR_DBL_A:
			mask = (prd->wid_dbl_info.dbl_fore == PR_DBL_A)
			    ? 0xf0f0f0 & (prd->planes & PIX_ALL_PLANES)
			    : 0x0f0f0f & ((prd->planes & PIX_ALL_PLANES) >> 4);
			break;

		    case PR_DBL_B:
			mask = (prd->wid_dbl_info.dbl_back == PR_DBL_A)
			    ? 0xf0f0f0 & (prd->planes & PIX_ALL_PLANES)
			    : 0x0f0f0f & ((prd->planes & PIX_ALL_PLANES) >> 4);
			break;

		    case PR_DBL_NONE:
			mask = 0;
			break;

		    case PR_DBL_BOTH:
		    default:
			mask = prd->planes & PIX_ALL_PLANES;
		}

                /* set the mask for every context this process uses */
                binfo.sbi_array = blocks;
                if (i = ioctl(prd->ioctl_fd, GP1IO_INFO_STATIC_BLOCK, &binfo))
                    return 0;
		if (binfo.sbi_count == 0)
		    return 0;


		/*
		 * 0x230 = flag set by kernel indicating we need to run
		 * gpconfig. a really ugly situation that should never occur
		 * but if the gp is totally hung, well, there's not a lot we
		 * can do.
		 */
		if (prd->gp_shmem[0x230])
		    cg12_restart(pr);

		/* 0x231 = flag indicating to turn of all pixrect gpsi */
		if (prd->gp_shmem[0x231] ||
		    (offset = gp1_alloc(prd->gp_shmem, 1, &bitvec,
			prd->minordev, prd->ioctl_fd)) == 0)
		    return 0;

                shmem_ptr = (short *) prd->gp_shmem + offset;
                    
                for (i=0; i < binfo.sbi_count; i++)
                {   
                    *shmem_ptr++ = GP1_USE_CONTEXT | binfo.sbi_array[i];
                    *shmem_ptr++ = GP3_SET_DB_PLANES_RGB;
                    GP1_PUT_I(shmem_ptr, mask);
                }          
 
		*shmem_ptr++ = GP1_EOCL | GP1_FREEBLKS;
		GP1_PUT_I(shmem_ptr, bitvec);
		gp1_post(prd->gp_shmem, offset, prd->ioctl_fd);
	    }
	    break;
#endif	/* ndef KERNEL */

	default:
	    return PIX_ERR;
    }
    return 0;
}

#ifndef	KERNEL
/*
 * This routine is only called when something VERY WRONG has happened.
 * The kernel has performed a "reset" on the board and reloaded the micro-
 * sequencer code.  Since the kernel can't easily read a file, it has set
 * a bit to get pixrect to run gpconfig for it.  If called from SunView,
 * a redisplay-all is performed as well.
 */

#include	<stdlib.h>		/* getenv()		 */
#include	<sys/file.h>		/* open()		 */

cg12_restart(pr)
Pixrect            *pr;
{
    char               *window_dev;
    int                 win_fd;
    struct rect         old_screen_rect, new_screen_rect;

    system("/usr/etc/gpconfig >/dev/console");
    if (window_dev = getenv("WINDOW_PARENT"))
    {
	if ((win_fd = open(window_dev, O_RDWR, 0)) >= 0)
	{
	    if (ioctl(win_fd, WINGETRECT, &old_screen_rect) >= 0)
	    {
		new_screen_rect.r_left = old_screen_rect.r_left;
		new_screen_rect.r_top = old_screen_rect.r_top;
		new_screen_rect.r_width = old_screen_rect.r_width;
		new_screen_rect.r_height = 1;
		ioctl(win_fd, WINSETRECT, &new_screen_rect);
		ioctl(win_fd, WINSETRECT, &old_screen_rect);
	    }
	}
    }
}
#endif	/* ndef KERNEL */

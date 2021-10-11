#ifndef lint
static char        sccsid[] =     "@(#)gt_ioctl.c	1.1 94/10/31 SMI";
#endif
/* Copyright 1988 by Sun Microsystems, Inc. */

#include <sys/types.h>
#include <sys/ioccom.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_planegroups.h> 
#include <pixrect/gtvar.h>
#include <pixrect/pr_dblbuf.h>

/* all these includes just to get WINWIDGET defined -Egret design*/
 
#include <sys/time.h>  
#include <sys/ioctl.h> 
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>   
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h> 
#include <sunwindow/win_input.h> 
#include <sunwindow/win_ioctl.h>        /* WINWIDGET    */
 
#ifdef KERNEL
#include "gt.h"
#define ERROR ENOTTY

#if  NGT > 0
#define ioctl gtioctl
#else NGT > 0
#define ioctl
#endif NGT > 0

#else KERNEL
#define ERROR PIX_ERR
#endif KERNEL

#define DSP_MASK(m) ((m) & FBDBL_BOTH)
#define GT_DBL_ACCESS	0
#define GT_DBL_RELEASE 1

int 
gt_ioctl(pr, cmd, data)
    Pixrect *pr;
    int	    cmd;
    caddr_t data;
{
    struct gt_data    *gtd;

    gtd = (struct gt_data *)gt_d(pr);
    
    switch(cmd){
	    
	case FBIOGPLNGRP:
	    *(int*) data = PIX_ATTRGROUP(gtd->planes);
	break;

	case FBIOAVAILPLNGRP:
	{
	   static int	gt_grps;
	    gt_grps = MAKEPLNGRP(PIXPG_24BIT_COLOR)|
			    MAKEPLNGRP(PIXPG_WID)|
			    MAKEPLNGRP(PIXPG_8BIT_COLOR)|
			    MAKEPLNGRP(PIXPG_CURSOR_ENABLE)|
 			    MAKEPLNGRP(PIXPG_CURSOR);
	    *(int *)data = gt_grps;
	}
	break;
	

	case FBIO_WID_ALLOC:
	{
	    struct fb_wid_alloc	*winfo;
	    
	    winfo = (struct fb_wid_alloc *)data;
	    switch (winfo->wa_type) {
		case FB_WID_SHARED_8:
		    if (gtd->wid8.dbl_wid.wa_index != -1)
			    winfo->wa_index =  (gtd->wid8.dbl_wid.wa_index);
		    else {
			if (ioctl(gtd->fd, FBIO_WID_ALLOC, (caddr_t)winfo,0) < 0)
			    return PIX_ERR;
			winfo->wa_index <<= gtd->wid_part;
		    }
		break;
		case FB_WID_SHARED_24:
		    if (gtd->wid24.dbl_wid.wa_index != -1)
			    winfo->wa_index = gtd->wid24.dbl_wid.wa_index;
		     else
			if (ioctl(gtd->fd, FBIO_WID_ALLOC, (caddr_t)winfo,0) < 0)
			    return PIX_ERR;
		break;
		case FB_WID_DBL_8:
		   if (winfo->wa_index == -1) {
		     if (ioctl(gtd->fd, FBIO_WID_ALLOC, (caddr_t)winfo, 0) < 0)
					    return PIX_ERR;
			winfo->wa_index <<= gtd->wid_part;	    
			gtd->wid8.dbl_wid = *winfo;
		   } else 
			winfo->wa_index =
			    gtd->wid8.dbl_wid.wa_index;
		break;
		case FB_WID_DBL_24:
		    
		    if (winfo->wa_index == -1) {
			/* Always allocate a new wid */
		     if (ioctl(gtd->fd, FBIO_WID_ALLOC, (caddr_t)winfo,0) < 0)
			    return PIX_ERR;
		     gtd->wid24.dbl_wid = *winfo;
		   } else
			winfo->wa_index =
			    gtd->wid24.dbl_wid.wa_index;
		break;		
	    }
	}
	break;
	case FBIO_WID_FREE:
	{
	    struct fb_wid_alloc	*winfo;
	    
	    winfo = (struct fb_wid_alloc *)data;
	    switch (winfo->wa_type) {
		case FB_WID_SHARED_8:
		case FB_WID_DBL_8:
		     gtd->wid8.dbl_wid.wa_index = -1;
		     gtd->wid8.dbl_wid.wa_count = 0;
		     winfo->wa_index >>= gtd->wid_part;
		break;
		case FB_WID_SHARED_24:
		case FB_WID_DBL_24:
		     gtd->wid24.dbl_wid.wa_index = -1;
		     gtd->wid24.dbl_wid.wa_count = 0;
		break;
	    }
	    if (ioctl(gtd->fd, FBIO_WID_FREE, (caddr_t)winfo,0) < 0) return PIX_ERR;
	}
	break; 
	case FBIOSWINFD:
	    gtd->windowfd = *(int *) data;
	break;
	
	case FBIO_FULLSCREEN_ELIMINATION_GROUPS:
	{
	    u_char *elim_groups = (u_char *) data;
	
	    bzero((char *) elim_groups, PIXPG_LAST_PLUS_ONE);
	    elim_groups[PIXPG_CURSOR] = 1;
	    elim_groups[PIXPG_CURSOR_ENABLE] = 1;
	}
	break;   
#ifdef KERNEL
	/*
	 * double buffering will have different kernel and user versions
	 * of DLBGINFO and DBLSINFO.  the kernel will never really be
	 * setting any double buffer attributes so the "get" can fake
	 * static values and the "set" can simply return. **ALL** of db is
	 * egret design and for compatibility, I am merely following it.
	 */
	case FBIODBLGINFO:
	{	
	    struct fbdblinfo   *dblinfo = (struct fbdblinfo *) data;
	    
	    dblinfo->dbl_devstate |= FBDBL_AVAIL;
	    if ((PIX_ATTRGROUP(gtd->planes) == PIXPG_8BIT_COLOR) ||
		    (PIX_ATTRGROUP(gtd->planes) == PIXPG_24BIT_COLOR))
		dblinfo->dbl_devstate |= FBDBL_AVAIL_PG;
	    dblinfo->dbl_display = FBDBL_A;
	    dblinfo->dbl_read = FBDBL_A;
	    dblinfo->dbl_write = FBDBL_A;
	    dblinfo->dbl_depth = pr->pr_depth;/***?????*/
	    dblinfo->dbl_wid = 1; 
	}
	break;    
	
	case FBIODBLSINFO:
	break;

#endif KERNEL
#ifndef KERNEL

	
	case FBIODBLGINFO:
	{	
	    struct fbdblinfo *dblinfo = (struct fbdblinfo *) data;
	    struct fb_wid_dbl_info  wdbl;
	    
	    /* Get info from WLUT */
	    if (PIX_ATTRGROUP(gtd->planes) == PIXPG_8BIT_COLOR) {
		if ((gtd->wid8.dbl_wid.wa_type == FB_WID_DBL_8) 
					&& (gtd->windowfd >= 0)) {
	           if (	ioctl(gtd->windowfd, WINWIDGET, (caddr_t *)&gtd->wid8, 0) < 0)
				return PIX_ERR;
		}
		wdbl = gtd->wid8;
		dblinfo->dbl_depth = 8;
	    } else if (PIX_ATTRGROUP(gtd->planes) == PIXPG_24BIT_COLOR) {
		if ((gtd->wid24.dbl_wid.wa_type == FB_WID_DBL_24)
					  && (gtd->windowfd >= 0)) {
		     if (ioctl(gtd->windowfd, WINWIDGET, 
					(caddr_t *)&gtd->wid24,0) < 0)
				return PIX_ERR;
		}
		wdbl = gtd->wid24;
		dblinfo->dbl_depth = 32;
	    } else {
		 return PIX_ERR;
	    }
	    dblinfo->dbl_devstate |= FBDBL_AVAIL;
	    dblinfo->dbl_wid = 1;

	     /*display */
	     if (wdbl.dbl_fore == PR_DBL_A) {
		  dblinfo->dbl_display = FBDBL_A;
	     } else if (wdbl.dbl_fore == PR_DBL_B) {
		 dblinfo->dbl_display = FBDBL_B;
	     } 
	    /* Read */
	     switch (wdbl.dbl_read_state) {
		case GT_DBL_FORE:
		    if (wdbl.dbl_fore == PR_DBL_A)
			dblinfo->dbl_read = FBDBL_A;
		    else if (wdbl.dbl_fore == PR_DBL_B) 
		        dblinfo->dbl_read = FBDBL_B;
		break;
		case GT_DBL_BACK:
		    if (wdbl.dbl_back == PR_DBL_A)
			dblinfo->dbl_read = FBDBL_A;
		    else if (wdbl.dbl_back == PR_DBL_B) 
			dblinfo->dbl_read = FBDBL_B;
		break;
	     }
	    /* Write */
	     switch (wdbl.dbl_write_state) {
                case GT_DBL_FORE:
                if (wdbl.dbl_fore == PR_DBL_A) {
                    dblinfo->dbl_write = FBDBL_A;
                } else if (wdbl.dbl_fore == PR_DBL_B) {
                    dblinfo->dbl_write = FBDBL_B;
                }
                break;
                case GT_DBL_BACK:
                if (wdbl.dbl_back == PR_DBL_A) {
                    dblinfo->dbl_write = FBDBL_A;
                } else if (wdbl.dbl_back == PR_DBL_B) {
                    dblinfo->dbl_write = FBDBL_B;
                }
                break;
                case GT_DBL_NONE:
                    dblinfo->dbl_write = 0;
                break;
                case GT_DBL_BOTH:
                default:
                    dblinfo->dbl_write = FBDBL_A | FBDBL_B;
                break;
	     }
	}
	break;
	
	case FBIODBLSINFO:
	{
	    register struct fbdblinfo *dblinfo = (struct fbdblinfo *) data;
	    struct fb_wid_dbl_info  wdbl;
	    struct fb_wid_item  wid_item;
	    struct fb_wid_list  wid_list;
	    
	    /* erroneous requests */
	    if ((dblinfo->dbl_display & FBDBL_NONE) ||
		(dblinfo->dbl_read & FBDBL_NONE) ||
		(DSP_MASK(dblinfo->dbl_display) == FBDBL_BOTH) ||
		(DSP_MASK(dblinfo->dbl_read) == FBDBL_BOTH)) return PIX_ERR;


	    if (PIX_ATTRGROUP(gtd->planes) == PIXPG_8BIT_COLOR) {
		if (gtd->windowfd >= 0) {
		     if (ioctl(gtd->windowfd, WINWIDGET, (caddr_t *)&gtd->wid8,0) < 0)
				return PIX_ERR;
		}
		wdbl = gtd->wid8; 
		wid_item.wi_type = gtd->wid8.dbl_wid.wa_type;
		wid_item.wi_index = (wdbl.dbl_wid.wa_index >> gtd->wid_part);
		wid_item.wi_values[GT_WID_ATTR_PLANE_GROUP] = GT_8BIT_OVERLAY;
		wid_item.wi_values[GT_WID_ATTR_CLUT_INDEX] = gtd->clut_id;
		wid_item.wi_values[GT_WID_ATTR_FCS]= 4;
	    } else if (PIX_ATTRGROUP(gtd->planes) == PIXPG_24BIT_COLOR) {

		if (gtd->windowfd >= 0) {
		    if (ioctl(gtd->windowfd, WINWIDGET, (caddr_t *)&gtd->wid24,0) < 0)
			return PIX_ERR;
		}
		wdbl = gtd->wid24;
		wid_item.wi_type = gtd->wid24.dbl_wid.wa_type;
		wid_item.wi_index = wdbl.dbl_wid.wa_index;  
		wid_item.wi_values[GT_WID_ATTR_PLANE_GROUP] = GT_24BIT;
		wid_item.wi_values[GT_WID_ATTR_CLUT_INDEX] = GT_CLUT_INDEX_24BIT;
		wid_item.wi_values[GT_WID_ATTR_FCS]= 4;
	    }
		
	    /*Read */
	    if (dblinfo->dbl_read) {
		if (dblinfo->dbl_read & FBDBL_A) {
		    wdbl.dbl_read_state = PR_DBL_A;
		    gtd->buf &= ~HKPP_BS_I_READ_B;
		} else if (dblinfo->dbl_read & FBDBL_B) {
		    wdbl.dbl_read_state = PR_DBL_B;
		    gtd->buf |= HKPP_BS_I_READ_B;
		}
	    }
	    
	    /* Write */
	    if (dblinfo->dbl_write) {
		if (dblinfo->dbl_write & FBDBL_NONE) {
		    wdbl.dbl_write_state = PR_DBL_NONE;
		    gtd->buf &= ~(HKPP_BS_I_WRITE_A |HKPP_BS_I_WRITE_B);
		} else if (DSP_MASK(dblinfo->dbl_write) == FBDBL_BOTH) {
		    wdbl.dbl_write_state = PR_DBL_BOTH;
		    gtd->buf |= (HKPP_BS_I_WRITE_A |HKPP_BS_I_WRITE_B);
		} else if (dblinfo->dbl_write & FBDBL_A) {
		    wdbl.dbl_write_state = PR_DBL_A;
		    gtd->buf &= ~HKPP_BS_I_WRITE_B;
		    gtd->buf |= HKPP_BS_I_WRITE_A;
		} else if (dblinfo->dbl_write & FBDBL_B) {
		    wdbl.dbl_write_state = PR_DBL_B;
		    gtd->buf &= ~HKPP_BS_I_WRITE_A;
		    gtd->buf |= HKPP_BS_I_WRITE_B;
		}
	    }	    
	    
	    /*Display */
	    if (dblinfo->dbl_display) {
		if (dblinfo->dbl_display & FBDBL_A) {
		    wid_item.wi_values[GT_WID_ATTR_IMAGE_BUFFER] = 
							GT_BUFFER_A;
		    wid_item.wi_values[GT_WID_ATTR_OVERLAY_BUFFER] = 
							GT_BUFFER_A;
		} else if (dblinfo->dbl_display & FBDBL_B) {
		    wid_item.wi_values[GT_WID_ATTR_IMAGE_BUFFER] = 
							GT_BUFFER_B;
		    wid_item.wi_values[GT_WID_ATTR_OVERLAY_BUFFER] = 
							GT_BUFFER_A;	
		}
		/* Post to wlut - what about the blocking when???*/
		wid_item.wi_attrs = 0x1f;  
		wid_list.wl_list = &wid_item;
		wid_list.wl_count = 1;
		wid_list.wl_flags = FB_BLOCK;
		if (ioctl(gtd->fd, FBIO_WID_PUT, (caddr_t *) &wid_list, 0) < 0)
			return PIX_ERR;
	    }
	}
	break;	    
	
	case FBIO_WID_DBL_SET:
	{   
	    struct fb_wid_dbl_info  wid_dbl_info;
	    
	    /* Double buffering only for 8 & 24 bits */
	    if ((pr->pr_depth != 8) && (pr->pr_depth != 32))
			return (0); 

	    /* This basically sets up the plane mask for rop -
	     * Make sure that the mask changes only if necessary
	     * i.e if the read and write states have changed
	    */
	    if (gtd->windowfd >= 0) {
		if (ioctl(gtd->windowfd, WINWIDGET, (caddr_t *)&wid_dbl_info,0) < 0)
				return PIX_ERR;
	    }

	    if (PIX_ATTRGROUP(gtd->planes) == PIXPG_8BIT_COLOR) {
	        if ((gtd->wid8.dbl_wid.wa_type ==  FB_WID_SHARED_8) 
			&& (gtd->windowfd < 0))
			    wid_dbl_info.dbl_wid = gtd->wid8.dbl_wid;
		    gtd->wid8 = wid_dbl_info;
	    } else if (PIX_ATTRGROUP(gtd->planes) ==  PIXPG_24BIT_COLOR) {
		    if ((gtd->wid24.dbl_wid.wa_type ==  FB_WID_SHARED_24)
			    && (gtd->windowfd < 0)) 
			        wid_dbl_info.dbl_wid = gtd->wid24.dbl_wid;
		    gtd->wid24 = wid_dbl_info;
	    } else
		    return PIX_ERR;
	    
            switch (wid_dbl_info.dbl_write_state) {
                case GT_DBL_FORE:
                if (wid_dbl_info.dbl_fore == PR_DBL_A) {
                    gtd->buf &= ~HKPP_BS_I_WRITE_B;
                    gtd->buf |= HKPP_BS_I_WRITE_A;
                } else if (wid_dbl_info.dbl_fore == PR_DBL_B) {
                    gtd->buf &= ~HKPP_BS_I_WRITE_A;
                    gtd->buf |= HKPP_BS_I_WRITE_B;
                }
                break;
                case GT_DBL_BACK:
                if (wid_dbl_info.dbl_back == PR_DBL_A) {
                    gtd->buf &= ~HKPP_BS_I_WRITE_B;
                    gtd->buf |= HKPP_BS_I_WRITE_A;
                } else if (wid_dbl_info.dbl_back == PR_DBL_B) {
                    gtd->buf &= ~HKPP_BS_I_WRITE_A;
                    gtd->buf |= HKPP_BS_I_WRITE_B;
                }
                break;
                case GT_DBL_NONE:
                    gtd->buf &= ~(HKPP_BS_I_WRITE_A |HKPP_BS_I_WRITE_B);
                break;
                case GT_DBL_BOTH:
                default:
                    gtd->buf |= (HKPP_BS_I_WRITE_A |HKPP_BS_I_WRITE_B);
                break;
            }
	    switch (wid_dbl_info.dbl_read_state) {
		case GT_DBL_FORE:
		    if (wid_dbl_info.dbl_fore == PR_DBL_A)
			gtd->buf &= ~HKPP_BS_I_READ_B;
		    else if (wid_dbl_info.dbl_fore == PR_DBL_B) 
		        gtd->buf |= HKPP_BS_I_READ_B;    
		break;
		case GT_DBL_BACK:
		    if (wid_dbl_info.dbl_back == PR_DBL_A)
			gtd->buf &= ~HKPP_BS_I_READ_B;
		    else if (wid_dbl_info.dbl_back == PR_DBL_B) 
			gtd->buf |= HKPP_BS_I_READ_B;
		break;
	    }
	}
	break;
	case FB_CLUTALLOC:
	{
	    register struct fb_clutalloc *clut = (struct fb_clutalloc *)data;
	    
	    if (ioctl(gtd->fd, FB_CLUTALLOC, (caddr_t *) clut, 0) < 0)
							    return PIX_ERR;
	    gtd->clut_id = clut->index;
	}
	break;
	case FB_CLUTFREE: 
	{
	   register struct fb_clutalloc *clut = (struct fb_clutalloc *)data;
	   
	   if (ioctl(gtd->fd, FB_CLUTFREE, (caddr_t *) clut, 0) < 0)
							    return PIX_ERR;
	    gtd->clut_id = 0;	/* Set to clut#0 for safety! */
	}
	break;
	case FB_FCSALLOC:
	{
	    register struct fb_fcsalloc *fcs = (struct fb_fcsalloc *)data;
	    
	    if (ioctl(gtd->fd, FB_FCSALLOC, (caddr_t *) fcs, 0) < 0)
							    return PIX_ERR;
	    gtd->mpg_fcs |= (fcs->index & HKPP_MPG_FCS_MASK);
	}
	break;
	case FB_FCSFREE:
	{
	    register struct fb_fcsalloc *fcs = (struct fb_fcsalloc *)data;
	    
	    if (ioctl(gtd->fd, FB_FCSFREE, (caddr_t *) fcs, 0) < 0)
							    return PIX_ERR;
	    gtd->mpg_fcs = HKPP_MPG_FCS_DISABLE;
	}
	break;
	case FB_GRABHW:
	{
	   int	rdata;
	   if (ioctl(gtd->fd, FB_GRABHW, (caddr_t *)&rdata, 0) < 0) return PIX_ERR;
	}
	break;
	case FB_UNGRABHW:
	{
	   int	rdata;
	   if (ioctl(gtd->fd, FB_UNGRABHW, (caddr_t *) &rdata, 0) < 0) return PIX_ERR;
	}
	break;

	case FBIO_ASSIGNWID:
	{

	   /* This ioctl exists because Phigs needs to copy the
	    * assigned wid to all the pr associated with a pw
 	    * Also remember to tell the kernel about this because
            * preparesurface_full asks the kernel! Dean has a story
	    * about men, 2 watches and time that has something to do
	    * this.
	    */
	   struct fb_wid_alloc *winfo = (struct fb_wid_alloc *)data;
           switch (winfo->wa_type) {
		case FB_WID_DBL_8:
		{
		    if (gtd->windowfd >= 0) {
                      if (ioctl(gtd->windowfd, WINWIDGET, 
					(caddr_t *)&gtd->wid8, 0) < 0)
			return PIX_ERR;
		    }
		    gtd->wid8.dbl_wid.wa_index = winfo->wa_index;
		    gtd->wid8.dbl_wid.wa_count = winfo->wa_count;
                    if (gtd->windowfd >= 0) {
                      if (ioctl(gtd->windowfd, WINWIDSET, 
					(caddr_t *)&gtd->wid8,0) < 0)
			 return PIX_ERR;
		    }
		}
		break;
		case FB_WID_DBL_24:
		{
                    if (gtd->windowfd >= 0){ 
                      if (ioctl(gtd->windowfd, WINWIDGET,
					(caddr_t *) &gtd->wid24,0) < 0)
				return PIX_ERR; 
		    }
		    gtd->wid24.dbl_wid.wa_index = winfo->wa_index;
		    gtd->wid24.dbl_wid.wa_count = winfo->wa_count;
                    if (gtd->windowfd >= 0) {
                       if (ioctl(gtd->windowfd, WINWIDSET, 
					(caddr_t *)&gtd->wid24, 0)< 0)
				return PIX_ERR;
		    }
		}
		break;
		default:
		break;
	   }
	  break;
	}
	case FBIO_STEREO:
	{
	    int st = *(int *)data;
	    /* If the frame buffer can support stereo and the window
             * is also doing stereo - FB_WIN_STEREO can be set and unset
	     * but not the display.
	     * gtd->gt_stereo = GT_DISP_STEREO|FB_WIN_STEREO;
	     */
	    gtd->gt_stereo= *(int *)data;
	    gtd->gt_stereo |= st;
	}

#endif !KERNEL
	default:
	break;
    }
    return (0);		/* if successful */
}


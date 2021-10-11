#ifndef lint
static char	sccsid[] =	"@(#)gt.c	1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1989-1990, Sun Microsystems, Inc.
 */

/*
 * Hawk pixrect create and destroy
 */
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_make.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/gtvar.h>
#include <pixrect/pr_dblbuf.h>
#include <sys/ioctl.h>	
	
#define GT_HW_TIMEOUT 500    

static	struct pr_devdata   *gtdevdata;
static  int group[] = {PIXPG_24BIT_COLOR, PIXPG_WID, PIXPG_CURSOR, 
			PIXPG_CURSOR_ENABLE, PIXPG_8BIT_COLOR, PIXPG_ZBUF};

static  int fdepth[] =  { GT_DEPTH_24, GT_DEPTH_WID,
			  GT_DEPTH_1, GT_DEPTH_1, GT_DEPTH_8,GT_DEPTH_24};
			 
static	int img_ptr[] = {GT_FB_VOFFSET, GT_FB_VOFFSET, GT_FB_VOFFSET,
		GT_FB_CE_VOFFSET, GT_FB_BACCESS_VOFFSET,GT_FB_VOFFSET};
			 
static	int pmask[] = {0x00ffffff, 0x00ffffff, 0x1, 0x1, 0xff, 0x00ffffff};

Pixrect	*
gt_make (fd, size, depth)
    int             fd;
    struct pr_size  size;
    int             depth;
{
    register Pixrect		*pr;
    register struct gt_data    *gtd;
    struct  pr_devdata		*dd;
    struct  gt_fb		*fbp;
    struct  fb_wid_list		w_list;
    struct  fb_wid_item		w_item;
    int				i;
    int				 mon_type, stereo;

    /* allocate pixrect and map frame buffer */
    if (!(pr = pr_makefromfd(fd, size, depth, &gtdevdata, &dd,
	    GT_DPORT_MMAPSIZE, sizeof(struct gt_data), GT_DPORT_VBASE)))
	return pr;
		
    pr->pr_size = size;
    pr->pr_depth = depth;
    pr->pr_ops = &gtp_ops;

    gtd = gt_d(pr);
    /* Get the monitor width from the kernel */
    if (ioctl(dd->fd, FB_GETMONITOR, &mon_type) < 0) {
	printf("Unable to determine monitor type - gt_make \n");
	return ((Pixrect *) 0);
    }
    switch (mon_type & HKPP_FBW_MASK) {
	case (HKPP_FBW_1280): stereo = 0; break;
	case (HKPP_FBW_960): stereo = 0; break;
	case (HKPP_FBW_960STEREO): stereo = GT_DISP_STEREO; break;
	default: printf("Unknown monitor type - gt_make \n"); 
				return ((Pixrect *) 0); 
    }
    /* Initialize the various frame buffers */
    for (fbp = gtd->fb, i = 0; i < GT_NFBS; i++, fbp++) {
	fbp->group = group[i];
	fbp->depth = fdepth[i];
	fbp->mprp.mpr.md_image = (short *)(dd->va + img_ptr[i]);
	fbp->mprp.mpr.md_linebytes = mpr_linebytes (GT_OVERLAY_LINEBYTES, fdepth[i]);
	fbp->mprp.mpr.md_offset.x = 0;
	fbp->mprp.mpr.md_offset.y = 0;
	fbp->mprp.mpr.md_primary = 1;
	fbp->mprp.mpr.md_flags = MP_DISPLAY | MP_PLANEMASK;
        fbp->mprp.planes = pmask[i];
    }

    gtd->fd = dd->fd;
    gtd->flag = 0;
    gtd->gt_rp = (H_rp *) (dd->va + GT_RP_HOST_VOFFSET);
    gtd->gt_fbi_go = (H_fbi_go *) (dd->va + GT_FBI_PFGO_VOFFSET);
    gtd->gt_fbi = (H_fbi *) (dd->va + GT_FBI_REG_VOFFSET);
    gtd->gt_fe_h = (H_fe_hold *) (dd->va + GT_FE_I860_VOFFSET);
    gtd->gt_fe_frozen = 0;
    gtd->gt_stereo = stereo;    
    /* set the front end timeout value */
    gtd->gt_fe_timeout = GT_HW_TIMEOUT;
    if ((char *) getenv("GT_HW_TIMEOUT")) {
	 if (atoi((char *) getenv("GT_HW_TIMEOUT")) > GT_HW_TIMEOUT)
		gtd->gt_fe_timeout = atoi((char *) getenv("GT_HW_TIMEOUT"));
    } 
    
    /* Set default to the 8-bit planegroup */
    bcopy((char *) &(gtd->fb[4].mprp), (char *) &(gtd->mprp),
           sizeof (gtd->fb[4].mprp));
    gtd->planes = (PIX_GROUP (PIXPG_8BIT_COLOR) |
                    (gtd->mprp.planes & PIX_ALL_PLANES));
    gtd->active = 4;
    gtd->clut_id = 0;
    gtd->windowfd = -1;
    gtd->mpg_fcs = HKPP_MPG_FCS_DISABLE;	/* Disable */ 
    gtd->buf = (HKPP_BS_I_WRITE_A); 
    
    /* Determine the partion register. the overlay wids are to be shifted
     * by this amount for preparation */
    gtd->wid_part = 5;
    
    /* Get the 2 wids from driver */

    /*Common 8bit wid */ 
    gtd->wid8.dbl_wid.wa_type = FB_WID_SHARED_8;
    gtd->wid8.dbl_wid.wa_count = 1;
    if (ioctl(fd, FBIO_WID_ALLOC, (caddr_t *)&(gtd->wid8.dbl_wid), 0) < 0) 
					return (Pixrect *)PIX_ERR; 
    gtd->wid8.dbl_wid.wa_index <<= gtd->wid_part;
    gtd->wid8.dbl_fore = PR_DBL_A;
    gtd->wid8.dbl_back = PR_DBL_B;
    gtd->wid8.dbl_write_state = GT_DBL_FORE;
    gtd->wid8.dbl_read_state = GT_DBL_FORE;
    
    w_list.wl_count = 1;
    w_list.wl_list = &w_item;
    w_list.wl_flags = 0;    

    w_item.wi_type = FB_WID_SHARED_8;
    w_item.wi_index = (gtd->wid8.dbl_wid.wa_index >> gtd->wid_part);
    w_item.wi_attrs = 0x1f;
    w_item.wi_values[GT_WID_ATTR_PLANE_GROUP] = GT_8BIT_OVERLAY;
    w_item.wi_values[GT_WID_ATTR_IMAGE_BUFFER] = GT_BUFFER_A;
    w_item.wi_values[GT_WID_ATTR_OVERLAY_BUFFER] = 0;	/* dummy*/
    w_item.wi_values[GT_WID_ATTR_CLUT_INDEX] = 0;
    w_item.wi_values[GT_WID_ATTR_FCS]= 4;
    if (ioctl(fd, FBIO_WID_PUT, (caddr_t*)&w_list , 0) < 0) return (Pixrect *)PIX_ERR;

    /* Common 24bit wid */
    gtd->wid24.dbl_wid.wa_type = FB_WID_SHARED_24;
    gtd->wid24.dbl_wid.wa_count = 1;
    if (ioctl(fd, FBIO_WID_ALLOC, (caddr_t *)&(gtd->wid24.dbl_wid), 0) < 0) 
					return (Pixrect *)PIX_ERR;
    gtd->wid24.dbl_fore = PR_DBL_A;
    gtd->wid24.dbl_back = PR_DBL_B;
    gtd->wid24.dbl_write_state = GT_DBL_FORE;
    gtd->wid24.dbl_read_state = GT_DBL_FORE;
		
    w_item.wi_type = FB_WID_SHARED_24;
    w_item.wi_index = gtd->wid24.dbl_wid.wa_index;
    w_item.wi_attrs = 0x1f;

    w_item.wi_values[GT_WID_ATTR_PLANE_GROUP] = GT_24BIT;
    w_item.wi_values[GT_WID_ATTR_IMAGE_BUFFER] = GT_BUFFER_A;
    w_item.wi_values[GT_WID_ATTR_OVERLAY_BUFFER] = 0;	/* dummy*/
    w_item.wi_values[GT_WID_ATTR_CLUT_INDEX] = GT_CLUT_INDEX_24BIT;
    w_item.wi_values[GT_WID_ATTR_FCS]= 4;
    if (ioctl(fd, FBIO_WID_PUT, (caddr_t *)&w_list, 0) < 0) return (Pixrect *)PIX_ERR;

    /* (re)set up the hardware registers - in default byte mode */
	gt_reset_regs(gtd->gt_rp, gtd->gt_fbi, gtd->gt_fe_timeout);
    return pr;
}


gt_destroy(pr)
    Pixrect *pr;

{
    if (pr) {
	register struct gt_data *gtd;

        if ((gtd = gt_d(pr)) != 0) {
	    if (gtd->mprp.mpr.md_primary)
		pr_unmakefromfd(gtd->fd, &gtdevdata);
                free((char *) gtd);
        }
        free((char *)pr);
    }
    return 0;
}


gt_reset_regs(rp, fbi, timeout)
    H_rp  *rp;
    H_fbi *fbi;
    int	  timeout;
{   
    gt_waitidle(rp, timeout);
    /* THese are all always constant */
    fbi->vwclp_x.wd	= HKPP_VCX_MASK & 0x000007FF;		
    fbi->vwclp_y.wd	= HKPP_VCY_MASK & 0x000007FF;
    fbi->stereo.wd	= HKPP_SWE_MASK & HKPP_SWE_MONO;
    fbi->con_z.wd	= HKPP_CZS_MASK & 0;
    fbi->z_ctrl.wd	= HKPP_ZC_MASK & 0;
    fbi->b_mode.wd	= HKPP_BMCS_MASK & HKPP_BMCS_ALPHA_OV;	
    fbi->mpg_set.wd	= HKPP_FCC_MASK & 0x4;
}

#ifndef	lint
static char sccsid[] = "@(#)cg12_vector.c 1.1 94/10/31 SMI";
#endif

/* Copyright 1990 Sun Microsystems, Inc. */

#include <pixrect/pixrect.h>
#include <pixrect/gp1cmds.h>
#include <pixrect/cg12_var.h>
#include <pixrect/mem32_var.h>
#include <pixrect/pr_planegroups.h> /* PIX_ALL_PLANES       */
#include <pixrect/pr_impl_util.h>
#include <pixrect/pr_dblbuf.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <math.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>

/* !!!### remove this line to re-enable acceleration ###!!! */
#define	VECTOR_DEBUG

/******************************************************************************/
/*                                                                            */
/*  cg12_vector

    Basic outline:

    translate x and y according to md_offset.x and md_offset.y

    gp1_alloc block in which to post buffer

    get op, planegroup, and color from parameters
    build op and planegroup
    build dx_dy from x and y
    build dw_dh from dpr->pr_size.x and dpr->pr_size.y, use fb size if 
      PIX_DONCLIP is set
    
    put data in buffer

    post buffer  */

/*                                                                            */
/******************************************************************************/


cg12_vector(dpr, x0, y0, x1, y1, op, color)
Pixrect            *dpr;
int                 x0,
                    y0,
                    x1,
                    y1,
                    op,
                    color;
{

  register int dpr_plngrp,planes_mask;
  register int op_color=PIXOP_COLOR(op),rop = PIXOP_OP(op),dw_dh,dbl_status_ok;
  register struct cg12_data *dprd = cg12_d(dpr);
  int result;
  extern int cg12_width, cg12_height;

#ifndef VECTOR_DEBUG

  dpr_plngrp = PIX_ATTRGROUP(dprd->planes);

  /* do only if 8 or 24 bit, */
  /* punt if the sum of dx and dy is less then 41.  This was derived 
     empirically.  First, we found a cutoff based on the length of the line
     between mem_vector and cg12_vector.  This cutoff was around 29
     pixels in length, i.e. lines shorter than 29 should not be drawn
     with GP3_PR_VEC24 but with mem_vector instead.  Second, to avoid
     calculating the length of the line for each call to this routine,
     we determined that on average the sum of dx and dy was about 41. 
     Note, this is purely an average and based on integer arithmetic only.
     This turns out to be quicker than any multiplication for a bounding
     box and roughly equivalent.  This fix was implemented for an RFE.
     See the sccs history for more info.  */

    if (!((_ABS((x1 - x0)) + _ABS((y1 - y0))) >= 41) || 
      (dpr_plngrp != PIXPG_8BIT_COLOR && dpr_plngrp != PIXPG_24BIT_COLOR) )
    {
      Pixrect             mem32_pr;
      struct mprp32_data  mem32_pr_data;

      CG12_PR_TO_MEM(dpr, mem32_pr);
      return mem_vector(dpr, x0, y0, x1, y1, op, color);
    } 

  if (dprd->windowfd >= 0)
    ioctl(dprd->windowfd, WINWIDGET, &dprd->wid_dbl_info);

  /* assign color and plane_mask if 8 bit mode */
  if (op_color) 
    {
      color = op_color;
    }

  planes_mask = dprd->planes & 0x00FFFFFF;

  if (dpr_plngrp == PIXPG_8BIT_COLOR) 
    {
      color &= 0xFF;
      color = (color << 16) | (color << 8) | color;
      planes_mask &= 0xFF;
      planes_mask = (planes_mask << 16) | (planes_mask << 8) | planes_mask;
     }
  else 
    {
      if (dprd->wid_dbl_info.dbl_write_state == PR_DBL_A ||
	  dprd->wid_dbl_info.dbl_write_state == PR_DBL_B)
	{
	  register int i;
	  i = ((dprd->wid_dbl_info.dbl_write_state == PR_DBL_A)
	       ? dprd->wid_dbl_info.dbl_fore
	       : dprd->wid_dbl_info.dbl_back);
	  if (i == PR_DBL_B)
	    {
	      color = (color >> 4);
	      planes_mask = 0x0f0f0f;
	    }
	  else
	    planes_mask = 0xf0f0f0;
	}
    }

  {
    /* build gpsi buffer and post it */
    register int *shmem_ptr;
    unsigned int bitvec;
    int shmem_off;

    /*
     * 0x230 = flag set by kernel indicating we need to run gpconfig. a
     * really ugly situation that should never occur but if the gp is totally
     * hung, well, there's not a lot we can do.
     */
    if (dprd->gp_shmem[0x230])
	cg12_restart(dpr);

    /* 0x231 = flag indicating to turn off acceleration */
    if (dprd->gp_shmem[0x231] ||
	(shmem_off = gp1_alloc((caddr_t) dprd->gp_shmem, 1, &bitvec,
			       dprd->minordev, dprd->ioctl_fd)) == 0)
      {
	Pixrect             mem32_pr;
	struct mprp32_data  mem32_pr_data;
 
	CG12_PR_TO_MEM(dpr, mem32_pr);
	return mem_vector(dpr, x0, y0, x1, y1, op, color);
      } 
 
    shmem_ptr = (int *)(dprd->gp_shmem);
    shmem_ptr += (shmem_off >> 1);

    *shmem_ptr++ = (GP3_PR_VEC24 << 16) | (dpr_plngrp << 8) | (rop << 1);

    /* build dx_dy and dw_dh */
    if (op & PIX_DONTCLIP) 
      {
	*shmem_ptr++ = 0;  /* dx = 0, dy = 0 */
	*shmem_ptr++ = (cg12_width << 16) | cg12_height;
      } 
    else 
      {
	*shmem_ptr++ = (dprd->mprp.mpr.md_offset.x << 16) | 
	  dprd->mprp.mpr.md_offset.y;
	*shmem_ptr++ = (dpr->pr_size.x << 16) | dpr->pr_size.y;
      }

    /* build P0 and P1 */
    *shmem_ptr++ = ((y0 + dprd->mprp.mpr.md_offset.y) << 16) | ((x0 + dprd->mprp.mpr.md_offset.x) & 0xFFFF);
    *shmem_ptr++ = ((y1 + dprd->mprp.mpr.md_offset.y) << 16) | ((x1 + dprd->mprp.mpr.md_offset.x) & 0xFFFF);

    /* build color and planes_mask */
    *shmem_ptr++ = color;
    *shmem_ptr++ = planes_mask;

    /* finish off buffer */
    *shmem_ptr++ = ((GP1_EOCL | GP1_FREEBLKS) << 16) | (bitvec >> 16);
    *shmem_ptr++ = (bitvec << 16);
    
    result = gp1_post(dprd->gp_shmem, shmem_off, dprd->ioctl_fd);
    gp1_sync((caddr_t)dprd->gp_shmem, dprd->fd);   
    return result; 
  }

#endif

#ifdef VECTOR_DEBUG
{
    Pixrect             mem32_pr;

    CG12_PR_TO_MEM(dpr, mem32_pr);
    return mem_vector(dpr, x0, y0, x1, y1, op, color);
}
 
#endif VECTOR_DEBUG


}

#ifndef lint
static char sccsid[] = "@(#)cg12_replrop.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1990, Sun Microsystems, Inc.
 */

#include <pixrect/pixrect.h>
#include <pixrect/gp1cmds.h>
#include <pixrect/cg12_var.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/pr_dblbuf.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>

/******************************************************************************/
/*                                                                            */
/*  cg12_replrop

    Basic outline:

    if (spr) 
      check width of spr, PIX_ERR if != 16?
      check depth, PIX_ERR if not 1
      check (dx - sx), PIX_ERR if <= 0?
      check (dy - sy), PIX_ERR if <= 0?
      check (md_offset & 15), PIX_ERR if > 0
      check (spr is mempr), PIX_ERR if not.
      check height of spr, PIX_ERR if != 16,8,4,2,1?
    else
      return PIX_ERR.

    if !PIXDONTCLIP
      adjust dx, dy to screen coords and clip to screen
      clip dw, dh to screen
    else
      adjust dx, dy to screen coords

    adjust sx, sy: XXX does this need to be done?
      (dx - sx) % 16 -> sx. 
      (dy - sy) % 16 -> sy. 
    adjust sw, sh?

    case 24bit:
      get color from op arg.
    case 8bit:
      get color from op arg and replicate for each DPU
    case 1bit:
      get color from op arg, if 0 then 0 -> color else if ~0 all F's -> color 
      
    get shmem offset from gp1_alloc.  If unavailable, return PIX_ERR
    add offset to shmem ptr.

    load gpsi buffer with data:
      GP3_PR_ROP24_TEX
      planegroup | rop
      dx, dy
      dw, dh
      sx, sy
      sw, sh
      color
      planemask from dpr

    calculate md_image of spr

    <sync>

    download md_image of spr to off-screen VRAM

    post buffer
    
                                                                              */
/*                                                                            */
/******************************************************************************/

cg12_replrop(dpr, dx, dy, dw, dh, op, spr, sx, sy)
     Pixrect *dpr;
     int     dx, dy;
     int     dw, dh, op;
     Pixrect *spr;
     int     sx, sy;

{

  register struct cg12_data *dprd = cg12_d(dpr);
  register int rop = PIXOP_OP(op);
  register int color = PIX_OPCOLOR(op);
  register int dpr_plngrp;
  unsigned int planes_mask,result;
  u_int bitvec;
  register int shmem_off;
  static int bit_swap[16] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
  extern int cg12_width, cg12_height;

  /*
   * Weed out the cases which are not handled by GP3_ROP24_TEX
   */

  dpr_plngrp = PIX_ATTRGROUP(dprd->planes);

  /* XXX should (dx - sx), (dy - sy) and (md_offset.x & 15) be weeded out? */

  if (spr) 
    {
      register struct mpr_data *sprd = mpr_d(spr);
      if ((dpr_plngrp != PIXPG_24BIT_COLOR && dpr_plngrp != PIXPG_8BIT_COLOR) ||
	  (spr->pr_width != 64 && spr->pr_width != 32 && spr->pr_width != 16 && 
	  spr->pr_width != 8 && spr->pr_width != 4 && spr->pr_width != 2 && 
	  spr->pr_width != 1) || spr->pr_depth > 1 || (dx - sx) < 0 || (dy - sy) < 0 ||
	  ((sprd->md_offset.x & 15) != 0) || MP_NOTMPR(spr) 
	  || sprd->md_linebytes < 2)
	{
	  return (PIX_ERR);
	}
    
    } else
      return PIX_ERR;   /* return PIX_ERR, no source given.  */

  /* translate dx, dy into screen coords */
  
  dx += dprd->mprp.mpr.md_offset.x;
  dy += dprd->mprp.mpr.md_offset.y;

  /* clip dx+dw, dy+dh to screen coords, XXX necessary? */
  /* assumes 0 <= dx <= 1151 and 0 <= dy <= 899 XXX correct? */

  if (!(op & PIX_DONTCLIP)) 
    {
      if (dx < 0) 
	dx = 0;
      else
	if (dx > cg12_width)
	  dx = cg12_width;

      if (dy < 0) 
	dy = 0;
      else
	if (dy > cg12_height)
	  dy = cg12_height;
      
      if ((dx + dw) > cg12_width)
	dw = cg12_width - dx;

      if ((dy + dh) > cg12_height)
	dh = cg12_height - dy;

      
    }

  /* adjust color if 8bit emulation or if overlay, XXX overlay tex rop ok? */

  planes_mask = (dprd->planes & 0xFFFFFF);
  if (dpr_plngrp == PIXPG_8BIT_COLOR) 
    {
      if (rop)
	color = 0xFFFFFF;
      else
	color = 0;
      planes_mask &= 0xFF;
      planes_mask = (planes_mask << 16) | (planes_mask << 8) | planes_mask;
    }
  else
    {
      if (dprd->windowfd >= 0)
	ioctl(dprd->windowfd, WINWIDGET, &dprd->wid_dbl_info);
       
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

    register int   *shmem_ptr;
    register struct mpr_data *sprd = mpr_d(spr);
    register int   tex_addr;
    register int   sh,sw;
    unsigned int saved_haccess,saved_hpage,saved_hctrl;
    unsigned int saved_pln_wr_mask;

    sw = spr->pr_width;
    sh = spr->pr_height;

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
      return PIX_ERR;

    shmem_ptr = (int *)(dprd->gp_shmem);
    shmem_ptr += (shmem_off >> 1);
    *shmem_ptr++ = (GP3_PR_ROP24_TEX << 16) | (dpr_plngrp << 8) | (rop << 1);

    sx = (dx % sw) - sx;
    sy = (dy % sh) - sy;  

    *shmem_ptr++ = (dx << 16) | dy;
    *shmem_ptr++ = (dw << 16) | dh;
    *shmem_ptr++ = (sx << 16) | (sy & 0xffff);

    *shmem_ptr++ = (sw << 16) | sh;
    *shmem_ptr++ = color;
    *shmem_ptr++ = planes_mask;

    *shmem_ptr++ = ((GP1_EOCL | GP1_FREEBLKS) << 16) | (bitvec >> 16);
    *shmem_ptr   = (bitvec << 16);

    
    /* calculate address of source tex data */
    tex_addr = (int)PTR_ADD(sprd->md_image, 
			   ((short)sprd->md_offset.y * sprd->md_linebytes) +
			   (sprd->md_offset.x >> 4 << 1));

    /* save EIC HOST CONTROL and set to not do double buffering */
    /* do before other apu/dpu regs */
    /* !!! */
    saved_hctrl = dprd->ctl_sp->eic.host_control;
    dprd->ctl_sp->eic.host_control &= ~(CG12_EIC_DBL);

    /* save HPAGE register and set to 24bit fb */
    saved_hpage = dprd->ctl_sp->apu.hpage;
    dprd->ctl_sp->apu.hpage =
	(cg12_width == 1280) ? CG12_HPAGE_24BIT_HR : CG12_HPAGE_24BIT;

    /* save HACCESS register and set to 4 bits/pixel */
    saved_haccess = dprd->ctl_sp->apu.haccess;
    dprd->ctl_sp->apu.haccess = CG12_OS_HACCESS;

    /* save PLN_WR_MASK_HOST and set to write to lower 4 bits of each byte */
    saved_pln_wr_mask = dprd->ctl_sp->dpu.pln_wr_msk_host;
    dprd->ctl_sp->dpu.pln_wr_msk_host = CG12_OS_PLN_WR_MASK;

    /* download texture into offscreen VRAM */

    {
      
      register int *os_vram_ptr = (int *)(dprd->mprp.mpr.md_image);
      register int src_count,src_data, vram_data, line_count, line_data;
      register unsigned short *data_ptr = (unsigned short *)tex_addr;
      extern int cg12_offscreen;
      
      /* initialize start of offscreen memory */
      /* used to be hard coded as:  ((1152 * 900) >> 3) + 40 */
      os_vram_ptr += cg12_offscreen;

      /* XXX need to sync gp? */
      gp1_sync((caddr_t)dprd->gp_shmem, dprd->fd);

      switch (sw) {
       case 64 :
	{
	  for (src_count = 0; src_count < (sh << 1); src_count++) 
	    {
	      src_data = *data_ptr++;
	      src_data = (src_data << 16) | *data_ptr++;
	      vram_data = 0;
	      vram_data |= ((bit_swap[((src_data) & 0xF)]));
	      vram_data |= ((bit_swap[((src_data >> 4) & 0xF)]) << 4);
	      vram_data |= ((bit_swap[((src_data >> 8) & 0xF)]) << 8);
	      vram_data |= ((bit_swap[((src_data >> 12) & 0xF)]) << 12);
	      vram_data |= ((bit_swap[((src_data >> 16) & 0xF)]) << 16);
	      vram_data |= ((bit_swap[((src_data >> 20) & 0xF)]) << 20);
	      vram_data |= ((bit_swap[((src_data >> 24) & 0xF)]) << 24);
	      vram_data |= ((bit_swap[((src_data >> 28) & 0xF)]) << 28);
	      *os_vram_ptr++ = vram_data;
	      
	    }
	}
	break;
      case 32 :
	{
	  for (src_count = 0; src_count < sh; src_count++) 
	    {
	      src_data = *data_ptr++;
	      src_data |= (src_data << 16) | *data_ptr++;
	      vram_data = 0;
	      vram_data |= ((bit_swap[((src_data) & 0xF)]));
	      vram_data |= ((bit_swap[((src_data >> 4) & 0xF)]) << 4);
	      vram_data |= ((bit_swap[((src_data >> 8) & 0xF)]) << 8);
	      vram_data |= ((bit_swap[((src_data >> 12) & 0xF)]) << 12);
	      vram_data |= ((bit_swap[((src_data >> 16) & 0xF)]) << 16);
	      vram_data |= ((bit_swap[((src_data >> 20) & 0xF)]) << 20);
	      vram_data |= ((bit_swap[((src_data >> 24) & 0xF)]) << 24);
	      vram_data |= ((bit_swap[((src_data >> 28) & 0xF)]) << 28);
	      *os_vram_ptr++ = vram_data;
	      *os_vram_ptr++ = vram_data;
	    }
	}
	break;
      case 16 :
	{
	  for (src_count = 0; src_count < sh; src_count++) 
	    {
	      vram_data = 0;
	      src_data = *data_ptr++;
	      vram_data |= ((bit_swap[((src_data) & 0xF)]));
	      vram_data |= ((bit_swap[((src_data >> 4) & 0xF)]) << 4);
	      vram_data |= ((bit_swap[((src_data >> 8) & 0xF)]) << 8);
	      vram_data |= ((bit_swap[((src_data >> 12) & 0xF)]) << 12);
	      *os_vram_ptr++ = (vram_data << 16) | vram_data;
	      *os_vram_ptr++ = (vram_data << 16) | vram_data;
 	    }
	}
	break;
      case 8 :
	{
	  for (src_count = 0; src_count < sh; src_count++) 
	    {
	      src_data = *data_ptr++;
	      vram_data = 0;
	      line_data = (src_data >> 8) & 0xFF;
	      vram_data |= ((bit_swap[((line_data) & 0xF)]));
	      vram_data |= ((bit_swap[((line_data >> 4) & 0xF)]) << 4);
	      vram_data = (vram_data << 24) | (vram_data << 16) |
		(vram_data << 8) | vram_data;
	      *os_vram_ptr++ = vram_data;
	      *os_vram_ptr++ = vram_data;
	    }
	}
	break;
      case 4 :
	{
	  for (src_count = 0; src_count < sh; src_count++) 
	    {
	      src_data = *data_ptr++;
	      vram_data = 0;
	      line_data = (src_data >> 12) & 0xF;
	      vram_data = ((bit_swap[((line_data) & 0xF)]));
	      vram_data |= (vram_data << 12) | (vram_data << 8) | 
		(vram_data << 4) | vram_data;
	      vram_data |= (vram_data << 16);
	      *os_vram_ptr++ = vram_data;
	      *os_vram_ptr++ = vram_data;
	    }
	}
	break;
      case 2 :
	{
	  for (src_count = 0; src_count < sh; src_count++)
	    {
	      switch ((*data_ptr++ >> 14) & 0x3) {
	      case 0 :
		*os_vram_ptr++ = 0;
		*os_vram_ptr++ = 0;
		break;
	      case 1 :
		*os_vram_ptr++ = 0xaaaaaaaa;
		*os_vram_ptr++ = 0xaaaaaaaa;
		break;
	      case 2 :
		*os_vram_ptr++ = 0x55555555;
		*os_vram_ptr++ = 0x55555555;
		break;
	      case 3 :
		*os_vram_ptr++ = 0xFFFFFFFF;
		*os_vram_ptr++ = 0xFFFFFFFF;
		break;
	      }
	    }
	}
	break;
      case 1 :
	for (src_count = 0; src_count < sh; src_count++)
	  {
	    src_data = *data_ptr++;
	    if (src_data & 0x80)
	      {
		*os_vram_ptr++ = 0xFFFFFFFF;
		*os_vram_ptr++ = 0xFFFFFFFF;
	      }
	    else
	      {
		*os_vram_ptr++ = 0;
		*os_vram_ptr++ = 0;
	      }
		
	  }
	
	
      }
    }
    /* reset host registers, do eic first see comments */

    /* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
    /* leave WSTATUS bit set always to inform PROM to save the
       context before displaying any character */
    dprd->ctl_sp->eic.host_control = saved_hctrl;
    /* !!! */
    dprd->ctl_sp->apu.hpage = saved_hpage;
    dprd->ctl_sp->apu.haccess = saved_haccess;
    dprd->ctl_sp->dpu.pln_wr_msk_host = saved_pln_wr_mask;

    result = gp1_post(dprd->gp_shmem, shmem_off, dprd->ioctl_fd);
    gp1_sync((caddr_t)dprd->gp_shmem, dprd->fd);
    return result;

  }
  
}

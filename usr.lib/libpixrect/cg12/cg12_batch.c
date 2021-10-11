#ifndef lint
static char sccsid[] = "@(#)cg12_batch.c 1.1 94/10/31 SMI";
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
/*  cg12_batchrop

    Basic outline:

    weed out cases not covered by GP3_ROP24_BAT:
      if planegroup is not 24bit or 8bit.

    if !PIXDONTCLIP
      adjust dx, dy to screen coords and clip to screen
      clip dw, dh to screen
    else
      adjust dx, dy to screen coords

    case 24bit:
      get color from op arg.
    case 8bit:
      get color from op arg and replicate for each DPU
      
    set bw, bh:
      bw = width of first pixrect.
      bh = height of first pixrect.

    check each bitmap to see if it's legit:
      check linebytes in spr, punt to gen_batchrop if not 2.
      check depth, punt to gen_batchrop if not 1
      check (md_offset.x & 15), punt to gen_batchrop if > 0
      check (spr is mempr), punt to gen_batchrop if not.
      check REVERSE_VIDEO flag, punt to gen_batchrop if set.  XXX can we do this?
      check if width and height of pr is same as bw and bh, respectively.

    get shmem offset from gp1_alloc.  If unavailable, return mem_batchrop
    add offset to shmem ptr.

    load gpsi buffer with data:
      GP3_PR_ROP24_BATCH
      planegroup | rop
      xoff, yoff   ( defines the clip window )
      dx, dy       ( defines the start location for drawing the pixrects )
      dw, dh
      bw, bh
      color
      planemask from dpr
   
    save host registers

    <sync>

    download md_image of spr to off-screen VRAM for line 0 of each bitmap,
      then line 1 of each bitmap, etc... until bh.
      note, the data is swapped, see bit_swap below.

    load gpsi buffer with count
    reset host registers
    post buffer
    
                                                                              */
/*                                                                            */
/******************************************************************************/

cg12_batchrop(dpr, dx, dy, op, src, count)
     Pixrect *dpr;
     register int dx, dy;
     int op;
     register struct pr_prpos *src;
     register int count;

{

  register struct cg12_data *dprd = cg12_d(dpr);
  register int color = PIX_OPCOLOR(op);
  int rop = PIX_OP(op);
  register int dpr_plngrp;
  register int dw, dh;
  unsigned int planes_mask;
  register int xoff, yoff, result, dbl_status_ok;
  static int bit_swap[16] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
  extern int cg12_width, cg12_height;

  /*
   * Weed out the cases which are not handled by GP3_ROP24_BATCH
   */

  if (!count) 
    return 0;

  dpr_plngrp = PIX_ATTRGROUP(dprd->planes);

  if (dprd->windowfd >= 0)
    ioctl(dprd->windowfd, WINWIDGET, &dprd->wid_dbl_info);
  if (dprd->wid_dbl_info.dbl_fore == dprd->wid_dbl_info.dbl_back)
    dbl_status_ok = 1;
  else
    if (dprd->wid_dbl_info.dbl_read_state == dprd->wid_dbl_info.dbl_write_state)
      dbl_status_ok = 1;
    else
      if (dprd->wid_dbl_info.dbl_write_state == PR_DBL_NONE)
	dbl_status_ok = 1;
      else
	if (dprd->wid_dbl_info.dbl_write_state == PR_DBL_BOTH)
	  dbl_status_ok = 1;
	else
	  dbl_status_ok = 0;
  
  if (((dpr_plngrp != PIXPG_24BIT_COLOR) && (dpr_plngrp != PIXPG_8BIT_COLOR)) |
      !dbl_status_ok)
    {
	/* ### */
	Pixrect             dmempr;

	CG12_PR_TO_MEM(dpr, dmempr);
	return (mem_batchrop(dpr, dx, dy, op, src, count));
    } 

  
  /* get dw and dh of area on screen to fill with src bitmaps */

  dw = dpr->pr_width;
  dh = dpr->pr_height;

   /* translate dx, dy into screen coords */

  xoff = dprd->mprp.mpr.md_offset.x; 
  yoff = dprd->mprp.mpr.md_offset.y;
  dx += xoff;
  dy += yoff;
  
  /* clip dx+dw, dy+dh to screen coords, */
  /* assumes 0 <= xoff <= 1151 and 0 <= yoff <= 899 XXX correct? */

  if (!(op & PIX_DONTCLIP)) 
    {
      /* adjust clip window to screen if necessary */

      if (xoff > cg12_width)
	return 0;
      else 
	if (xoff < 0)
	  xoff = 0;

      if (yoff > cg12_height)
	return 0;
      else
	if (yoff < 0)
	  yoff = 0;

      if ((xoff + dw) > cg12_width)
	dw = cg12_width - xoff;

      if ((yoff + dh) > cg12_height)
	dh = cg12_height - yoff;


    }
  else
    {
      /* just clip to screen */
      xoff = 0;
      yoff = 0;
      dw = cg12_width;
      dh = cg12_height;
    }


  /* adjust color if 8bit emulation or 12bit double buffered */

  planes_mask = dprd->planes & 0xffffff;

  if (dpr_plngrp == PIXPG_8BIT_COLOR)
    {
      if (!color) 
	{
	  if (rop)
	    color = 0xFFFFFF;
        } else
	  { 
	    color &= 0xFF;
	    color = (color << 16) | (color << 8) | color;
	  }
	    
      planes_mask &= 0xFF;
      planes_mask = (planes_mask << 16) | (planes_mask << 8) | planes_mask;
    }
  else
    {
      /* determine if pixrect is double buffered.  shift color >> 4 if writing
	 to buffer B, see cg12_rop for more info */

      if (dprd->wid_dbl_info.dbl_write_state == PR_DBL_A ||
	  dprd->wid_dbl_info.dbl_write_state == PR_DBL_B)
	{
	  register int i;
	  i = ((dprd->wid_dbl_info.dbl_write_state == PR_DBL_A)
	       ? dprd->wid_dbl_info.dbl_fore
	       : dprd->wid_dbl_info.dbl_back);
	  if (i == PR_DBL_B)
	    {
	      if (!color)
		if (rop)
		  color = 0x0f0f0f;
	      else
		color = (color >> 4);
	      planes_mask = 0x0f0f0f;
	    }
	  else
	    {
	      if (!color)
		if (rop)
		  color = 0xf0f0f0;
	      
	      planes_mask = 0xf0f0f0;
	    }
	}
      else
	if (dprd->wid_dbl_info.dbl_write_state == PR_DBL_BOTH)
	  {
	    if (!color)
	      if (rop)
		color = 0xffffff;
	  }
    }

  { 

    register int   *shmem_ptr;
    register Pixrect *spr;
    register struct mpr_data *sprd;
    register unsigned short  *bitmap_ptr;
    register int   bh,bw,bitmap_data,bitmap_count,src_bitmap_data,i;
    int shmem_off;
    unsigned int bitvec,saved_haccess,saved_pln_wr_mask, saved_hpage, saved_hctrl;
    unsigned int new_bitmap_0, new_bitmap_1, bit,j,f_bits,new_4_bits;
    register struct pr_prpos *src_ptr;
    register int *os_vram_ptr = (int *)(dprd->mprp.mpr.md_image);
    extern int cg12_offscreen;

    /* initialize start of offscreen memory */
    /* used to be hard coded as:  ((1152 * 900) >> 3) + 40 */
    os_vram_ptr += cg12_offscreen;

    /* initialize loop information */
    src_ptr = src;
    spr = src->pr;
    bw  = spr->pr_width;
    bh  = spr->pr_height;

    /* first check each bitmap to see if it is legit. */

    for (bitmap_count = 0; bitmap_count < count; bitmap_count++) 
      {
	/* weed out cases not covered by cg12_batch */
	sprd = mpr_d(spr);
	if (MP_NOTMPR(spr) || spr->pr_depth != 1 || sprd->md_linebytes != 2 ||
	    sprd->md_offset.x != 0 || sprd->md_flags & MP_REVERSEVIDEO ||
	    spr->pr_width != bw || spr->pr_height != bh)
	  {
	    Pixrect             dmempr;
	    
	    dx -= dprd->mprp.mpr.md_offset.x; 
	    dy -= dprd->mprp.mpr.md_offset.y; 
	    CG12_PR_TO_MEM(dpr, dmempr);
	    return (mem_batchrop(dpr, dx, dy, op, src, count));
	  } 

	spr = (++src_ptr)->pr;
      }


    /*
     * 0x230 = flag set by kernel indicating we need to run gpconfig.
     * a really ugly situation that should never occur but if the gp
     * is totally hung, well, there's not a lot we can do.
     */
    if (dprd->gp_shmem[0x230])
	cg12_restart(dpr);

    /* 0x231 = flag indicating to turn off acceleration */
    if (dprd->gp_shmem[0x231] ||
	(shmem_off = gp1_alloc((caddr_t) dprd->gp_shmem, 1, &bitvec,
			       dprd->minordev, dprd->ioctl_fd)) == 0)
      {
	Pixrect             dmempr;
	
	dx -= dprd->mprp.mpr.md_offset.x; 
	dy -= dprd->mprp.mpr.md_offset.y; 
	CG12_PR_TO_MEM(dpr, dmempr);
	return (mem_batchrop(dpr, dx, dy, op, src, count));
      } 

    shmem_ptr = (int *)(dprd->gp_shmem);
    shmem_ptr += (shmem_off >> 1);

    *shmem_ptr++ = (GP3_PR_ROP24_BATCH << 16) | (dpr_plngrp << 8) | rop; 
    *shmem_ptr++ = (xoff << 16) | yoff;
    *shmem_ptr++ = (dx << 16) | dy;
    *shmem_ptr++ = (dw << 16) | dh;
    *shmem_ptr++ = (bw << 16) | bh;
    *shmem_ptr++ = color;
    *shmem_ptr++ = planes_mask;
    *shmem_ptr++ = count;

    /* save EIC HOST CONTROL and set to not do double buffering */
    /* do before other apu/dpu reg's */
    /* !!! */
    saved_hctrl = dprd->ctl_sp->eic.host_control;
    dprd->ctl_sp->eic.host_control &= ~(CG12_EIC_DBL);

    /* save HPAGE register and set to CG12_HPAGE_24BIT */
    saved_hpage = dprd->ctl_sp->apu.hpage;
    dprd->ctl_sp->apu.hpage =
	(cg12_width == 1280) ? CG12_HPAGE_24BIT_HR : CG12_HPAGE_24BIT;

    /* save HACCESS register and set to 4 bits/pixel */
    saved_haccess = dprd->ctl_sp->apu.haccess;
    dprd->ctl_sp->apu.haccess = CG12_OS_HACCESS;

    /* save PLN_WR_MASK_HOST and set to write to lower 4 bits of each byte */
    saved_pln_wr_mask = dprd->ctl_sp->dpu.pln_wr_msk_host;
    dprd->ctl_sp->dpu.pln_wr_msk_host = CG12_OS_PLN_WR_MASK;

    src_ptr = src;
    spr = src_ptr->pr;
    for (bitmap_count = 0; bitmap_count < count; bitmap_count++) 
      {
	/* load off.x and off.y into gpsi buffer */
	*shmem_ptr++ = (src_ptr->pos.x << 16) | (src_ptr->pos.y & 0xffff);
	spr = (++src_ptr)->pr;
      }

    /* sync gp, assumes that no other process will interrupt us */
    gp1_sync((caddr_t)dprd->gp_shmem, dprd->fd);

    /* download bitmap to VRAM */
    /* memory organization is:
       < bitmap 0 line 0> <bitmap 1 line 0> .... <bitmap n line 0>
       < bitmap 0 line 1> <bitmap 1 line 1> .... <bitmap n line 1>
       etc...       
       
       also note that each chunk of 4bits is swapped according to the
       bit_swap array.  this is done because the gpsi pr_batch routine
       expects the high order bit to correspond to the low order pixel,
       i.e. pixel 0 ==> bit 31 in an int with bits 0 - 31 */

    for (bitmap_data = 0; bitmap_data < bh; bitmap_data++) 
      {
	src_ptr = src;
	spr = src_ptr->pr;

	for (bitmap_count = 0; bitmap_count < (count >> 1); bitmap_count++) 
	  {
	    /* compute address of bitmap data */
	    sprd = mpr_d(spr);
	    bitmap_ptr = (unsigned short *)(sprd->md_image + sprd->md_offset.y
					    + bitmap_data);

	    new_bitmap_0 = 0;
	    src_bitmap_data = *bitmap_ptr;
	    new_bitmap_0 |= ((bit_swap[((src_bitmap_data) & 0xF)]));
	    new_bitmap_0 |= ((bit_swap[((src_bitmap_data >> 4) & 0xF)]) << 4);
	    new_bitmap_0 |= ((bit_swap[((src_bitmap_data >> 8) & 0xF)]) << 8);
	    new_bitmap_0 |= ((bit_swap[((src_bitmap_data >> 12) & 0xF)]) << 12);

	    spr = (++src_ptr)->pr;
	    sprd = mpr_d(spr);
	    bitmap_ptr = (unsigned short *)(sprd->md_image + sprd->md_offset.y
					    + bitmap_data);
	    new_bitmap_1 = 0;
	    src_bitmap_data = *bitmap_ptr;
	    new_bitmap_1 |= ((bit_swap[((src_bitmap_data) & 0xF)]));
	    new_bitmap_1 |= ((bit_swap[((src_bitmap_data >> 4) & 0xF)]) << 4);
	    new_bitmap_1 |= ((bit_swap[((src_bitmap_data >> 8) & 0xF)]) << 8);
	    new_bitmap_1 |= ((bit_swap[((src_bitmap_data >> 12) & 0xF)]) << 12);

	    *os_vram_ptr++ = (new_bitmap_0 << 16) | (new_bitmap_1);  
	    spr = (++src_ptr)->pr;
	    
	  }

	if (count & 0x1)
	  {
	    sprd = mpr_d(spr);
	    bitmap_ptr = (unsigned short *)(sprd->md_image + sprd->md_offset.y
					    + bitmap_data);

	    new_bitmap_0 = 0;
	    src_bitmap_data = *bitmap_ptr;
	    new_bitmap_0 |= ((bit_swap[((src_bitmap_data) & 0xF)]));
	    new_bitmap_0 |= ((bit_swap[((src_bitmap_data >> 4) & 0xF)]) << 4);
	    new_bitmap_0 |= ((bit_swap[((src_bitmap_data >> 8) & 0xF)]) << 8);
	    new_bitmap_0 |= ((bit_swap[((src_bitmap_data >> 12) & 0xF)]) << 12);

	    *os_vram_ptr++ = (new_bitmap_0 << 16);
	  } 
      }

    *shmem_ptr++ = ((GP1_EOCL | GP1_FREEBLKS) << 16) | (bitvec >> 16);
    *shmem_ptr++ = (bitvec << 16);

    /* reset host registers */

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


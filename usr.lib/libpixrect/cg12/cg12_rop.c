#ifndef lint
static char sccsid[] = "@(#)cg12_rop.c 1.1 94/10/31 SMI";
#endif
   
/*
 * Copyright 1989-1990, Sun Microsystems, Inc.
 */
  
#include <pixrect/pixrect.h>
#include <pixrect/gp1cmds.h>
#include <pixrect/cg12_var.h>
#include <pixrect/mem32_var.h>
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
/*  cg12_rop
      
      Basic outline:
      
      Clip:
      
      if !PIXDONTCLIP then
        clip src to dest
        reset dw and dh
      else
        clip src to screen.
      
      determine double buffer status

      if (dpr is cg12 and either spr is null or spr is cg12) then
        identify weird type of fill and adjust color, op, and spr if necessary
      
      get shmem offset from gp1_alloc.  If unavailable, return mem_rop result
      add offset to shmem ptr.
      
      if (spr is cg12) then
        put following info into gpsi buffer:
        rop <= from op argument
        planegroup <= from dpr's planegroup
        dx, dy <= adjusted dx dy from clipping, if clipped
        sx, sy <= adjusted sx sy from clipping, if clipped
        dw, dh <= adjusted dw dh from clipping, if clipped
        planemask (replicated if 8 bit)

      if (spr is NULL) then
        adjust color if 8bit or 1bit dest
        put following info into gpsi buffer:
        planegroup <= from dpr's planegroup
        rop <= from op argument
        dx, dy <= dx dy
        dw, dh <= adjusted dw dh from clipping
        color <= color from op argument 
	  if 8bit, then color is replicated
	  if 24bit, double buffered and writing to buffer B, then shift >> 4
        planemask (also replicated if 8 bit)

                                                                              */
/*                                                                            */
/******************************************************************************/
  
  
cg12_rop(dpr, dx, dy, dw, dh, op, spr, sx, sy)
     Pixrect *dpr;
     int dx, dy, dw, dh;
     int op;
     Pixrect *spr;
     int sx, sy;

{
  
  register int dpr_plngrp;
  register int color,clip,dbl_status_ok;
  register struct cg12_data *dprd = cg12_d(dpr),*sprd;
  struct mprp32_data dst_mem32_pr_data, src_mem32_pr_data;
  extern int cg12_width, cg12_height;
  
  /* if dest is cg12 and src is null or cg12 */
  
  dpr_plngrp = PIX_ATTRGROUP(dprd->planes);

  /* determine double buffer status of pixrects */
    
  /* check to see if dpr is gp1_rop before calling win ioctl */
  if (dpr->pr_ops->pro_rop == gp1_rop && (!spr || spr->pr_ops->pro_rop == gp1_rop))
    {
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

      /*  If the pixrect's double buffer status is OK, then continue, else punt */
      if (dbl_status_ok)
	{	  
  
	  register int rop = PIXOP_OP(op);

	  dpr_plngrp = PIX_ATTRGROUP(dprd->planes);

	  /* 
	   * Clip dest to src
	   */
	  if (clip = !(op & PIX_DONTCLIP)) 
	    {
	      register struct pr_subregion dst;
	      register struct pr_prpos src;
	  
	      dst.pr = dpr;
	      dst.pos.x = dx;
	      dst.pos.y = dy;
	      dst.size.x = dw;
	      dst.size.y = dh;
	      
	      if ((src.pr = spr) != 0) 
		{
		  src.pos.x = sx;
		  src.pos.y = sy;
		}
	      
	      (void) pr_clip(&dst, &src);
	      
	      dx = dst.pos.x;
	      dy = dst.pos.y;
	      dw = dst.size.x;
	      dh = dst.size.y;
	      
	      if (spr != 0) 
		{
		  sx = src.pos.x;
		  sy = src.pos.y;
		}
	    }
	  
	  if (dw <= 0 || dh <= 0 )
	    return 0;
	  
	  
	  if (PIXOP_NEEDS_SRC(rop)) 
	    {
	      register Pixrect *rspr = spr;
	      if (rspr) 
		{
		  /*
		   * identify weird type of fill: src is 1 x 1
		   */
		  if (rspr->pr_size.x == 1
		      && rspr->pr_size.y == 1) 
		    {
		      color =  pr_get(rspr, 0, 0);
		      if (rspr->pr_depth == 1 && color
			  && (color = PIX_OPCOLOR(op)) == 0)
			color = ~color;
		      spr = 0;
		    } 
		} else 
		  {
		    /*
		     * No src given; always
		     * use color from rop arg.
		     */
		    color = PIX_OPCOLOR(op);
		  }
	    } else
	      spr = 0;
	  
	  /* translate dx,dy to screen coords */
	  dx += dprd->mprp.mpr.md_offset.x;
	  dy += dprd->mprp.mpr.md_offset.y;
	  if (spr)
	    {
	      sprd = cg12_d(spr);
	      sx += sprd->mprp.mpr.md_offset.x;
	      sy += sprd->mprp.mpr.md_offset.y;
	    }
	  
	  
	  /* clip dx,dy dw,dh to screen coords if DONTCLIP bit set */
	  if (!clip) 
	    {
	      
	      if (dx > cg12_width) /* out of bounds in x */
		return 0;
	      else
		if (dx < 0)
		  dx = 0;
	      
	      if (dy > cg12_height) /* out of bounds in y */
		return 0;
	      else
		if (dy < 0)
		  dy = 0;
	      
	      if ((dx + dw) > cg12_width)
		dw = cg12_width - dx;
	      
	      if ((dy + dh) > cg12_height)
		dh = cg12_height - dy;
	      
	      /* adjust dw and dh if spr's width and height are smaller */
	      if (spr) 
		{
		  if (dw > spr->pr_size.x)
		    dw = spr->pr_size.x;
		  
		  if (dh > spr->pr_size.y)
		    dh = spr->pr_size.y;
		}
	    }
	  
	  
	  {
	    
	    /* build gpsi buffer and post it */
	    register int *shmem_ptr;
	    unsigned int bitvec,planes_mask,result;
	    int shmem_off;
	    
	    /*
	     * 0x230 = flag set by kernel indicating we need to run gpconfig. a
	     * really ugly situation that should never occur but if the gp is
	     * totally hung, well, there's not a lot we can do.
	     */
	    if (dprd->gp_shmem[0x230])
	      cg12_restart(dpr);
	    
	    /* 0x231 = flag indicating to turn off acceleration */
	    if (dprd->gp_shmem[0x231] ||
		(shmem_off = gp1_alloc((caddr_t) dprd->gp_shmem, 1, &bitvec,
				       dprd->minordev, dprd->ioctl_fd)) == 0)
	      {
		/* case is not handled by cg12 accelerator */
		Pixrect smempr, dmempr;
		
		CG12_PR_TO_MEM(dpr, dmempr);
		dx -= dprd->mprp.mpr.md_offset.x;
		dy -= dprd->mprp.mpr.md_offset.y;
		
		if (spr)
		  {
		    
		    sprd = cg12_d(spr);
		    CG12_PR_TO_MEM(spr, smempr);
		    
		    sx -= sprd->mprp.mpr.md_offset.x;
		    sy -= sprd->mprp.mpr.md_offset.y;
		    
		  }
		
		return cg12_no_accel_rop(dpr, dx, dy, dw, dh, op|PIX_DONTCLIP,
					 spr, sx, sy); 
	      }
	    
	    
	    shmem_ptr = (int *)(dprd->gp_shmem);
	    shmem_ptr += (shmem_off >> 1);
	    planes_mask = dprd->planes & 0x00FFFFFF;
	    result = planes_mask;
	    if (spr) 
	      {
		/* src is cg12, ignore color */
		
		switch (dpr_plngrp) 
		  {
		  case PIXPG_8BIT_COLOR :     
		    planes_mask &= 0xFF;
		    planes_mask = (planes_mask << 16) | (planes_mask << 8) | 
		      planes_mask;
		    break;
		  case PIXPG_OVERLAY :
		    planes_mask = CG12_PLN_WR_OVERLAY;
		    break;
		  case PIXPG_OVERLAY_ENABLE :
		    planes_mask = CG12_PLN_WR_ENABLE;
		    break;
		  case PIXPG_WID :
		    planes_mask = CG12_PLN_WR_WID;
		    break;
		    default :
		      break;
		  }
		
		*shmem_ptr++ = (GP3_PR_ROP24_FF << 16) | (dpr_plngrp << 8) | (rop <<1);
		*shmem_ptr++ = (dx << 16) | dy;
		*shmem_ptr++ = (dw << 16) | dh;
		*shmem_ptr++ = (sx << 16) | sy;
		*shmem_ptr++ = planes_mask;
	      } else
		{
		  /* src is null, convert color if 8bit or 1bit dest */
		  switch (dpr_plngrp) 
		    {
		    case PIXPG_8BIT_COLOR :     
		      color &= 0xFF;
		      color = (color << 16) | (color << 8) | color;
		      planes_mask &= 0xFF;
		      planes_mask = (planes_mask << 16) | (planes_mask << 8) | 
			planes_mask;
		      break;
		    case PIXPG_OVERLAY :
		      if (color > 0)
			color = 0x00FFFFFF; 
		      planes_mask =  CG12_PLN_WR_OVERLAY;
		      break;
		    case PIXPG_OVERLAY_ENABLE :
		      if (color > 0) 
			color = 0x00FFFFFF; 
		      planes_mask = CG12_PLN_WR_ENABLE;
		      break;
		    case PIXPG_WID :
		      color = (color & 0xFF) << 16;
		      planes_mask = CG12_PLN_WR_WID;
		      break;
		      default :
			break;
		    case PIXPG_24BIT_COLOR :
		      { 
			/* if 24bit pixrect and double buffered, then shift color
			   right by 4 for buffer B */
			
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
		      
		      break;
		    }		  
		  
		  *shmem_ptr++ = (GP3_PR_ROP24_NF << 16) | (dpr_plngrp << 8) | (rop << 1);
		  *shmem_ptr++ = (dx << 16) | dy;
		  *shmem_ptr++ = (dw << 16) | dh;
		  *shmem_ptr++ = color;
		  *shmem_ptr++ = planes_mask;
		}
	    
	    *shmem_ptr++ = ((GP1_EOCL | GP1_FREEBLKS) << 16) | (bitvec >> 16);
	    *shmem_ptr   = (bitvec << 16);
	    
	    result = gp1_post(dprd->gp_shmem, shmem_off, dprd->ioctl_fd);
	    gp1_sync((caddr_t)dprd->gp_shmem, dprd->fd);    
	    
	    return(result); 
	  }
	  
	} else
	  {
	    /* double buffer status NOT OK, so not handled by cg12 accelerator */
	    Pixrect smempr, dmempr;
	    
	    CG12_PR_TO_MEM(dpr, dmempr);
	    CG12_PR_TO_MEM(spr, smempr);
	    return cg12_no_accel_rop(dpr, dx, dy, dw, dh, op, spr, sx, sy);
	  }
    }  else
      {
	/* closes off the check for the rop ops */
	/* dest is not cg12 or else source is a non-cg12 pr */
	Pixrect smempr, dmempr;
	
	CG12_PR_TO_MEM(dpr, dmempr);
	CG12_PR_TO_MEM(spr, smempr);
	return cg12_no_accel_rop(dpr, dx, dy, dw, dh, op, spr, sx, sy);
      }
  }


/*
 * The following code is only necessary because the cg12 8-bit and 24-bit
 * frame buffers share the same vram.  The normal mem_rop code (properly)
 * handles left and right end non-32-bit-aligned entries by reading a 32-bit
 * int and leaving the unused portion unaffected, then writing back the
 * 32-bit int.  On the cg12, this has the unfortunate effect of converting
 * the unused portion to 8-bit values which is bad if the underlying pixels
 * were 24-bit values.
 *
 * we can only flow to this code if the operation could not be accelerated
 * and we will only use this code if the destination is 8-bit.  otherwise,
 * mem_rop will still be used.
 */

/*
 * Macros may make code harder to debug and maintain but are definitely
 * appropriate in some cases.
 *
 * The source pixrect may be null, or have depth 1, or 8.  The raster
 * operation will be one of 16 possibilities.  The following macros
 * simplify the generation of a potential 64 loops.  Obviously, code
 * space is being traded off for performance here.
 */

#define	NO_SRC

#define	SRC_OP	src_val = PIX_OPCOLOR(op);

#define	SRC_1								\
    src_val = (src[(wsx>>3) + src_offset] >> (7-(wsx%8))) & 1;		\
    if (src_data->mpr.md_flags & MP_REVERSEVIDEO)			\
	src_val = ~src_val & 1;						\
    src_val = src_val ? ((PIX_OPCOLOR(op)) ? PIX_OPCOLOR(op) : ~0) : 0;

#define	SRC_8	src_val = src[wsx + src_offset];

/*
 * If source is necessary for the raster-operation, then generate a loop
 * for each possible source depth.
 */

#define	NEED_SRC(OPER)							\
    if (spr)								\
    {									\
	switch (spr->pr_depth)						\
	{								\
	    case 1:							\
		ROP_LOOP(SRC_1, OPER);					\
		break;							\
									\
	    case 8:							\
		ROP_LOOP(SRC_8, OPER);					\
	}								\
    }									\
    else ROP_LOOP(SRC_OP, OPER)

#define	ROP_LOOP(SRC, OPER)						\
    for (; dst_line != end; src_line += incr, dst_line += incr)		\
    {									\
	src_offset = src_line * src_bytes;				\
	dst_offset = dst_line * dst_bytes;				\
									\
	if (j == 1)							\
	{								\
	    wsx = sx;							\
	    wdx = dx + dst_offset;					\
	}								\
	else								\
	{								\
	    wsx = sx + w;						\
	    wdx = dx + w + dst_offset;					\
	}								\
									\
	for (; wsx != i; wsx += j,wdx += j)				\
	{								\
	    SRC;							\
	    OPER;							\
	}								\
    }

cg12_no_accel_rop(dpr, dx, dy, w, h, op, spr, sx, sy)
Pixrect            *dpr, *spr;
int                 dx, dy, w, h, op, sx, sy;
{
    int                 i, j, incr, end;
    int                 wsx, src_offset, src_bytes, src_line;
    int                 wdx, dst_offset, dst_bytes, dst_line;
    unsigned char      *src, src_val, *dst;
    struct mprp_data   *src_data, *dst_data;

    /*
     * we only need this routine for 8-bit sharing memory with 24-bit.
     * if the destination is not 8-bit, then mem_rop can handle things
     * just fine.
     */
    
    if (dpr->pr_depth != 8)
	return pr_rop(dpr, dx, dy, w, h, op, spr, sx, sy);

    /* make sure all coordinates are within screen or region bounds */

    if (!(op & PIX_DONTCLIP))
    {
	struct pr_subregion d;
	struct pr_prpos     s;
	extern int          pr_clip();

	d.pr = dpr;
	d.pos.x = dx;
	d.pos.y = dy;
	d.size.x = w;
	d.size.y = h;

	s.pr = spr;
	if (spr)
	{
	    s.pos.x = sx;
	    s.pos.y = sy;
	}

	(void) pr_clip(&d, &s);

	dx = d.pos.x;
	dy = d.pos.y;
	w = d.size.x;
	h = d.size.y;

	if (spr)
	{
	    sx = s.pos.x;
	    sy = s.pos.y;
	}
    }

    if (w <= 0 || h <= 0)
	return 0;

    /* translate the origin */

    dx += mprp_d(dpr)->mpr.md_offset.x;
    dy += mprp_d(dpr)->mpr.md_offset.y;

    if (spr)
    {
	sx += mprp_d(spr)->mpr.md_offset.x;
	sy += mprp_d(spr)->mpr.md_offset.y;
    }

    --w;

    /*
     * Because of overlapping copies, must decide bottom up or top down. If
     * null source, then initialize the first destination line as the source
     * and start the copy from the second line.
     */

    if ((sy < dy) && (spr && spr->pr_ops == dpr->pr_ops))
    {
	incr = -1;
	end = dy - 1;
	if (spr)
	{
	    dst_line = dy + h - 1;
	    src_line = sy + h - 1;
	}
	else
	{
	    dst_line = dy + h - 2;
	    sx = dx;
	    src_line = dy + h - 1;
	}
    }
    else
    {
	incr = 1;
	end = dy + h;
	if (spr)
	{
	    dst_line = dy;
	    src_line = sy;
	}
	else
	{
	    dst_line = dy + 1;
	    sx = dx;
	    src_line = dy;
	}
    }

    /* data structure and destination memory address */

    dst_data = mprp_d(dpr);
    dst = (unsigned char *) dst_data->mpr.md_image;
    dst_bytes = dst_data->mpr.md_linebytes;

    if (spr)
    {
	src_data = mprp_d(spr);
	src = (unsigned char *) src_data->mpr.md_image;
	src_bytes = src_data->mpr.md_linebytes;
    }
    else
    {
	src_bytes = 0;
	dst_line -= incr;		/* didn't seed for bitblt */
    }

    if (sx > dx)			/* forward */
    {
	i = sx + w + 1;
	j = 1;
    }
    else
    {
	i = sx - 1;
	j = -1;
    }

    switch (PIX_OP(op) >> 1)
    {
	case 0:			/* CLR */
	    ROP_LOOP(NO_SRC, dst[wdx] = 0);
	    break;

	case 1:			/* ~s&~d or ~(s|d) */
	    NEED_SRC(dst[wdx] = ~(src_val | dst[wdx]));
	    break;

	case 2:			/* ~s&d */
	    NEED_SRC(dst[wdx] = ~src_val & dst[wdx]);
	    break;

	case 3:			/* ~s */
	    NEED_SRC(dst[wdx] = ~src_val);
	    break;

	case 4:			/* s&~d */
	    NEED_SRC(dst[wdx] = src_val & ~dst[wdx]);
	    break;

	case 5:			/* ~d */
	    ROP_LOOP(NO_SRC, dst[wdx] = ~dst[wdx]);
	    break;

	case 6:			/* s^d */
	    NEED_SRC(dst[wdx] = src_val ^ dst[wdx]);
	    break;

	case 7:			/* ~s|~d or ~(s&d) */
	    NEED_SRC(dst[wdx] = ~(src_val & dst[wdx]));
	    break;

	case 8:			/* s&d */
	    NEED_SRC(dst[wdx] = src_val & dst[wdx]);
	    break;

	case 9:			/* ~(s^d) */
	    NEED_SRC(dst[wdx] = ~(src_val ^ dst[wdx]));
	    break;

	case 10:			/* DST */
	    break;

	case 11:			/* ~s|d */
	    NEED_SRC(dst[wdx] = ~src_val | dst[wdx]);
	    break;

	case 12:			/* SRC */
	    NEED_SRC(dst[wdx] = src_val);
	    break;

	case 13:			/* s|~d */
	    NEED_SRC(dst[wdx] = src_val | ~dst[wdx]);
	    break;

	case 14:			/* s|d */
	    NEED_SRC(dst[wdx] = src_val | dst[wdx]);
	    break;

	case 15:			/* SET */
	    ROP_LOOP(NO_SRC, dst[wdx] = PIX_ALL_PLANES);
	    break;
    }
    return 0;
}

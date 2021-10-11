#ifndef lint
static char sccsid[] = "@(#)mem_rop_v2.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Memory pixrect raster op
 */

#ifdef KERNEL
#include "../h/types.h"
#include "../pixrect/pixrect.h"
#include "../pixrect/pr_util.h"
#include "../pixrect/memreg.h"
#include "../pixrect/memvar.h"
#include "../pixrect/newmacs.h"
#include "../pixrect/newmacs1.h"
#else
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>
#ifndef lint
#include "newmacs1.h"		/* <> */
#include "newmacs.h"		/* <> */
#else
#include "mem20lmacs.h"		/* <> */
#include "newlmacs.h"		/* <> */
#endif
#endif
int   mem_needinit;
char  pr_reversedst[];
char  pr_reversesrc[];

/*
* Memory pixrect raster ops in C. Program is long because
* loops are unwrapped. The basic idea is to decide which case
* is being used and then go into an inner loop with
* no conditional branches. 
*
* The order of decisions is constsrc, alignment, direction, which skew greater,
* lenth component,op. By the time these decisions are made, the loop which
* has no conditionals and the minimum amount of data movement is
* selected.
*
* The following "hacks" are used: 1) while (--i != -1) is
* used to get the compiler to generate a dbra instruction,
* 2) a pointer to longwords is used whenever possible to
* optimize for the 68020 instead of the 68010, 3) the
* register variables dstincr and srcincr are occasionally
* used to hold color data.
*/
mem_rop (dst, op, src)
struct pr_subregion dst;
struct pr_prpos src;
{
  register  u_short * dstp, *srcp;/* ptrs to src and dst */
  register  u_int * tsrc, *tdst;/* longw ptrs to src and dst */
  register int  cdist2, cdist;	/*  offsets for skewing */
  register int  dtemp;		/*  scratch register */
  register short  i, count;	/*  x,y loop control */
  register short  dstincr;	/*  words per line */
  register  srcincr;		/*  words per line */
  short   sixtf, endf;		/* word config of rop */
  u_short smr, sml;		/*  masks for end */
  u_short dml, dmr;
  int   d4_dir;			/*  up/down, left/right indicator */
  struct mpr_data *a3_dprd, *a2_sprd;
  short   sskew, dskew, dsize;
  short   width, ocount, constsrc, sflag;
  int   originalop = op;
  int   color, dist;
  short   scolor;
  int   optemp;
  short   ct2;

  if (mem_needinit)		/* initialize raster op chip if necessary
				   (probably should be removed) */
    mem_init ();
  if (MP_NOTMPR (dst.pr))	/* abort  and call approriate rop if pixrect is
				   not mem */
  {
    return ((*dst.pr -> pr_ops -> pro_rop) (dst, op, src));
  }
  if (src.pr && (dst.pr -> pr_depth != src.pr -> pr_depth))
  /* return if pixrect depths don't match */
    return (-1);
  if (op & PIX_DONTCLIP)
  /* get rid of don't clip bit for case statements */
    op &= ~PIX_DONTCLIP;
  else
  {				/* clip */
    if (src.pr)
    {				/* clip source if nec. */
      pr_clip1 (&dst, &dst.size, &src);
      pr_clip1 (&src, &dst.size, &dst);
    }
    else
      pr_clip2 (&dst, &dst.size);
    if (dst.size.x <= 0 || dst.size.y <= 0)
      return (0);
  }
  color = PIX_OPCOLOR (op);	/* extract color from op */
  op = op & 0x1F;
  if ((u_int) op > PIX_SET)	/* return if op is invalid */
    return (-1);
  a3_dprd = mpr_d (dst.pr);
  if (a3_dprd -> md_flags & MP_REVERSEVIDEO)
    op = (pr_reversedst[op >> 1] << 1);
  if (PIXOP_NEEDS_SRC (op) && src.pr)
  {
    sflag = 1;
    if (MP_NOTMPR (src.pr))
    {
      return ((*src.pr -> pr_ops -> pro_rop) (dst, originalop, src));
    }
    a2_sprd = mpr_d (src.pr);
  }
  else
  {
    sflag = 0;
    src.pr = 0;
    a2_sprd = a3_dprd;		/* dummy value */
  }
  i = dst.size.y - 1;
  if (i < 0)
    return (0);
  if (src.pr == 0 || a2_sprd -> md_image != a3_dprd -> md_image)
    d4_dir = ROP_UP | ROP_LEFT;
  else
  {
    d4_dir = rop_direction (src.pos, a2_sprd -> md_offset,
	dst.pos, a3_dprd -> md_offset);
    if (rop_isdown (d4_dir))
    {
      src.pos.y += i;
      dst.pos.y += i;
    }
    if (rop_isright (d4_dir))
    {
      src.pos.x += dst.size.x;
      dst.pos.x += dst.size.x;
    }
  }
  if (dst.pr -> pr_depth > 1)
  {				/* handle more bits per pixel */
    if (sflag)
    {
      if (a2_sprd -> md_flags & MP_REVERSEVIDEO)
	op = (pr_reversesrc[op >> 1] << 1);
      srcp = (u_short *) mprd8_addr (a2_sprd, src.pos.x, src.pos.y,
	  src.pr -> pr_depth);
      constsrc = 0;
    }
    else
    {
      if (dst.pr -> pr_depth <= 8)
	color |= color << 8;
      color |= color << 16;
      srcp = (u_short *) (&color);
      constsrc = 1;
    }
    dstp = (u_short *) mprd8_addr (a3_dprd, dst.pos.x, dst.pos.y,
	dst.pr -> pr_depth);
    dsize = pr_product (dst.size.x, dst.pr -> pr_depth);
    if (rop_isright (d4_dir))
    {
      (char *) srcp += dst.pr -> pr_depth / 8 - 1;
      sskew = ((int) srcp & 1) ? 16 : 8;
      srcp = ((u_short *) ((int) srcp & ~1)) + 1;
      (char *) dstp += dst.pr -> pr_depth / 8 - 1;
      dskew = ((int) dstp & 1) ? 16 : 8;
      dstp = ((u_short *) ((int) dstp & ~1)) + 1;
    }
    else
    {
      sskew = ((int) srcp & 1) ? 8 : 0;
      srcp = (u_short *) ((int) srcp & ~1);
      dskew = ((int) dstp & 1) ? 8 : 0;
      dstp = (u_short *) ((int) dstp & ~1);
    }
  }
  else
  {
    color = (color) ? -1 : 0;
    if (sflag)
    {
      constsrc = 0;
      if (a2_sprd -> md_flags & MP_REVERSEVIDEO)
	op = (pr_reversesrc[op >> 1] << 1);
      srcp = (u_short *) mprd_addr (a2_sprd, src.pos.x, src.pos.y);
      sskew = mprd_skew (a2_sprd, src.pos.x, src.pos.y);
    }
    else
    {
      if (op == PIX_SET)
	color = -1;
      srcp = (u_short *) (&color);
      constsrc = 1;
    }
    dstp = (u_short *) mprd_addr (a3_dprd, dst.pos.x, dst.pos.y);
    dskew = mprd_skew (a3_dprd, dst.pos.x, dst.pos.y);
    width = dsize = dst.size.x;
  }
  if (constsrc)
  {				/* src can be ignored */
    dtemp = color;
    dstincr = a3_dprd -> md_linebytes >> 1;
    if ((dsize + dskew) >> 4)	/* is rop wider than one word? */
    {				/*  rop is greater than one word */
      if (dskew)
      {				/* only compute mask if necessesary */
	sml = (u_short) (0x0000ffff >> dskew);
	dsize -= (16 - dskew);
      }
      else
      {
	sml = 0;
      }
      dml = ~sml;
      smr = (u_short) (0xffff0000 >> (((dskew + width)) & 15));
      dmr = ~smr;
      sixtf = (dsize & 16) != 0;
      endf = (dsize & 15) != 0;
      count = cdist = (dsize >> 5) - 1;
    }
    else
    {				/* little rop */
      count = -1;
      endf = sixtf = 0;
      sml = (u_short) (0x0000ffff >> dskew);
      sml &= (u_short) (0xffff0000 >> (((dskew + width)) & 15));
      dml = ~sml;
    }
    if (count > -1)
    {				/* are there long words */
      ct2 = ((count + 1) << 1);
      if (sixtf)
      {				/*  short words? */
	if (sml)
	{
	  cdist2 = dtemp & sml;
	  ct2 += 2;
	  dstincr -= ct2;
	  if (endf)
	  {			/* all four */
	    dist = dtemp & smr;
	    switch (op)
	    {
	      case PIX_CLR: 
		do
		{
		  LR_X_CL_B ();
		  LR_X_CL_LG ();
		  LR_X_CL_W ();
		  LR_X_CL_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC | PIX_DST): 
		do
		{
		  LR_X_NB_SR_O_DS_B ();
		  LR_X_NB_SR_O_DS_LG ();
		  LR_X_NB_SR_O_DS_W ();
		  LR_X_NB_SR_O_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		do
		{
		  LR_X_N_SR_A_DS_B ();
		  LR_X_N_SR_A_DS_LG ();
		  LR_X_N_SR_A_DS_W ();
		  LR_X_N_SR_A_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC): 
		do
		{
		  LR_X_N_SR_B ();
		  LR_X_N_SR_LG ();
		  LR_X_N_SR_W ();
		  LR_X_N_SR_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_A_N_DS_B ();
		  LR_X_SR_A_N_DS_LG ();
		  LR_X_SR_A_N_DS_W ();
		  LR_X_SR_A_N_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_DST): 
		do
		{
		  LR_X_N_DS_B ();
		  LR_X_N_DS_LG ();
		  LR_X_N_DS_W ();
		  LR_X_N_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC ^ PIX_DST: 
		do
		{
		  LR_X_SR_X_DS_B ();
		  LR_X_SR_X_DS_LG ();
		  LR_X_SR_X_DS_W ();
		  LR_X_SR_X_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC & PIX_DST)): 
		do
		{
		  LR_X_NB_SR_A_DS_B ();
		  LR_X_NB_SR_A_DS_LG ();
		  LR_X_NB_SR_A_DS_W ();
		  LR_X_NB_SR_A_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & PIX_DST: 
		do
		{
		  LR_X_SR_A_DS_B ();
		  LR_X_SR_A_DS_LG ();
		  LR_X_SR_A_DS_W ();
		  LR_X_SR_A_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		do
		{
		  LR_X_N_SR_X_DS_B ();
		  LR_X_N_SR_X_DS_LG ();
		  LR_X_N_SR_X_DS_W ();
		  LR_X_N_SR_X_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_DST: 
		return;
	      case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		do
		{
		  LR_X_N_SR_O_DS_B ();
		  LR_X_N_SR_O_DS_LG ();
		  LR_X_N_SR_O_DS_W ();
		  LR_X_N_SR_O_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC: 
		do
		{
		  LR_X_SR_B ();
		  LR_X_SR_LG ();
		  LR_X_SR_W ();
		  LR_X_SR_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_O_N_DS_B ();
		  LR_X_SR_O_N_DS_LG ();
		  LR_X_SR_O_N_DS_W ();
		  LR_X_SR_O_N_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | PIX_DST: 
		do
		{
		  LR_X_SR_O_DS_B ();
		  LR_X_SR_O_DS_LG ();
		  LR_X_SR_O_DS_W ();
		  LR_X_SR_O_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SET: 
		do
		{
		  LR_X_SE_B ();
		  LR_X_SE_LG ();
		  LR_X_SE_W ();
		  LR_X_SE_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	    }
	  }
	  else
	  {			/* begin, long, and short */
	    switch (op)
	    {
	      case PIX_CLR: 
		do
		{
		  LR_X_CL_B ();
		  LR_X_CL_LG ();
		  LR_X_CL_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC | PIX_DST): 
		do
		{
		  LR_X_NB_SR_O_DS_B ();
		  LR_X_NB_SR_O_DS_LG ();
		  LR_X_NB_SR_O_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		do
		{
		  LR_X_N_SR_A_DS_B ();
		  LR_X_N_SR_A_DS_LG ();
		  LR_X_N_SR_A_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC): 
		do
		{
		  LR_X_N_SR_B ();
		  LR_X_N_SR_LG ();
		  LR_X_N_SR_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_A_N_DS_B ();
		  LR_X_SR_A_N_DS_LG ();
		  LR_X_SR_A_N_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_DST): 
		do
		{
		  LR_X_N_DS_B ();
		  LR_X_N_DS_LG ();
		  LR_X_N_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC ^ PIX_DST: 
		do
		{
		  LR_X_SR_X_DS_B ();
		  LR_X_SR_X_DS_LG ();
		  LR_X_SR_X_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC & PIX_DST)): 
		do
		{
		  LR_X_NB_SR_A_DS_B ();
		  LR_X_NB_SR_A_DS_LG ();
		  LR_X_NB_SR_A_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & PIX_DST: 
		do
		{
		  LR_X_SR_A_DS_B ();
		  LR_X_SR_A_DS_LG ();
		  LR_X_SR_A_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		do
		{
		  LR_X_N_SR_X_DS_B ();
		  LR_X_N_SR_X_DS_LG ();
		  LR_X_N_SR_X_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_DST: 
		return;
	      case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		do
		{
		  LR_X_N_SR_O_DS_B ();
		  LR_X_N_SR_O_DS_LG ();
		  LR_X_N_SR_O_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC: 
		do
		{
		  LR_X_SR_B ();
		  LR_X_SR_LG ();
		  LR_X_SR_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_O_N_DS_B ();
		  LR_X_SR_O_N_DS_LG ();
		  LR_X_SR_O_N_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | PIX_DST: 
		do
		{
		  LR_X_SR_O_DS_B ();
		  LR_X_SR_O_DS_LG ();
		  LR_X_SR_O_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SET: 
		do
		{
		  LR_X_SE_B ();
		  LR_X_SE_LG ();
		  LR_X_SE_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	    }
	  }
	}
	else
	{			/* no begin piece */
	  ct2++;
	  dstincr -= ct2;
	  if (endf)
	  {			/* long, short, end */
	    dist = dtemp & smr;
	    switch (op)
	    {
	      case PIX_CLR: 
		do
		{
		  LR_X_CL_LG ();
		  LR_X_CL_W ();
		  LR_X_CL_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC | PIX_DST): 
		do
		{
		  LR_X_NB_SR_O_DS_LG ();
		  LR_X_NB_SR_O_DS_W ();
		  LR_X_NB_SR_O_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		do
		{
		  LR_X_N_SR_A_DS_LG ();
		  LR_X_N_SR_A_DS_W ();
		  LR_X_N_SR_A_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC): 
		do
		{
		  LR_X_N_SR_LG ();
		  LR_X_N_SR_W ();
		  LR_X_N_SR_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_A_N_DS_LG ();
		  LR_X_SR_A_N_DS_W ();
		  LR_X_SR_A_N_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_DST): 
		do
		{
		  LR_X_N_DS_LG ();
		  LR_X_N_DS_W ();
		  LR_X_N_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC ^ PIX_DST: 
		do
		{
		  LR_X_SR_X_DS_LG ();
		  LR_X_SR_X_DS_W ();
		  LR_X_SR_X_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC & PIX_DST)): 
		do
		{
		  LR_X_NB_SR_A_DS_LG ();
		  LR_X_NB_SR_A_DS_W ();
		  LR_X_NB_SR_A_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & PIX_DST: 
		do
		{
		  LR_X_SR_A_DS_LG ();
		  LR_X_SR_A_DS_W ();
		  LR_X_SR_A_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		do
		{
		  LR_X_N_SR_X_DS_LG ();
		  LR_X_N_SR_X_DS_W ();
		  LR_X_N_SR_X_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_DST: 
		return;
	      case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		do
		{
		  LR_X_N_SR_O_DS_LG ();
		  LR_X_N_SR_O_DS_W ();
		  LR_X_N_SR_O_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC: 
		do
		{
		  LR_X_SR_LG ();
		  LR_X_SR_W ();
		  LR_X_SR_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_O_N_DS_LG ();
		  LR_X_SR_O_N_DS_W ();
		  LR_X_SR_O_N_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | PIX_DST: 
		do
		{
		  LR_X_SR_O_DS_LG ();
		  LR_X_SR_O_DS_W ();
		  LR_X_SR_O_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SET: 
		do
		{
		  LR_X_SE_LG ();
		  LR_X_SE_W ();
		  LR_X_SE_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	    }
	  }
	  else
	  {			/* long, short only */
	    switch (op)
	    {
	      case PIX_CLR: 
		do
		{
		  LR_X_CL_LG ();
		  LR_X_CL_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC | PIX_DST): 
		do
		{
		  LR_X_NB_SR_O_DS_LG ();
		  LR_X_NB_SR_O_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		do
		{
		  LR_X_N_SR_A_DS_LG ();
		  LR_X_N_SR_A_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC): 
		do
		{
		  LR_X_N_SR_LG ();
		  LR_X_N_SR_W ();
		  BUMP_DST ();
		} while (--i != -1);
		do
		{
		  LR_X_N_SR_LG ();
		  LR_X_N_SR_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_A_N_DS_LG ();
		  LR_X_SR_A_N_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		do
		{
		  LR_X_SR_A_N_DS_LG ();
		  LR_X_SR_A_N_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_DST): 
		do
		{
		  LR_X_N_DS_LG ();
		  LR_X_N_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		do
		{
		  LR_X_N_DS_LG ();
		  LR_X_N_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC ^ PIX_DST: 
		do
		{
		  LR_X_SR_X_DS_LG ();
		  LR_X_SR_X_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		do
		{
		  LR_X_SR_X_DS_LG ();
		  LR_X_SR_X_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC & PIX_DST)): 
		do
		{
		  LR_X_NB_SR_A_DS_LG ();
		  LR_X_NB_SR_A_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & PIX_DST: 
		do
		{
		  LR_X_SR_A_DS_LG ();
		  LR_X_SR_A_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		do
		{
		  LR_X_N_SR_X_DS_LG ();
		  LR_X_N_SR_X_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_DST: 
		return;
	      case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		do
		{
		  LR_X_N_SR_O_DS_LG ();
		  LR_X_N_SR_O_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC: 
		do
		{
		  LR_X_SR_LG ();
		  LR_X_SR_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_O_N_DS_LG ();
		  LR_X_SR_O_N_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | PIX_DST: 
		do
		{
		  LR_X_SR_O_DS_LG ();
		  LR_X_SR_O_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SET: 
		do
		{
		  LR_X_SE_LG ();
		  LR_X_SE_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	    }
	  }
	}
      }
      else
      {				/*  long only */
	if (sml)
	{
	  cdist2 = dtemp & sml;
	  ct2++;
	  dstincr -= ct2;
	  if (endf)
	  {			/* begin, long and end */
	    dist = dtemp & smr;
	    switch (op)
	    {
	      case PIX_CLR: 
		do
		{
		  LR_X_CL_B ();
		  LR_X_CL_LG ();
		  LR_X_CL_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC | PIX_DST): 
		do
		{
		  LR_X_NB_SR_O_DS_B ();
		  LR_X_NB_SR_O_DS_LG ();
		  LR_X_NB_SR_O_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		do
		{
		  LR_X_N_SR_A_DS_B ();
		  LR_X_N_SR_A_DS_LG ();
		  LR_X_N_SR_A_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC): 
		do
		{
		  LR_X_N_SR_B ();
		  LR_X_N_SR_LG ();
		  LR_X_N_SR_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_A_N_DS_B ();
		  LR_X_SR_A_N_DS_LG ();
		  LR_X_SR_A_N_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_DST): 
		do
		{
		  LR_X_N_DS_B ();
		  LR_X_N_DS_LG ();
		  LR_X_N_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC ^ PIX_DST: 
		do
		{
		  LR_X_SR_X_DS_B ();
		  LR_X_SR_X_DS_LG ();
		  LR_X_SR_X_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC & PIX_DST)): 
		do
		{
		  LR_X_NB_SR_A_DS_B ();
		  LR_X_NB_SR_A_DS_LG ();
		  LR_X_NB_SR_A_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & PIX_DST: 
		do
		{
		  LR_X_SR_A_DS_B ();
		  LR_X_SR_A_DS_LG ();
		  LR_X_SR_A_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		do
		{
		  LR_X_N_SR_X_DS_B ();
		  LR_X_N_SR_X_DS_LG ();
		  LR_X_N_SR_X_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_DST: 
		return;
	      case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		do
		{
		  LR_X_N_SR_O_DS_B ();
		  LR_X_N_SR_O_DS_LG ();
		  LR_X_N_SR_O_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC: 
		do
		{
		  LR_X_SR_B ();
		  LR_X_SR_LG ();
		  LR_X_SR_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_O_N_DS_B ();
		  LR_X_SR_O_N_DS_LG ();
		  LR_X_SR_O_N_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | PIX_DST: 
		do
		{
		  LR_X_SR_O_DS_B ();
		  LR_X_SR_O_DS_LG ();
		  LR_X_SR_O_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SET: 
		do
		{
		  LR_X_SE_B ();
		  LR_X_SE_LG ();
		  LR_X_SE_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	    }
	  }
	  else
	  {			/* begin  and long */
	    switch (op)
	    {
	      case PIX_CLR: 
		do
		{
		  LR_X_CL_B ();
		  LR_X_CL_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC | PIX_DST): 
		do
		{
		  LR_X_NB_SR_O_DS_B ();
		  LR_X_NB_SR_O_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		do
		{
		  LR_X_N_SR_A_DS_B ();
		  LR_X_N_SR_A_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC): 
		do
		{
		  LR_X_N_SR_B ();
		  LR_X_N_SR_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_A_N_DS_B ();
		  LR_X_SR_A_N_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_DST): 
		do
		{
		  LR_X_N_DS_B ();
		  LR_X_N_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC ^ PIX_DST: 
		do
		{
		  LR_X_SR_X_DS_B ();
		  LR_X_SR_X_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC & PIX_DST)): 
		do
		{
		  LR_X_NB_SR_A_DS_B ();
		  LR_X_NB_SR_A_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & PIX_DST: 
		do
		{
		  LR_X_SR_A_DS_B ();
		  LR_X_SR_A_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		do
		{
		  LR_X_N_SR_X_DS_B ();
		  LR_X_N_SR_X_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_DST: 
		return;
	      case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		do
		{
		  LR_X_N_SR_O_DS_B ();
		  LR_X_N_SR_O_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC: 
		do
		{
		  LR_X_SR_B ();
		  LR_X_SR_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_O_N_DS_B ();
		  LR_X_SR_O_N_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | PIX_DST: 
		do
		{
		  LR_X_SR_O_DS_B ();
		  LR_X_SR_O_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SET: 
		do
		{
		  LR_X_SE_B ();
		  LR_X_SE_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	    }
	  }
	}
	else
	{			/* long, only */
	  dstincr -= ct2;
	  if (endf)
	  {			/* long,  end */
	    dist = dtemp & smr;
	    switch (op)
	    {
	      case PIX_CLR: 
		do
		{
		  LR_X_CL_LG ();
		  LR_X_CL_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC | PIX_DST): 
		do
		{
		  LR_X_NB_SR_O_DS_LG ();
		  LR_X_NB_SR_O_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		do
		{
		  LR_X_N_SR_A_DS_LG ();
		  LR_X_N_SR_A_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC): 
		do
		{
		  LR_X_N_SR_LG ();
		  LR_X_N_SR_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_A_N_DS_LG ();
		  LR_X_SR_A_N_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_DST): 
		do
		{
		  LR_X_N_DS_LG ();
		  LR_X_N_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC ^ PIX_DST: 
		do
		{
		  LR_X_SR_X_DS_LG ();
		  LR_X_SR_X_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC & PIX_DST)): 
		do
		{
		  LR_X_NB_SR_A_DS_LG ();
		  LR_X_NB_SR_A_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & PIX_DST: 
		do
		{
		  LR_X_SR_A_DS_LG ();
		  LR_X_SR_A_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		do
		{
		  LR_X_N_SR_X_DS_LG ();
		  LR_X_N_SR_X_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_DST: 
		return;
	      case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		do
		{
		  LR_X_N_SR_O_DS_LG ();
		  LR_X_N_SR_O_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC: 
		do
		{
		  LR_X_SR_LG ();
		  LR_X_SR_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_O_N_DS_LG ();
		  LR_X_SR_O_N_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | PIX_DST: 
		do
		{
		  LR_X_SR_O_DS_LG ();
		  LR_X_SR_O_DS_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SET: 
		do
		{
		  LR_X_SE_LG ();
		  LR_X_SE_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	    }
	  }
	  else
	  {			/* long only */
	    switch (op)
	    {
	      case PIX_CLR: 
		do
		{
		  LR_X_CL_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC | PIX_DST): 
		do
		{
		  LR_X_NB_SR_O_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		do
		{
		  LR_X_N_SR_A_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC): 
		do
		{
		  LR_X_N_SR_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_A_N_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_DST): 
		do
		{
		  LR_X_N_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC ^ PIX_DST: 
		do
		{
		  LR_X_SR_X_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC & PIX_DST)): 
		do
		{
		  LR_X_NB_SR_A_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & PIX_DST: 
		do
		{
		  LR_X_SR_A_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		do
		{
		  LR_X_N_SR_X_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_DST: 
		return;
	      case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		do
		{
		  LR_X_N_SR_O_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC: 
		do
		{
		  LR_X_SR_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_O_N_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | PIX_DST: 
		do
		{
		  LR_X_SR_O_DS_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SET: 
		do
		{
		  LR_X_SE_LG ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	    }
	  }
	}
      }
    }
    else
    {				/* no long words */
      if (sixtf)
      {				/*  short words? */
	if (sml)
	{
	  cdist2 = dtemp & sml;
	  dstincr -= 2;
	  if (endf)
	  {			/* all three */
	    cdist = dtemp & smr;
	    switch (op)
	    {
	      case PIX_CLR: 
		do
		{
		  LR_X_CL_B ();
		  LR_X_CL_W ();
		  LR_X_CL_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC | PIX_DST): 
		do
		{
		  LR_X_NB_SR_O_DS_B ();
		  LR_X_NB_SR_O_DS_W ();
		  LR_X_NB_SR_O_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		do
		{
		  LR_X_N_SR_A_DS_B ();
		  LR_X_N_SR_A_DS_W ();
		  LR_X_N_SR_A_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC): 
		do
		{
		  LR_X_N_SR_B ();
		  LR_X_N_SR_W ();
		  LR_X_N_SR_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_A_N_DS_B ();
		  LR_X_SR_A_N_DS_W ();
		  LR_X_SR_A_N_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_DST): 
		do
		{
		  LR_X_N_DS_B ();
		  LR_X_N_DS_W ();
		  LR_X_N_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC ^ PIX_DST: 
		do
		{
		  LR_X_SR_X_DS_B ();
		  LR_X_SR_X_DS_W ();
		  LR_X_SR_X_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC & PIX_DST)): 
		do
		{
		  LR_X_NB_SR_A_DS_B ();
		  LR_X_NB_SR_A_DS_W ();
		  LR_X_NB_SR_A_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & PIX_DST: 
		do
		{
		  LR_X_SR_A_DS_B ();
		  LR_X_SR_A_DS_W ();
		  LR_X_SR_A_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		do
		{
		  LR_X_N_SR_X_DS_B ();
		  LR_X_N_SR_X_DS_W ();
		  LR_X_N_SR_X_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_DST: 
		return;
	      case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		do
		{
		  LR_X_N_SR_O_DS_B ();
		  LR_X_N_SR_O_DS_W ();
		  LR_X_N_SR_O_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC: 
		do
		{
		  LR_X_SR_B ();
		  LR_X_SR_W ();
		  LR_X_SR_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_O_N_DS_B ();
		  LR_X_SR_O_N_DS_W ();
		  LR_X_SR_O_N_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | PIX_DST: 
		do
		{
		  LR_X_SR_O_DS_B ();
		  LR_X_SR_O_DS_W ();
		  LR_X_SR_O_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SET: 
		do
		{
		  LR_X_SE_B ();
		  LR_X_SE_W ();
		  LR_X_SE_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	    }
	  }
	  else
	  {			/* begin and short */
	    switch (op)
	    {
	      case PIX_CLR: 
		do
		{
		  LR_X_CL_B ();
		  LR_X_CL_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC | PIX_DST): 
		do
		{
		  LR_X_NB_SR_O_DS_B ();
		  LR_X_NB_SR_O_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		do
		{
		  LR_X_N_SR_A_DS_B ();
		  LR_X_N_SR_A_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC): 
		do
		{
		  LR_X_N_SR_B ();
		  LR_X_N_SR_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_A_N_DS_B ();
		  LR_X_SR_A_N_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_DST): 
		do
		{
		  LR_X_N_DS_B ();
		  LR_X_N_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC ^ PIX_DST: 
		do
		{
		  LR_X_SR_X_DS_B ();
		  LR_X_SR_X_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC & PIX_DST)): 
		do
		{
		  LR_X_NB_SR_A_DS_B ();
		  LR_X_NB_SR_A_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & PIX_DST: 
		do
		{
		  LR_X_SR_A_DS_B ();
		  LR_X_SR_A_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		do
		{
		  LR_X_N_SR_X_DS_B ();
		  LR_X_N_SR_X_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_DST: 
		return;
	      case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		do
		{
		  LR_X_N_SR_O_DS_B ();
		  LR_X_N_SR_O_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC: 
		do
		{
		  LR_X_SR_B ();
		  LR_X_SR_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_O_N_DS_B ();
		  LR_X_SR_O_N_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | PIX_DST: 
		do
		{
		  LR_X_SR_O_DS_B ();
		  LR_X_SR_O_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SET: 
		do
		{
		  LR_X_SE_B ();
		  LR_X_SE_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	    }
	  }
	}
	else
	{			/* short, no begin */
	  dstincr--;
	  if (endf)
	  {			/* short, end */
	    cdist = dtemp & smr;
	    switch (op)
	    {
	      case PIX_CLR: 
		do
		{
		  LR_X_CL_W ();
		  LR_X_CL_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC | PIX_DST): 
		do
		{
		  LR_X_NB_SR_O_DS_W ();
		  LR_X_NB_SR_O_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		do
		{
		  LR_X_N_SR_A_DS_W ();
		  LR_X_N_SR_A_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC): 
		do
		{
		  LR_X_N_SR_W ();
		  LR_X_N_SR_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_A_N_DS_W ();
		  LR_X_SR_A_N_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_DST): 
		do
		{
		  LR_X_N_DS_W ();
		  LR_X_N_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC ^ PIX_DST: 
		do
		{
		  LR_X_SR_X_DS_W ();
		  LR_X_SR_X_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC & PIX_DST)): 
		do
		{
		  LR_X_NB_SR_A_DS_W ();
		  LR_X_NB_SR_A_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & PIX_DST: 
		do
		{
		  LR_X_SR_A_DS_W ();
		  LR_X_SR_A_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		do
		{
		  LR_X_N_SR_X_DS_W ();
		  LR_X_N_SR_X_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_DST: 
		return;
	      case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		do
		{
		  LR_X_N_SR_O_DS_W ();
		  LR_X_N_SR_O_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC: 
		do
		{
		  LR_X_SR_W ();
		  LR_X_SR_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_O_N_DS_W ();
		  LR_X_SR_O_N_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | PIX_DST: 
		do
		{
		  LR_X_SR_O_DS_W ();
		  LR_X_SR_O_DS_E_O ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SET: 
		do
		{
		  LR_X_SE_W ();
		  LR_X_SE_E ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	    }
	  }
	  else
	  {			/* short only */
	    switch (op)
	    {
	      case PIX_CLR: 
		do
		{
		  LR_X_CL_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC | PIX_DST): 
		do
		{
		  LR_X_NB_SR_O_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		do
		{
		  LR_X_N_SR_A_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC): 
		do
		{
		  LR_X_N_SR_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_A_N_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_DST): 
		do
		{
		  LR_X_N_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC ^ PIX_DST: 
		do
		{
		  LR_X_SR_X_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC & PIX_DST)): 
		do
		{
		  LR_X_NB_SR_A_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC & PIX_DST: 
		do
		{
		  LR_X_SR_A_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		do
		{
		  LR_X_N_SR_X_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_DST: 
		return;
	      case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		do
		{
		  LR_X_N_SR_O_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC: 
		do
		{
		  LR_X_SR_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | (PIX_NOT (PIX_DST)): 
		do
		{
		  LR_X_SR_O_N_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SRC | PIX_DST: 
		do
		{
		  LR_X_SR_O_DS_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	      case PIX_SET: 
		do
		{
		  LR_X_SE_W ();
		  BUMP_DST ();
		} while (--i != -1);
		break;
	    }
	  }
	}
      }
      else
      {				/* no middle */
	dstincr--;
	cdist2 = dtemp & sml;
	if (endf)
	{			/* just begin and end */
	  cdist = dtemp & smr;
	  switch (op)
	  {
	    case PIX_CLR: 
	      do
	      {
		LR_X_CL_B ();
		LR_X_CL_E ();
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_NOT (PIX_SRC | PIX_DST): 
	      do
	      {
		LR_X_NB_SR_O_DS_B ();
		LR_X_NB_SR_O_DS_E_O (); /* only E case isn't fully optimized */
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
	      do
	      {
		LR_X_N_SR_A_DS_B ();
		LR_X_N_SR_A_DS_E_O (); /* only E case isn't fully optimized */
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_NOT (PIX_SRC): 
	      do
	      {
		LR_X_N_SR_B ();
		LR_X_N_SR_E_O (); /* only E case isn't fully optimized */
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_SRC & (PIX_NOT (PIX_DST)): 
	      do
	      {
		LR_X_SR_A_N_DS_B ();
		LR_X_SR_A_N_DS_E_O (); /* only E case isn't fully optimized */
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_NOT (PIX_DST): 
	      do
	      {
		LR_X_N_DS_B ();
		LR_X_N_DS_E_O (); /* only E case isn't fully optimized */
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_SRC ^ PIX_DST: 
	      do
	      {
		LR_X_SR_X_DS_B ();
		LR_X_SR_X_DS_E_O (); /* only E case isn't fully optimized */
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_NOT ((PIX_SRC & PIX_DST)): 
	      do
	      {
		LR_X_NB_SR_A_DS_B ();
		LR_X_NB_SR_A_DS_E_O (); /* only E case isn't fully optimized */
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_SRC & PIX_DST: 
	      do
	      {
		LR_X_SR_A_DS_B ();
		LR_X_SR_A_DS_E_O (); /* only E case isn't fully optimized */
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
	      do
	      {
		LR_X_N_SR_X_DS_B ();
		LR_X_N_SR_X_DS_E_O (); /* only E case isn't fully optimized */
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_DST: 
	      return;
	    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
	      do
	      {
		LR_X_N_SR_O_DS_B ();
		LR_X_N_SR_O_DS_E_O (); /* only E case isn't fully optimized */
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_SRC: 
	      do
	      {
		LR_X_SR_B ();
		LR_X_SR_E_O (); /* only E case isn't fully optimized */
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_SRC | (PIX_NOT (PIX_DST)): 
	      do
	      {
		LR_X_SR_O_N_DS_B ();
		LR_X_SR_O_N_DS_E_O (); /* only E case isn't fully optimized */
	      do
	      {
		LR_X_SR_O_DS_B ();
		LR_X_SR_O_DS_E_O (); /* only E case isn't fully optimized */
		BUMP_DST ();
	      } while (--i != -1);
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_SRC | PIX_DST: 
	      break;
	    case PIX_SET: 
	      do
	      {
		LR_X_SE_B ();
		LR_X_SE_E ();
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	  }
	}
	else
	{			/* little rops */
	  switch (op)
	  {
	    case PIX_CLR: 
	      do
	      {
		LR_X_CL_B ();
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_NOT (PIX_SRC | PIX_DST): 
	      do
	      {
		LR_X_NB_SR_O_DS_B ();
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
	      do
	      {
		LR_X_N_SR_A_DS_B ();
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_NOT (PIX_SRC): 
	      do
	      {
		LR_X_N_SR_B ();
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_SRC & (PIX_NOT (PIX_DST)): 
	      do
	      {
		LR_X_SR_A_N_DS_B ();
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_NOT (PIX_DST): 
	      do
	      {
		LR_X_N_DS_B ();
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_SRC ^ PIX_DST: 
	      do
	      {
		LR_X_SR_X_DS_B ();
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_NOT ((PIX_SRC & PIX_DST)): 
	      do
	      {
		LR_X_NB_SR_A_DS_B ();
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_SRC & PIX_DST: 
	      do
	      {
		LR_X_SR_A_DS_B ();
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
	      do
	      {
		LR_X_N_SR_X_DS_B ();
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_DST: 
	      return;
	    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
	      do
	      {
		LR_X_N_SR_O_DS_B ();
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_SRC: 
	      do
	      {
		LR_X_SR_B ();
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_SRC | (PIX_NOT (PIX_DST)): 
	      do
	      {
		LR_X_SR_O_N_DS_B ();
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_SRC | PIX_DST: 
	      do
	      {
		LR_X_SR_O_DS_B ();
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	    case PIX_SET: 
	      do
	      {
		LR_X_SE_B ();
		BUMP_DST ();
	      } while (--i != -1);
	      break;
	  }
	}
      }
    }
  }
  else
  {				/* src is not constant */
    if (rop_isright (d4_dir))
    {				/* right to left */
    /* right to left uses 16-bit instead of 32-bit */
    /* set up words per line */
      if (!rop_isdown (d4_dir))
      {				/* rop is up */
	srcincr = a2_sprd -> md_linebytes >> 1;
	dstincr = a3_dprd -> md_linebytes >> 1;
      }
      else
      {
	srcincr = -a2_sprd -> md_linebytes >> 1;
	dstincr = -a3_dprd -> md_linebytes >> 1;
      }
      endf = ((width - dskew) & 15) != 0;
    /* set up masks for end conditions */
      if (dskew < width)	/* is rop wider than one word? */
      {				/* normal width */
	dmr = (u_short) (0x0000ffff >> (dskew));
	smr = ~dmr;
/* 		if ((width - dskew) & 15) */
	if (endf)
	{
	  dml = (u_short) (0x0000ffff << (((width - dskew)) & 15));
	}
	else
	{
	  dml = (u_short) (0xffff);
	}
	sml = ~dml;
	dsize -= dskew;
	ocount = count = (dsize >> 4) - 1;
      }
      else
      {				/* less than one word. Only one mask is
				   necessary */
	count = -1;
	smr = (u_short) (0xffff0000 >> (dskew));
	smr &= ~(u_short) (0xffff0000 >> (((dskew - width)) & 15));
	dmr = ~smr;
	endf = 0;
      }
      if (dskew == sskew)
      {				/* src and dest are aligned */
	if (count > -1)
	{			/* are there words */
	  ct2 = (count + 1);
	  if (smr)
	  {
	    ct2++;
	    dstincr += ct2;
	    srcincr += ct2;
	    if (endf)
	    {			/* all four */
	      switch (op)
	      {
		case PIX_NOT (PIX_SRC | PIX_DST): 
		  do
		  {
		    RL_A_NB_SR_O_DS_B ();
		    RL_A_NB_SR_O_DS_W ();
		    RL_A_NB_SR_O_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		  do
		  {
		    RL_A_N_SR_A_DS_B ();
		    RL_A_N_SR_A_DS_W ();
		    RL_A_N_SR_A_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT (PIX_SRC): 
		  do
		  {
		    RL_A_N_SR_B ();
		    RL_A_N_SR_W ();
		    RL_A_N_SR_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC & (PIX_NOT (PIX_DST)): 
		  do
		  {
		    RL_A_SR_A_N_DS_B ();
		    RL_A_SR_A_N_DS_W ();
		    RL_A_SR_A_N_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC ^ PIX_DST: 
		  do
		  {
		    RL_A_SR_X_DS_B ();
		    RL_A_SR_X_DS_W ();
		    RL_A_SR_X_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC & PIX_DST)): 
		  do
		  {
		    RL_A_NB_SR_A_DS_B ();
		    RL_A_NB_SR_A_DS_W ();
		    RL_A_NB_SR_A_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC & PIX_DST: 
		  do
		  {
		    RL_A_SR_A_DS_B ();
		    RL_A_SR_A_DS_W ();
		    RL_A_SR_A_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		  do
		  {
		    RL_A_N_SR_X_DS_B ();
		    RL_A_N_SR_X_DS_W ();
		    RL_A_N_SR_X_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		  do
		  {
		    RL_A_N_SR_O_DS_B ();
		    RL_A_N_SR_O_DS_W ();
		    RL_A_N_SR_O_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC: 
		  do
		  {
		    RL_A_SR_B ();
		    RL_A_SR_W ();
		    RL_A_SR_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC | (PIX_NOT (PIX_DST)): 
		  do
		  {
		    RL_A_SR_O_N_DS_B ();
		    RL_A_SR_O_N_DS_W ();
		    RL_A_SR_O_N_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC | PIX_DST: 
		  do
		  {
		    RL_A_SR_O_DS_B ();
		    RL_A_SR_O_DS_W ();
		    RL_A_SR_O_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
	      }
	    }
	    else
	    {			/* begin, long, and short */
	      switch (op)
	      {
		case PIX_NOT (PIX_SRC | PIX_DST): 
		  do
		  {
		    RL_A_NB_SR_O_DS_B ();
		    RL_A_NB_SR_O_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		  do
		  {
		    RL_A_N_SR_A_DS_B ();
		    RL_A_N_SR_A_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT (PIX_SRC): 
		  do
		  {
		    RL_A_N_SR_B ();
		    RL_A_N_SR_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC & (PIX_NOT (PIX_DST)): 
		  do
		  {
		    RL_A_SR_A_N_DS_B ();
		    RL_A_SR_A_N_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC ^ PIX_DST: 
		  do
		  {
		    RL_A_SR_X_DS_B ();
		    RL_A_SR_X_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC & PIX_DST)): 
		  do
		  {
		    RL_A_NB_SR_A_DS_B ();
		    RL_A_NB_SR_A_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC & PIX_DST: 
		  do
		  {
		    RL_A_SR_A_DS_B ();
		    RL_A_SR_A_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		  do
		  {
		    RL_A_N_SR_X_DS_B ();
		    RL_A_N_SR_X_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		  do
		  {
		    RL_A_N_SR_O_DS_B ();
		    RL_A_N_SR_O_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC: 
		  do
		  {
		    RL_A_SR_B ();
		    RL_A_SR_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC | (PIX_NOT (PIX_DST)): 
		  do
		  {
		    RL_A_SR_O_N_DS_B ();
		    RL_A_SR_O_N_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC | PIX_DST: 
		  do
		  {
		    RL_A_SR_O_DS_B ();
		    RL_A_SR_O_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
	      }
	    }
	  }
	  else
	  {			/* no begin piece */
	    dstincr += ct2;
	    srcincr += ct2;
	    dstp--; /* must predecrement to get to data */
	    srcp--;
	    if (endf)
	    {			/* long, short, end */
	      switch (op)
	      {
		case PIX_NOT (PIX_SRC | PIX_DST): 
		  do
		  {
		    RL_A_NB_SR_O_DS_W ();
		    RL_A_NB_SR_O_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		  do
		  {
		    RL_A_N_SR_A_DS_W ();
		    RL_A_N_SR_A_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT (PIX_SRC): 
		  do
		  {
		    RL_A_N_SR_W ();
		    RL_A_N_SR_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC & (PIX_NOT (PIX_DST)): 
		  do
		  {
		    RL_A_SR_A_N_DS_W ();
		    RL_A_SR_A_N_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC ^ PIX_DST: 
		  do
		  {
		    RL_A_SR_X_DS_W ();
		    RL_A_SR_X_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC & PIX_DST)): 
		  do
		  {
		    RL_A_NB_SR_A_DS_W ();
		    RL_A_NB_SR_A_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC & PIX_DST: 
		  do
		  {
		    RL_A_SR_A_DS_W ();
		    RL_A_SR_A_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		  do
		  {
		    RL_A_N_SR_X_DS_W ();
		    RL_A_N_SR_X_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		  do
		  {
		    RL_A_N_SR_O_DS_W ();
		    RL_A_N_SR_O_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC: 
		  do
		  {
		    RL_A_SR_W ();
		    RL_A_SR_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC | (PIX_NOT (PIX_DST)): 
		  do
		  {
		    RL_A_SR_O_N_DS_W ();
		    RL_A_SR_O_N_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC | PIX_DST: 
		  do
		  {
		    RL_A_SR_O_DS_W ();
		    RL_A_SR_O_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
	      }
	    }
	    else
	    {			/* words only */
	      switch (op)
	      {
		case PIX_NOT (PIX_SRC | PIX_DST): 
		  do
		  {
		    RL_A_NB_SR_O_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		  do
		  {
		    RL_A_N_SR_A_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT (PIX_SRC): 
		  do
		  {
		    RL_A_N_SR_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC & (PIX_NOT (PIX_DST)): 
		  do
		  {
		    RL_A_SR_A_N_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC ^ PIX_DST: 
		  do
		  {
		    RL_A_SR_X_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC & PIX_DST)): 
		  do
		  {
		    RL_A_NB_SR_A_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC & PIX_DST: 
		  do
		  {
		    RL_A_SR_A_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		  do
		  {
		    RL_A_N_SR_X_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		  do
		  {
		    RL_A_N_SR_O_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC: 
		  do
		  {
		    RL_A_SR_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC | (PIX_NOT (PIX_DST)): 
		  do
		  {
		    RL_A_SR_O_N_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC | PIX_DST: 
		  do
		  {
		    RL_A_SR_O_DS_W ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
	      }
	    }
	  }
	}
	else
	{			/* no words */
	  dstincr++;
	  srcincr++;
	  if (endf)
	  {			/* begin and end */
	    switch (op)
	    {
	      case PIX_NOT (PIX_SRC | PIX_DST): 
		do
		{
		  RL_A_NB_SR_O_DS_B ();
		  RL_A_NB_SR_O_DS_E ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		do
		{
		  RL_A_N_SR_A_DS_B ();
		  RL_A_N_SR_A_DS_E ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC): 
		do
		{
		  RL_A_N_SR_B ();
		  RL_A_N_SR_E ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_SRC & (PIX_NOT (PIX_DST)): 
		do
		{
		  RL_A_SR_A_N_DS_B ();
		  RL_A_SR_A_N_DS_E ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_SRC ^ PIX_DST: 
		do
		{
		  RL_A_SR_X_DS_B ();
		  RL_A_SR_X_DS_E ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC & PIX_DST)): 
		do
		{
		  RL_A_NB_SR_A_DS_B ();
		  RL_A_NB_SR_A_DS_E ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_SRC & PIX_DST: 
		do
		{
		  RL_A_SR_A_DS_B ();
		  RL_A_SR_A_DS_E ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		do
		{
		  RL_A_N_SR_X_DS_B ();
		  RL_A_N_SR_X_DS_E ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		do
		{
		  RL_A_N_SR_O_DS_B ();
		  RL_A_N_SR_O_DS_E ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_SRC: 
		do
		{
		  RL_A_SR_B ();
		  RL_A_SR_E ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_SRC | (PIX_NOT (PIX_DST)): 
		do
		{
		  RL_A_SR_O_N_DS_B ();
		  RL_A_SR_O_N_DS_E ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_SRC | PIX_DST: 
		do
		{
		  RL_A_SR_O_DS_B ();
		  RL_A_SR_O_DS_E ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	    }
	  }
	  else
	  {			/* little rops */
	    switch (op)
	    {
	      case PIX_NOT (PIX_SRC | PIX_DST): 
		do
		{
		  RL_A_NB_SR_O_DS_B ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		do
		{
		  RL_A_N_SR_A_DS_B ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_NOT (PIX_SRC): 
		do
		{
		  RL_A_N_SR_B ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_SRC & (PIX_NOT (PIX_DST)): 
		do
		{
		  RL_A_SR_A_N_DS_B ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_SRC ^ PIX_DST: 
		do
		{
		  RL_A_SR_X_DS_B ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC & PIX_DST)): 
		do
		{
		  RL_A_NB_SR_A_DS_B ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_SRC & PIX_DST: 
		do
		{
		  RL_A_SR_A_DS_B ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		do
		{
		  RL_A_N_SR_X_DS_B ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		do
		{
		  RL_A_N_SR_O_DS_B ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_SRC: 
		do
		{
		  RL_A_SR_B ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_SRC | (PIX_NOT (PIX_DST)): 
		do
		{
		  RL_A_SR_O_N_DS_B ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	      case PIX_SRC | PIX_DST: 
		do
		{
		  RL_A_SR_O_DS_B ();
		  BUMP_BOTH ();
		} while (--i != -1);
		break;
	    }
	  }
	}
      }
      else
      {				/* src and dst are not aligned */
	if (dskew > sskew)
	{			/*  dst skew is > src skew */
	  cdist2 = dskew - sskew;
	  cdist = 16 - cdist2;
	  if (count > -1)
	  {			/* are there long words */
	    ct2 = (count + 1);
	    if (smr)
	    {
	      ct2 += 1;
	      dstincr += ct2;
	      if (endf)
	      {			/* all four */
		srcincr += ct2 + 1;
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      RL_D_NB_SR_O_DS_B ();
		      RL_D_NB_SR_O_DS_W ();
		      RL_D_NB_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      RL_D_N_SR_A_DS_B ();
		      RL_D_N_SR_A_DS_W ();
		      RL_D_N_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      RL_D_N_SR_B ();
		      RL_D_N_SR_W ();
		      RL_D_N_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_D_SR_A_N_DS_B ();
		      RL_D_SR_A_N_DS_W ();
		      RL_D_SR_A_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      RL_D_SR_X_DS_B ();
		      RL_D_SR_X_DS_W ();
		      RL_D_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      RL_D_NB_SR_A_DS_B ();
		      RL_D_NB_SR_A_DS_W ();
		      RL_D_NB_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      RL_D_SR_A_DS_B ();
		      RL_D_SR_A_DS_W ();
		      RL_D_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      RL_D_N_SR_X_DS_B ();
		      RL_D_N_SR_X_DS_W ();
		      RL_D_N_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      RL_D_N_SR_O_DS_B ();
		      RL_D_N_SR_O_DS_W ();
		      RL_D_N_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      RL_D_SR_B ();
		      RL_D_SR_W ();
		      RL_D_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_D_SR_O_N_DS_B ();
		      RL_D_SR_O_N_DS_W ();
		      RL_D_SR_O_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      RL_D_SR_O_DS_B ();
		      RL_D_SR_O_DS_W ();
		      RL_D_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	      else
	      {			/* begin, long, and short */
		srcincr += ct2;
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      RL_D_NB_SR_O_DS_B ();
		      RL_D_NB_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      RL_D_N_SR_A_DS_B ();
		      RL_D_N_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      RL_D_N_SR_B ();
		      RL_D_N_SR_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_D_SR_A_N_DS_B ();
		      RL_D_SR_A_N_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      RL_D_SR_X_DS_B ();
		      RL_D_SR_X_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      RL_D_NB_SR_A_DS_B ();
		      RL_D_NB_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      RL_D_SR_A_DS_B ();
		      RL_D_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      RL_D_N_SR_X_DS_B ();
		      RL_D_N_SR_X_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      RL_D_N_SR_O_DS_B ();
		      RL_D_N_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      RL_D_SR_B ();
		      RL_D_SR_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_D_SR_O_N_DS_B ();
		      RL_D_SR_O_N_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      RL_D_SR_O_DS_B ();
		      RL_D_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	    }
	    else
	    {			/* long, short */
	      ct2++;
	      dstincr += ct2;
	    dstp--; /* must predecrement to get to data */
	      if (endf)
	      {			/* long, short, end */
		srcincr += ct2 + 1;
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      RL_D_NB_SR_O_DS_W ();
		      RL_D_NB_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      RL_D_N_SR_A_DS_W ();
		      RL_D_N_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      RL_D_N_SR_W ();
		      RL_D_N_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_D_SR_A_N_DS_W ();
		      RL_D_SR_A_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      RL_D_SR_X_DS_W ();
		      RL_D_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      RL_D_NB_SR_A_DS_W ();
		      RL_D_NB_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      RL_D_SR_A_DS_W ();
		      RL_D_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      RL_D_N_SR_X_DS_W ();
		      RL_D_N_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      RL_D_N_SR_O_DS_W ();
		      RL_D_N_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      RL_D_SR_W ();
		      RL_D_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_D_SR_O_N_DS_W ();
		      RL_D_SR_O_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      RL_D_SR_O_DS_W ();
		      RL_D_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	      else
	      {			/* long, short only */
		srcincr += ct2;
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      RL_D_NB_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      RL_D_N_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      RL_D_N_SR_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_D_SR_A_N_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      RL_D_SR_X_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      RL_D_NB_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      RL_D_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      RL_D_N_SR_X_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      RL_D_N_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      RL_D_SR_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_D_SR_O_N_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      RL_D_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	    }
	  }
	  else
	  {			/* no long words */
	    dstincr++;
	    if (endf)
	    {			/* begin and end */
	      srcincr += 2;
	      switch (op)
	      {
		case PIX_NOT (PIX_SRC | PIX_DST): 
		  do
		  {
		    RL_D_NB_SR_O_DS_B ();
		    RL_D_NB_SR_O_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		  do
		  {
		    RL_D_N_SR_A_DS_B ();
		    RL_D_N_SR_A_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT (PIX_SRC): 
		  do
		  {
		    RL_D_N_SR_B ();
		    RL_D_N_SR_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC & (PIX_NOT (PIX_DST)): 
		  do
		  {
		    RL_D_SR_A_N_DS_B ();
		    RL_D_SR_A_N_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC ^ PIX_DST: 
		  do
		  {
		    RL_D_SR_X_DS_B ();
		    RL_D_SR_X_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC & PIX_DST)): 
		  do
		  {
		    RL_D_NB_SR_A_DS_B ();
		    RL_D_NB_SR_A_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC & PIX_DST: 
		  do
		  {
		    RL_D_SR_A_DS_B ();
		    RL_D_SR_A_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		  do
		  {
		    RL_D_N_SR_X_DS_B ();
		    RL_D_N_SR_X_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		  do
		  {
		    RL_D_N_SR_O_DS_B ();
		    RL_D_N_SR_O_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC: 
		  do
		  {
		    RL_D_SR_B ();
		    RL_D_SR_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC | (PIX_NOT (PIX_DST)): 
		  do
		  {
		    RL_D_SR_O_N_DS_B ();
		    RL_D_SR_O_N_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC | PIX_DST: 
		  do
		  {
		    RL_D_SR_O_DS_B ();
		    RL_D_SR_O_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
	      }
	    }
	    else
	    {			/* little rops */
	      srcincr++;
	      switch (op)
	      {
		case PIX_NOT (PIX_SRC | PIX_DST): 
		  do
		  {
		    RL_D_NB_SR_O_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		  do
		  {
		    RL_D_N_SR_A_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT (PIX_SRC): 
		  do
		  {
		    RL_D_N_SR_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC & (PIX_NOT (PIX_DST)): 
		  do
		  {
		    RL_D_SR_A_N_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC ^ PIX_DST: 
		  do
		  {
		    RL_D_SR_X_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC & PIX_DST)): 
		  do
		  {
		    RL_D_NB_SR_A_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC & PIX_DST: 
		  do
		  {
		    RL_D_SR_A_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		  do
		  {
		    RL_D_N_SR_X_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		  do
		  {
		    RL_D_N_SR_O_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC: 
		  do
		  {
		    RL_D_SR_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC | (PIX_NOT (PIX_DST)): 
		  do
		  {
		    RL_D_SR_O_N_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC | PIX_DST: 
		  do
		  {
		    RL_D_SR_O_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
	      }
	    }
	  }
	}
	else
	{			/* src skew is > dst skew */
	  cdist2 = sskew - dskew;
	  cdist = 16 - cdist2;
	  if (count > -1)
	  {			/* are there long words */
	    ct2 = (count + 1);
	    if (smr)
	    {
	      ct2 += 1;
	      dstincr += ct2;
	      if (endf)
	      {			/* all four */
		srcincr += ct2;
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      RL_S_NB_SR_O_DS_B ();
		      RL_S_NB_SR_O_DS_W ();
		      RL_S_NB_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      RL_S_N_SR_A_DS_B ();
		      RL_S_N_SR_A_DS_W ();
		      RL_S_N_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      RL_S_N_SR_B ();
		      RL_S_N_SR_W ();
		      RL_S_N_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_S_SR_A_N_DS_B ();
		      RL_S_SR_A_N_DS_W ();
		      RL_S_SR_A_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      RL_S_SR_X_DS_B ();
		      RL_S_SR_X_DS_W ();
		      RL_S_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      RL_S_NB_SR_A_DS_B ();
		      RL_S_NB_SR_A_DS_W ();
		      RL_S_NB_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      RL_S_SR_A_DS_B ();
		      RL_S_SR_A_DS_W ();
		      RL_S_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      RL_S_N_SR_X_DS_B ();
		      RL_S_N_SR_X_DS_W ();
		      RL_S_N_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      RL_S_N_SR_O_DS_B ();
		      RL_S_N_SR_O_DS_W ();
		      RL_S_N_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      RL_S_SR_B ();
		      RL_S_SR_W ();
		      RL_S_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_S_SR_O_N_DS_B ();
		      RL_S_SR_O_N_DS_W ();
		      RL_S_SR_O_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      RL_S_SR_O_DS_B ();
		      RL_S_SR_O_DS_W ();
		      RL_S_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	      else
	      {			/* begin, long, and short */
		srcincr += ct2-1;
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      RL_S_NB_SR_O_DS_B ();
		      RL_S_NB_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      RL_S_N_SR_A_DS_B ();
		      RL_S_N_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      RL_S_N_SR_B ();
		      RL_S_N_SR_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_S_SR_A_N_DS_B ();
		      RL_S_SR_A_N_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      RL_S_SR_X_DS_B ();
		      RL_S_SR_X_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      RL_S_NB_SR_A_DS_B ();
		      RL_S_NB_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      RL_S_SR_A_DS_B ();
		      RL_S_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      RL_S_N_SR_X_DS_B ();
		      RL_S_N_SR_X_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      RL_S_N_SR_O_DS_B ();
		      RL_S_N_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      RL_S_SR_B ();
		      RL_S_SR_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_S_SR_O_N_DS_B ();
		      RL_S_SR_O_N_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      RL_S_SR_O_DS_B ();
		      RL_S_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	    }
	    else
	    {			/* short */
	      dstincr += ct2;
	    dstp--; /* must predecrement to get to data */
	      if (endf)
	      {			/* short, end */
		srcincr += ct2 + 1;
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      RL_S_NB_SR_O_DS_W ();
		      RL_S_NB_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      RL_S_N_SR_A_DS_W ();
		      RL_S_N_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      RL_S_N_SR_W ();
		      RL_S_N_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_S_SR_A_N_DS_W ();
		      RL_S_SR_A_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      RL_S_SR_X_DS_W ();
		      RL_S_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      RL_S_NB_SR_A_DS_W ();
		      RL_S_NB_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      RL_S_SR_A_DS_W ();
		      RL_S_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      RL_S_N_SR_X_DS_W ();
		      RL_S_N_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      RL_S_N_SR_O_DS_W ();
		      RL_S_N_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      RL_S_SR_W ();
		      RL_S_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_S_SR_O_N_DS_W ();
		      RL_S_SR_O_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      RL_S_SR_O_DS_W ();
		      RL_S_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	      else
	      {			/* long, short only */
		srcincr += ct2;
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      RL_S_NB_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      RL_S_N_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      RL_S_N_SR_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_S_SR_A_N_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      RL_S_SR_X_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      RL_S_NB_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      RL_S_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      RL_S_N_SR_X_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      RL_S_N_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      RL_S_SR_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_S_SR_O_N_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      RL_S_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	    }
	  }
	  else
	  {			/* no long words */
	    if (endf)
	    {			/* begin and end */
	      srcincr++;
	      dstincr++;
	      switch (op)
	      {
		case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      RL_S_NB_SR_O_DS_B ();
		      RL_S_NB_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      RL_S_N_SR_A_DS_B ();
		      RL_S_N_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_NOT (PIX_SRC): 
		    do
		    {
		      RL_S_N_SR_B ();
		      RL_S_N_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_S_SR_A_N_DS_B ();
		      RL_S_SR_A_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      RL_S_SR_X_DS_B ();
		      RL_S_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      RL_S_NB_SR_A_DS_B ();
		      RL_S_NB_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_SRC & PIX_DST: 
		    do
		    {
		      RL_S_SR_A_DS_B ();
		      RL_S_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      RL_S_N_SR_X_DS_B ();
		      RL_S_N_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      RL_S_N_SR_O_DS_B ();
		      RL_S_N_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_SRC: 
		    do
		    {
		      RL_S_SR_B ();
		      RL_S_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_S_SR_O_N_DS_B ();
		      RL_S_SR_O_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_SRC | PIX_DST: 
		    do
		    {
		      RL_S_SR_O_DS_B ();
		      RL_S_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
	      }
	    }
	    else
	    {			/* little rops */
	      dstincr++;
	      switch (op)
	      {
		case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      RL_S_NB_SR_O_DS_B ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      RL_S_N_SR_A_DS_B ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_NOT (PIX_SRC): 
		    do
		    {
		      RL_S_N_SR_B ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_S_SR_A_N_DS_B ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      RL_S_SR_X_DS_B ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      RL_S_NB_SR_A_DS_B ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_SRC & PIX_DST: 
		    do
		    {
		      RL_S_SR_A_DS_B ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      RL_S_N_SR_X_DS_B ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      RL_S_N_SR_O_DS_B ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_SRC: 
		    do
		    {
		      RL_S_SR_B ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      RL_S_SR_O_N_DS_B ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
		case PIX_SRC | PIX_DST: 
		    do
		    {
		      RL_S_SR_O_DS_B ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		  break;
	      }
	    }
	  }
	}
      }
    }
    else
    {				/* left to right */
    /* compute number of words per line */
      if (!rop_isdown (d4_dir))
      {				/* rop is up */
	srcincr = a2_sprd -> md_linebytes >> 1;
	dstincr = a3_dprd -> md_linebytes >> 1;
      }
      else
      {
	srcincr = -a2_sprd -> md_linebytes >> 1;
	dstincr = -a3_dprd -> md_linebytes >> 1;
      }
      if ((dsize + dskew) >> 4)	/* is rop wider than one word? */
      {				/*  rop is greater than one word */
	if (dskew)
	{			/* only compute mask if necessesary */
	  sml = (u_short) (0x0000ffff >> dskew);
	  dsize -= (16 - dskew);
	}
	else
	{
	  sml = 0;
	}
	dml = ~sml;
	smr = (u_short) (0xffff0000 >> (((dskew + width)) & 15));
	dmr = ~smr;
	sixtf = (dsize & 16) != 0;
	endf = (dsize & 15) != 0;
	count = ocount = (dsize >> 5) - 1;
      }
      else
      {				/* little rop */
	count = -1;
	endf = sixtf = 0;
	sml = (u_short) (0x0000ffff >> dskew);
	sml &= (u_short) (0xffff0000 >> (((dskew + width)) & 15));
	dml = ~sml;
      }
      if (dskew == sskew)
      {				/* src and dst are aligned */
	if (count > -1)
	{			/* are there long words */
	  cdist = count;
	  ct2 = ((count + 1) << 1);
	  if (sixtf)
	  {			/*  short words? */
	    if (sml)
	    {
	      ct2 += 2;
	      dstincr -= ct2;
	      srcincr -= ct2;
	      if (endf)
	      {			/* all four */
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      LR_A_NB_SR_O_DS_B ();
		      LR_A_NB_SR_O_DS_LG ();
		      LR_A_NB_SR_O_DS_W ();
		      LR_A_NB_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      LR_A_N_SR_A_DS_B ();
		      LR_A_N_SR_A_DS_LG ();
		      LR_A_N_SR_A_DS_W ();
		      LR_A_N_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      LR_A_N_SR_B ();
		      LR_A_N_SR_LG ();
		      LR_A_N_SR_W ();
		      LR_A_N_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_A_N_DS_B ();
		      LR_A_SR_A_N_DS_LG ();
		      LR_A_SR_A_N_DS_W ();
		      LR_A_SR_A_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      LR_A_SR_X_DS_B ();
		      LR_A_SR_X_DS_LG ();
		      LR_A_SR_X_DS_W ();
		      LR_A_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      LR_A_NB_SR_A_DS_B ();
		      LR_A_NB_SR_A_DS_LG ();
		      LR_A_NB_SR_A_DS_W ();
		      LR_A_NB_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      LR_A_SR_A_DS_B ();
		      LR_A_SR_A_DS_LG ();
		      LR_A_SR_A_DS_W ();
		      LR_A_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      LR_A_N_SR_X_DS_B ();
		      LR_A_N_SR_X_DS_LG ();
		      LR_A_N_SR_X_DS_W ();
		      LR_A_N_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      LR_A_N_SR_O_DS_B ();
		      LR_A_N_SR_O_DS_LG ();
		      LR_A_N_SR_O_DS_W ();
		      LR_A_N_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      LR_A_SR_B ();
		      LR_A_SR_LG ();
		      LR_A_SR_W ();
		      LR_A_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_O_N_DS_B ();
		      LR_A_SR_O_N_DS_LG ();
		      LR_A_SR_O_N_DS_W ();
		      LR_A_SR_O_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      LR_A_SR_O_DS_B ();
		      LR_A_SR_O_DS_LG ();
		      LR_A_SR_O_DS_W ();
		      LR_A_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	      else
	      {			/* begin, long, and short */
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      LR_A_NB_SR_O_DS_B ();
		      LR_A_NB_SR_O_DS_LG ();
		      LR_A_NB_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      LR_A_N_SR_A_DS_B ();
		      LR_A_N_SR_A_DS_LG ();
		      LR_A_N_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      LR_A_N_SR_B ();
		      LR_A_N_SR_LG ();
		      LR_A_N_SR_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_A_N_DS_B ();
		      LR_A_SR_A_N_DS_LG ();
		      LR_A_SR_A_N_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      LR_A_SR_X_DS_B ();
		      LR_A_SR_X_DS_LG ();
		      LR_A_SR_X_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      LR_A_NB_SR_A_DS_B ();
		      LR_A_NB_SR_A_DS_LG ();
		      LR_A_NB_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      LR_A_SR_A_DS_B ();
		      LR_A_SR_A_DS_LG ();
		      LR_A_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      LR_A_N_SR_X_DS_B ();
		      LR_A_N_SR_X_DS_LG ();
		      LR_A_N_SR_X_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      LR_A_N_SR_O_DS_B ();
		      LR_A_N_SR_O_DS_LG ();
		      LR_A_N_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      LR_A_SR_B ();
		      LR_A_SR_LG ();
		      LR_A_SR_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_O_N_DS_B ();
		      LR_A_SR_O_N_DS_LG ();
		      LR_A_SR_O_N_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      LR_A_SR_O_DS_B ();
		      LR_A_SR_O_DS_LG ();
		      LR_A_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	    }
	    else
	    {			/* no begin piece */
	      ct2++;
	      dstincr -= ct2;
	      srcincr -= ct2;
	      if (endf)
	      {			/* long, short, end */
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      LR_A_NB_SR_O_DS_LG ();
		      LR_A_NB_SR_O_DS_W ();
		      LR_A_NB_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      LR_A_N_SR_A_DS_LG ();
		      LR_A_N_SR_A_DS_W ();
		      LR_A_N_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      LR_A_N_SR_LG ();
		      LR_A_N_SR_W ();
		      LR_A_N_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_A_N_DS_LG ();
		      LR_A_SR_A_N_DS_W ();
		      LR_A_SR_A_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      LR_A_SR_X_DS_LG ();
		      LR_A_SR_X_DS_W ();
		      LR_A_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      LR_A_NB_SR_A_DS_LG ();
		      LR_A_NB_SR_A_DS_W ();
		      LR_A_NB_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      LR_A_SR_A_DS_LG ();
		      LR_A_SR_A_DS_W ();
		      LR_A_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      LR_A_N_SR_X_DS_LG ();
		      LR_A_N_SR_X_DS_W ();
		      LR_A_N_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      LR_A_N_SR_O_DS_LG ();
		      LR_A_N_SR_O_DS_W ();
		      LR_A_N_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      LR_A_SR_LG ();
		      LR_A_SR_W ();
		      LR_A_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_O_N_DS_LG ();
		      LR_A_SR_O_N_DS_W ();
		      LR_A_SR_O_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      LR_A_SR_O_DS_LG ();
		      LR_A_SR_O_DS_W ();
		      LR_A_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	      else
	      {			/* long, short only */
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      LR_A_NB_SR_O_DS_LG ();
		      LR_A_NB_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      LR_A_N_SR_A_DS_LG ();
		      LR_A_N_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      LR_A_N_SR_LG ();
		      LR_A_N_SR_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_A_N_DS_LG ();
		      LR_A_SR_A_N_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      LR_A_SR_X_DS_LG ();
		      LR_A_SR_X_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      LR_A_NB_SR_A_DS_LG ();
		      LR_A_NB_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      LR_A_SR_A_DS_LG ();
		      LR_A_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      LR_A_N_SR_X_DS_LG ();
		      LR_A_N_SR_X_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      LR_A_N_SR_O_DS_LG ();
		      LR_A_N_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      LR_A_SR_LG ();
		      LR_A_SR_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_O_N_DS_LG ();
		      LR_A_SR_O_N_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      LR_A_SR_O_DS_LG ();
		      LR_A_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	    }
	  }
	  else
	  {			/*  long only */
	    if (sml)
	    {
	      ct2++;
	      dstincr -= ct2;
	      srcincr -= ct2;
	      if (endf)
	      {			/* begin, long and end */
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      LR_A_NB_SR_O_DS_B ();
		      LR_A_NB_SR_O_DS_LG ();
		      LR_A_NB_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      LR_A_N_SR_A_DS_B ();
		      LR_A_N_SR_A_DS_LG ();
		      LR_A_N_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      LR_A_N_SR_B ();
		      LR_A_N_SR_LG ();
		      LR_A_N_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_A_N_DS_B ();
		      LR_A_SR_A_N_DS_LG ();
		      LR_A_SR_A_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      LR_A_SR_X_DS_B ();
		      LR_A_SR_X_DS_LG ();
		      LR_A_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      LR_A_NB_SR_A_DS_B ();
		      LR_A_NB_SR_A_DS_LG ();
		      LR_A_NB_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      LR_A_SR_A_DS_B ();
		      LR_A_SR_A_DS_LG ();
		      LR_A_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      LR_A_N_SR_X_DS_B ();
		      LR_A_N_SR_X_DS_LG ();
		      LR_A_N_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      LR_A_N_SR_O_DS_B ();
		      LR_A_N_SR_O_DS_LG ();
		      LR_A_N_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      LR_A_SR_B ();
		      LR_A_SR_LG ();
		      LR_A_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_O_N_DS_B ();
		      LR_A_SR_O_N_DS_LG ();
		      LR_A_SR_O_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      LR_A_SR_O_DS_B ();
		      LR_A_SR_O_DS_LG ();
		      LR_A_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	      else
	      {			/* begin  and long */
	      /* predecrement to take care of incrementing by dst */
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      LR_A_NB_SR_O_DS_B ();
		      LR_A_NB_SR_O_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      LR_A_N_SR_A_DS_B ();
		      LR_A_N_SR_A_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      LR_A_N_SR_B ();
		      LR_A_N_SR_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_A_N_DS_B ();
		      LR_A_SR_A_N_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      LR_A_SR_X_DS_B ();
		      LR_A_SR_X_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      LR_A_NB_SR_A_DS_B ();
		      LR_A_NB_SR_A_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      LR_A_SR_A_DS_B ();
		      LR_A_SR_A_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      LR_A_N_SR_X_DS_B ();
		      LR_A_N_SR_X_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      LR_A_N_SR_O_DS_B ();
		      LR_A_N_SR_O_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      LR_A_SR_B ();
		      LR_A_SR_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_O_N_DS_B ();
		      LR_A_SR_O_N_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      LR_A_SR_O_DS_B ();
		      LR_A_SR_O_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	    }
	    else
	    {			/* long, only */
	      dstincr -= ct2;
	      srcincr -= ct2;
	      if (endf)
	      {			/* long,  end */
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      LR_A_NB_SR_O_DS_LG ();
		      LR_A_NB_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      LR_A_N_SR_A_DS_LG ();
		      LR_A_N_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      LR_A_N_SR_LG ();
		      LR_A_N_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_A_N_DS_LG ();
		      LR_A_SR_A_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      LR_A_SR_X_DS_LG ();
		      LR_A_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      LR_A_NB_SR_A_DS_LG ();
		      LR_A_NB_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      LR_A_SR_A_DS_LG ();
		      LR_A_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      LR_A_N_SR_X_DS_LG ();
		      LR_A_N_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      LR_A_N_SR_O_DS_LG ();
		      LR_A_N_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      LR_A_SR_LG ();
		      LR_A_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_O_N_DS_LG ();
		      LR_A_SR_O_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      LR_A_SR_O_DS_LG ();
		      LR_A_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	      else
	      {			/* long only */
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      LR_A_NB_SR_O_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      LR_A_N_SR_A_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      LR_A_N_SR_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_A_N_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      LR_A_SR_X_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      LR_A_NB_SR_A_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      LR_A_SR_A_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      LR_A_N_SR_X_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      LR_A_N_SR_O_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      LR_A_SR_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_O_N_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      LR_A_SR_O_DS_LG ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	    }
	  }
	}
	else
	{			/* no long words */
	  if (sixtf)
	  {			/*  short words? */
	    if (sml)
	    {
	      dstincr -= 2;
	      srcincr -= 2;
	      if (endf)
	      {			/* all three */
		srcincr -= 2;
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      LR_A_NB_SR_O_DS_B ();
		      LR_A_NB_SR_O_DS_W ();
		      LR_A_NB_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      LR_A_N_SR_A_DS_B ();
		      LR_A_N_SR_A_DS_W ();
		      LR_A_N_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      LR_A_N_SR_B ();
		      LR_A_N_SR_W ();
		      LR_A_N_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_A_N_DS_B ();
		      LR_A_SR_A_N_DS_W ();
		      LR_A_SR_A_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      LR_A_SR_X_DS_B ();
		      LR_A_SR_X_DS_W ();
		      LR_A_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      LR_A_NB_SR_A_DS_B ();
		      LR_A_NB_SR_A_DS_W ();
		      LR_A_NB_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      LR_A_SR_A_DS_B ();
		      LR_A_SR_A_DS_W ();
		      LR_A_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      LR_A_N_SR_X_DS_B ();
		      LR_A_N_SR_X_DS_W ();
		      LR_A_N_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      LR_A_N_SR_O_DS_B ();
		      LR_A_N_SR_O_DS_W ();
		      LR_A_N_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      LR_A_SR_B ();
		      LR_A_SR_W ();
		      LR_A_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_O_N_DS_B ();
		      LR_A_SR_O_N_DS_W ();
		      LR_A_SR_O_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      LR_A_SR_O_DS_B ();
		      LR_A_SR_O_DS_W ();
		      LR_A_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	      else
	      {			/* begin and short */
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      LR_A_NB_SR_O_DS_B ();
		      LR_A_NB_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      LR_A_N_SR_A_DS_B ();
		      LR_A_N_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      LR_A_N_SR_B ();
		      LR_A_N_SR_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_A_N_DS_B ();
		      LR_A_SR_A_N_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      LR_A_SR_X_DS_B ();
		      LR_A_SR_X_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      LR_A_NB_SR_A_DS_B ();
		      LR_A_NB_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      LR_A_SR_A_DS_B ();
		      LR_A_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      LR_A_N_SR_X_DS_B ();
		      LR_A_N_SR_X_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      LR_A_N_SR_O_DS_B ();
		      LR_A_N_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      LR_A_SR_B ();
		      LR_A_SR_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_O_N_DS_B ();
		      LR_A_SR_O_N_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      LR_A_SR_O_DS_B ();
		      LR_A_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	    }
	    else
	    {			/* short, no begin */
	      dstincr--;
	      srcincr--;
	      if (endf)
	      {			/* short, end */
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      LR_A_NB_SR_O_DS_W ();
		      LR_A_NB_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      LR_A_N_SR_A_DS_W ();
		      LR_A_N_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      LR_A_N_SR_W ();
		      LR_A_N_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_A_N_DS_W ();
		      LR_A_SR_A_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      LR_A_SR_X_DS_W ();
		      LR_A_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      LR_A_NB_SR_A_DS_W ();
		      LR_A_NB_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      LR_A_SR_A_DS_W ();
		      LR_A_SR_A_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      LR_A_N_SR_X_DS_W ();
		      LR_A_N_SR_X_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      LR_A_N_SR_O_DS_W ();
		      LR_A_N_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      LR_A_SR_W ();
		      LR_A_SR_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_O_N_DS_W ();
		      LR_A_SR_O_N_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      LR_A_SR_O_DS_W ();
		      LR_A_SR_O_DS_E ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	      else
	      {			/* short only */
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      LR_A_NB_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      LR_A_N_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      LR_A_N_SR_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_A_N_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      LR_A_SR_X_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      LR_A_NB_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      LR_A_SR_A_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      LR_A_N_SR_X_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      LR_A_N_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      LR_A_SR_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_A_SR_O_N_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      LR_A_SR_O_DS_W ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	    }
	  }
	  else
	  {
	    dstincr--;
	    srcincr--;
	    if (endf)
	    {			/* just begin and end */
	      switch (op)
	      {
		case PIX_NOT (PIX_SRC | PIX_DST): 
		  do
		  {
		    LR_A_NB_SR_O_DS_B ();
		    LR_A_NB_SR_O_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		  do
		  {
		    LR_A_N_SR_A_DS_B ();
		    LR_A_N_SR_A_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT (PIX_SRC): 
		  do
		  {
		    LR_A_N_SR_B ();
		    LR_A_N_SR_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC & (PIX_NOT (PIX_DST)): 
		  do
		  {
		    LR_A_SR_A_N_DS_B ();
		    LR_A_SR_A_N_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC ^ PIX_DST: 
		  do
		  {
		    LR_A_SR_X_DS_B ();
		    LR_A_SR_X_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC & PIX_DST)): 
		  do
		  {
		    LR_A_NB_SR_A_DS_B ();
		    LR_A_NB_SR_A_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC & PIX_DST: 
		  do
		  {
		    LR_A_SR_A_DS_B ();
		    LR_A_SR_A_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		  do
		  {
		    LR_A_N_SR_X_DS_B ();
		    LR_A_N_SR_X_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		  do
		  {
		    LR_A_N_SR_O_DS_B ();
		    LR_A_N_SR_O_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC: 
		  do
		  {
		    LR_A_SR_B ();
		    LR_A_SR_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC | (PIX_NOT (PIX_DST)): 
		  do
		  {
		    LR_A_SR_O_N_DS_B ();
		    LR_A_SR_O_N_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC | PIX_DST: 
		  do
		  {
		    LR_A_SR_O_DS_B ();
		    LR_A_SR_O_DS_E ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
	      }
	    }
	    else
	    {			/* little rops */
	      switch (op)
	      {
		case PIX_NOT (PIX_SRC | PIX_DST): 
		  do
		  {
		    LR_A_NB_SR_O_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		  do
		  {
		    LR_A_N_SR_A_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT (PIX_SRC): 
		  do
		  {
		    LR_A_N_SR_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC & (PIX_NOT (PIX_DST)): 
		  do
		  {
		    LR_A_SR_A_N_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC ^ PIX_DST: 
		  do
		  {
		    LR_A_SR_X_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC & PIX_DST)): 
		  do
		  {
		    LR_A_NB_SR_A_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC & PIX_DST: 
		  do
		  {
		    LR_A_SR_A_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		  do
		  {
		    LR_A_N_SR_X_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		  do
		  {
		    LR_A_N_SR_O_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC: 
		  do
		  {
		    LR_A_SR_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC | (PIX_NOT (PIX_DST)): 
		  do
		  {
		    LR_A_SR_O_N_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
		case PIX_SRC | PIX_DST: 
		  do
		  {
		    LR_A_SR_O_DS_B ();
		    BUMP_BOTH ();
		  } while (--i != -1);
		  break;
	      }
	    }
	  }
	}
      }
      else
      {
	if (dskew > sskew)
	{			/*  dst skew is > src skew */
	  dist = dskew - sskew;
	  cdist = 16 - dist;
	  cdist2 = 16 + dist;
	  if (count > -1)
	  {			/* are there long words */
	    ct2 = ((count + 1) << 1);
	    if (sixtf)
	    {			/*  short words? */
	      ct2++;
	      if (sml)
	      {
		if (endf)
		{		/* all four */
		  ct2++;
		  dstincr -= ct2;
		  srcincr -= ct2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_D_NB_SR_O_DS_B ();
			LR_D_NB_SR_O_DS_LG ();
			LR_D_NB_SR_O_DS_W ();
			LR_D_NB_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_D_N_SR_A_DS_B ();
			LR_D_N_SR_A_DS_LG ();
			LR_D_N_SR_A_DS_W ();
			LR_D_N_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_D_N_SR_B ();
			LR_D_N_SR_LG ();
			LR_D_N_SR_W ();
			LR_D_N_SR_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_A_N_DS_B ();
			LR_D_SR_A_N_DS_LG ();
			LR_D_SR_A_N_DS_W ();
			LR_D_SR_A_N_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_D_SR_X_DS_B ();
			LR_D_SR_X_DS_LG ();
			LR_D_SR_X_DS_W ();
			LR_D_SR_X_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_D_NB_SR_A_DS_B ();
			LR_D_NB_SR_A_DS_LG ();
			LR_D_NB_SR_A_DS_W ();
			LR_D_NB_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_D_SR_A_DS_B ();
			LR_D_SR_A_DS_LG ();
			LR_D_SR_A_DS_W ();
			LR_D_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_D_N_SR_X_DS_B ();
			LR_D_N_SR_X_DS_LG ();
			LR_D_N_SR_X_DS_W ();
			LR_D_N_SR_X_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_D_N_SR_O_DS_B ();
			LR_D_N_SR_O_DS_LG ();
			LR_D_N_SR_O_DS_W ();
			LR_D_N_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_D_SR_B ();
			LR_D_SR_LG ();
			LR_D_SR_W ();
			LR_D_SR_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_O_N_DS_B ();
			LR_D_SR_O_N_DS_LG ();
			LR_D_SR_O_N_DS_W ();
			LR_D_SR_O_N_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_D_SR_O_DS_B ();
			LR_D_SR_O_DS_LG ();
			LR_D_SR_O_DS_W ();
			LR_D_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
		else
		{		/* begin, long, and short */
		  srcincr -= ct2;
		  ct2++;
		  dstincr -= ct2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_D_NB_SR_O_DS_B ();
			LR_D_NB_SR_O_DS_LG ();
			LR_D_NB_SR_O_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_D_N_SR_A_DS_B ();
			LR_D_N_SR_A_DS_LG ();
			LR_D_N_SR_A_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_D_N_SR_B ();
			LR_D_N_SR_LG ();
			LR_D_N_SR_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_A_N_DS_B ();
			LR_D_SR_A_N_DS_LG ();
			LR_D_SR_A_N_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_D_SR_X_DS_B ();
			LR_D_SR_X_DS_LG ();
			LR_D_SR_X_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_D_NB_SR_A_DS_B ();
			LR_D_NB_SR_A_DS_LG ();
			LR_D_NB_SR_A_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_D_SR_A_DS_B ();
			LR_D_SR_A_DS_LG ();
			LR_D_SR_A_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_D_N_SR_X_DS_B ();
			LR_D_N_SR_X_DS_LG ();
			LR_D_N_SR_X_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_D_N_SR_O_DS_B ();
			LR_D_N_SR_O_DS_LG ();
			LR_D_N_SR_O_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_D_SR_B ();
			LR_D_SR_LG ();
			LR_D_SR_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_O_N_DS_B ();
			LR_D_SR_O_N_DS_LG ();
			LR_D_SR_O_N_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_D_SR_O_DS_B ();
			LR_D_SR_O_DS_LG ();
			LR_D_SR_O_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
	      }
	      else
	      {			/* no begin piece */
		dstincr -= ct2;
		if (endf)
		{		/* long, short, end */
		  ct2++;
		  srcincr -= ct2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_D_NB_SR_O_DS_LG ();
			LR_D_NB_SR_O_DS_W ();
			LR_D_NB_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_D_N_SR_A_DS_LG ();
			LR_D_N_SR_A_DS_W ();
			LR_D_N_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_D_N_SR_LG ();
			LR_D_N_SR_W ();
			LR_D_N_SR_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_A_N_DS_LG ();
			LR_D_SR_A_N_DS_W ();
			LR_D_SR_A_N_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_D_SR_X_DS_LG ();
			LR_D_SR_X_DS_W ();
			LR_D_SR_X_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_D_NB_SR_A_DS_LG ();
			LR_D_NB_SR_A_DS_W ();
			LR_D_NB_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_D_SR_A_DS_LG ();
			LR_D_SR_A_DS_W ();
			LR_D_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_D_N_SR_X_DS_LG ();
			LR_D_N_SR_X_DS_W ();
			LR_D_N_SR_X_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_D_N_SR_O_DS_LG ();
			LR_D_N_SR_O_DS_W ();
			LR_D_N_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_D_SR_LG ();
			LR_D_SR_W ();
			LR_D_SR_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_O_N_DS_LG ();
			LR_D_SR_O_N_DS_W ();
			LR_D_SR_O_N_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_D_SR_O_DS_LG ();
			LR_D_SR_O_DS_W ();
			LR_D_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
		else
		{		/* long, short only */
		  srcincr -= ct2;
		  dstincr -= ct2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_D_NB_SR_O_DS_LG ();
			LR_D_NB_SR_O_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_D_N_SR_A_DS_LG ();
			LR_D_N_SR_A_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_D_N_SR_LG ();
			LR_D_N_SR_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_A_N_DS_LG ();
			LR_D_SR_A_N_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_D_SR_X_DS_LG ();
			LR_D_SR_X_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_D_NB_SR_A_DS_LG ();
			LR_D_NB_SR_A_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_D_SR_A_DS_LG ();
			LR_D_SR_A_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_D_N_SR_X_DS_LG ();
			LR_D_N_SR_X_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_D_N_SR_O_DS_LG ();
			LR_D_N_SR_O_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_D_SR_LG ();
			LR_D_SR_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_O_N_DS_LG ();
			LR_D_SR_O_N_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_D_SR_O_DS_LG ();
			LR_D_SR_O_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
	      }
	    }
	    else
	    {			/*  long only */
	      if (sml)
	      {
		ct2++;
		dstincr -= ct2;
		if (endf)
		{		/* begin, long and end */
		  srcincr -= ct2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_D_NB_SR_O_DS_B ();
			LR_D_NB_SR_O_DS_LG ();
			LR_D_NB_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_D_N_SR_A_DS_B ();
			LR_D_N_SR_A_DS_LG ();
			LR_D_N_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_D_N_SR_B ();
			LR_D_N_SR_LG ();
			LR_D_N_SR_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_A_N_DS_B ();
			LR_D_SR_A_N_DS_LG ();
			LR_D_SR_A_N_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_D_SR_X_DS_B ();
			LR_D_SR_X_DS_LG ();
			LR_D_SR_X_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_D_NB_SR_A_DS_B ();
			LR_D_NB_SR_A_DS_LG ();
			LR_D_NB_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_D_SR_A_DS_B ();
			LR_D_SR_A_DS_LG ();
			LR_D_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_D_N_SR_X_DS_B ();
			LR_D_N_SR_X_DS_LG ();
			LR_D_N_SR_X_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_D_N_SR_O_DS_B ();
			LR_D_N_SR_O_DS_LG ();
			LR_D_N_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_D_SR_B ();
			LR_D_SR_LG ();
			LR_D_SR_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_O_N_DS_B ();
			LR_D_SR_O_N_DS_LG ();
			LR_D_SR_O_N_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_D_SR_O_DS_B ();
			LR_D_SR_O_DS_LG ();
			LR_D_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
		else
		{		/* begin  and long */
		  srcincr -= --ct2;
		/* predecrement to take care of incrementing by dst */
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_D_NB_SR_O_DS_B ();
			LR_D_NB_SR_O_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_D_N_SR_A_DS_B ();
			LR_D_N_SR_A_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_D_N_SR_B ();
			LR_D_N_SR_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_A_N_DS_B ();
			LR_D_SR_A_N_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_D_SR_X_DS_B ();
			LR_D_SR_X_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_D_NB_SR_A_DS_B ();
			LR_D_NB_SR_A_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_D_SR_A_DS_B ();
			LR_D_SR_A_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_D_N_SR_X_DS_B ();
			LR_D_N_SR_X_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_D_N_SR_O_DS_B ();
			LR_D_N_SR_O_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_D_SR_B ();
			LR_D_SR_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_O_N_DS_B ();
			LR_D_SR_O_N_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_D_SR_O_DS_B ();
			LR_D_SR_O_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
	      }
	      else
	      {			/* long, only */
		dstincr -= ct2;
		if (endf)
		{		/* long,  end */
		  ct2++;
		  srcincr -= ct2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_D_NB_SR_O_DS_LG ();
			LR_D_NB_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_D_N_SR_A_DS_LG ();
			LR_D_N_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_D_N_SR_LG ();
			LR_D_N_SR_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_A_N_DS_LG ();
			LR_D_SR_A_N_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_D_SR_X_DS_LG ();
			LR_D_SR_X_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_D_NB_SR_A_DS_LG ();
			LR_D_NB_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_D_SR_A_DS_LG ();
			LR_D_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_D_N_SR_X_DS_LG ();
			LR_D_N_SR_X_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_D_N_SR_O_DS_LG ();
			LR_D_N_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_D_SR_LG ();
			LR_D_SR_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_O_N_DS_LG ();
			LR_D_SR_O_N_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_D_SR_O_DS_LG ();
			LR_D_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
		else
		{		/* long only */
		  srcincr -= ct2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_D_NB_SR_O_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_D_N_SR_A_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_D_N_SR_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_A_N_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_D_SR_X_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_D_NB_SR_A_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_D_SR_A_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_D_N_SR_X_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_D_N_SR_O_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_D_SR_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_O_N_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_D_SR_O_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
	      }
	    }
	  }
	  else
	  {			/* no long words */
	    cdist2 = dist;	/* optimization: puts dist in regist. */
	    if (sixtf)
	    {			/*  short words? */
	      if (sml)
	      {
		dstincr -= 2;
		if (endf)
		{		/* all three */
		  srcincr -= 2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_D_NB_SR_O_DS_B ();
			LR_D_NB_SR_O_DS_W_O ();
			LR_D_NB_SR_O_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_D_N_SR_A_DS_B ();
			LR_D_N_SR_A_DS_W_O ();
			LR_D_N_SR_A_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_D_N_SR_B ();
			LR_D_N_SR_W_O ();
			LR_D_N_SR_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_A_N_DS_B ();
			LR_D_SR_A_N_DS_W_O ();
			LR_D_SR_A_N_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_D_SR_X_DS_B ();
			LR_D_SR_X_DS_W_O ();
			LR_D_SR_X_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_D_NB_SR_A_DS_B ();
			LR_D_NB_SR_A_DS_W_O ();
			LR_D_NB_SR_A_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_D_SR_A_DS_B ();
			LR_D_SR_A_DS_W_O ();
			LR_D_SR_A_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_D_N_SR_X_DS_B ();
			LR_D_N_SR_X_DS_W_O ();
			LR_D_N_SR_X_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_D_N_SR_O_DS_B ();
			LR_D_N_SR_O_DS_W_O ();
			LR_D_N_SR_O_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_D_SR_B ();
			LR_D_SR_W_O ();
			LR_D_SR_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_O_N_DS_B ();
			LR_D_SR_O_N_DS_W_O ();
			LR_D_SR_O_N_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_D_SR_O_DS_B ();
			LR_D_SR_O_DS_W_O ();
			LR_D_SR_O_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
		else
		{		/* begin and short */
		  srcincr--;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_D_NB_SR_O_DS_B_O ();
			LR_D_NB_SR_O_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_D_N_SR_A_DS_B_O ();
			LR_D_N_SR_A_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_D_N_SR_B_O ();
			LR_D_N_SR_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_A_N_DS_B_O ();
			LR_D_SR_A_N_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_D_SR_X_DS_B_O ();
			LR_D_SR_X_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_D_NB_SR_A_DS_B_O ();
			LR_D_NB_SR_A_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_D_SR_A_DS_B_O ();
			LR_D_SR_A_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_D_N_SR_X_DS_B_O ();
			LR_D_N_SR_X_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_D_N_SR_O_DS_B_O ();
			LR_D_N_SR_O_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_D_SR_B_O ();
			LR_D_SR_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_O_N_DS_B_O ();
			LR_D_SR_O_N_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_D_SR_O_DS_B_O ();
			LR_D_SR_O_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
	      }
	      else
	      {			/* short, no begin */
		dstincr--;
		if (endf)
		{		/* short, end */
		  srcincr -= 2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_D_NB_SR_O_DS_W_O ();
			LR_D_NB_SR_O_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_D_N_SR_A_DS_W_O ();
			LR_D_N_SR_A_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_D_N_SR_W_O ();
			LR_D_N_SR_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_A_N_DS_W_O ();
			LR_D_SR_A_N_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_D_SR_X_DS_W_O ();
			LR_D_SR_X_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_D_NB_SR_A_DS_W_O ();
			LR_D_NB_SR_A_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_D_SR_A_DS_W_O ();
			LR_D_SR_A_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_D_N_SR_X_DS_W_O ();
			LR_D_N_SR_X_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_D_N_SR_O_DS_W_O ();
			LR_D_N_SR_O_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_D_SR_W_O ();
			LR_D_SR_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_O_N_DS_W_O ();
			LR_D_SR_O_N_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_D_SR_O_DS_W_O ();
			LR_D_SR_O_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
		else
		{		/* short only */
		  srcincr--;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_D_NB_SR_O_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_D_N_SR_A_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_D_N_SR_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_A_N_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_D_SR_X_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_D_NB_SR_A_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_D_SR_A_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_D_N_SR_X_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_D_N_SR_O_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_D_SR_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_D_SR_O_N_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_D_SR_O_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
	      }
	    }
	    else
	    {
	      dstincr--;
	      if (endf)
	      {			/* just begin and end */
		srcincr--;
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      LR_D_NB_SR_O_DS_B_O ();
		      LR_D_NB_SR_O_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      LR_D_N_SR_A_DS_B_O ();
		      LR_D_N_SR_A_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      LR_D_N_SR_B_O ();
		      LR_D_N_SR_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_D_SR_A_N_DS_B_O ();
		      LR_D_SR_A_N_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      LR_D_SR_X_DS_B_O ();
		      LR_D_SR_X_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      LR_D_NB_SR_A_DS_B_O ();
		      LR_D_NB_SR_A_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      LR_D_SR_A_DS_B_O ();
		      LR_D_SR_A_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      LR_D_N_SR_X_DS_B_O ();
		      LR_D_N_SR_X_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      LR_D_N_SR_O_DS_B_O ();
		      LR_D_N_SR_O_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      LR_D_SR_B_O ();
		      LR_D_SR_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_D_SR_O_N_DS_B_O ();
		      LR_D_SR_O_N_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      LR_D_SR_O_DS_B_O ();
		      LR_D_SR_O_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	      else
	      {			/* little rops */
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      LR_D_NB_SR_O_DS_B_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      LR_D_N_SR_A_DS_B_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      LR_D_N_SR_B_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_D_SR_A_N_DS_B_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      LR_D_SR_X_DS_B_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      LR_D_NB_SR_A_DS_B_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      LR_D_SR_A_DS_B_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      LR_D_N_SR_X_DS_B_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      LR_D_N_SR_O_DS_B_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      LR_D_SR_B_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_D_SR_O_N_DS_B_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      LR_D_SR_O_DS_B_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	    }
	  }
	}
	else
	{			/* src skew is > dst skew */
	/*  dist is used differently to make full use of registers */
	  cdist = sskew - dskew;
	  dist = 16 - cdist;
	  cdist2 = 32 - cdist;
	  if (count > -1)
	  {			/* are there long words */
	    ct2 = ((count + 1) << 1);
	    if (sixtf)
	    {			/*  short words? */
	      if (sml)
	      {
		ct2 += 2;
		dstincr -= ct2;
		if (endf)
		{		/* all four */
		  srcincr -= ++ct2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_S_NB_SR_O_DS_B ();
			LR_S_NB_SR_O_DS_LG ();
			LR_S_NB_SR_O_DS_W ();
			LR_S_NB_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_S_N_SR_A_DS_B ();
			LR_S_N_SR_A_DS_LG ();
			LR_S_N_SR_A_DS_W ();
			LR_S_N_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_S_N_SR_B ();
			LR_S_N_SR_LG ();
			LR_S_N_SR_W ();
			LR_S_N_SR_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_A_N_DS_B ();
			LR_S_SR_A_N_DS_LG ();
			LR_S_SR_A_N_DS_W ();
			LR_S_SR_A_N_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_S_SR_X_DS_B ();
			LR_S_SR_X_DS_LG ();
			LR_S_SR_X_DS_W ();
			LR_S_SR_X_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_S_NB_SR_A_DS_B ();
			LR_S_NB_SR_A_DS_LG ();
			LR_S_NB_SR_A_DS_W ();
			LR_S_NB_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_S_SR_A_DS_B ();
			LR_S_SR_A_DS_LG ();
			LR_S_SR_A_DS_W ();
			LR_S_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_S_N_SR_X_DS_B ();
			LR_S_N_SR_X_DS_LG ();
			LR_S_N_SR_X_DS_W ();
			LR_S_N_SR_X_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_S_N_SR_O_DS_B ();
			LR_S_N_SR_O_DS_LG ();
			LR_S_N_SR_O_DS_W ();
			LR_S_N_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_S_SR_B ();
			LR_S_SR_LG ();
			LR_S_SR_W ();
			LR_S_SR_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_O_N_DS_B ();
			LR_S_SR_O_N_DS_LG ();
			LR_S_SR_O_N_DS_W ();
			LR_S_SR_O_N_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_S_SR_O_DS_B ();
			LR_S_SR_O_DS_LG ();
			LR_S_SR_O_DS_W ();
			LR_S_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
		else
		{		/* begin, long, and short */
		  srcincr -= ct2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_S_NB_SR_O_DS_B ();
			LR_S_NB_SR_O_DS_LG ();
			LR_S_NB_SR_O_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_S_N_SR_A_DS_B ();
			LR_S_N_SR_A_DS_LG ();
			LR_S_N_SR_A_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_S_N_SR_B ();
			LR_S_N_SR_LG ();
			LR_S_N_SR_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_A_N_DS_B ();
			LR_S_SR_A_N_DS_LG ();
			LR_S_SR_A_N_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_S_SR_X_DS_B ();
			LR_S_SR_X_DS_LG ();
			LR_S_SR_X_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_S_NB_SR_A_DS_B ();
			LR_S_NB_SR_A_DS_LG ();
			LR_S_NB_SR_A_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_S_SR_A_DS_B ();
			LR_S_SR_A_DS_LG ();
			LR_S_SR_A_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_S_N_SR_X_DS_B ();
			LR_S_N_SR_X_DS_LG ();
			LR_S_N_SR_X_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_S_N_SR_O_DS_B ();
			LR_S_N_SR_O_DS_LG ();
			LR_S_N_SR_O_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_S_SR_B ();
			LR_S_SR_LG ();
			LR_S_SR_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_O_N_DS_B ();
			LR_S_SR_O_N_DS_LG ();
			LR_S_SR_O_N_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_S_SR_O_DS_B ();
			LR_S_SR_O_DS_LG ();
			LR_S_SR_O_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
	      }
	      else
	      {			/* long, short, no begin */
		ct2++;
		dstincr -= ct2;
		if (endf)
		{		/* long, short, end */
		  srcincr -= ++ct2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_S_NB_SR_O_DS_LG ();
			LR_S_NB_SR_O_DS_W ();
			LR_S_NB_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_S_N_SR_A_DS_LG ();
			LR_S_N_SR_A_DS_W ();
			LR_S_N_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_S_N_SR_LG ();
			LR_S_N_SR_W ();
			LR_S_N_SR_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_A_N_DS_LG ();
			LR_S_SR_A_N_DS_W ();
			LR_S_SR_A_N_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_S_SR_X_DS_LG ();
			LR_S_SR_X_DS_W ();
			LR_S_SR_X_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_S_NB_SR_A_DS_LG ();
			LR_S_NB_SR_A_DS_W ();
			LR_S_NB_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_S_SR_A_DS_LG ();
			LR_S_SR_A_DS_W ();
			LR_S_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_S_N_SR_X_DS_LG ();
			LR_S_N_SR_X_DS_W ();
			LR_S_N_SR_X_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_S_N_SR_O_DS_LG ();
			LR_S_N_SR_O_DS_W ();
			LR_S_N_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_S_SR_LG ();
			LR_S_SR_W ();
			LR_S_SR_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_O_N_DS_LG ();
			LR_S_SR_O_N_DS_W ();
			LR_S_SR_O_N_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_S_SR_O_DS_LG ();
			LR_S_SR_O_DS_W ();
			LR_S_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
		else
		{		/* long, short only */
		  srcincr -= ct2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_S_NB_SR_O_DS_LG ();
			LR_S_NB_SR_O_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_S_N_SR_A_DS_LG ();
			LR_S_N_SR_A_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_S_N_SR_LG ();
			LR_S_N_SR_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_A_N_DS_LG ();
			LR_S_SR_A_N_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_S_SR_X_DS_LG ();
			LR_S_SR_X_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_S_NB_SR_A_DS_LG ();
			LR_S_NB_SR_A_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_S_SR_A_DS_LG ();
			LR_S_SR_A_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_S_N_SR_X_DS_LG ();
			LR_S_N_SR_X_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_S_N_SR_O_DS_LG ();
			LR_S_N_SR_O_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_S_SR_LG ();
			LR_S_SR_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_O_N_DS_LG ();
			LR_S_SR_O_N_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_S_SR_O_DS_LG ();
			LR_S_SR_O_DS_W ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
	      }
	    }
	    else
	    {			/*  long only */
	      if (sml)
	      {
		ct2++;
		dstincr -= ct2;
		if (endf)
		{		/* begin, long, end */
		  srcincr -= ++ct2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_S_NB_SR_O_DS_B ();
			LR_S_NB_SR_O_DS_LG ();
			LR_S_NB_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_S_N_SR_A_DS_B ();
			LR_S_N_SR_A_DS_LG ();
			LR_S_N_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_S_N_SR_B ();
			LR_S_N_SR_LG ();
			LR_S_N_SR_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_A_N_DS_B ();
			LR_S_SR_A_N_DS_LG ();
			LR_S_SR_A_N_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_S_SR_X_DS_B ();
			LR_S_SR_X_DS_LG ();
			LR_S_SR_X_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_S_NB_SR_A_DS_B ();
			LR_S_NB_SR_A_DS_LG ();
			LR_S_NB_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_S_SR_A_DS_B ();
			LR_S_SR_A_DS_LG ();
			LR_S_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_S_N_SR_X_DS_B ();
			LR_S_N_SR_X_DS_LG ();
			LR_S_N_SR_X_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_S_N_SR_O_DS_B ();
			LR_S_N_SR_O_DS_LG ();
			LR_S_N_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_S_SR_B ();
			LR_S_SR_LG ();
			LR_S_SR_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_O_N_DS_B ();
			LR_S_SR_O_N_DS_LG ();
			LR_S_SR_O_N_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_S_SR_O_DS_B ();
			LR_S_SR_O_DS_LG ();
			LR_S_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
		else
		{		/* begin, and long */
		  srcincr -= ct2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_S_NB_SR_O_DS_B ();
			LR_S_NB_SR_O_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_S_N_SR_A_DS_B ();
			LR_S_N_SR_A_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_S_N_SR_B ();
			LR_S_N_SR_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_A_N_DS_B ();
			LR_S_SR_A_N_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_S_SR_X_DS_B ();
			LR_S_SR_X_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_S_NB_SR_A_DS_B ();
			LR_S_NB_SR_A_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_S_SR_A_DS_B ();
			LR_S_SR_A_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_S_N_SR_X_DS_B ();
			LR_S_N_SR_X_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_S_N_SR_O_DS_B ();
			LR_S_N_SR_O_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_S_SR_B ();
			LR_S_SR_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_O_N_DS_B ();
			LR_S_SR_O_N_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_S_SR_O_DS_B ();
			LR_S_SR_O_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
	      }
	      else
	      {			/* long, no short or begin */
		dstincr -= ct2;
		if (endf)
		{		/* long, end */
		  srcincr -= ++ct2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_S_NB_SR_O_DS_LG ();
			LR_S_NB_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_S_N_SR_A_DS_LG ();
			LR_S_N_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_S_N_SR_LG ();
			LR_S_N_SR_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_A_N_DS_LG ();
			LR_S_SR_A_N_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_S_SR_X_DS_LG ();
			LR_S_SR_X_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_S_NB_SR_A_DS_LG ();
			LR_S_NB_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_S_SR_A_DS_LG ();
			LR_S_SR_A_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_S_N_SR_X_DS_LG ();
			LR_S_N_SR_X_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_S_N_SR_O_DS_LG ();
			LR_S_N_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_S_SR_LG ();
			LR_S_SR_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_O_N_DS_LG ();
			LR_S_SR_O_N_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_S_SR_O_DS_LG ();
			LR_S_SR_O_DS_E ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
		else
		{		/* long only */
		  srcincr -= ct2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_S_NB_SR_O_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_S_N_SR_A_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_S_N_SR_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_A_N_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_S_SR_X_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_S_NB_SR_A_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_S_SR_A_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_S_N_SR_X_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_S_N_SR_O_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_S_SR_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_O_N_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_S_SR_O_DS_LG ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
	      }
	    }
	  }
	  else
	  {			/* no long words */
	    cdist2 = dist;
	    if (sixtf)
	    {			/*  short words? */
	      if (sml)
	      {
		dstincr -= 2;
		if (endf)
		{		/* all four */
		  srcincr -= 3;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_S_NB_SR_O_DS_B_O ();
			LR_S_NB_SR_O_DS_W_O ();
			LR_S_NB_SR_O_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_S_N_SR_A_DS_B_O ();
			LR_S_N_SR_A_DS_W_O ();
			LR_S_N_SR_A_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_S_N_SR_B_O ();
			LR_S_N_SR_W_O ();
			LR_S_N_SR_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_A_N_DS_B_O ();
			LR_S_SR_A_N_DS_W_O ();
			LR_S_SR_A_N_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_S_SR_X_DS_B_O ();
			LR_S_SR_X_DS_W_O ();
			LR_S_SR_X_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_S_NB_SR_A_DS_B_O ();
			LR_S_NB_SR_A_DS_W_O ();
			LR_S_NB_SR_A_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_S_SR_A_DS_B_O ();
			LR_S_SR_A_DS_W_O ();
			LR_S_SR_A_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_S_N_SR_X_DS_B_O ();
			LR_S_N_SR_X_DS_W_O ();
			LR_S_N_SR_X_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_S_N_SR_O_DS_B_O ();
			LR_S_N_SR_O_DS_W_O ();
			LR_S_N_SR_O_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_S_SR_B_O ();
			LR_S_SR_W_O ();
			LR_S_SR_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_O_N_DS_B_O ();
			LR_S_SR_O_N_DS_W_O ();
			LR_S_SR_O_N_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_S_SR_O_DS_B_O ();
			LR_S_SR_O_DS_W_O ();
			LR_S_SR_O_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
		else
		{		/* begin and short */
		  srcincr -= 2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_S_NB_SR_O_DS_B_O ();
			LR_S_NB_SR_O_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_S_N_SR_A_DS_B_O ();
			LR_S_N_SR_A_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_S_N_SR_B_O ();
			LR_S_N_SR_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_A_N_DS_B_O ();
			LR_S_SR_A_N_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_S_SR_X_DS_B_O ();
			LR_S_SR_X_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_S_NB_SR_A_DS_B_O ();
			LR_S_NB_SR_A_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_S_SR_A_DS_B_O ();
			LR_S_SR_A_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_S_N_SR_X_DS_B_O ();
			LR_S_N_SR_X_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_S_N_SR_O_DS_B_O ();
			LR_S_N_SR_O_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_S_SR_B_O ();
			LR_S_SR_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_O_N_DS_B_O ();
			LR_S_SR_O_N_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_S_SR_O_DS_B_O ();
			LR_S_SR_O_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
	      }
	      else
	      {			/* , short */
		dstincr--;
		if (endf)
		{		/* short, end */
		  srcincr -= 2;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_S_NB_SR_O_DS_W_O ();
			LR_S_NB_SR_O_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_S_N_SR_A_DS_W_O ();
			LR_S_N_SR_A_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_S_N_SR_W_O ();
			LR_S_N_SR_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_A_N_DS_W_O ();
			LR_S_SR_A_N_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_S_SR_X_DS_W_O ();
			LR_S_SR_X_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_S_NB_SR_A_DS_W_O ();
			LR_S_NB_SR_A_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_S_SR_A_DS_W_O ();
			LR_S_SR_A_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_S_N_SR_X_DS_W_O ();
			LR_S_N_SR_X_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_S_N_SR_O_DS_W_O ();
			LR_S_N_SR_O_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_S_SR_W_O ();
			LR_S_SR_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_O_N_DS_W_O ();
			LR_S_SR_O_N_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_S_SR_O_DS_W_O ();
			LR_S_SR_O_DS_E_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
		else
		{		/* short only */
		  srcincr--;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_S_NB_SR_O_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_S_N_SR_A_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_S_N_SR_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_A_N_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_S_SR_X_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_S_NB_SR_A_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_S_SR_A_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_S_N_SR_X_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_S_N_SR_O_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_S_SR_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_O_N_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_S_SR_O_DS_W_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
	      }
	    }
	    else
	    {			/* no middle */
	      if (endf)
	      {			/* just begin and end */
		srcincr -= 2;
		dstincr--;
		switch (op)
		{
		  case PIX_NOT (PIX_SRC | PIX_DST): 
		    do
		    {
		      LR_S_NB_SR_O_DS_B_O ();
		      LR_S_NB_SR_O_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		    do
		    {
		      LR_S_N_SR_A_DS_B_O ();
		      LR_S_N_SR_A_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT (PIX_SRC): 
		    do
		    {
		      LR_S_N_SR_B_O ();
		      LR_S_N_SR_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_S_SR_A_N_DS_B_O ();
		      LR_S_SR_A_N_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC ^ PIX_DST: 
		    do
		    {
		      LR_S_SR_X_DS_B_O ();
		      LR_S_SR_X_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC & PIX_DST)): 
		    do
		    {
		      LR_S_NB_SR_A_DS_B_O ();
		      LR_S_NB_SR_A_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC & PIX_DST: 
		    do
		    {
		      LR_S_SR_A_DS_B_O ();
		      LR_S_SR_A_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		    do
		    {
		      LR_S_N_SR_X_DS_B_O ();
		      LR_S_N_SR_X_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		    do
		    {
		      LR_S_N_SR_O_DS_B_O ();
		      LR_S_N_SR_O_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC: 
		    do
		    {
		      LR_S_SR_B_O ();
		      LR_S_SR_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | (PIX_NOT (PIX_DST)): 
		    do
		    {
		      LR_S_SR_O_N_DS_B_O ();
		      LR_S_SR_O_N_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		  case PIX_SRC | PIX_DST: 
		    do
		    {
		      LR_S_SR_O_DS_B_O ();
		      LR_S_SR_O_DS_E_O ();
		      BUMP_BOTH ();
		    } while (--i != -1);
		    break;
		}
	      }
	      else
	      {
		{		/* little rops */
		  srcincr--;
		  dstincr--;
		  switch (op)
		  {
		    case PIX_NOT (PIX_SRC | PIX_DST): 
		      do
		      {
			LR_S_NB_SR_O_DS_B_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) & PIX_DST: 
		      do
		      {
			LR_S_N_SR_A_DS_B_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT (PIX_SRC): 
		      do
		      {
			LR_S_N_SR_B_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_A_N_DS_B_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC ^ PIX_DST: 
		      do
		      {
			LR_S_SR_X_DS_B_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC & PIX_DST)): 
		      do
		      {
			LR_S_NB_SR_A_DS_B_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC & PIX_DST: 
		      do
		      {
			LR_S_SR_A_DS_B_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_NOT ((PIX_SRC ^ PIX_DST)): 
		      do
		      {
			LR_S_N_SR_X_DS_B_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case (PIX_NOT (PIX_SRC)) | PIX_DST: 
		      do
		      {
			LR_S_N_SR_O_DS_B_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC: 
		      do
		      {
			LR_S_SR_B_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | (PIX_NOT (PIX_DST)): 
		      do
		      {
			LR_S_SR_O_N_DS_B_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		    case PIX_SRC | PIX_DST: 
		      do
		      {
			LR_S_SR_O_DS_B_O ();
			BUMP_BOTH ();
		      } while (--i != -1);
		      break;
		  }
		}
	      }
	    }
	  }
	}
      }
    }
  }
  return (0);
}

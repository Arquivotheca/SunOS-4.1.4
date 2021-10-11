#ifndef lint
static char         sccsid[] = "@(#)cg9_rop.c	1.1 94/10/31 SMI";
#endif

/* Copyright 1989 Sun Microsystems, Inc. */

#include <pixrect/cg9var.h>		/* CG9*, cg9_d(pr)	 */
#include <pixrect/mem32_var.h>		/* mem_ops		 */
#include <pixrect/pr_planegroups.h>	/* PIXPG*		 */

/* all these includes just to get WINDBLCURRENT defined */

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>	/* WINDBLCURRENT	 */

#define	CG9_BITBLT_WIDTH	1024	/* hardware fifo size	 */

cg9_rop(dpr, dx, dy, w, h, op, spr, sx, sy)
Pixrect            *dpr,
                   *spr;
int                 dx,
                    dy,
                    w,
                    h,
                    op,
                    sx,
                    sy;
{
    Pixrect             smempr,
                        dmempr;

#ifdef	KERNEL
#include "cgnine.h"

#if NCGNINE > 0
#define	ioctl	cgnineioctl

    if (dpr->pr_depth > 1 || (spr && spr->pr_depth > 1))
    {
	printf("kernel: cg9_rop error: attempt at 32 bit rop\n");
	return 0;
    }
#else					/* NCGNINE !> 0 */
#define ioctl
#endif					/* NCGNINE > 0 */

#endif					/* KERNEL */

#ifndef	KERNEL
    /* Let mem_rop handle the overlay planes. */

    if (dpr->pr_depth == 32)
    {
	int                 i,
			    j,
			    end,
			    incr,
			    oper,
			    planes,
			    width1,
			    width2,
			    dst_line,
			    src_line,
			    dbl_flag,
			    dst_bytes,
			    dst_indexed,
			    original_planes,
			    restore_dbl_buf;
	unsigned int       *dst;
	unsigned int       *dbl_reg;
	struct mprp32_data  dst_mem32_pr_data;
	struct mprp32_data  src_mem32_pr_data;
	struct cg9_control_sp *cg9_ctrl;

	/*
	 * Big time kludge for cg9 double buffer model to exist with the
	 * current sunview dbl_buffering model.  Basically, we see if we are
	 * the current dbl buf window, if not turn off dbl buffering for the
	 * duration of our operation.
	 */

	dbl_flag = 0;
	if (dpr->pr_ops != &mem_ops && cg9_d(dpr)->real_windowfd >= 0)
	{
	    struct rect         dbl_rect;

	    ioctl(cg9_d(dpr)->real_windowfd, WINDBLCURRENT, &dbl_rect);
	    restore_dbl_buf = !(dbl_rect.r_width || dbl_rect.r_height);
	    if (!restore_dbl_buf)
	    {
		dbl_flag = 1;
		dbl_reg = &cg9_d(dpr)->ctrl_sp->dbl_reg;
		restore_dbl_buf = *dbl_reg;
		*dbl_reg = 0;
	    }
	}

	/*
	 * Screen to screen copy means we can use the blt hardware. Null spr
	 * with appropriate op means we can seed a line and then blt. Double
	 * buffering and bitblt have trouble.
	 */

	/*
	 * New case for PHIGS performance: if the source pixrect is null or
	 * the source and destination are in the same double buffer, then it
	 * is okay to use the bitblt.
	 */

	if (dpr->pr_ops != &mem_ops || mpr_d(dpr)->md_flags & MP_PLANEMASK)
	    planes = mprp_d(dpr)->planes & PIX_ALL_PLANES;
	else
	    planes = PIX_ALL_PLANES;

	dst_indexed = (dpr->pr_ops != &mem_ops && cg9_d(dpr)->windowfd >= -1 &&
	    strcmp(cg9_d(dpr)->cms.cms_name,"monochrome"));
	oper = PIX_OP(op);		/* get just the operation portion */

	if ((dpr->pr_ops != &mem_ops) &&
	    (planes == PIX_ALL_PLANES ||
	    (dst_indexed && planes == cg9_d(dpr)->cms.cms_size - 1)) &&
	    ((spr && spr->pr_ops != &mem_ops && oper == PIX_SRC) ||
	    (!spr && (oper == PIX_SRC || oper == PIX_SET || oper == PIX_CLR)))&&
	    (!(cg9_d(dpr)->ctrl_sp->dbl_reg & CG9_ENB_12) ||
	    (!spr || (((cg9_d(dpr)->ctrl_sp->dbl_reg & CG9_READ_B) != 0) &&
	    ((cg9_d(dpr)->ctrl_sp->dbl_reg & CG9_NO_WRITE_B) == 0)) ||
	    (((cg9_d(dpr)->ctrl_sp->dbl_reg & CG9_READ_B) == 0) &&
	    ((cg9_d(dpr)->ctrl_sp->dbl_reg & CG9_NO_WRITE_A) == 0)))))
	{

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
	    {
		if (dbl_flag)
		    *dbl_reg = restore_dbl_buf;
		return 0;
	    }

	    /* translate the origin */

	    dx += mprp_d(dpr)->mpr.md_offset.x;
	    dy += mprp_d(dpr)->mpr.md_offset.y;

	    if (spr)
	    {
		sx += mprp_d(spr)->mpr.md_offset.x;
		sy += mprp_d(spr)->mpr.md_offset.y;
	    }

	    /*
	     * Hardware design decision to have 1024 rather than 2048 FIFO
	     * for blt.  Also, hardware counter halts on 0 to -1 transistion
	     * so decrease w by 1 if using hardware.
	     */

	    if (--w > CG9_BITBLT_WIDTH)
	    {
		width1 = CG9_BITBLT_WIDTH;
		width2 = w - CG9_BITBLT_WIDTH;
	    }
	    else
	    {
		width1 = w;
		width2 = 0;
	    }

	    /*
	     * Because of overlapping copies, must decide bottom up or top
	     * down. If null source, then initialize the first destination
	     * line as the source and start the copy from the second line.
	     */

	    if (sy > dy)
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
	    else
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

	    /* data structure and destination memory address */

	    dst = (unsigned int *) mprp_d(dpr)->mpr.md_image;
	    dst_bytes = (int) mprp_d(dpr)->mpr.md_linebytes / sizeof(int);

	    cg9_ctrl = cg9_d(dpr)->ctrl_sp;
	    original_planes = cg9_ctrl->plane_mask;

	    /* if null source, then initialize a fake source line */

	    if (!spr)
	    {
		switch (oper)
		{
		    case PIX_SET:
			op = PIX_ALL_PLANES;
			break;
		    case PIX_CLR:
			op = 0;
			break;
		    case PIX_SRC:
			op = PIX_OPCOLOR(op);
		}

		if (dst_indexed)
		{
		    Pixrect            *save_pr;

		    op &= planes;
		    save_pr = dpr;
		    CG9_PR_TO_MEM32(dpr, dmempr, dst_mem32_pr_data);
		    op = mem32_get_true(dpr, op, planes);
		    dpr = save_pr;
		    cg9_ctrl->plane_mask = PIX_ALL_PLANES;
		}
		else
		    cg9_ctrl->plane_mask = planes;

		for (i = w; i >= 0; i--)
		    dst[sx + i + src_line * dst_bytes] = op;
	    }

	    /* Overlapping copy must decide left to right or right to left. */

	    if (sx > dx)
	    {
		cg9_ctrl->bitblt &= ~CG9_BACKWARD;
		i = 0;
		j = width1;
	    }
	    else
	    {
		cg9_ctrl->bitblt |= CG9_BACKWARD;
		i = w;
		j = width2;
	    }

	    /* loop for the "height" times */

	    for (; dst_line != end; src_line += incr, dst_line += incr)
	    {
		while (cg9_ctrl->bitblt & CG9_GO);	/* spin till ready */
		cg9_ctrl->pixel_count = width1;
		cg9_ctrl->src = sx + i + src_line * dst_bytes;
		cg9_ctrl->dest = dx + i + dst_line * dst_bytes;
		cg9_ctrl->bitblt |= CG9_GO;

		if (width2)		/* do only if necessary */
		{
		    while (cg9_ctrl->bitblt & CG9_GO);	/* spin till ready */
		    cg9_ctrl->pixel_count = width2;
		    cg9_ctrl->src = sx + j + src_line * dst_bytes;
		    cg9_ctrl->dest = dx + j + dst_line * dst_bytes;
		    cg9_ctrl->bitblt |= CG9_GO;
		}
	    }
	    while (cg9_ctrl->bitblt & CG9_GO);	/* spin till done */
	    cg9_ctrl->plane_mask = original_planes;
	    if (dbl_flag)
		*dbl_reg = restore_dbl_buf;
	    return 0;
	}

	/* 32-bit destination but can't bitblt. */

	CG9_PR_TO_MEM32(dpr, dmempr, dst_mem32_pr_data);
	CG9_PR_TO_MEM32(spr, smempr, src_mem32_pr_data);
	i = pr_rop(dpr, dx, dy, w, h, op, spr, sx, sy);
	if (dbl_flag)
	    *dbl_reg = restore_dbl_buf;
	return i;
    }
#endif					/* KERNEL */

    CG9_PR_TO_MEM(dpr, dmempr);
    CG9_PR_TO_MEM(spr, smempr);
    return pr_rop(dpr, dx, dy, w, h, op, spr, sx, sy);
}

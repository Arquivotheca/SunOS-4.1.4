#ifndef lint
static char         sccsid[] = "@(#)mem32_rop.c	1.1 94/10/31 SMI";
#endif

/* Copyright 1989 Sun Microsystems, Inc. */

#include <pixrect/mem32_var.h>		/* mem32_d(pr)		 */
#include <pixrect/pr_planegroups.h>	/* PIXPG*		 */
#include <sunwindow/cms.h>		/* colormapseg, cms_map	 */

/* macro to change a mem32 pixrect into a memory pixrect */

#define PR_TO_MEM(src, mem)						\
    if (src && src->pr_ops != &mem_ops)					\
    {									\
	mem.pr_ops	= &mem_ops;					\
	mem.pr_size	= src->pr_size;					\
	mem.pr_depth	= src->pr_depth;				\
	mem.pr_data	= (char *) &mprp32_d(src)->mprp;		\
	src		= &mem;						\
    }

/*
 * The pixrect may be nill, or have depth 1, 8, or 32.  The raster
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

#define	SRC_32								\
    src_val = *((int *) (src + (wsx<<2) + src_offset));			\
    CONV(src_indexed, src_cms_addr, src_cms_size, src_cmap, src_val);

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
		break;							\
									\
	    case 32:							\
		ROP_LOOP(SRC_32, OPER);					\
		break;							\
	}								\
    }									\
    else ROP_LOOP(SRC_OP, OPER)

/*
 * CONV will convert it's operand from a true color value to an 8-bit
 * indexed value.
 */

#define	CONV(indexed, cms_addr, cms_size, cmap, x)			\
    if (indexed)							\
    {									\
	int			index;					\
	unsigned	int	true_color;				\
									\
	x &= PIX_ALL_PLANES;						\
	true_color = x;							\
	for (index = cms_addr; index < cms_size; index++)		\
	{								\
	    if (x == cmap[index])					\
	    {								\
		x = index;						\
		break;							\
	    }								\
	}								\
	if (fullscreen || index == cms_size)				\
	{								\
	    if (true_color == cmap[0])					\
		x = 0;							\
	    else							\
		if (true_color == cmap[MEM32_8BIT_CMAPSIZE - 1] ||	\
		    x==true_color)					\
		    x = MEM32_8BIT_CMAPSIZE - 1;			\
	}								\
    }

#define	ROP_LOOP(SRC, OPER)						\
    for (; dst_line != end; src_line += incr, dst_line += incr)		\
    {									\
	if (j == 1)							\
	{								\
	    wsx = sx;							\
	    wdx = dx;							\
	}								\
	else								\
	{								\
	    wsx = sx + w;						\
	    wdx = dx + w;						\
	}								\
									\
	src_offset = src_line * src_bytes;				\
	dst_offset = dst_line * dst_bytes;				\
									\
	for (; wsx != i; wsx += j,wdx += j)				\
	{								\
	    old_dst_val = dst[wdx + dst_offset];			\
	    CONV(dst_indexed, dst_cms_addr, dst_cms_size, dst_cmap,	\
		old_dst_val);						\
	    dst_val = old_dst_val;					\
	    SRC;							\
	    OPER;							\
	    dst[wdx + dst_offset] = (dst_indexed) 			\
		? dst_cmap[(not_planes&old_dst_val) | (planes&dst_val)]	\
		: (not_planes&old_dst_val) | (planes&dst_val);		\
	}								\
    }

mem32_rop(dpr, dx, dy, w, h, op, spr, sx, sy)
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

    int                 i,
			j,
			oper,
			incr,
			end,
			planes,
			not_planes,
			fullscreen,
			dst_indexed,
			dst_bytes,
			dst_line,
			src_indexed,
			src_bytes,
			src_line,
			dst_cms_addr,
			dst_cms_size,
			src_cms_addr,
			src_cms_size;
    struct mprp_data   *src_data,
		       *dst_data;
    int                 wsx,
			wdx;
    int                 src_offset,
			dst_offset;
    unsigned int       *dst,
			src_val,
			dst_val,
			old_dst_val;
    unsigned int        dst_cmap[MEM32_8BIT_CMAPSIZE],
			src_cmap[MEM32_8BIT_CMAPSIZE];
    unsigned char       dst_red[MEM32_8BIT_CMAPSIZE],
			dst_green[MEM32_8BIT_CMAPSIZE],
			dst_blue[MEM32_8BIT_CMAPSIZE],
			src_red[MEM32_8BIT_CMAPSIZE],
			src_green[MEM32_8BIT_CMAPSIZE],
			src_blue[MEM32_8BIT_CMAPSIZE];
    unsigned char      *src;
    struct colormapseg  cms;
    struct cms_map      cmap;

    /* If neither source or destination is indexed, then let mem_rop take it */

    if (dst_indexed =
	(dpr->pr_ops != &mem_ops && mprp32_d(dpr)->windowfd >= -1 &&
	    strcmp(mprp32_d(dpr)->cms.cms_name,"monochrome")))
    {
	cmap.cm_red = dst_red;
	cmap.cm_green = dst_green;
	cmap.cm_blue = dst_blue;
	mem32_get_index_cmap(dpr, &cms, &cmap);

	for (i = 0; i < MEM32_8BIT_CMAPSIZE; i++)
	    dst_cmap[i] =
		(int) (dst_blue[i] << 16 | dst_green[i] << 8 | dst_red[i]);

	dst_cms_addr = cms.cms_addr;
	dst_cms_size = cms.cms_addr + cms.cms_size;
    }

    if (src_indexed =
	(spr && spr->pr_ops != &mem_ops && mprp32_d(spr)->windowfd >= -1 &&
	    strcmp(mprp32_d(spr)->cms.cms_name,"monochrome")))
    {
	cmap.cm_red = src_red;
	cmap.cm_green = src_green;
	cmap.cm_blue = src_blue;
	mem32_get_index_cmap(spr, &cms, &cmap);

	for (i = 0; i < MEM32_8BIT_CMAPSIZE; i++)
	    src_cmap[i] =
		(int) (src_blue[i] << 16 | src_green[i] << 8 | src_red[i]);

	src_cms_addr = cms.cms_addr;
	src_cms_size = cms.cms_addr + cms.cms_size;
    }

    if (!dst_indexed && !src_indexed)
    {
	PR_TO_MEM(dpr, dmempr);
	PR_TO_MEM(spr, smempr);
	return pr_rop(dpr, dx, dy, w, h, op, spr, sx, sy);
    }

    if (dpr->pr_ops != &mem_ops || mpr_d(dpr)->md_flags & MP_PLANEMASK)
	planes = mprp_d(dpr)->planes & PIX_ALL_PLANES;
    else
	planes = PIX_ALL_PLANES;
    not_planes = ~planes;
    oper = PIX_OP(op);			/* get just the operation portion */

    /*
     * An op that does not require the destination can let mem_rop do the
     * work a long as no special plane_mask has been defined and as long
     * as it is a screen to screen operation or a NULL source operation.
     */

    if ((dpr->pr_ops != &mem_ops) &&
	((spr && spr->pr_ops != &mem_ops && oper == PIX_SRC) ||
	(!spr && (oper == PIX_SRC || oper == PIX_SET || oper == PIX_CLR))) &&
	(planes==PIX_ALL_PLANES || (dst_indexed && planes==cms.cms_size-1)))
    {
	int                 new_op,
			    cc,
			    original_planes;
	Pixrect            *save_dst;

	pr_getattributes(dpr, &original_planes);

	if (!spr)
	{
	    switch (oper)
	    {
		case PIX_SET:
		    new_op = PIX_ALL_PLANES;
		    break;
		case PIX_CLR:
		    new_op = 0;
		    break;
		case PIX_SRC:
		    new_op = PIX_OPCOLOR(op);
	    }

	    if (dst_indexed)
	    {
		if (planes >= MEM32_8BIT_CMAPSIZE)
		    planes = MEM32_8BIT_CMAPSIZE - 1;
		new_op &= planes;
		new_op = mem32_get_true(dpr, new_op, planes);
	    }
	    op = PIX_SRC | PIX_COLOR(new_op) | (op & PIX_DONTCLIP);
	}

	if (dst_indexed)
	{
	    cc = PIX_ALL_PLANES;
	    pr_putattributes(dpr, &cc);
	}
	save_dst = dpr;
	PR_TO_MEM(dpr, dmempr);
	PR_TO_MEM(spr, smempr);
	cc = pr_rop(dpr, dx, dy, w, h, op, spr, sx, sy);
	pr_putattributes(save_dst, &original_planes);
	return cc;
    }

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

    --w;				/* much more complicated in cg9 */

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
    dst = (unsigned int *) dst_data->mpr.md_image;
    dst_bytes = (int) dst_data->mpr.md_linebytes / sizeof(int);

    fullscreen = 0;
    if (planes >= MEM32_8BIT_CMAPSIZE)
    {
	if (spr &&
	    (spr->pr_ops != &mem_ops || mpr_d(spr)->md_flags & MP_PLANEMASK))
	{
	    planes = mprp_d(spr)->planes & PIX_ALL_PLANES;
	}
	else
	{
	    fullscreen = 1;
	    planes = MEM32_8BIT_CMAPSIZE - 1;
	    if (!src_indexed && spr && spr->pr_depth == 32)
	    {
		for (i = 0; i < MEM32_8BIT_CMAPSIZE; i++)
		    src_cmap[i] = dst_cmap[i];
		src_indexed = dst_indexed;
		src_cms_addr = dst_cms_addr;
		src_cms_size = dst_cms_size;
	    }
	}
	not_planes = ~planes;
    }

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

    switch (oper >> 1)
    {
	case 0:			/* CLR */
	    ROP_LOOP(NO_SRC, dst_val = 0);
	    break;

	case 1:			/* ~s&~d or ~(s|d) */
	    NEED_SRC(dst_val = ~(src_val | dst_val));
	    break;

	case 2:			/* ~s&d */
	    NEED_SRC(dst_val = ~src_val & dst_val);
	    break;

	case 3:			/* ~s */
	    NEED_SRC(dst_val = ~src_val);
	    break;

	case 4:			/* s&~d */
	    NEED_SRC(dst_val = src_val & ~dst_val);
	    break;

	case 5:			/* ~d */
	    ROP_LOOP(NO_SRC, dst_val = ~dst_val);
	    break;

	case 6:			/* s^d */
	    NEED_SRC(dst_val = src_val ^ dst_val);
	    break;

	case 7:			/* ~s|~d or ~(s&d) */
	    NEED_SRC(dst_val = ~(src_val & dst_val));
	    break;

	case 8:			/* s&d */
	    NEED_SRC(dst_val = src_val & dst_val);
	    break;

	case 9:			/* ~(s^d) */
	    NEED_SRC(dst_val = ~(src_val ^ dst_val));
	    break;

	case 10:			/* DST */
	    break;

	case 11:			/* ~s|d */
	    NEED_SRC(dst_val = ~src_val | dst_val);
	    break;

	case 12:			/* SRC */
	    NEED_SRC(dst_val = src_val);
	    break;

	case 13:			/* s|~d */
	    NEED_SRC(dst_val = src_val | ~dst_val);
	    break;

	case 14:			/* s|d */
	    NEED_SRC(dst_val = src_val | dst_val);
	    break;

	case 15:			/* SET */
	    ROP_LOOP(NO_SRC, dst_val = PIX_ALL_PLANES);
	    break;
    }
    return 0;
}

#ifndef lint
static char     sccsid[] = "@(#)cg8_old_ro_rop.c 1.1 10/31/94, SMI";
#endif

/* Copyright 1989 Sun Microsystems, Inc. */

#include <sys/ioctl.h>			/* fbio.h		*/
#include <sun/fbio.h>			/* FBIOGETCMAP		*/
#include <pixrect/pixrect.h>		/* PIX*			*/
#include <pixrect/cg8var.h>		/* CG8*, cg8_d(pr)	*/
#include <pixrect/memvar.h>		/* mem_ops		*/
#include <pixrect/pr_planegroups.h>	/* PIXPG*		*/
#include <sunwindow/cms.h>		/* colormapseg, cms_map	*/

#define CG8_RAMDAC_CMAPSIZE 256

/* macro to change a cg8 pixrect into a memory pixrect */

#define PR_TO_MEM(src, mem)						\
    if (src && src->pr_ops != &mem_ops)					\
    {									\
	mem.pr_ops	= &mem_ops;					\
	mem.pr_size	= src->pr_size;					\
	mem.pr_depth	= src->pr_depth;				\
	mem.pr_data	= (char *) &cg8_d(src)->mprp;			\
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
    src_val = src_val ? ~0 : 0;						\

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
	x &= PIX_ALL_PLANES; \
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
		if (true_color == cmap[CG8_RAMDAC_CMAPSIZE - 1] ||	\
		    x==true_color)					\
		    x = CG8_RAMDAC_CMAPSIZE - 1;			\
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

cg8_old_ro_rop(dpr, dx, dy, w, h, op, spr, sx, sy)
Pixrect		*dpr, *spr;
int		dx, dy, w, h, op, sx, sy;
{
    Pixrect			smempr, dmempr;

#ifdef	KERNEL

#define	ioctl	cgeightioctl

    if (dpr->pr_depth>1 || (spr && spr->pr_depth>1))
    {
	printf("kernel: cg8_rop error: attempt at 32 bit rop\n");
	return 0;
    }
#endif
#ifndef	KERNEL

    /* Let mem_rop handle the overlay planes. */

    if (dpr->pr_depth == 32)
    {
	int			i, j, oper, incr, end, width1, width2,
				planes, not_planes, fullscreen,
				dst_indexed, dst_bytes, dst_line,
				src_indexed, src_bytes, src_line,
				dst_cms_addr, dst_cms_size,
				src_cms_addr, src_cms_size;
	struct	mprp_data	*src_data, *dst_data;
	int			wsx, wdx;
	int			src_offset, dst_offset;
	unsigned	int	*dst, src_val, dst_val, old_dst_val;
	unsigned	int	dst_cmap[CG8_RAMDAC_CMAPSIZE],
				src_cmap[CG8_RAMDAC_CMAPSIZE];
	unsigned	char	dst_red[CG8_RAMDAC_CMAPSIZE],
				dst_green[CG8_RAMDAC_CMAPSIZE],
				dst_blue[CG8_RAMDAC_CMAPSIZE],
				src_red[CG8_RAMDAC_CMAPSIZE],
				src_green[CG8_RAMDAC_CMAPSIZE],
				src_blue[CG8_RAMDAC_CMAPSIZE];
	unsigned	char	*src;
	struct	colormapseg	cms;
	struct	cms_map		cmap;

	/* make sure all coordinates are within screen or region bounds */

	if (!(op & PIX_DONTCLIP))
	{
	    struct pr_subregion	d;
	    struct pr_prpos	s;
	    extern int		pr_clip();

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

	--w; /* much more complicated in cg9 */

	/*
	 * Because of overlapping copies, must decide bottom up or top down.
	 * If null source, then initialize the first destination line as
	 * the source and start the copy from the second line.
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

	oper = PIX_OP(op);	/* get just the operation portion */

	/* data structure and destination memory address */

	dst_data = mprp_d(dpr);
	dst = (unsigned int *) dst_data->mpr.md_image;
	dst_bytes = (int) dst_data->mpr.md_linebytes / sizeof(int);
        if (dpr->pr_ops != &mem_ops || mpr_d(dpr)->md_flags & MP_PLANEMASK) 
            planes = dst_data->planes & PIX_ALL_PLANES;
        else 
            planes = PIX_ALL_PLANES;
	not_planes = ~planes;

	if (dst_indexed = (dpr->pr_ops!=&mem_ops && (cg8_d(dpr)->windowfd>=-1)))
	{
	    cmap.cm_red = dst_red;
	    cmap.cm_green = dst_green;
	    cmap.cm_blue = dst_blue;
	    cg8_get_index_cmap(dpr, &cms, &cmap);

	    for (i = 0; i < CG8_RAMDAC_CMAPSIZE; i++)
		dst_cmap[i] =
		    (int) (dst_blue[i]<<16 | dst_green[i]<<8 | dst_red[i]);

	    dst_cms_addr = cms.cms_addr;
	    dst_cms_size = cms.cms_addr + cms.cms_size;
	}
	

	/*
	 * cg8 has no bitblt hardware but the same cases mean a solid
	 * color use so mem_rop can do the work.
	 */

	if ((dpr->pr_ops != &mem_ops) &&
	    (planes==PIX_ALL_PLANES || (dst_indexed&&planes==cms.cms_size-1)) &&
	    ((spr && spr->pr_ops != &mem_ops && oper == PIX_SRC) ||
	    (!spr && (oper == PIX_SRC || oper == PIX_SET || oper == PIX_CLR))))
	{
	    int		new_op, cc, original_planes;
	    Pixrect	*save_dst;

	    pr_getattributes(dpr, &original_planes);

	    if (!spr)
	    {
		switch (oper)
		{
		    case PIX_SET:	new_op = PIX_ALL_PLANES;	break;
		    case PIX_CLR:	new_op = 0;			break;
		    case PIX_SRC:	new_op = PIX_OPCOLOR(op);
		}

		if (dst_indexed)
		{
		    if (planes >= CG8_RAMDAC_CMAPSIZE)
			planes = CG8_RAMDAC_CMAPSIZE - 1;
		    new_op &= planes;
		    new_op = cg8_get_true(dpr, new_op, planes);
		    cc = PIX_ALL_PLANES;
		    pr_putattributes(dpr, &cc);
		}
		op = PIX_SRC | PIX_COLOR(new_op) | (op & PIX_DONTCLIP);
	    }

	    w++;
	    dx -= dst_data->mpr.md_offset.x;
	    dy -= dst_data->mpr.md_offset.y;
	    if (spr)
	    {
		sx -= mprp_d(spr)->mpr.md_offset.x;
		sy -= mprp_d(spr)->mpr.md_offset.y;
	    }

	    save_dst = dpr;
	    PR_TO_MEM(spr, smempr);
	    PR_TO_MEM(dpr, dmempr);
	    cc = pr_rop(dpr, dx, dy, w, h, op, spr, sx, sy);
	    pr_putattributes(save_dst, &original_planes);
	    return cc;
	}

	if (src_indexed = (spr && spr->pr_ops != &mem_ops &&
	    (cg8_d(spr)->windowfd >= -1)))
	{
	    cmap.cm_red = src_red;
	    cmap.cm_green = src_green;
	    cmap.cm_blue = src_blue;
	    cg8_get_index_cmap(spr, &cms, &cmap);

	    for (i = 0; i < CG8_RAMDAC_CMAPSIZE; i++)
		src_cmap[i] =
		    (int) (src_blue[i]<<16 | src_green[i]<<8 | src_red[i]);

	    src_cms_addr = cms.cms_addr;
	    src_cms_size = cms.cms_addr + cms.cms_size;
	}

	if (!dst_indexed && !src_indexed)
	{
	    w++;
	    dx -= dst_data->mpr.md_offset.x;
	    dy -= dst_data->mpr.md_offset.y;
	    if (spr)
	    {
		sx -= mprp_d(spr)->mpr.md_offset.x;
		sy -= mprp_d(spr)->mpr.md_offset.y;
	    }
	    PR_TO_MEM(spr, smempr);
	    PR_TO_MEM(dpr, dmempr);
	    return pr_rop(dpr, dx, dy, w, h, op, spr, sx, sy);
	}
	
	if (planes >= CG8_RAMDAC_CMAPSIZE)
	{
	    fullscreen = 1;
	    planes = CG8_RAMDAC_CMAPSIZE - 1;
	    not_planes = ~planes;
	}
	else 
	    fullscreen = 0;

	if (spr)
	{
	    src_data = mprp_d(spr);
	    src = (unsigned char *) src_data->mpr.md_image;
	    src_bytes = src_data->mpr.md_linebytes;
	}
	else
	    src_bytes = 0;

	if (sx > dx)	/* forward */
	{
	    i = sx + w + 1;
	    j = 1;
	}
	else
	{
	    i = sx - 1;
	    j = -1;
	}

	switch (oper>>1)
	{
	    case 0:	/* CLR */
		ROP_LOOP(NO_SRC, dst_val = 0;);
		break;

	    case 1:	/* ~s&~d or ~(s|d) */
		NEED_SRC(dst_val = ~(src_val | dst_val););
		break;

	    case 2:	/* ~s&d */
		NEED_SRC(dst_val = ~src_val & dst_val;);
		break;

	    case 3:	/* ~s */
		NEED_SRC(dst_val = ~src_val;);
		break;

	    case 4:	/* s&~d */
		NEED_SRC(dst_val = src_val & ~dst_val;);
		break;
	    
	    case 5:	/* ~d */
		ROP_LOOP(NO_SRC, dst_val = ~dst_val;);
		break;

	    case 6:	/* s^d */
		NEED_SRC(dst_val = src_val ^ dst_val;);
		break;

	    case 7:	/* ~s|~d or ~(s&d) */
		NEED_SRC(dst_val = ~(src_val & dst_val););
		break;

	    case 8:	/* s&d */
		NEED_SRC(dst_val = src_val & dst_val;);
		break;

	    case 9:	/* ~(s^d) */
		NEED_SRC(dst_val = ~(src_val ^ dst_val););
		break;

	     case 10:	/* DST */
		break;
	    
	    case 11:	/* ~s|d */
		NEED_SRC(dst_val = ~src_val | dst_val;);
		break;

	    case 12:	/* SRC */
		NEED_SRC(dst_val = src_val;);
		break;

	    case 13:	/* s|~d */
		NEED_SRC(dst_val = src_val | ~dst_val;);
		break;

	    case 14:	/*s|d */
		NEED_SRC(dst_val = src_val | dst_val;);
		break;

	    case 15:	/* SET */
		ROP_LOOP(NO_SRC, dst_val = PIX_ALL_PLANES;);
		break;
	}
	return 0;
    }

    if (dpr->pr_ops != &mem_ops && cg8_d(dpr)->flags & CG8_COLOR_OVERLAY)
    {
	pr_set_plane_group(dpr, PIXPG_OVERLAY_ENABLE);
	PR_TO_MEM(spr, smempr);
	PR_TO_MEM(dpr, dmempr);
	(void) pr_rop(dpr, dx, dy, w, h, op, spr, sx, sy);
	dpr->pr_ops->pro_putattributes = cg8_putattributes;
	pr_set_plane_group(dpr, PIXPG_TRANSPARENT_OVERLAY);
	op = PIX_COLOR(PIX_OPCOLOR(op)>>1) | PIX_OP_CLIP(op);
    }
#endif	/* KERNEL */

    PR_TO_MEM(spr, smempr);
    PR_TO_MEM(dpr, dmempr);
    return pr_rop(dpr, dx, dy, w, h, op, spr, sx, sy);
}

/*********************************************************************
 * Support for 8-bit emulation of pixels.
 * these eight files are necessary just for cmschange!
 */
        
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>
        
cg8_win_getcms(windowfd, cms, cmap)
int                     windowfd;
struct  colormapseg     *cms;
struct  cms_map         *cmap;
{       
    struct cmschange cmschange;
        
    cmschange.cc_cms = *cms;
    cmschange.cc_map = *cmap;
    return (ioctl(windowfd, WINGETCMS, &cmschange), WINGETCMS);
}
 
cg8_get_index_cmap(pr, cms, cmap)
Pixrect                 *pr;
struct  colormapseg     *cms;
struct  cms_map         *cmap;
{
    int                         i;
    unsigned    char            r[CG8_RAMDAC_CMAPSIZE],
                                g[CG8_RAMDAC_CMAPSIZE],
                                b[CG8_RAMDAC_CMAPSIZE];
    struct      colormapseg     tmp_cms;
    struct      cms_map         tmp_cmap;
 
    tmp_cms = *cms = cg8_d(pr)->cms;
    cg8_getcolormap(pr, 0, CG8_RAMDAC_CMAPSIZE, cmap->cm_red, cmap->cm_green,
        cmap->cm_blue);
 
    /* if winfd >= 0, then index set by pwo_putcolormap (pixwin application) */
 
    if (cg8_d(pr)->windowfd >= 0)
    {
        /* win_getcms requires addr to be zero based */
        tmp_cms.cms_addr = 0;
        tmp_cmap.cm_red = r;
        tmp_cmap.cm_green = g;
        tmp_cmap.cm_blue = b;
        cg8_win_getcms(cg8_d(pr)->windowfd, &tmp_cms, &tmp_cmap);
        for (i=0; i<cms->cms_size; i++)
        {
            cmap->cm_red[i+cms->cms_addr]       = tmp_cmap.cm_red[i];
            cmap->cm_green[i+cms->cms_addr]     = tmp_cmap.cm_green[i];
            cmap->cm_blue[i+cms->cms_addr]      = tmp_cmap.cm_blue[i];
        }
    }
 
    /* otherwise index set by pixrect application */
 
    return 0;
}
 
cg8_get_index(pr, true_color)
Pixrect *pr;
int     true_color;
{
    int                         i, j;
    unsigned    char            r[CG8_RAMDAC_CMAPSIZE],
                                g[CG8_RAMDAC_CMAPSIZE],
                                b[CG8_RAMDAC_CMAPSIZE];
    struct      colormapseg     cms;
    struct      cms_map         cmap;
 
    cmap.cm_red = r;
    cmap.cm_green = g;
    cmap.cm_blue = b;
    cg8_get_index_cmap(pr, &cms, &cmap);
    for (i=cms.cms_addr, j=cms.cms_addr+cms.cms_size; i<j; i++)
    {   
        if (true_color == b[i]<<16 | g[i]<<8 | r[i])
            return i;
    }   
    return i-1;
}       
        
cg8_get_true(pr, index, planes)
Pixrect *pr;
int     index, planes;
{
    unsigned    char            r[CG8_RAMDAC_CMAPSIZE],
                                g[CG8_RAMDAC_CMAPSIZE],
                                b[CG8_RAMDAC_CMAPSIZE];
    struct      colormapseg     cms;
    struct      cms_map         cmap;
 
    cmap.cm_red = r;
    cmap.cm_green = g;
    cmap.cm_blue = b;
    cg8_get_index_cmap(pr, &cms, &cmap);
    index &= CG8_RAMDAC_CMAPSIZE - 1;
    if (index < cms.cms_addr && (planes <= (cms.cms_size-1)))
        index += cms.cms_addr;
    return(b[index]<<16 | g[index]<<8 | r[index]);
}       

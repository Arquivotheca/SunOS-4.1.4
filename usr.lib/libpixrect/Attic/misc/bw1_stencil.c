#ifndef lint
static        char sccsid[] = "@(#)bw1_stencil.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Sun1 Black-and-White pixrect rasterop
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/bw1var.h>
#include <pixrect/memvar.h>

/*
 * Stencil rasterop involving bw1 framebuf.  Normally we are called
 * as the destination via one of the macros in <pixrect/pixrect.h>,
 * but we may also be the source to stencil bits into memory.
 */

bw1_stencil(dst, op, sten, src)
	struct pr_subregion dst;
	struct pr_prpos src;
	struct pr_prpos sten;
{
	struct pr_prpos tdst, tsrc;

	if (op & PIX_DONTCLIP)
		op &= ~PIX_DONTCLIP;
	else {
		struct pr_subregion d;
		d.pr = dst.pr;
		d.pos = dst.pos;
		d.size = dst.size;
		pr_clip(&d, &src);
		pr_clip(&dst, &sten);
		pr_clip(&dst, &src);
	}
	if (MP_NOTMPR(sten.pr))/* stencil must be memory pixrect */
		return (-1);	
	if (sten.pr->pr_depth != 1)	/* with depth = 1 */
		return (-1);
	if (dst.size.x <= 0 || dst.size.y <= 0)
		return (0);
	tdst.pr = 0;
	tsrc.pr = 0;
	if (dst.pr->pr_ops != &bw1_ops && src.pr->pr_ops != &bw1_ops)
		return (-1);		/* neither src nor dest is bw1 */

/*
 * The bw1 stencils are implemented via rops.  We pay a small efficiency
 * penalty for this.  Writing the special purpose code is not considered
 * worthwhile for the bitmap stencils since they probably won't get used
 * nearly as much as rop.  We can speed them up later if necessary.
 */

	if (src.pr && MP_NOTMPR(src.pr)) {
		/*
		 * BUG: op is ignored on rop from framebuffers to memory!
		 * If source is not memory, ask the other pixrect to read
		 * itself into temporary memory to make the problem easier.
		 */
		tsrc.pr = mem_create(dst.size, 1);
		tsrc.pos.x = tsrc.pos.y = 0;
		(*src.pr->pr_ops->pro_rop)(tsrc, dst.size, PIX_SRC, src);
		src = tsrc;
	}
	tdst.pr = mem_create(dst.size, 1);
	tdst.pos.x = tdst.pos.y = 0;
	
	(*dst.pr->pr_ops->pro_rop)	/* TMP = Dest */
		( tdst, dst.size, PIX_SRC | PIX_DONTCLIP, dst.pr, dst.pos);
				
	(*tdst.pr->pr_ops->pro_rop)	/* TMP = D op Source */
		( tdst, dst.size, op | PIX_DONTCLIP, src);
				
	(*tdst.pr->pr_ops->pro_rop)	/* TMP = (D op S) & STencil */
		( tdst, dst.size, (PIX_SRC & PIX_DST) | PIX_DONTCLIP, sten);
				
	(*dst.pr->pr_ops->pro_rop)	/* dest = Dest & ~STencil) */
		( dst, (PIX_DST & PIX_NOT( PIX_SRC)) | PIX_DONTCLIP, sten);
				
	(*dst.pr->pr_ops->pro_rop)	/* dest = (D & ~ST) | ((D op S)& ST) */ 
		( dst, PIX_DST | PIX_SRC | PIX_DONTCLIP,tdst);

	if (tdst.pr)
		mem_destroy(tdst.pr);
	if (tsrc.pr)
		mem_destroy(tsrc.pr);
	return( 0);
}

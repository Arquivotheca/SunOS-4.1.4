#ifndef lint
static char sccsid[] = "@(#)mem_stencil.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

/*
 * Memory pixrect stencil op
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>

mem_stencil(dpr, dx, dy, dw, dh, op, stpr, stx, sty, spr, sx, sy)
	Pixrect *dpr;
	int dx, dy;
	register int dw, dh, op;
	Pixrect *stpr;
	int stx, sty;
	Pixrect *spr;
	int sx, sy;
{
	Pixrect *tpr;
	register int errors = 0;

	/* stencil must be a depth-1 memory pixrect */
	if (MP_NOTMPR(stpr) || stpr->pr_depth != 1)
		return PIX_ERR;

	/* poor way to clip */
	if (!(op & PIX_DONTCLIP)) {
		struct pr_subregion dst;
		struct pr_prpos src, sten;

		dst.pr = dpr;
		dst.pos.x = dx;
		dst.pos.y = dy;
		dst.size.x = dw;
		dst.size.y = dh;

		if (src.pr = spr) {
			src.pos.x = sx;
			src.pos.y = sy;
			(void) pr_clip(&dst, &src);
			dst.pos.x = dx;
			dst.pos.y = dy;
			dst.size.x = dw;
			dst.size.y = dh;
		}

		sten.pr = stpr;
		sten.pos.x = stx;
		sten.pos.y = sty;
		(void) pr_clip(&dst, &sten);
		stx = sten.pos.x;
		sty = sten.pos.y;

		if (spr) {
			(void) pr_clip(&dst, &src);
			sx = src.pos.x;
			sy = src.pos.y;
		}

		dx = dst.pos.x;
		dy = dst.pos.y;
		dw = dst.size.x;
		dh = dst.size.y;
	}

	if (dw <= 0 || dh <= 0)
		return 0;

	/*
	 * Create tmp and perform stencil with 4 or 5 rops.
	 */
	if (!(tpr = mem_create(dw, dh, dpr->pr_depth)))
		return PIX_ERR;

	/* tmp = dst */
	if (PIXOP_NEEDS_DST(op))
		errors |= (*dpr->pr_ops->pro_rop)(tpr, 0, 0, dw, dh, 
			PIX_SRC | PIX_DONTCLIP,
			dpr, dx, dy);

	/* tmp = dst op src */
	errors |= mem_rop(tpr, 0, 0, dw, dh, 
		op | PIX_DONTCLIP, 
		spr, sx, sy);

	/* tmp = (dst op src) ^ dst */
	errors |= (*dpr->pr_ops->pro_rop)(tpr, 0, 0, dw, dh, 
		PIX_DST ^ PIX_SRC | PIX_DONTCLIP, 
		dpr, dx, dy);

	/* tmp = (dst op src) ^ dst & stencil */
	errors |= mem_rop(tpr, 0, 0, dw, dh, 
		PIX_DST & PIX_SRC | PIX_DONTCLIP, 
		stpr, stx, sty);

	/* dst ^= (dst op src) ^ dst & stencil */ 
	errors |= pr_rop(dpr, dx, dy, dw, dh, 
		PIX_DST ^ PIX_SRC | PIX_DONTCLIP, 
		tpr, 0, 0);

	/* destroy temporary pixrect */
	(void) mem_destroy(tpr);

	return errors;
}

#ifndef lint
static char sccsid[] = "@(#)cg2_rop.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1984, 1987 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>
#include <pixrect/gp1var.h>
#include <pixrect/gp1cmds.h>
#include <pixrect/memvar.h>

/* minimum # words to make ropmode swapping worthwhile */
#define	SWAP_ROPMODE_MIN	6

/* magic ropmode xor mask*/
#define	SWAP_ROPMODE_MASK	((PRWWRD ^ PWWWRD) << 3)

/*
 * cg2/gp1 rasterop.
 *
 * In user mode this source file is compiled into both cg2_rop and
 * gp1_rop (-DGP1_ROP).  In the kernel cg2_rop handles both cg2 and 
 * gp1 pixrects.
 *
 * GP commands are currently used for fills and scrolls, but this is
 * questionable since these operations can often be performed more
 * quickly by the host CPU (unless it's a 68010).
 */

cg2_rop (dpr, dx, dy, dw, dh, op, spr, sx, sy)
	Pixrect *dpr;
	int dx, dy, dw, dh;
	int op;
	Pixrect *spr;
	int sx, sy;
{
	register struct cg2fb *fb;
	register int soffset, doffset;
	register LOOP_T w;
	struct cg2pr *sprd;
	int sdepth;
#define dummy IF68000(w,0)
#ifndef KERNEL
	Pixrect *tpr = 0;
	int tx, ty;
#endif KERNEL
	if ((op & PIX_DONTCLIP) == 0) {
		struct pr_subregion dst;
		struct pr_prpos src;
		extern int pr_clip();
		dst.pr = dpr;
		dst.pos.x = dx;
		dst.pos.y = dy;
		dst.size.x = dw;
		dst.size.y = dh;
		if ((src.pr = spr) != 0) {
			src.pos.x = sx;
			src.pos.y = sy;
		}
		(void) pr_clip(&dst, &src);
		dx = dst.pos.x;
		dy = dst.pos.y;
		dw = dst.size.x;
		dh = dst.size.y;
		if (spr != 0) {
			sx = src.pos.x;
			sy = src.pos.y;
		}
	}
	if (dw <= 0 || dh <= 0 )
		return 0;
	if (dpr->pr_ops->pro_rop != cg2_rop) {
		register u_char *d, *s;
		if (spr == 0 || 
			spr->pr_ops->pro_rop != cg2_rop ||
			dpr->pr_depth != CG2_DEPTH) 
			return PIX_ERR;
		sprd = cg2_d(spr);
#ifdef KERNEL
		GP1_PRD_SYNC(sprd, return PIX_ERR);
#endif KERNEL
		{
			register struct mprp_data *mprd = mprp_d(dpr);
			if (MP_NOTMPR(dpr) ||
				(op & PIX_SET) != PIX_SRC ||
				(mprd->mpr.md_flags & MP_PLANEMASK &&
					(mprd->planes & 255) != 255)) {
#ifndef KERNEL
				tpr = dpr;
				if (!(dpr = mem_create(dw, dh, CG2_DEPTH)))
					return PIX_ERR;
				mprd = mprp_d(dpr);
				tx = dx;  dx = 0;
				ty = dy;  dy = 0;
#else KERNEL
				return PIX_ERR;
#endif KERNEL
			}
			doffset = mprd->mpr.md_linebytes;
			d = (u_char *) PTR_ADD(mprd->mpr.md_image, 
				pr_product(dy + mprd->mpr.md_offset.y, 
					doffset) +
				dx + mprd->mpr.md_offset.x);
		}
		sx += sprd->cgpr_offset.x;
		sy += sprd->cgpr_offset.y;
		fb = sprd->cgpr_va;
		if (sprd->flags & CG2D_FASTREAD)
			fb->misc.nozoom.dblbuf.reg.fast_read = 0;
		fb->ppmask.reg = 255;
		fb->status.reg.ropmode = SRWPIX;
		w = dw;
		doffset -= w;
		s = cg2_roppixaddr(fb, sx, sy);
		soffset = cg2_linebytes(fb, SRWPIX) - w;
		w--;
		PR_LOOPP(dh - 1,
			PR_LOOPP(w, *d++ = *s++);
			d += doffset;
			s += soffset);
#ifndef KERNEL
		if (tpr) {
			int error;
			error = pr_rop(tpr, tx, ty, dw, dh, 
				op | PIX_DONTCLIP, dpr, 0, 0);
			(void) mem_destroy(dpr);
			return error;
		}
#endif KERNEL
		return 0;
	}
	{
		register struct cg2pr *dprd;
		register Pixrect *rspr = spr;
		register int rop = op, color, planes;
		op = PIXOP_NEEDS_DST(rop);
		if (PIXOP_NEEDS_SRC(rop)) {
#ifndef KERNEL
			
			if (rspr && 
				rspr->pr_size.x == 1 && rspr->pr_size.y == 1) {
				color = pr_get(rspr, 0, 0);
				if (rspr->pr_depth == 1 && 
					color && (color = 
						PIXOP_COLOR(rop)) == 0)
						color = ~color;
				spr = rspr = 0;
			}
			else
#endif KERNEL
			color = PIXOP_COLOR(rop);
		}
		else {
			color = 0;
			spr = rspr = 0;
		}
		dprd = cg2_d(dpr);
		fb = dprd->cgpr_va;
		planes = dprd->cgpr_planes & 255;
		dx += dprd->cgpr_offset.x;
		dy += dprd->cgpr_offset.y;
#ifdef KERNEL
		GP1_PRD_SYNC(dprd, return PIX_ERR);
		
		if (dprd->flags & CG2D_FASTREAD)
			fb->misc.nozoom.dblbuf.reg.fast_read = 0;
#endif KERNEL
		fb->ppmask.reg = planes;
		cg2_setpattern(fb, CG2_ALLROP, 0);
		rop = PIXOP_OP(rop);
		if (rspr) {
			cg2_setfunction(fb, CG2_ALLROP, rop);
			switch (sdepth = rspr->pr_depth) {
			case CG2_DEPTH:
				break;
			case 1:
				
				if (color == 0)
					color = ~color;
				
				if (!MP_NOTMPR(rspr) &&
					mpr_d(rspr)->md_flags 
					& MP_REVERSEVIDEO) 
					cg2_setfunction(fb, CG2_ALLROP, 
						rop = PIX_OP_REVERSESRC(rop));
				
				if ((color = ~color) & planes) {
					fb->ppmask.reg = color;
					cg2_setfunction(fb, CG2_ALLROP, 
						rop << 2 | rop & 3);
					fb->ppmask.reg = planes;
				}
				break;
			default:
				return PIX_ERR;
			}
		}
		else {
			
			cg2_setfunction(fb, CG2_ALLROP, rop << 2 | rop & 3);
			
			if (color & planes) {
				fb->ppmask.reg = color;
				cg2_setfunction(fb, CG2_ALLROP, 
					rop & 0xC | rop >> 2);
				fb->ppmask.reg = planes;
			}
		}
	}
	
	if (spr == 0) {
		register short *d;
		cg2_setlmask(fb, CG2_ALLROP, 
			mrc_lmask(w = cg2_prskew(dx)));
		cg2_setrmask(fb, CG2_ALLROP, 
			mrc_rmask(cg2_prskew(w += dw - 1)));
		w >>= 4;
		cg2_setwidth(fb, CG2_ALLROP, w, w);
		fb->status.reg.ropmode = PWRWRD;
		doffset = cg2_linebytes(fb, PWRWRD) - w - w;
		d = cg2_ropwordaddr(fb, 0, dx, dy);
#ifndef KERNEL
		if (--w < 0)
			PR_LOOPP(dh - 1, *d = IF68000(w, 0); 
				PTR_INCR(short *, d, doffset));
		else if (w <= SWAP_ROPMODE_MIN || op)
			PR_LOOPP(dh - 1,
				PR_LOOPP(w, *d++ = IF68000(w, 0));
				*d = IF68000(w, 0);
				PTR_INCR(short *, d, doffset));
		else {
			register short *status = &fb->status.word;
			cg2_setwidth(fb, CG2_ALLROP, 2, 2);
			w -= 2;
			PR_LOOPP(dh - 1, 
				*d++ = IF68000(w, 0);
				*d++ = IF68000(w, 0);
				
				*status ^= SWAP_ROPMODE_MASK;
				PR_LOOPP(w, *d++ = IF68000(w, 0));
				
				*status ^= SWAP_ROPMODE_MASK;
				*d = IF68000(w, 0);
				PTR_INCR(short *, d, doffset));
		}
#else KERNEL
		doffset -= 2;
		PR_LOOPP(dh - 1,
			PR_LOOPP(w, *d++ = IF68000(w, 0));
			PTR_INCR(short *, d, doffset));
#endif KERNEL
		return 0;
	}
	
	if (spr->pr_ops->pro_rop == cg2_rop &&
		(sprd = cg2_d(spr))->cgpr_va == fb) {
		register short *d, *s;
		register int skew;
		int rtolrop = 0;
		sx += sprd->cgpr_offset.x;
		sy += sprd->cgpr_offset.y;
#ifndef KERNEL
		if (sprd->flags & CG2D_FASTREAD)
			fb->misc.nozoom.dblbuf.reg.fast_read = 1;
#endif KERNEL
		cg2_setlmask(fb, CG2_ALLROP,
			mrc_lmask(skew = cg2_prskew(dx)));
		cg2_setrmask(fb, CG2_ALLROP,
			mrc_rmask(cg2_prskew(w = skew + dw - 1)));
		w >>= 4;
		cg2_setwidth(fb, CG2_ALLROP, w, w);
		skew -= cg2_prskew(sx);
		cg2_setshift(fb, CG2_ALLROP, cg2_prskew(skew), 1);
		skew = skew <= 0 ? 2 : 0;
		fb->status.reg.ropmode = PWRWRD;
		doffset = cg2_linebytes(fb, PWRWRD);
		
		if (dx + dw > sx && dx < sx + dw) {
			
			if (dy > sy) {
				if (dy < sy + dh) {
					sy += dh - 1;
					dy += dh - 1;
					doffset = -doffset;
				}
			}
			
			else if (dy == sy && dx > sx && w) {
				rtolrop = 1;
				cg2_setrmask(fb, CG2_ALLROP,
					mrc_lmask(cg2_prskew(dx)));
				cg2_setlmask(fb, CG2_ALLROP,
					mrc_rmask(skew =
						cg2_prskew(dx += dw - 1)));
				skew -= cg2_prskew(sx += dw - 1);
				cg2_setshift(fb, CG2_ALLROP, 
					cg2_prskew(skew), 0);
				skew = skew >= 0 ? 2 : 0;
			}
		}
		s = cg2_ropwordaddr(fb, 0, sx, sy);
		d = cg2_ropwordaddr(fb, 0, dx, dy);
		doffset = doffset - w - w;
		soffset = doffset - skew;
		
		if (rtolrop) {
			register u_short *rsource = 
				&fb->ropcontrol[CG2_ALLROP]
					.ropregs.mrc_source1;
			doffset += w << 2;
			soffset = doffset + skew;
			w--;
			PR_LOOPP(dh - 1,
				if (skew)
					*rsource = *s--;
				*d = *s;
				PR_LOOPP(w, *--d = *--s);
				PTR_INCR(short *, d, doffset);
				PTR_INCR(short *, s, soffset));
		}
		else
		
#ifndef KERNEL
		if (skew) {
			register u_short *somereg = 
				&fb->ropcontrol[CG2_ALLROP].
					ropregs.mrc_source2;
			if (--w < 0) 
				PR_LOOPP(dh - 1,
					*somereg = *s++;
					*d = *s;
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			else if (w <= SWAP_ROPMODE_MIN || op)
				PR_LOOPP(dh - 1,
					*somereg = *s++;
					PR_LOOPP(w, *d++ = *s++);
					*d = *s;
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			else {
				somereg = (u_short *) &fb->status.word;
				w--;
				PR_LOOPP(dh - 1,
					cg2_setlsource(fb, CG2_ALLROP, *s++);
					*d++ = *s++;
					
					*somereg ^= SWAP_ROPMODE_MASK;
					PR_LOOPP(w, *d++ = *s++);
					
					*somereg ^= SWAP_ROPMODE_MASK;
					*d = *s;
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			}
		}
		else if (--w < 0) 
			PR_LOOPP(dh - 1, 
				*d = *s;
				PTR_INCR(short *, d, doffset);
				PTR_INCR(short *, s, soffset));
		else 
#else KERNEL
		{
			register u_short *lsource = 
				&fb->ropcontrol[CG2_ALLROP]
					.ropregs.mrc_source2;
			doffset -= 2;
			soffset -= 2;
#endif KERNEL
#ifndef KERNEL
			PR_LOOPP(dh - 1,
				PR_LOOPP(w, *d++ = *s++);
				*d = *s;
				PTR_INCR(short *, d, doffset);
				PTR_INCR(short *, s, soffset));
#else
			PR_LOOPP(dh - 1,
				if (skew)
					*lsource = *s++;
				PR_LOOPP(w, *d++ = *s++);
				PTR_INCR(short *, d, doffset);
				PTR_INCR(short *, s, soffset));
#endif KERNEL
#ifndef KERNEL
		if (sprd->flags & CG2D_FASTREAD)
			fb->misc.nozoom.dblbuf.reg.fast_read = 0;
#else KERNEL
		}
#endif KERNEL
		return 0;
	}
	
	if (MP_NOTMPR(spr)) {
#ifndef KERNEL
		tpr = spr;
		if (!(spr = mem_create(dw, dh, sdepth)))
			return PIX_ERR;
		if ((*tpr->pr_ops->pro_rop)(spr, 0, 0, dw, dh, 
			PIX_SRC | PIX_DONTCLIP, tpr, sx, sy)) {
			(void) mem_destroy(spr);
			return PIX_ERR;
		}
		sx = 0; 
		sy = 0;
#else KERNEL
		return PIX_ERR;
#endif KERNEL
	}
	
	switch (sdepth) {
	case CG2_DEPTH: {
		register short *d, *s;
#ifndef KERNEL
		int edges;
#endif KERNEL
		{
			register struct mpr_data *mprd = mpr_d(spr);
			
			cg2_setlmask(fb, CG2_ALLROP,
				mrc_lmask(w = cg2_prskew(dx)));
			cg2_setrmask(fb, CG2_ALLROP,
				mrc_rmask(cg2_prskew(w + dw - 1)));
			w = dw | dx & 1;
#ifndef KERNEL
			edges = w & 1;
#endif KERNEL
			w = (w - 1) >> 1;
			cg2_setwidth(fb, CG2_ALLROP, w, w);
			d = (short *) cg2_roppixaddr(fb, dx & ~1, dy);
			doffset = cg2_linebytes(fb, SWWPIX) - w - w;
			fb->status.reg.ropmode = SWWPIX;
			soffset = mprd->md_linebytes;
			s = (short *) PTR_ADD(mprd->md_image,
				pr_product(sy + mprd->md_offset.y, soffset) + 
				((sx += mprd->md_offset.x) & ~1));
			soffset = soffset - w - w;
		}
		
		if (!((dx ^ sx) & 1)) {
			cg2_setshift(fb, CG2_ALLROP, 0, 1);
			soffset -= 2;
#ifndef KERNEL
			if (!edges && !op) {
				fb->status.reg.ropmode = SRWPIX;
				cg2_setlmask(fb, CG2_ALLROP, 0);
				cg2_setrmask(fb, CG2_ALLROP, 0);
				op = 1;
			}
			if (w < SWAP_ROPMODE_MIN || op) 
#endif KERNEL
			{
				register u_short *rsource_pix = 
					&fb->ropcontrol[CG2_ALLROP].prime
						.ropregs.mrc_source1;
				PR_LOOPP(dh - 1,
					*rsource_pix = *s++;
					PR_LOOP(w, *d++ = *s++);
					*d = IF68000(w, 0);
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			}
#ifndef KERNEL
			else {
				register short *status = &fb->status.word;
				cg2_setwidth(fb, CG2_ALLROP, 2, 2);
				w -= 3;
				PR_LOOPP(dh - 1,
					cg2_setrsource_pix(fb, CG2_ALLROP, 
						*s++);
					*d++ = *s++;
					*d++ = *s++;
					
					*status ^= SWAP_ROPMODE_MASK;
					PR_LOOPP(w, *d++ = *s++);
					
					*status ^= SWAP_ROPMODE_MASK;
					*d = IF68000(w, 0);
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			}
#endif KERNEL
		}
		
		else {
			register short *shiftreg = 
				&fb->ropcontrol[CG2_ALLROP].ropregs.mrc_shift;
			register int shift;
			int initshift;
			
			initshift = 1 << 8 | (dx & 14) + 1;
			
			if (!(dx & 1)) {
				soffset -= 2;
				initshift |= -1 << 16;
			}
#ifndef KERNEL
			if (w < SWAP_ROPMODE_MIN || op) 
#endif KERNEL
			{
				doffset -= 2;
				soffset -= 2;
				PR_LOOPP(dh - 1,
					if ((shift = initshift) < 0)
						cg2_setrsource_pix(
							fb, CG2_ALLROP, *s++);
					PR_LOOPP(w, 
						*shiftreg = (shift |= 1 << 8);
						shift += 2;
						*d++ = *s++);
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			}
#ifndef KERNEL
			else {
				cg2_setwidth(fb, CG2_ALLROP, 2, 2);
				w -= 3;
				PR_LOOPP(dh - 1,
					if ((shift = initshift) < 0)
						cg2_setrsource_pix(
							fb, CG2_ALLROP, *s++);
					*shiftreg = shift;
					shift += 2;
					*d++ = *s++;
					*shiftreg = shift;
					shift += 2;
					*d++ = *s++;
					
					fb->status.word ^= SWAP_ROPMODE_MASK;
					PR_LOOPP(w, 
						*shiftreg = shift;
						shift += 2;
						shift |= 1 << 8;
						*d++ = *s++);
					
					fb->status.word ^= SWAP_ROPMODE_MASK;
					*shiftreg = shift;
					*d = *s;
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			}
#endif KERNEL
		}
	}
		break;
	case 1: {
		register short *d, *s;
		register int skew;
		d = cg2_ropwordaddr(fb, 0, dx, dy);
		{
			register struct mpr_data *mprd = mpr_d(spr);
			soffset = mprd->md_linebytes;
			skew = (sx += mprd->md_offset.x) & 15;
			s = (short *) PTR_ADD(mprd->md_image,
				pr_product(sy + mprd->md_offset.y, soffset) +
				(sx >> 4 << 1));
		}
		cg2_setlmask(fb, CG2_ALLROP,
			mrc_lmask(w = cg2_prskew(dx)));
		cg2_setshift(fb, CG2_ALLROP, cg2_prskew(w - skew), 1);
		skew = skew >= w ? 2 : 0;
		cg2_setrmask(fb, CG2_ALLROP,
			mrc_rmask(cg2_prskew(w += dw - 1)));
		w >>= 4;
		cg2_setwidth(fb, CG2_ALLROP, w, w);
		doffset = cg2_linebytes(fb, PWWWRD) - w - w;
		fb->status.reg.ropmode = PWWWRD;
		soffset = soffset - w - w - skew;
#ifndef KERNEL
		if (skew) {
			register u_short *somereg = 
#else KERNEL
		{
			register u_short *rsource =
#endif KERNEL
				&fb->ropcontrol[CG2_ALLROP].
					ropregs.mrc_source1;
#ifndef KERNEL
			if (--w < 0)
				PR_LOOPP(dh - 1, 
					*somereg = *s++;
					*d = *s;
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			else if (w <= SWAP_ROPMODE_MIN || op)
				PR_LOOPP(dh - 1,
					*somereg = *s++;
					PR_LOOPP(w, *d++ = *s++);
					*d = *s;
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			else {
				somereg = (u_short *) &fb->status.word;
				w -= 2;
				cg2_setwidth(fb, CG2_ALLROP, 2, 2);
				PR_LOOPP(dh - 1,
					cg2_setrsource(fb, CG2_ALLROP, *s++);
					*d++ = *s++;
					*d++ = *s++;
					
					*somereg ^= SWAP_ROPMODE_MASK;
					PR_LOOPP(w, *d++ = *s++);
					
					*somereg ^= SWAP_ROPMODE_MASK;
					*d = *s;
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			}
#else KERNEL
			doffset -= 2;
			soffset -= 2;
			PR_LOOPP(dh - 1,
				if (skew)
					*rsource = *s++;
				PR_LOOPP(w, *d++ = *s++);
				PTR_INCR(short *, d, doffset);
				PTR_INCR(short *, s, soffset));
#endif KERNEL
		}
#ifndef KERNEL
		else {
			if (--w < 0) 
				PR_LOOPP(dh - 1, 
					*d = *s;
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			else if (w <= SWAP_ROPMODE_MIN || op)
				PR_LOOPP(dh - 1,
					PR_LOOPP(w, *d++ = *s++);
					*d = *s;
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			else {
				register short *status = &fb->status.word;
				w -= 2;
				cg2_setwidth(fb, CG2_ALLROP, 2, 2);
				PR_LOOPP(dh - 1,
					*d++ = *s++;
					*d++ = *s++;
					
					*status ^= SWAP_ROPMODE_MASK;
					PR_LOOPP(w, *d++ = *s++);
					
					*status ^= SWAP_ROPMODE_MASK;
					*d = *s;
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			}
		}
#endif KERNEL
	}
		break;
	}
	
#ifndef KERNEL
	
	if (tpr)
		(void) mem_destroy(spr);
#endif KERNEL
	return 0;
}

#ifndef KERNEL
gp1_cg2_rop (dpr, dx, dy, dw, dh, op, spr, sx, sy)
	Pixrect *dpr;
	int dx, dy, dw, dh;
	int op;
	Pixrect *spr;
	int sx, sy;
{
	register struct cg2fb *fb;
	register int soffset, doffset;
	register LOOP_T w;
	struct cg2pr *sprd;
	int sdepth;
	Pixrect *tpr = 0;
	int tx, ty;
	
	if (dpr->pr_ops->pro_rop == gp1_rop) {
		register Pixrect *rspr = spr;
		register struct cg2pr *dprd = cg2_d(dpr);
		if (rspr == 0 ||
			rspr->pr_ops->pro_rop == gp1_rop &&
			(sprd = cg2_d(rspr))->cgpr_va == dprd->cgpr_va) {
			register short *shp = (short *) dprd->gp_shmem;
			u_int bitvec;
			if ((doffset = gp1_alloc((caddr_t) shp, 1, &bitvec,
				dprd->minordev, dprd->ioctl_fd)) == 0)
				return PIX_ERR;
			shp += doffset;
			*shp++ = (rspr ? GP1_PR_ROP_FF : GP1_PR_ROP_NF) |
				dprd->cgpr_planes & 255;
			*shp++ = dprd->cg2_index | 
				(dprd->flags & CG2D_FASTREAD ? 0x8000 : 0);
			*shp++ = op;
			*shp++ = dprd->cgpr_offset.x;
			*shp++ = dprd->cgpr_offset.y;
			*shp++ = dpr->pr_size.x;
			*shp++ = dpr->pr_size.y;
			*shp++ = dx;
			*shp++ = dy;
			*shp++ = dw;
			*shp++ = dh;
			if (rspr) {
				*shp++ = sprd->cgpr_offset.x;
				*shp++ = sprd->cgpr_offset.y;
				*shp++ = rspr->pr_size.x;
				*shp++ = rspr->pr_size.y;
				*shp++ = sx;
				*shp++ = sy;
			}
			*shp++ = GP1_EOCL | GP1_FREEBLKS;
			*shp++ = bitvec >> 16;
			*shp   = bitvec;
			return gp1_post(dprd->gp_shmem, doffset, 
				dprd->ioctl_fd);
		}
	}
	if ((op & PIX_DONTCLIP) == 0) {
		struct pr_subregion dst;
		struct pr_prpos src;
		extern int pr_clip();
		dst.pr = dpr;
		dst.pos.x = dx;
		dst.pos.y = dy;
		dst.size.x = dw;
		dst.size.y = dh;
		if ((src.pr = spr) != 0) {
			src.pos.x = sx;
			src.pos.y = sy;
		}
		(void) pr_clip(&dst, &src);
		dx = dst.pos.x;
		dy = dst.pos.y;
		dw = dst.size.x;
		dh = dst.size.y;
		if (spr != 0) {
			sx = src.pos.x;
			sy = src.pos.y;
		}
	}
	if (dw <= 0 || dh <= 0 )
		return 0;
	if (dpr->pr_ops->pro_rop != gp1_rop) {
		register u_char *d, *s;
		if (spr == 0 || 
			spr->pr_ops->pro_rop != gp1_rop ||
			dpr->pr_depth != CG2_DEPTH) 
			return PIX_ERR;
		sprd = cg2_d(spr);
		GP1_PRD_SYNC(sprd, return PIX_ERR);
		{
			register struct mprp_data *mprd = mprp_d(dpr);
			if (MP_NOTMPR(dpr) ||
				(op & PIX_SET) != PIX_SRC ||
				(mprd->mpr.md_flags & MP_PLANEMASK &&
					(mprd->planes & 255) != 255)) {
				tpr = dpr;
				if (!(dpr = mem_create(dw, dh, CG2_DEPTH)))
					return PIX_ERR;
				mprd = mprp_d(dpr);
				tx = dx;  dx = 0;
				ty = dy;  dy = 0;
			}
			doffset = mprd->mpr.md_linebytes;
			d = (u_char *) PTR_ADD(mprd->mpr.md_image, 
				pr_product(dy + mprd->mpr.md_offset.y, 
					doffset) +
				dx + mprd->mpr.md_offset.x);
		}
		sx += sprd->cgpr_offset.x;
		sy += sprd->cgpr_offset.y;
		fb = sprd->cgpr_va;
		if (sprd->flags & CG2D_FASTREAD)
			fb->misc.nozoom.dblbuf.reg.fast_read = 0;
		fb->ppmask.reg = 255;
		fb->status.reg.ropmode = SRWPIX;
		w = dw;
		doffset -= w;
		s = cg2_roppixaddr(fb, sx, sy);
		soffset = cg2_linebytes(fb, SRWPIX) - w;
		w--;
		PR_LOOPP(dh - 1,
			PR_LOOPP(w, *d++ = *s++);
			d += doffset;
			s += soffset);
		if (tpr) {
			int error;
			error = pr_rop(tpr, tx, ty, dw, dh, 
				op | PIX_DONTCLIP, dpr, 0, 0);
			(void) mem_destroy(dpr);
			return error;
		}
		return 0;
	}
	{
		register struct cg2pr *dprd;
		register Pixrect *rspr = spr;
		register int rop = op, color, planes;
		op = PIXOP_NEEDS_DST(rop);
		if (PIXOP_NEEDS_SRC(rop)) {
			
			if (rspr && 
				rspr->pr_size.x == 1 && rspr->pr_size.y == 1) {
				color = pr_get(rspr, 0, 0);
				if (rspr->pr_depth == 1 && 
					color && (color = 
						PIXOP_COLOR(rop)) == 0)
						color = ~color;
				rop = PIX_COLOR(color) | rop & 31;
				spr = rspr = 0;
			}
			else
			color = PIXOP_COLOR(rop);
		}
		else {
			color = 0;
			spr = rspr = 0;
		}
		
		if (rspr == 0)
			return gp1_rop(dpr, dx, dy, dw, dh, 
				rop | PIX_DONTCLIP, rspr, sx, sy);
		dprd = cg2_d(dpr);
		fb = dprd->cgpr_va;
		planes = dprd->cgpr_planes & 255;
		dx += dprd->cgpr_offset.x;
		dy += dprd->cgpr_offset.y;
		GP1_PRD_SYNC(dprd, return PIX_ERR);
		fb->ppmask.reg = planes;
		cg2_setpattern(fb, CG2_ALLROP, 0);
		rop = PIXOP_OP(rop);
			cg2_setfunction(fb, CG2_ALLROP, rop);
			switch (sdepth = rspr->pr_depth) {
			case CG2_DEPTH:
				break;
			case 1:
				
				if (color == 0)
					color = ~color;
				
				if (!MP_NOTMPR(rspr) &&
					mpr_d(rspr)->md_flags 
					& MP_REVERSEVIDEO) 
					cg2_setfunction(fb, CG2_ALLROP, 
						rop = PIX_OP_REVERSESRC(rop));
				
				if ((color = ~color) & planes) {
					fb->ppmask.reg = color;
					cg2_setfunction(fb, CG2_ALLROP, 
						rop << 2 | rop & 3);
					fb->ppmask.reg = planes;
				}
				break;
			default:
				return PIX_ERR;
			}
	}
	
	if (MP_NOTMPR(spr)) {
		tpr = spr;
		if (!(spr = mem_create(dw, dh, sdepth)))
			return PIX_ERR;
		if ((*tpr->pr_ops->pro_rop)(spr, 0, 0, dw, dh, 
			PIX_SRC | PIX_DONTCLIP, tpr, sx, sy)) {
			(void) mem_destroy(spr);
			return PIX_ERR;
		}
		sx = 0; 
		sy = 0;
	}
	
	switch (sdepth) {
	case CG2_DEPTH: {
		register short *d, *s;
		int edges;
		{
			register struct mpr_data *mprd = mpr_d(spr);
			
			cg2_setlmask(fb, CG2_ALLROP,
				mrc_lmask(w = cg2_prskew(dx)));
			cg2_setrmask(fb, CG2_ALLROP,
				mrc_rmask(cg2_prskew(w + dw - 1)));
			w = dw | dx & 1;
			edges = w & 1;
			w = (w - 1) >> 1;
			cg2_setwidth(fb, CG2_ALLROP, w, w);
			d = (short *) cg2_roppixaddr(fb, dx & ~1, dy);
			doffset = cg2_linebytes(fb, SWWPIX) - w - w;
			fb->status.reg.ropmode = SWWPIX;
			soffset = mprd->md_linebytes;
			s = (short *) PTR_ADD(mprd->md_image,
				pr_product(sy + mprd->md_offset.y, soffset) + 
				((sx += mprd->md_offset.x) & ~1));
			soffset = soffset - w - w;
		}
		
		if (!((dx ^ sx) & 1)) {
			cg2_setshift(fb, CG2_ALLROP, 0, 1);
			soffset -= 2;
			if (!edges && !op) {
				fb->status.reg.ropmode = SRWPIX;
				cg2_setlmask(fb, CG2_ALLROP, 0);
				cg2_setrmask(fb, CG2_ALLROP, 0);
				op = 1;
			}
			if (w < SWAP_ROPMODE_MIN || op) 
			{
				register u_short *rsource_pix = 
					&fb->ropcontrol[CG2_ALLROP].prime
						.ropregs.mrc_source1;
				PR_LOOPP(dh - 1,
					*rsource_pix = *s++;
					PR_LOOP(w, *d++ = *s++);
					*d = IF68000(w, 0);
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			}
			else {
				register short *status = &fb->status.word;
				cg2_setwidth(fb, CG2_ALLROP, 2, 2);
				w -= 3;
				PR_LOOPP(dh - 1,
					cg2_setrsource_pix(fb, CG2_ALLROP, 
						*s++);
					*d++ = *s++;
					*d++ = *s++;
					
					*status ^= SWAP_ROPMODE_MASK;
					PR_LOOPP(w, *d++ = *s++);
					
					*status ^= SWAP_ROPMODE_MASK;
					*d = IF68000(w, 0);
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			}
		}
		
		else {
			register short *shiftreg = 
				&fb->ropcontrol[CG2_ALLROP].ropregs.mrc_shift;
			register int shift;
			int initshift;
			
			initshift = 1 << 8 | (dx & 14) + 1;
			
			if (!(dx & 1)) {
				soffset -= 2;
				initshift |= -1 << 16;
			}
			if (w < SWAP_ROPMODE_MIN || op) 
			{
				doffset -= 2;
				soffset -= 2;
				PR_LOOPP(dh - 1,
					if ((shift = initshift) < 0)
						cg2_setrsource_pix(
							fb, CG2_ALLROP, *s++);
					PR_LOOPP(w, 
						*shiftreg = (shift |= 1 << 8);
						shift += 2;
						*d++ = *s++);
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			}
			else {
				cg2_setwidth(fb, CG2_ALLROP, 2, 2);
				w -= 3;
				PR_LOOPP(dh - 1,
					if ((shift = initshift) < 0)
						cg2_setrsource_pix(
							fb, CG2_ALLROP, *s++);
					*shiftreg = shift;
					shift += 2;
					*d++ = *s++;
					*shiftreg = shift;
					shift += 2;
					*d++ = *s++;
					
					fb->status.word ^= SWAP_ROPMODE_MASK;
					PR_LOOPP(w, 
						*shiftreg = shift;
						shift += 2;
						shift |= 1 << 8;
						*d++ = *s++);
					
					fb->status.word ^= SWAP_ROPMODE_MASK;
					*shiftreg = shift;
					*d = *s;
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			}
		}
	}
		break;
	case 1: {
		register short *d, *s;
		register int skew;
		d = cg2_ropwordaddr(fb, 0, dx, dy);
		{
			register struct mpr_data *mprd = mpr_d(spr);
			soffset = mprd->md_linebytes;
			skew = (sx += mprd->md_offset.x) & 15;
			s = (short *) PTR_ADD(mprd->md_image,
				pr_product(sy + mprd->md_offset.y, soffset) +
				(sx >> 4 << 1));
		}
		cg2_setlmask(fb, CG2_ALLROP,
			mrc_lmask(w = cg2_prskew(dx)));
		cg2_setshift(fb, CG2_ALLROP, cg2_prskew(w - skew), 1);
		skew = skew >= w ? 2 : 0;
		cg2_setrmask(fb, CG2_ALLROP,
			mrc_rmask(cg2_prskew(w += dw - 1)));
		w >>= 4;
		cg2_setwidth(fb, CG2_ALLROP, w, w);
		doffset = cg2_linebytes(fb, PWWWRD) - w - w;
		fb->status.reg.ropmode = PWWWRD;
		soffset = soffset - w - w - skew;
		if (skew) {
			register u_short *somereg = 
				&fb->ropcontrol[CG2_ALLROP].
					ropregs.mrc_source1;
			if (--w < 0)
				PR_LOOPP(dh - 1, 
					*somereg = *s++;
					*d = *s;
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			else if (w <= SWAP_ROPMODE_MIN || op)
				PR_LOOPP(dh - 1,
					*somereg = *s++;
					PR_LOOPP(w, *d++ = *s++);
					*d = *s;
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			else {
				somereg = (u_short *) &fb->status.word;
				w -= 2;
				cg2_setwidth(fb, CG2_ALLROP, 2, 2);
				PR_LOOPP(dh - 1,
					cg2_setrsource(fb, CG2_ALLROP, *s++);
					*d++ = *s++;
					*d++ = *s++;
					
					*somereg ^= SWAP_ROPMODE_MASK;
					PR_LOOPP(w, *d++ = *s++);
					
					*somereg ^= SWAP_ROPMODE_MASK;
					*d = *s;
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			}
		}
		else {
			if (--w < 0) 
				PR_LOOPP(dh - 1, 
					*d = *s;
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			else if (w <= SWAP_ROPMODE_MIN || op)
				PR_LOOPP(dh - 1,
					PR_LOOPP(w, *d++ = *s++);
					*d = *s;
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			else {
				register short *status = &fb->status.word;
				w -= 2;
				cg2_setwidth(fb, CG2_ALLROP, 2, 2);
				PR_LOOPP(dh - 1,
					*d++ = *s++;
					*d++ = *s++;
					
					*status ^= SWAP_ROPMODE_MASK;
					PR_LOOPP(w, *d++ = *s++);
					
					*status ^= SWAP_ROPMODE_MASK;
					*d = *s;
					PTR_INCR(short *, d, doffset);
					PTR_INCR(short *, s, soffset));
			}
		}
	}
		break;
	}
	
	
	if (tpr)
		(void) mem_destroy(spr);
	return 0;
}
#endif KERNEL

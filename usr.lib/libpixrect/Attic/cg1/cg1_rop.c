#ifndef lint
static	char sccsid[] = "@(#)cg1_rop.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Sun1 Color pixrect rasterop
 */
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/cg1reg.h>
#include <pixrect/cg1var.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>

/*
 * Rasterop involving cg1 colorbuf.  Normally we are called
 * as the destination via one of the macros in <pixrect/pixrect.h>,
 * but we may also be the source if another implementor wants
 * us to retrieve bits into memory.
 */
cg1_rop(dst, op, src)
	struct pr_subregion dst;
	struct pr_prpos src;
{
	register u_char *bx, *by, *ma;	/* ROOM FOR ONE MORE REGISTER */
	u_char *ma_top;
	register ma_vert;
	register short w, color;
	struct cg1fb *fb;
	struct cg1pr *sbd, *dbd;
	int skew, pumpprimer, errs=0;
#ifndef KERNEL
	struct pr_prpos tem1, tem2;

	tem1.pr = 0;
#endif

	if (op & PIX_DONTCLIP)
		op &= ~PIX_DONTCLIP;
	else
		pr_clip(&dst, &src);
	if (dst.size.x <= 0 || dst.size.y <= 0)
		return (0);
	if (dst.pr->pr_ops == &cg1_ops)
		goto cg1_write;
	if (src.pr->pr_ops != &cg1_ops) {
		return (-1);	/* neither src nor dst is cg */
	}

/*
 * Case 1 -- we are the source for the rasterop, but not the destination.
 * This is a rasterop from the color frame buffer to another kind of pixrect.
 * If the destination is not memory, then we will go indirect by allocating
 * a memory temporary, and then asking the destination to rasterop from there
 * into itself.
 */
	sbd = cg1_d(src.pr);
	src.pos.x += sbd->cgpr_offset.x;
	src.pos.y += sbd->cgpr_offset.y;

#ifndef KERNEL
	if (MP_NOTMPR(dst.pr)) {
		tem1.pr = dst.pr;
		tem1.pos = dst.pos;
		dst.pr = mem_create(dst.size, CG1_DEPTH);
		dst.pos.x = dst.pos.y = 0;
	}
	tem2.pr = 0;
	if ((op&PIX_SET) != PIX_SRC) {	/* handle non PIX_SRC ops since */
		tem2.pr = dst.pr;	/* hdwre rops don't work on fb reads */
		tem2.pos = dst.pos;
		dst.pr = mem_create(dst.size, CG1_DEPTH);
		dst.pos.x = dst.pos.y = 0;
	}
#endif	!KERNEL

	fb = cg1_fbfrompr(src.pr);
	bx = cg1_xpipe(fb, src.pos.x, 1, 0);	/* update on x reads */
	by = cg1_ypipe(fb, src.pos.y, 0, 0);
	ma_top = mprs8_addr(dst);
	ma_vert = mpr_mdlinebytes(dst.pr);

	w = dst.size.y;
	while (w-- > 0) {
		ma = ma_top;
		ma_top += ma_vert;
		cg1_touch(*by++);
		pumpprimer = *bx++;
		rop_fastloop(dst.size.x, *ma++ = *bx++)
		bx -= dst.size.x + 1; /* +1 is for pumpprimer */
	}
#ifndef KERNEL
	if (tem2.pr) {		/* temp mem to mem for non SRC op */
		errs |= (*tem2.pr->pr_ops->pro_rop)
		    (tem2, dst.size, op, dst.pr, dst.pos);
		mem_destroy(dst.pr);
		dst.pr = tem2.pr;
		dst.pos = tem2.pos;
	}
	if (tem1.pr) {		/* temp mem to other kind of pixrect */
		errs |= (*tem1.pr->pr_ops->pro_rop)
		    (tem1, dst.size, PIX_SRC, dst.pr, dst.pos);
		mem_destroy(dst.pr);
	}
#endif !KERNEL
	return (errs);
/* End Case 1 */

/*
 * Case 2 -- writing to the sun1 color frame buffer this consists of 4 different
 * cases depending on where the data is coming from:  from nothing, from
 * memory, from another type of pixrect, and from the frame buffer itself.
 */
cg1_write:
	dbd = cg1_d(dst.pr);
	dst.pos.x += dbd->cgpr_offset.x;
	dst.pos.y += dbd->cgpr_offset.y;
	fb = cg1_fbfrompr(dst.pr);
	color = PIX_OPCOLOR(op);
	op = (op>>1)&0xf;
	op = (op<<4)|op;

	if (src.pr == 0) {
		/*
		 * Case 2a: source pixrect is specified as null.
		 * In this case we interpret the source as color.
		 */
		short a;

		op = (CG_NOTMASK & CG_DEST)|(CG_MASK & op);
		cg1_setfunction(fb, op);
		cg1_setmask(fb, dbd->cgpr_planes);
		bx = cg1_xdst(fb, dst.pos.x); by = cg1_ydst(fb, dst.pos.y);
		if (dst.size.x < 6 || PIXOP_NEEDS_DST(op)) {
			while (dst.size.x-- > 0) {/* can't use paint mode */
				cg1_touch(*bx);
				rop_fastloop( dst.size.y, *by++ = color);
				bx++;
				by -= dst.size.y;
			}
		} else {			/* use 5 pixel paint mode */
			w = dst.pos.x % 5;
			if (w) {		/* fill to 1st 5 pixel bndry */
				a = w = 5-w;
				while (w-- > 0) {
					cg1_touch(*bx);
					rop_fastloop(dst.size.y, *by++ = color)
					bx++;
					by -= dst.size.y;
				}
				dst.size.x -= a;
			}
			cg1_setreg(fb, CG_STATUS, CG_VIDEOENABLE|CG_PAINT);
			while (dst.size.x > 4) {/* fill 5 pixel wide colums */
				cg1_touch(*bx);
				rop_fastloop( dst.size.y, *by++ = color);
				bx += 5;
				by -= dst.size.y;
				dst.size.x -= 5;
			}
			cg1_setreg(fb, CG_STATUS, CG_VIDEOENABLE);
			while (dst.size.x-- > 0) {	/* fill remainder */
				cg1_touch(*bx);
				rop_fastloop( dst.size.y, *by++ = color);
				bx++;
				by -= dst.size.y;
			}
		}
		return (0);
	}

#ifndef KERNEL
	if (src.pr->pr_ops == &cg1_ops) {
		/*
		 * Case 2b: rasterop within the frame buffer.
		 * Examine the source and destination for overlap so we don't
		 * clobber ourselves.
		 *
		 * BUG: If there are more than 1 of our kind of frame buffer
		 * another clause is required in the if statement above.
		 * We should treat a different sun-1 frame buffer as though
		 * it were a foreign device.
		 */
		int dir;
		u_char *sx;
		register u_char *sy;

		sbd = cg1_d(src.pr);
		src.pos.x += sbd->cgpr_offset.x;
		src.pos.y += sbd->cgpr_offset.y;
		dir = rop_direction(src.pos, sbd->cgpr_offset,
				dst.pos, dbd->cgpr_offset);
		op = (CG_NOTMASK & CG_DEST)|(CG_MASK & op);
		cg1_setfunction(fb, op);
		cg1_setmask(fb, dbd->cgpr_planes);
		if (rop_isdown(dir)) {
			src.pos.y += dst.size.y; dst.pos.y += dst.size.y;
		}
		if (rop_isright(dir)) {
			src.pos.x += dst.size.x; dst.pos.x += dst.size.x;
		}
		sx = cg1_xsrc(fb, src.pos.x); sy = cg1_ysrc(fb, src.pos.y);
		by = cg1_ydst(fb, dst.pos.y); bx = cg1_xdst(fb, dst.pos.x);
		while (dst.size.x-- > 0) {
			if (rop_isright(dir)) {
				sx--; bx--;
			}
			cg1_touch(*bx); cg1_touch(*sx);
			if (rop_isup(dir)) {
				w = *sy++;
				rop_fastloop(dst.size.y, *by++ = *sy++)
				by -= dst.size.y; sy -= dst.size.y+1;
			} else {
				w = *--sy;
				rop_fastloop(dst.size.y-1, *--by = *--sy)
				*--by = *sy;
				by += dst.size.y; sy += dst.size.y;
			}
			if (rop_isleft(dir)) {
				sx++; bx++;
			}
		}
		return (0);
	}

	if (MP_NOTMPR(src.pr)) {
		/*
		 * Case 2c: Source is some other kind of pixrect, but not
		 * memory.  Ask the other pixrect to read itself into
		 * temporary memory to make the problem easier.
		 */
		tem1.pr = mem_create(dst.size, CG1_DEPTH);
		tem1.pos.x = tem1.pos.y = 0;
		errs |= (*src.pr->pr_ops->pro_rop)(tem1, dst.size, PIX_SRC,src);
		src = tem1;
	}
#endif !KERNEL

	/*
	 * Case 2d: Source is a memory pixrect.  This
	 * case we can handle because the format of memory
	 * pixrects is public knowledge.
	 */
	ma_vert = mpr_mdlinebytes(src.pr);
	if (src.pr->pr_size.x == 1 && src.pr->pr_size.y == 1) {
		short a;

		bx = cg1_xdst(fb, dst.pos.x);
		by = cg1_ydst(fb, dst.pos.y);
		op = (CG_NOTMASK & CG_DEST)|(CG_MASK & op);
		cg1_setfunction(fb, op);
		cg1_setmask(fb, dbd->cgpr_planes);
		/*
		 * Case 2d.1: Source memory pixrect has width = height = 1.
		 * The source is this single pixel.  If the source pixrect
		 * depth is 1 the source value is zero or mapsize-1.
		 */
		if (src.pr->pr_depth == 1) {
			ma_top = (u_char*)mprs_addr(src);
			skew = mprs_skew(src);
			color = ((*(short*)ma_top<<skew) < 0) ? color : 0;
		} else {
			ma_top = mprs8_addr(src);
			color = *ma_top;
		}
		if (dst.size.x < 6) {		/* if narrow region */
			while (dst.size.x-- > 0) {
				cg1_touch(*bx);
				rop_fastloop( dst.size.y, *by++ = color);
				bx++;
				by -= dst.size.y;
			}
		} else {			/* use 5 pixel paint mode */
			w = dst.pos.x % 5;
			if (w) {		/* fill to 1st 5 pixel bndry */
				a = w = 5-w;
				while (w-- > 0) {
					cg1_touch(*bx);
					rop_fastloop(dst.size.y, *by++ = color)
					bx++;
					by -= dst.size.y;
				}
				dst.size.x -= a;
			}
			cg1_setreg(fb, CG_STATUS, CG_VIDEOENABLE|CG_PAINT);
			while (dst.size.x > 4) {/* fill 5 pixel wide colums */
				cg1_touch(*bx);
				rop_fastloop( dst.size.y, *by++ = color);
				bx += 5;
				by -= dst.size.y;
				dst.size.x -= 5;
			}
			cg1_setreg(fb, CG_STATUS, CG_VIDEOENABLE);
			while (dst.size.x-- > 0) {	/* fill remainder */
				cg1_touch(*bx);
				rop_fastloop( dst.size.y, *by++ = color);
				bx++;
				by -= dst.size.y;
			}
		}
	} else {		/* source pixrect is not degenerate */
		/*
		 * Case 2d.2: Source memory pixrect has width > 1 || height > 1.
		 * We copy the source pixrect to the destination.  If the source
		 * pixrect depth = 1 the source value is zero or color.
		 */
		bx = cg1_xpipe(fb, dst.pos.x, 1, 1);	/* update on x writes */
		by = cg1_ypipe(fb, dst.pos.y, 0, 1);
		if (src.pr->pr_depth == 1) {
			short i,n;
			register unsigned short *mwa;

			if (color == 0)
				color = -1;
			ma_top = (u_char*)mprs_addr(src);
			op = (CG_NOTMASK & CG_DEST)|(CG_MASK & op);
			cg1_setfunction(fb, op);
			cg1_setmask(fb, dbd->cgpr_planes);
			skew = mprs_skew(src);
			n = (dst.size.x > 15-skew) ? 16-skew : dst.size.x;
			while (dst.size.x > 0) {
			    i = dst.size.y;
			    mwa = ((unsigned short*)ma_top)++;
			    while( i-- > 0) {		/* do vertical band */
				cg1_touch( *by++);
				w = *mwa << skew;
				rop_fastloop(n,
					if (w<0) *bx++ = color;
						else *bx++ =0;
					w <<= 1 );
				(char *)mwa += ma_vert;
				bx -= n;
			    }
			    dst.size.x -= n;
			    bx += n;
			    by -= dst.size.y;
			    n = (dst.size.x > 15) ? 16 : dst.size.x;
			    skew = 0;
			}
		} else if (src.pr->pr_depth == 8) {
			ma_top = mprs8_addr(src);
			op = (CG_NOTMASK & CG_DEST)|(CG_MASK & op);
			cg1_setfunction(fb, op);
			cg1_setmask(fb, dbd->cgpr_planes);
			while (dst.size.y-- > 0) {
			    ma = ma_top;
			    ma_top += ma_vert;
			    cg1_touch(*by++);
			    rop_fastloop( dst.size.x, *bx++ = *ma++); 
			    bx -= dst.size.x;
			}
		}
	}

#ifndef KERNEL
	/*
	 * Finish off 2c cases by discarding temporary memory pixrect.
	 */
	if (tem1.pr)
		mem_destroy(tem1.pr);
#endif !KERNEL
#ifdef lint
	pumpprimer = pumpprimer;
#endif
	return (errs);			/* s u c c e s s ... */
}

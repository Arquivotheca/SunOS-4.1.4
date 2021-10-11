#ifndef lint
static	char sccsid[] = "@(#)bw1_rop.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Sun1 Black-and-White pixrect rasterop
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/bw1reg.h>
#include <pixrect/bw1var.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>

extern char	pr_reversesrc[];
extern char	pr_reversedst[];

/*
 * Rasterop involving bw1 framebuf.  Normally we are called
 * as the destination via one of the macros in <pixrect/pixrect.h>,
 * but we may also be the source if another implementor wants
 * us to retrieve bits into memory.
 */
bw1_rop(dst, op, src)
	struct pr_subregion dst;
	struct pr_prpos src;
{
	register short *bx, *by, *ma;	/* ROOM FOR ONE MORE REGISTER */
	short *ma_top;
	register ma_vert, w, color;
	struct bw1fb *fb;
	struct bw1pr *sbd, *dbd;
	int revvid, skew, pumpprimer, errs=0;
#ifndef KERNEL
	struct pr_prpos tem1, tem2;
	struct pr_size dsize;
	struct pr_pos dpos;

	tem1.pr = 0;
#endif

	if (op & PIX_DONTCLIP)
		op &= ~PIX_DONTCLIP;
	else
		pr_clip(&dst, &src);
	color = PIX_OPCOLOR(op);
	if (dst.size.x <= 0 || dst.size.y <= 0)
		return (0);
	if (dst.pr->pr_ops == &bw1_ops)
		goto bw1_write;
	if (src.pr->pr_ops != &bw1_ops)
		return (-1);		/* neither nor */

/*
 * Case 1 -- we are the source for the rasterop, but
 * not the destination.  This is a rasterop from the
 * frame buffer to another kind of pixrect.  If
 * the destination is not memory, then we will go indirect
 * by allocating a memory temporary, and then asking the
 * destination to rasterop from there into itself.
 */
	sbd = bw1_d(src.pr);
	src.pos.x += sbd->bwpr_offset.x;
	src.pos.y += sbd->bwpr_offset.y;

#ifndef KERNEL
	if (MP_NOTMPR(dst.pr)) {
		tem1.pr = dst.pr;
		tem1.pos = dst.pos;
		dst.pr = mem_create(dst.size,1);
		dst.pos.x = dst.pos.y = 0;
	}
	tem2.pr = 0;
	if ((op&PIX_SET) != PIX_SRC) {	/* handle non PIX_SRC ops since */
		tem2.pr = dst.pr;	/* hdwre rops don't work on fb reads */
		tem2.pos = dst.pos;
		dsize = dst.size;
		dst.pr = mem_create(dst.size, 1);
		dst.pos.x = dst.pos.y = 0;
		dpos = dst.pos;
	}
#endif

	revvid = bw1_d(src.pr)->bwpr_flags & BW_REVERSEVIDEO;
	fb = bw1_fbfrompr(src.pr);
	bw1_setwidth(fb, 16);
	by = bw1_ysrc(fb, src.pos.y);
	ma_vert = mpr_mdlinebytes(dst.pr);

	while (dst.size.x > 0) {
		skew = mprs_skew(dst);
		if (skew || dst.size.x < 16) {
			/* partial-word transfer, for both start and end */
			register dm = 0xffff0000>>skew, sm;
			int wv = 16-skew;

			if (dst.size.x < wv) {
				wv = dst.size.x;
				dm |= 0xffff>>(skew+wv);
			}
			sm = ~dm;
			bx = bw1_xsrc(fb, (src.pos.x-skew) & 1023);
			bw1_touch(*bx);
			pumpprimer = *by++;
			ma = mprs_addr(dst);
			if (revvid) {
				rop_slowloop(dst.size.y,
				    *ma = ((~*by++)&sm)|(*ma&dm);
				    (char *)ma += ma_vert)
			} else {
				rop_slowloop(dst.size.y,
				    *ma = (*by++&sm)|(*ma&dm);
				    (char *)ma += ma_vert)
			}
			dst.pos.x += wv;
			src.pos.x += wv;
			dst.size.x -= wv;
			by -= dst.size.y + 1; /* +1 is for pumpprimer */
		}

		/* Bulk of the copy: full 16-bit word transfers to memory */
		ma_top = mprs_addr(dst);
		bx = bw1_xsrc(fb, src.pos.x);
		while (dst.size.x > 15) {
			ma = ma_top++;
			bw1_touch(*bx);
			pumpprimer = *by++;
			if (revvid) {
				rop_fastloop(dst.size.y,
				    *ma = ~*by++; (char *)ma += ma_vert)
			} else {
				rop_fastloop(dst.size.y,
				    *ma = *by++; (char *)ma += ma_vert)
			}
			dst.pos.x += 16;
			src.pos.x += 16;
			bx += 16;
			dst.size.x -= 16;
			by -= dst.size.y + 1; /* +1 is for pumpprimer */
		}
	}
#ifndef KERNEL
	if (tem2.pr) {		/* temp mem to mem for non SRC op */
		errs |= (*tem2.pr->pr_ops->pro_rop)
		    (tem2, dsize, op, dst.pr, dpos);
		mem_destroy(dst.pr);
		dst.pr = tem2.pr;
		dst.pos = tem2.pos;
	}
	if (tem1.pr) {
		errs |= (*tem1.pr->pr_ops->pro_rop)
		    (tem1, dst.size, PIX_SRC, dst.pr, dst.pos);
		mem_destroy(dst.pr);
	}
#endif !KERNEL
	return (errs);
/* End Case 1 */

/*
 * Case 2 -- writing to the sun-1 frame buffer.
 * This consists of 4 differen cases depending
 * on where the data is coming from:  from nothing,
 * from memory, from another type of pixrect,
 * and from the frame buffer itself.
 */
bw1_write:
	op = (op>>1)&0xF;
	dbd = bw1_d(dst.pr);
	dst.pos.x += dbd->bwpr_offset.x;
	dst.pos.y += dbd->bwpr_offset.y;
	fb = bw1_fbfrompr(dst.pr);
	revvid = bw1_d(dst.pr)->bwpr_flags & BW_REVERSEVIDEO;

	if (src.pr == 0) {
		/*
		 * Case 2a: source pixrect is specified as null.
		 * In this case we interpret the source as the
		 * background color.
		 */
		color = (color) ? -1 : 0;
		if (revvid)
			op = pr_reversedst[op];
		bx = bw1_xdst(fb, dst.pos.x);
		by = bw1_ydst(fb, dst.pos.y);
		bw1_setfunction(fb, op);
		bw1_setwidth(fb, 16);
		bw1_setmask(fb, 0);
		while (dst.size.x > 0) {
			if (dst.size.x < 16)
				bw1_setwidth(fb, dst.size.x);
			bw1_touch(*bx);
			rop_fastloop(dst.size.y, *by++ = color)
			bx += 16;
			dst.size.x -= 16;
			by -= dst.size.y;
		}
		return (0);
	}


#ifndef KERNEL
	if (src.pr->pr_ops == &bw1_ops) {
		/*
		 * Case 2b: rasterop within the frame buffer.
		 * In this case we must carefully examine the
		 * source and destination for overlap so we don't
		 * clobber ourselves.
		 *
		 * BUG: We don't understand that there
		 * can be more than 1 of our kind of frame buffer.
		 * This should introduce another clause in the if
		 * statement above; we should treat a different
		 * sun-1 frame buffer as though it were a foreign device.
		 */
		int dir;
		short *sx;
		register short *sy;

		sbd = bw1_d(src.pr);
		src.pos.x += sbd->bwpr_offset.x;
		src.pos.y += sbd->bwpr_offset.y;
		dir = rop_direction(src.pos, sbd->bwpr_offset,
				dst.pos, dbd->bwpr_offset);
		if (revvid)
			op = pr_reversesrc[pr_reversedst[op]];
		bw1_setfunction(fb, op);
		if (rop_isdown(dir)) {
			src.pos.y += dst.size.y;
			dst.pos.y += dst.size.y;
		}
		if (rop_isright(dir)) {
			src.pos.x += dst.size.x;
			dst.pos.x += dst.size.x;
		}
		sx = bw1_xsrc(fb, src.pos.x);
		sy = bw1_ysrc(fb, src.pos.y);
		by = bw1_ydstcopy(fb, dst.pos.y);
		bx = bw1_xdst(fb, dst.pos.x);
		bw1_setwidth(fb, 16);
		bw1_setmask(fb, 0);
		w = 16;
		while (dst.size.x > 0) {
			if (dst.size.x < 16)
				bw1_setwidth(fb, w = dst.size.x);
			if (rop_isright(dir)) {
				sx -= w;
				bx -= w;
			}
			bw1_touch(*bx);
			bw1_touch(*sx);
			if (rop_isup(dir)) {
				rop_fastloop(dst.size.y, *by++ = *sy++)
				by -= dst.size.y;
				sy -= dst.size.y;
			} else {
				rop_fastloop(dst.size.y, *--by = *--sy)
				by += dst.size.y;
				sy += dst.size.y;
			}
			if (rop_isleft(dir)) {
				sx += 16;
				bx += 16;
			}
			dst.size.x -= 16;
		}
		return (0);
	}

	if (MP_NOTMPR(src.pr)) {
		/*
		 * Case 2c: Source is some other kind of
		 * pixrect, but not memory.  Ask the other
		 * pixrect to read itself into temporary memory
		 * to make the problem easier.
		 */
		tem1.pr = mem_create(dst.size, 1);
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
	if (revvid)
		op = pr_reversedst[op];
	bx = bw1_xdst(fb, dst.pos.x);
	by = bw1_ydst(fb, dst.pos.y);
	bw1_setfunction(fb, op|(BW_MASK&BW_DEST));
	bw1_setwidth(fb, 16);
	bw1_setmask(fb, 0);
	ma_vert = mpr_mdlinebytes(src.pr);

	/*
	 * If the source data doesn't start at the
	 * end of the source word, then we use the mask
	 * register to help toss the data away.  We depend
	 * on the fact that the frame buffer wraps around
	 * within an x-line preserving y, and have to be
	 * aware that the mask register is kept word-aligned
	 * by the hardware!
	 */
	skew = mprs_skew(src);
	if (skew) {
		short a[2];

		src.pos.x -= skew;
		dst.size.x += skew;
		bx -= skew;		/* may underflow; exercise caution */
		a[0] = a[1] = 0xffff0000 >> skew;
		bw1_setmask(fb, ((long *)a)[0] >> ((dst.pos.x - skew)&15));
	}
	bw1_touch(*(bx + (dst.pos.x < skew ? 1024 : 0)));	 /* cautios! */
	ma_top = mprs_addr(src);
	if (dst.size.y == 1) {
		/*
		 * We optimize the case where the source is a single
		 * scanline, since this is used when putting up textured
		 * polygons in the graphics library, and in other
		 * scanline algorithms.
		 */
		ma = ma_top; 
		if (dst.size.x < 16) {
			/*
			 * Both endpoints are in same word;
			 * we did a lot of work just to get here!
			 */
			bw1_setwidth(fb, dst.size.x);
			*by = *ma;
		} else {
			/*
			 * Endpoints are in separate words;
			 * run a loop for all but the last word,
			 * and handle the last word by carefully
			 * setting the word width.
			 */
			*by = *ma++;
			bw1_setmask(fb, 0);
			(char *)bx += 32 + bw1_activatex;
			w = 32;
			rop_fastloop((dst.size.x>>4)-1,
			    *bx = *ma++; (char *)bx += w);
			if ((skew = dst.size.x & 15) != 0) {
				bw1_setwidth(fb, skew);
				*bx = *ma;
			}
		}
	} else {
		/*
		 * Source is more than one scan line.
		 * Do column-by-column.
		 */
		for (;;) {
			ma = ma_top++; 
			if (dst.size.x < 16) 
				bw1_setwidth(fb, dst.size.x);
			rop_fastloop(dst.size.y,
			    *by++ = *ma; (char *)ma += ma_vert);
			if ((dst.size.x -= 16) <= 0)
				break;
			bw1_touch(*(bx += 16));
			by -= dst.size.y;
			bw1_setmask(fb, 0);
		}
	}
#ifndef	KERNEL
	/*
	 * Finish off 2c cases by discarding temporary memory pixrect.
	 */
	if (tem1.pr)
		mem_destroy(tem1.pr);
#endif
#ifdef lint
	pumpprimer = pumpprimer;	/* XXX */
#endif
	return (errs);			/* s u c c e s s ... */
}

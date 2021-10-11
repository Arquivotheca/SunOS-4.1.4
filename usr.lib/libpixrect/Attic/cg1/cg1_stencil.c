#ifndef lint
static        char sccsid[] = "@(#)cg1_stencil.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Sun1 Color pixrect stencil rasterop
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
cg1_stencil(dst, op, sten, src)
	struct pr_subregion dst;
	int op;
	struct pr_prpos sten, src;
{
	register u_char *bx, *by, *ma;	/* ROOM FOR ONE MORE REGISTER */
	u_char *ma_top;
	register ma_vert;
	register short w, color;
	struct cg1fb *fb;
	struct cg1pr *sbd, *dbd;
	struct pr_prpos tem;
	int skew, pumpprimer;

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
	if (dst.size.x <= 0 || dst.size.y <= 0 )
		return (0);
	if (MP_NOTMPR(sten.pr))
		return (-1);		/* stencil must be memory pr */
	if (sten.pr->pr_depth != 1) 	/* only depth 1 stencils */
		return (-1);
	tem.pr = 0;
	if (dst.pr->pr_ops == &cg1_ops)
		goto cg1_write;
	if (src.pr->pr_ops != &cg1_ops) {
		return (-1);		/* neither src nor dst is cg */
	}

/*
 * Case 1 -- cg1 is the source for the rasterop, but not the destination.
 * This is a rasterop from the color frame buffer to another kind of pixrect.
 * If the destination is not memory, then we will go indirect by allocating
 * a memory temporary, and then asking the destination to stencil from there
 * into itself.
 */
	sbd = cg1_d(src.pr);
	src.pos.x += sbd->cgpr_offset.x;
	src.pos.y += sbd->cgpr_offset.y;

#ifndef KERNEL
	if (MP_NOTMPR(dst.pr)) {
		tem.pr = dst.pr;
		tem.pos = dst.pos;
		dst.pr = mem_create(dst.size, CG1_DEPTH);
		dst.pos.x = dst.pos.y = 0;
	}
#endif	!KERNEL

	fb = cg1_fbfrompr(src.pr);
	bx = cg1_xpipe(fb, src.pos.x, 1, 0);
	by = cg1_ypipe(fb, src.pos.y, 0, 0);	/* update on x reads */
	ma_top = mprs8_addr(dst);
	ma_vert = mpr_mdlinebytes(dst.pr);
	if (sten.pr->pr_depth == 1) {		/* case: depth 1 stencils */
		short *stentop, *st, stenvert;
		short i, n, sizey;
		stentop = mprs_addr(sten);
		stenvert = mpr_mdlinebytes(sten.pr);
		skew = mprs_skew(sten);
		n = (dst.size.x > 15-skew) ? 16-skew : dst.size.x;
		while (dst.size.x > 0) {
			st = stentop;
			ma = ma_top;
			sizey = dst.size.y;
			while (sizey-- > 0) {
				pumpprimer = *bx++;
				cg1_touch(*by++);
				w = *st << skew;
				i = n;
				while(i-- > 0) {
					if (w<0)
						*ma++ = *bx++;
					else {
						pumpprimer = *bx++; ma++;
					}
					w <<= 1;
				}
				bx -= n+1;	/* +1 is for pumpprimer */
				ma += ma_vert - n;
				(char *)st += stenvert;
			}
			by -= dst.size.y;
			bx += n;
			dst.size.x -= n;
			ma_top += n;
			stentop++;
			n = (dst.size.x > 15) ? 16 : dst.size.x;
			skew = 0;
		}
	}
#ifndef KERNEL
	if (tem.pr) {
		int rc = (*tem.pr->pr_ops->pro_rop)
		    (tem, dst.size, PIX_SRC, dst.pr, dst.pos);

		mem_destroy(dst.pr);
		return (rc);
	}
#endif !KERNEL
	return (0);
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
	color = PIX_OPCOLOR( op);
	op = (op>>1)&0xf;
	op = (op<<4)|op;

	if (src.pr == 0) {
		/*
		 * Case 2a: source pixrect is specified as null.
		 * In this case we interpret the source as color.
		 */
		short *stentop, *st, stenvert;
		short i, n, sizey;
		stentop = mprs_addr(sten);
		stenvert = mpr_mdlinebytes(sten.pr);
		skew = mprs_skew(sten);
		n = (dst.size.x > 15-skew) ? 16-skew : dst.size.x;

		op = (CG_NOTMASK & CG_DEST)|(CG_MASK & op);
		cg1_setfunction(fb, op);
		cg1_setmask(fb, dbd->cgpr_planes);
		bx = cg1_xpipe(fb, dst.pos.x, 1, 0);	/* update on x reads */
		by = cg1_ypipe(fb, dst.pos.y, 0, 0);
		while (dst.size.x > 0) {
			st = stentop;
			sizey = dst.size.y;
			while (sizey-- > 0) {
				cg1_touch(*by++);
				w = *st << skew;
				i = n;
				while(i-- > 0) {
					if (w<0)
						*bx++ = color;
					else {
						bx++;
					}
					w <<= 1;
				}
				bx -= n;
				(char *)st += stenvert;
			}
			by -= dst.size.y;
			bx += n;
			dst.size.x -= n;
			stentop++;
			n = (dst.size.x > 15) ? 16 : dst.size.x;
			skew = 0;
		}
		return (0);
	}

#ifndef KERNEL
	if (src.pr->pr_ops == &cg1_ops) {
		/*
		 * Case 2b: stencil within the frame buffer.
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
		short *stentop, *st, stenvert;
		short i, n, sizey, ropup;

		sbd = cg1_d(src.pr);
		src.pos.x += sbd->cgpr_offset.x;
		src.pos.y += sbd->cgpr_offset.y;
		dir = rop_direction(src.pos, sbd->cgpr_offset,
				dst.pos, dbd->cgpr_offset);
		op = (CG_NOTMASK & CG_DEST)|(CG_MASK & op);
		cg1_setfunction(fb, op);
		cg1_setmask(fb, dbd->cgpr_planes);
		ropup = rop_isup( dir);
		if (rop_isdown(dir)) {
			src.pos.y += dst.size.y - 1;
			sten.pos.y += dst.size.y - 1;
			dst.pos.y += dst.size.y;
		}
		if (rop_isright(dir)) {
			src.pos.x += dst.size.x;
			sten.pos.x += dst.size.x - 1;
			dst.pos.x += dst.size.x;
		}
		sx = cg1_xpipe(fb, src.pos.x, 1, 0);	/* read */
		sy = cg1_ypipe(fb, src.pos.y, 0, 0);
		bx = cg1_xpipe(fb, dst.pos.x,  1, 1);	/* write */
		by = cg1_ypipe(fb, dst.pos.y,  0, 1);
		stentop = mprs_addr(sten);
		stenvert = mpr_mdlinebytes(sten.pr);
		skew = mprs_skew(sten);
		if (rop_isright(dir)){
			n = skew + 1;
			skew = 16 - n;
		} else {
			n = 16 - skew;
		}
		n = (dst.size.x > n) ? n : dst.size.x;
		while (dst.size.x > 0) {
			st = stentop;
			if (rop_isleft(dir)) {
				sizey = dst.size.y;
				while (sizey-- > 0) {
					cg1_touch(*by); cg1_touch(*sy);
					pumpprimer = *sx++;
					w = *st << skew;
					i = n;
					while(i-- > 0) {
						if (w<0)
						    *bx++ = *sx++;
						else {
						    pumpprimer = *sx++; bx++;
						}
						w <<= 1;
					}
					bx -= n;
					sx -= n+1;	/* +1 for pumpprimer */
					if (ropup) {
						(char *)st += stenvert;
						by++; sy++;
					} else {
						(char *)st -= stenvert;
						by--; sy--;
					}
				}
				sx += n; bx += n;
				if (ropup) {
					by -= dst.size.y; sy -= dst.size.y;
				} else {
					by += dst.size.y; sy += dst.size.y;
				}
				stentop++;
				dst.size.x -= n;
				n = (dst.size.x > 15) ? 16 : dst.size.x;
				skew = 0;
			} else /* if (rop_isright(dir)) */ {
				sizey = dst.size.y;
				while (sizey-- > 0) {
					cg1_touch(*by); cg1_touch(*sy);
					pumpprimer = *--sx;
					w = *st >> skew;
					i = n;
					while(i-- > 0) {
						if (w&1)
						    *--bx = *--sx;
						else {
						    pumpprimer = *--sx; --bx;
						}
						w >>= 1;
					}
					bx += n;
					sx += n+1;	/* +1 for pumpprimer */
					if (ropup) {
						(char *)st += stenvert;
						by++; sy++;
					} else {
						(char *)st -= stenvert;
						by--; sy--;
					}
				}
				sx -= n; bx -= n;
				if (ropup) {
					by -= dst.size.y; sy -= dst.size.y;
				} else {
					by += dst.size.y; sy += dst.size.y;
				}
				stentop--;
				dst.size.x -= n;
				n = (dst.size.x > 15) ? 16 : dst.size.x;
				skew = 0;
			}
		}
		return (0);
	}

	if (src.pr && MP_NOTMPR(src.pr)) {
		/*
		 * Case 2c: Source is some other kind of pixrect, but not
		 * memory.  Ask the other pixrect to read itself into
		 * temporary memory to make the problem easier.
		 */
		tem.pr = mem_create(dst.size, 8);
		tem.pos.x = tem.pos.y = 0;
		(*src.pr->pr_ops->pro_rop)(tem, dst.size, PIX_SRC, src);
		src = tem;
	}
#endif !KERNEL

	/*
	 * Case 2d: Source is a memory pixrect.  This
	 * case we can handle because the format of memory
	 * pixrects is public knowledge.
	 */
		/*
		 * Case 2d.2: Source memory pixrect has width > 1 || height > 1.
		 * We copy the source pixrect to the destination.  If the source
		 * pixrect depth = 1 the source value is zero or 255.
		 */
		bx = cg1_xpipe(fb, dst.pos.x, 1, 1);	/* update on x writes */
		by = cg1_ypipe(fb, dst.pos.y, 0, 1);
		if (src.pr->pr_depth == 1) {
			short *stentop, *st, stenvert, stskew, wsten;
			short i, n, nxt, sizey;
			short srcb, stenb, bump;
			register unsigned short *mwa;

			op = (CG_NOTMASK & CG_DEST)|(CG_MASK & op);
			cg1_setfunction(fb, op);
			cg1_setmask(fb, dbd->cgpr_planes);

			stentop = mprs_addr(sten);
			stenvert = mpr_mdlinebytes(sten.pr);
			stskew = mprs_skew(sten);

			ma_top = (u_char*)mprs_addr(src);
			ma_vert = mpr_mdlinebytes(src.pr);
			skew = mprs_skew(src);

			srcb = 16 - skew;	/* src-sten alignment setup */
			stenb = 16 - stskew;
			bump = (stskew >= skew)<<1;	/* stencil bump bit 1 */
			bump |=  (stskew <= skew);	/* source bump bit 0 */
			if (bump & 1) {
				n = srcb; nxt = stenb;
			} else {
				n = stenb; nxt = srcb;
			}
			n =  (dst.size.x > n) ? n : dst.size.x;

			mwa = ((unsigned short*)ma_top);
			st = stentop;
			while (dst.size.x > 0) {
			    sizey = dst.size.y;
			    while( sizey-- > 0) {	/* do vertical band */
				cg1_touch( *by++);
				w = *mwa << skew;
				wsten = *st << stskew;
				i = n;
				while (i-- > 0) {
					if (wsten < 0) {
						if (w < 0) {*bx++ = color;}
						else {      *bx++ = 0;    }
					} else {
						bx++;
					}
					wsten <<= 1; w <<= 1;
				}
				(char *)st += stenvert;
				(char *)mwa += ma_vert;
				bx -= n;
			    }
			    dst.size.x -= n;
			    bx += n;
			    by -= dst.size.y;
			    switch (bump) {
			    case 1: 
				mwa = ++((unsigned short*)ma_top);
				st = stentop;
				skew = 0; stskew += n;
			        n = nxt - n; 	nxt = 16;
				bump = 2;
				break;
			    case 2:
				mwa = ((unsigned short*)ma_top);
				st = ++stentop;
				stskew = 0; skew += n;
			        n = nxt - n; 	nxt = 16;
				bump = 1;
				break;
			    case 3:		/* src and sten in sync */
				mwa = ++((unsigned short*)ma_top);
				st = ++stentop;
				stskew = 0; skew = 0;
				n = 16;
				bump = 4;
				break;
			    case 4:
				mwa = ++((unsigned short*)ma_top);
				st = ++stentop;
				break;
			    }
			    n = (dst.size.x > n) ? n : dst.size.x;
			}
		} else if (src.pr->pr_depth == 8) {
			short *stentop, *st, stenvert;
			short i, n, sizey;

/* 			fb = cg1_fbfrompr(src.pr); */
			op = (CG_NOTMASK & CG_DEST)|(CG_MASK & op);
			cg1_setfunction(fb, op);
			cg1_setmask(fb, dbd->cgpr_planes);
			ma_top = mprs8_addr(src);
			ma_vert = mpr_mdlinebytes(src.pr);
			stentop = mprs_addr(sten);
			stenvert = mpr_mdlinebytes(sten.pr);
			skew = mprs_skew(sten);
			n = (dst.size.x > 15-skew) ? 16-skew : dst.size.x;
			
			while (dst.size.x > 0) {
				st = stentop;
				ma = ma_top;
				sizey = dst.size.y;
				while (sizey-- > 0) {
					cg1_touch(*by++);
					w = *st << skew;
					i = n;
					while(i-- > 0) {
						if (w<0)
							*bx++ = *ma++;
						else {
							bx++; ma++;
						}
						w <<= 1;
					}
					bx -= n;
			    		(char *)ma += ma_vert - n;
					(char *)st += stenvert;
				}
				by -= dst.size.y;
				bx += n;
				dst.size.x -= n;
/*  				ma_top++;  */
   				ma_top+= n;  
				stentop++;
				n = (dst.size.x > 15) ? 16 : dst.size.x;
				skew = 0;
			}
		}

#ifndef KERNEL
	/*
	 * Finish off 2c cases by discarding temporary memory pixrect.
	 */
	if (tem.pr)
		mem_destroy(tem.pr);
#endif !KERNEL
	return (0);			/* s u c c e s s ... */
}


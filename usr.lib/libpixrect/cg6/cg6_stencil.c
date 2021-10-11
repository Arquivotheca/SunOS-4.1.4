#ifndef lint
static char sccsid[] = "@(#)cg6_stencil.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1988-1989 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/cg6var.h>
#include <pixrect/pr_impl_util.h>

cg6_stencil(dpr, dx, dy, dw, dh, op, stpr, stx, sty, spr, sx, sy)
	Pixrect *dpr, *stpr, *spr;
	register int dh;
{
	int soffset, stoffset, x1, stskew, sskew, clip;
	register struct mpr_data *mprd;
	register unsigned int *s, *st;
	register int lshift, rshift;
	register struct fbc *fbc;
	register LOOP_T w;
	Pixrect *tpr = 0;
	int rw, stshift, sdepth;
	register unsigned int stw;

	/* stencil must be single plane memory pixrect */
	if (MP_NOTMPR(stpr) || stpr->pr_depth != 1)
		return PIX_ERR;

	/*
	 * Punt on source or stencil which is not a multiple of
	 * 4 bytes wide. This should only occur with images generated
	 * prior to 4.0 and used with mpr_static.
	 */ 
	mprd = mpr_d(stpr);
	if (mprd->md_linebytes & 3 ||
		mprd->md_flags & MP_REVERSEVIDEO ||
		spr && mpr_d(spr)->md_linebytes & 3) {
		fbc = cg6_d(dpr)->cg6_fbc;
		cg6_waitidle(fbc);
		return mem_stencil(dpr, dx, dy, dw, dh, op, 
			stpr, stx, sty, spr, sx, sy);
	}

	/* XXX use hardware for dst clipping */
	if (clip = !(op & PIX_DONTCLIP)) {
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

	if (dw <= 0 || dh <= 0 )
		return 0;

	/* initialize hardware */
	{
		register struct cg6pr *dprd = cg6_d(dpr);
		register Pixrect *rspr = spr;
		register int opcode = PIXOP_OP(op);
		register int color;

		fbc = dprd->cg6_fbc;


		if (PIXOP_NEEDS_SRC(op)) {
			/* identify weird type of fill: src is 1 x 1 */
			if (rspr && 
				rspr->pr_size.x == 1 && rspr->pr_size.y == 1) {
				color = pr_get(rspr, 0, 0);
				if (rspr->pr_depth == 1 && 
					color && (color = 
						PIXOP_COLOR(op)) == 0)
						color = ~color;

				rspr = 0;
			}
			else
				color = PIXOP_COLOR(op);
		}
		else {
			color = 0;
			rspr = 0;
		}

		spr = rspr;
		stoffset = mprd->md_linebytes;
		stx += mprd->md_offset.x;
		sty += mprd->md_offset.y;

		/* get 4 byte aligned stencil address */
		st = (u_int*) PTR_ADD(mpr_d(stpr)->md_image,
			(short) sty * (short) stoffset + (stx >> 5 << 2));

		if (rspr) {
			sdepth = rspr->pr_depth;

			/* non-memory pixrect source */
			if (MP_NOTMPR(rspr)) {
				if (sdepth != CG6_DEPTH &&
					sdepth != 1 ||
					!(rspr = mem_create(dw, dh, sdepth)))
					return PIX_ERR;

				if ((*spr->pr_ops->pro_rop)(rspr, 
					0, 0, dw, dh, 
					PIX_SRC | PIX_DONTCLIP, 
					spr, sx, sy)) {
					(void) mem_destroy(rspr);
					return PIX_ERR;
				}

				sx = 0; 
				sy = 0;

				tpr = rspr;
				spr = rspr;
			}

			mprd = mpr_d(rspr);
			soffset = mprd->md_linebytes;
			sx += mprd->md_offset.x;
			sy += mprd->md_offset.y;

			switch (sdepth) {
			case 1:
				if (color == 0) 
					color = ~color;

				/*
				 * check for reverse video source 
				 */
				if (mprd->md_flags & MP_REVERSEVIDEO) {
					opcode = PIX_OP_REVERSESRC(opcode);
				}

				/* get 4 byte aligned source address */
				s = (u_int *) PTR_ADD(mprd->md_image,
					(short) sy * (short) soffset +
					 (sx >> 5 << 2));

				/* get source skew */
				sskew = sx & 31;
				break;

			case CG6_DEPTH:
				color = ~color;

				/* get 4 byte aligned source address */
				s = (u_int *) PTR_ADD(mprd->md_image,
					(short) sy * (short) soffset +
					 (sx & ~3)); 

				/* get source skew */
				sskew = sx & 3;
				break;

			default:
				return PIX_ERR;
			}
		}

		/* set FBC registers */
		cg6_setregs(fbc,
			0, 0,				/* offset */
			opcode,
			dprd->mprp.planes,		/* plane mask */
			color,				/* fcolor (bcol = 0) */
			L_FBC_RASTEROP_PATTERN_ONES,	/* pattern */
			L_FBC_RASTEROP_POLYG_OVERLAP	/* polygon overlap */
		);

		/* turn on pixelmask */
		* ((u_int *) &(fbc)->l_fbc_rasterop) |=
			(int) L_FBC_RASTEROP_PIXEL_MASK << 28;


		/*
		 * set clip. The purpose of this is mostly to ensure
		 * that the left and right edges of the destination
		 * rectangle are not affected by extraneuos writes. This
		 * is analogous to the left/right masks used on the CG2.
		 */
		dx += dprd->mprp.mpr.md_offset.x;
		dy += dprd->mprp.mpr.md_offset.y;
		if (!clip) {
		  register int max_wd,max_ht;
 
		  max_wd = dprd->cg6_size.x - 1;
		  max_ht = dprd->cg6_size.y - 1;

		  if (dx < 0) 
		    dx = 0;
		  else if (dx > max_wd)
		    dx = max_wd;

		  if (dy < 0) 
		    dy = 0;
		  else if (dy > max_ht)
		    dy = max_ht;

		  if ((dx + dw) > max_wd)
		    dw = (max_wd - dx) + 1;

		  if ((dy + dh) > max_ht)
		    dh = (max_ht - dy) + 1;

		  /* adjust dw and dh if spr's/stpr's width and height are smaller */
		  if (spr) {
		    if (dw > spr->pr_size.x)
		      dw = spr->pr_size.x;
		    if (dh > spr->pr_size.y)
		      dh = spr->pr_size.y;
		  }
		  if (stpr) {
		    if (dw > stpr->pr_size.x)
		      dw = stpr->pr_size.x;
		    if (dh > stpr->pr_size.y)
		      dh = stpr->pr_size.y;
		  }
		}
		cg6_clip(fbc, dx,dy, dx+dw - 1, dy+dh -1);
	      }
	if (spr == 0) {
		/* no source */

		/*
		 * adjust destination to account
		 * for stencil skew. This assumes that
		 * clip rectangle left x has been set 
		 * to "dx".
		 */
		if (w = (stx & 31)) {
			dx -= w;
			dw += w;
		}

		/* font operation on (dx to dx + 31) */
		x1 = dx + 31;
		rshift = dx & 31;			/*destination skew */

		/* get width in 32 byte chunks */
		w = rshift + dw;			/* dest skew + width*/
		w = (w >> 5) + ((w & 31) > 0);		/* (w/32) + (w % 32) */

		/*
		 * make stencil offset 4 byte multiple 
		 * and decrement by width of dest.
		 */
		stoffset = (stoffset >> 2) - w;		
		w--;					/* adj for PR_LOOPP*/

		/* auto inc x by 32 (32 bit font write)*/
		cg6_setinx(fbc, 32, 0);

		/* single bit mode */
		cg6_color_mode(fbc, L_FBC_MISC_DATA_COLOR1);

		/* inverse destination skew */
		lshift = 32 - rshift;

		/*
		 * do stencil operation using ~0 as source value.
		 * Stencil longword is placed in the pixelmask
		 * such that pixelmask[dx & 31] == first bit of stencil.
		 * This is done by rotating the stencil word:
		 *	(stw>>rshift) | (stw << lshift).
		 */
		PR_LOOPP(dh - 1,
			cg6_setfontxy(fbc, dx, x1, dy++);
			PR_LOOPP(w,
				stw = *st++;
				fbc->l_fbc_pixelmask =
					 (stw>>rshift)|(stw<<lshift);
				fbc->l_fbc_font = ~0);
			st += stoffset);
		goto done;
	}

	/*
	 * if source not 4 byte aligned adjust destination
	 * offset and width.
	 */
	if (w = sskew) {
		dx -= w;
		dw += w; 
	}

	stskew = stx & 31;

	/* get destination skew and
	 * inverse skew. This is used to place
	 * stencil in pixelmask so that
	 * that pixelmask[dx & 31] == first bit of stencil.
	 */
	rshift = dx & 31;
	lshift = 32 - rshift;

	switch (sdepth) {
	case CG6_DEPTH:

		/* write 4 bytes at a time (x -> x+3) */
		x1 = dx + 3;

		/* find width of destination in 32 byte chunks. */ 
		w = (dw >> 2);

		/* find remainder of bytes on right */
		rw = (w & 7) + (dw & 3);
		w = w >> 3;

		soffset = (soffset >> 2) - (w << 3) - rw;
		stoffset = (stoffset >> 2) - w - (rw > 0) ;

		/* adjust loop counter for PR_LOOPP */
		rw--;

		cg6_setinx(fbc, 4, 0);
		cg6_color_mode(fbc, L_FBC_MISC_DATA_COLOR8);

		/*
		 * do stencil operation.
		 * Stencil longword is placed in the pixelmask
		 * such that pixelmask[dx & 31] == first bit of stencil.
		 * This is done by rotating the stencil word:
		 *	(stw>>rshift) | (stw << lshift).
		 */
		if (stskew >= sskew) {
			stskew -= sskew;
			stshift = 32 - stskew;
			if (stshift < 32) {
			PR_LOOPP(dh - 1,
				cg6_setfontxy(fbc, dx, x1, dy++);
				PR_LOOP(w,
					stw = (st[0] << stskew) |
						 (st[1] >> stshift);
					st++;
					fbc->l_fbc_pixelmask =
						(stw>>rshift)|(stw<<lshift);
					PR_LOOPP(7, fbc->l_fbc_font = *s++));

				if (rw >= 0) {
					stw = (st[0] << stskew) |
						 (st[1] >> stshift);
					st++;
					fbc->l_fbc_pixelmask =
						(stw>>rshift)|(stw<<lshift);
					PR_LOOPP(rw, fbc->l_fbc_font = *s++);
				}
				st += stoffset;
				s += soffset);
			} else {
			PR_LOOPP(dh - 1, 
                                cg6_setfontxy(fbc, dx, x1, dy++);
                                PR_LOOP(w,
                                        stw = st[0]; 
                                        st++;
                                        fbc->l_fbc_pixelmask =
                                                (stw>>rshift)|(stw<<lshift);
                                        PR_LOOPP(7, fbc->l_fbc_font = *s++));

                                if (rw >= 0) {
                                        stw = st[0];
                                        st++;
                                        fbc->l_fbc_pixelmask =
                                                (stw>>rshift)|(stw<<lshift);
                                        PR_LOOPP(rw, fbc->l_fbc_font = *s++);
                                }
                                st += stoffset;
                                s += soffset);
			}
		} 
		else {
			register int skew_check;
			stskew = sskew - stskew;
			stshift = 32 - stskew;
	
			if (w < 0) { 
				rw = -1;
				soffset -= 8;
			} else 
			    if (w > 0) 
			         stoffset++;
			w--;
			skew_check = w;

	 	 	PR_LOOPP(dh - 1,
				cg6_setfontxy(fbc, dx, x1, dy++);

				stw = *st >> stskew;
				fbc->l_fbc_pixelmask =
					 (stw>>rshift)|(stw<<lshift);

				if (w >= 0)
				    PR_LOOPP(7, fbc->l_fbc_font = *s++);

				 PR_LOOP(w,
					stw = (st[0] << stshift) |
						(st[1] >> stskew);
					st++;
					fbc->l_fbc_pixelmask =
						(stw>>rshift)|(stw<<lshift);
					PR_LOOPP(7, fbc->l_fbc_font = *s++) );   
				if (rw >= 0) {
					if (skew_check >= 0) {
						stw = (st[0] << stshift) | 
						(st[1] >> stskew);
					        st++;
					        fbc->l_fbc_pixelmask =
						    (stw>>rshift)|(stw<<lshift);
					} else skew_check = 1;
					PR_LOOPP(rw, fbc->l_fbc_font = *s++);
				} 
				st += stoffset;
				s += soffset);     

				} 
		break;

	case 1:
		/* doing font writes to (x -> x + 31) */
		x1 = dx + 31;

		/* get width of destination in 32 byte chunks */ 
		w = (dw >> 5) + ((dw & 31) > 0);

		/*	
		 * make offsets 4 byte multiples & subtract
		 * width of destination.
		 */
		stoffset = (stoffset >> 2) - w;
		soffset = (soffset >> 2) - w;

		/* auto increment x 32 bits */
		cg6_setinx(fbc, 32, 0);

		/* single bit color mode */
		cg6_color_mode(fbc, L_FBC_MISC_DATA_COLOR1);

		/*
		 * do stencil operation.
		 * Stencil longword is placed in the pixelmask
		 * such that pixelmask[dx & 31] == first bit of stencil.
		 * This is done by rotating the stencil word:
		 *	(stw>>rshift) | (stw << lshift).
		 * pick up and align two parts of stencil word
		 * if it is skewed.
		 */
		if (stskew >= sskew) {
			stskew -= sskew;
			stshift = 32 - stskew;
			w--;	/* adjust for PR_LOOPP */
		 	if (stshift < 32) {  
			PR_LOOPP(dh - 1, 
				cg6_setfontxy(fbc, dx, x1, dy++);
				PR_LOOPP(w,
					stw = (st[0] << stskew) |
						 (st[1] >> stshift);
					st++;
					fbc->l_fbc_pixelmask =
						(stw>>rshift)|(stw<<lshift);
					fbc->l_fbc_font = *s++);
				st += stoffset;
				s += soffset);
			} else {
                        PR_LOOPP(dh - 1,  
                                cg6_setfontxy(fbc, dx, x1, dy++);
                                PR_LOOPP(w,
                                        stw = st[0];
                                        st++;  
                                        fbc->l_fbc_pixelmask =
                                                (stw>>rshift)|(stw<<lshift);
                                        fbc->l_fbc_font = *s++);
                                st += stoffset;
                                s += soffset);
                        }  

		}
		else {
			stskew = sskew - stskew;
			stshift = 32 - stskew;

			stoffset++;
			w--;	/* XXX is this right? */
			PR_LOOPP(dh - 1, 
				cg6_setfontxy(fbc, dx, x1, dy++);

				stw = *st >> stskew;
				fbc->l_fbc_pixelmask =
					(stw>>rshift)|(stw<<lshift);
				fbc->l_fbc_font = *s++;

				PR_LOOP(w,
					stw = (st[0] << stshift) |
						(st[1] >> stskew);
					st++;
					fbc->l_fbc_pixelmask =
						(stw>>rshift) | (stw<<lshift);
					fbc->l_fbc_font = *s++);
				st += stoffset;
				s += soffset);

		}
		break;
	}

done:
	/* destroy temporary pixrect */
	if (tpr)
		(void) mem_destroy(tpr);

	/* reset pixel mask to fully enabled */
	fbc->l_fbc_pixelmask = ~0;
	return 0;
}

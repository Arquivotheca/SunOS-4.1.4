#ifndef lint
static char sccsid[] = "@(#)cg6_rop.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1988-1989, Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/cg6var.h>

 
cg6_rop(dpr, dx, dy, dw, dh, op, spr, sx, sy)
	Pixrect *dpr;
	int dx, dy, dw, dh;
	int op;
	Pixrect *spr;
	int sx, sy;
{
	register int soffset,clip;
	Pixrect mpr;

	/* Clip dest to src.
	 * The CG6 spec states that blit coordinates
	 * must be 'correct', implying that it
	 * will not do basic blit clipping.
	 * [ XXX  WRONG -- it clips to the dst but not the source ]
	 */
	if (clip = !(op & PIX_DONTCLIP)) {
		struct pr_subregion dst;
		struct pr_prpos src;

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

/*
 * Case 1 -- we are the source for the rasterop, but not the destination.
 * Pretend to be a memory pixrect and punt to dest's rop routine.
 */
	if (dpr->pr_ops->pro_rop != cg6_rop) {
		register struct cg6pr *sprd;
		int error;

		if (spr == 0 ||
			spr->pr_ops->pro_rop != cg6_rop ||
			dpr->pr_depth != CG6_DEPTH)
			return PIX_ERR;

		mpr = *spr;
		mpr.pr_ops = &mem_ops;
		sprd = cg6_d(spr);
		cg6_waitidle(sprd->cg6_fbc);
		return pr_rop(dpr, dx, dy, dw, dh, op, &mpr, sx, sy);
	}

/* End Case 1 */

/*
 * Case 2 -- writing to the CG6 color frame buffer this consists of 4 different
 * cases depending on where the data is coming from:  from nothing, from
 * memory, from another type of pixrect, and from the frame buffer itself.
 */

	{
	register struct fbc *fbc;
	Pixrect *tpr;

	/* do hardware initialization - rop, color,  & clip */
	{
		register int opcode = PIXOP_OP(op);
		register struct cg6pr *dprd = cg6_d(dpr);

		register int color;

		fbc = dprd->cg6_fbc;


		if (PIXOP_NEEDS_SRC(op)) {
			register Pixrect *rspr = spr;
			if (rspr) {
				/*
				 * identify weird type of fill: src is 1 x 1 
				 */
				if (rspr->pr_size.x == 1
				  && rspr->pr_size.y == 1) {
					color =  pr_get(rspr, 0, 0);
					if (rspr->pr_depth == 1 && color
					   && (color = PIX_OPCOLOR(op)) == 0)
						color = ~color;
					spr = 0;
				} else {
			  	   color = PIX_OPCOLOR(op);
				   /*
				    * If source given & eight bits, ignore color.
				    *   (eg. use ~0)
				    *
				    * If source given & 1 bit & color == 0
				    *   use color = ~0
				    */
				    if (rspr->pr_depth == 8 || color == 0) {
				    	   color = ~0;
				    }
  				}
			} else {
				/*
				 * No src given; always
				 * use color from rop arg.
				 */
				color = PIX_OPCOLOR(op);
			}
		}
		else 
			spr = 0;

		/* set raster op */
		cg6_setregs(fbc, 
			0, 0,					/* offset */
			opcode,
			dprd->mprp.planes,			/* planemask */
			color,					/* fcolor (bcol = 0) */
			L_FBC_RASTEROP_PATTERN_ONES,		/* pattern */
			L_FBC_RASTEROP_POLYG_OVERLAP		/* polygon overlap */
		);


		/* set clip rectangle */
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

		  /* adjust dw and dh if spr's width and height are smaller */
		  if (spr) {
		    if (dw > spr->pr_size.x)
		      dw = spr->pr_size.x;
		    if (dh > spr->pr_size.y)
		      dh = spr->pr_size.y;
		  }
		} 

		cg6_clip(fbc, dx, dy, dx+dw - 1, dy+dh - 1);

	      }

	/*
	 * Case 2a: source pixrect is specified as null.
	 * In this case we interpret the source as color.
	 * 
	 * Use the DRAW command with all ones, which will draw
	 * the foreground color. Note that 1's select was made when
	 * raster op register was set above.
	 */
	if (spr == 0) {
		register int x = dx;
		register int y = dy;
		register int r;

		fbc->l_fbc_irectabsy = y;
		fbc->l_fbc_irectabsx = x;

		fbc->l_fbc_irectabsy = y + dh - 1;
		fbc->l_fbc_irectabsx = x + dw - 1;

		/* now read DRAW_STATUS register to effect the DRAW op */
		
		cg6_draw_done(fbc, r);
		if (fbc_exception(r))
			return mem_rop(dpr,
				x - cg6_d(dpr)->mprp.mpr.md_offset.x,
				y - cg6_d(dpr)->mprp.mpr.md_offset.y,
				dw, dh, op, (Pixrect *) 0, 0, 0);
		return 0;
	}

	/*
	 * Case 2b: rasterop within the frame buffer.
	 * Use the CG6 blit engine.
	 */
	if (spr->pr_ops->pro_rop == cg6_rop &&
		cg6_d(spr)->cg6_fbc == fbc) {
		register struct cg6pr *sprd = cg6_d(spr);
		register rsx = sx;
		register rsy = sy;
		register rdx = dx;
		register rdy = dy;
		register r;

		rsx += sprd->mprp.mpr.md_offset.x;
		rsy += sprd->mprp.mpr.md_offset.y;

		fbc->l_fbc_x0 = rsx;
		fbc->l_fbc_y0 = rsy;
		fbc->l_fbc_x1 = rsx + dw;
		fbc->l_fbc_y1 = rsy + dh;
		fbc->l_fbc_x2 = rdx;
		fbc->l_fbc_y2 = rdy;
		fbc->l_fbc_x3 = rdx + dw;
		fbc->l_fbc_y3 = rdy + dh;

		do
			r = (int) fbc->l_fbc_blitstatus;
		while (fbc_exception(r) && (r & L_FBC_FULL));

		if (fbc_exception(r)) {
			mpr = *spr;
			mpr.pr_ops = &mem_ops;
			return mem_rop(dpr,
				rdx - cg6_d(dpr)->mprp.mpr.md_offset.x,
				rdy - cg6_d(dpr)->mprp.mpr.md_offset.y,
				dw, dh, op, &mpr, sx, sy);
		}
				
		return (0);
	}

	/*
	 * Case 2c: Source is some other kind of pixrect, but not
	 * memory.  Ask the other pixrect to read itself into
	 * temporary memory to make the problem easier.
	 */
	if (MP_NOTMPR(spr)) {
		tpr = spr;
		if (! (spr = mem_create(dw, dh, spr->pr_depth)))
			return (PIX_ERR);

		if ((*tpr->pr_ops->pro_rop)(spr, 0, 0, dw, dh,
			PIX_SRC | PIX_DONTCLIP, tpr, sx, sy)) {
			(void) mem_destroy(spr);
			return (PIX_ERR);
		}
	
		sx = 0;
		sy = 0;
	}
	else
		tpr = 0;

	/*
	 * Case 2d: Source is a memory pixrect.
	 * We can handle this case because the format of memory
	 * pixrects is public knowledge.
	 */
	switch (spr->pr_depth) {

	case CG6_DEPTH: 
		{
		   register struct mpr_data *mprd = &mprp_d(spr)->mpr;
		   register LOOP_T w;
		   int ss;
		   register rdx = dx;
			
		   cg6_color_mode(fbc, L_FBC_MISC_DATA_COLOR8);

		   soffset = mprd->md_linebytes;
		   ss = (int) PTR_ADD(mprd->md_image,
			(short)(sy + mprd->md_offset.y) * (short) soffset +
				(sx += mprd->md_offset.x) & ~1);

		   if (sx & 1) {
			dw++;
			rdx--;
		   }

		   /*
		    * If line size of mem pixrect is a multiple
		    * of 4 then we can do 4 byte transfer.
		    */
		   if ((soffset & 3) == 0) { 
			register unsigned int *s;
			register int off;


			/*
			 * Make source 4 byte aligned
			 */
			if (off = (ss & 3)) {
				ss -= off;
				dw += off;
				rdx -= off;
			}

			s = (unsigned int*)ss;
		
			w = (dw >> 2) + ((dw & 3) > 0);
			soffset = (soffset >> 2) - w;

			w--;

			/* we could optimize the case where w == 0,
			 * but it is not worth the extra test, as it is unlikely
			 * that pr_rop will be used on a 4 pixel pixrect.
			 */
			{
				register rdy = dy;
				register x1 = rdx+3;

				cg6_setinx(fbc, 4, 0);
				PR_LOOPP(dh - 1,
					cg6_setfontxy(fbc, rdx, x1, rdy++);
					PR_LOOPP(w, fbc->l_fbc_font = *s++);
					s += soffset);
			}
			goto done;
		   }

		   /* Pixrect width is not a multiple of 4 bytes. Use word moves.
		    */
		   {
			register unsigned short *s = (unsigned short*) ss;

			w = (dw >> 1) + (dw & 1);
			soffset = (soffset >> 1) - w;
	
			if (--w < 0) {
				/* width  < 2 bytes */
				
				cg6_setinx(fbc, 0, 1);
				cg6_setfontxy(fbc, rdx, rdx+1, dy);
				PR_LOOPP(dh - 1,
					fbc->l_fbc_font = *s << 16;
					s += soffset);
				break;

			} else {
				/* width >= 2 bytes*/
				register rdy = dy;
				register x1 = rdx + 1;

				cg6_setinx(fbc, 2, 0);
				PR_LOOPP(dh - 1,
					cg6_setfontxy(fbc, rdx, x1, rdy++);
					PR_LOOPP(w, fbc->l_fbc_font = *s++ << 16);
					s += soffset);
			}
		   }
		}
		break;

	case 1:
		/* single bit memory pixrect */
		{
		   register struct mpr_data *mprd = mpr_d(spr);
		   register LOOP_T w;
		   register rdx = dx;
		   register int skew;
		   int ss;

		/* XXX yuck */
		if (mprd->md_flags & MP_REVERSEVIDEO) {
			register int opcode = PIXOP_OP(op);

			opcode = PIX_OP_REVERSESRC(opcode);
			* ((u_int *) &(fbc)->l_fbc_rasterop) =
			(* ((u_int *) &(fbc)->l_fbc_rasterop) & ~0xffff) |
			opcode << 8 |
			(opcode & 3) << 2 |
			opcode & 3;
		}

		   /*
		    * Place the CG6 in COLOR1 mode, then use FONT register
		    * directly.
		    */
		   cg6_color_mode(fbc, L_FBC_MISC_DATA_COLOR1);

		   skew = (sx += mprd->md_offset.x) & 15;
		   soffset = mprd->md_linebytes;
		   ss = (int) PTR_ADD(mprd->md_image,
			(short)(sy + mprd->md_offset.y) * (short) soffset +
				(sx >> 4 << 1));

		   /*
		    * Get rid of skew
		    */
		   if (skew) {
			dw += skew;
			rdx -= skew;
		   }

		   /*
		    * If line size of mem pixrect is a multiple
		    * of 4 then we can do 4 byte transfer.
		    */
		   if ((soffset & 3) == 0) { 
			register unsigned int *s;

			/*
			 * If source memory address is off 4 byte alignment,
			 * push it left & inc width.
			 */
			if (skew = (ss & 3)) {
				ss -= skew;
				skew = 8 * skew;  /* 8 bits per byte */
				rdx -= skew;
				dw += skew;
			}

			s = (unsigned int*)ss;

			w = (dw >> 5) + ((dw & 31) > 0);
			soffset = (soffset >> 2) - w;

			if (--w < 0) {
				/* width is less than 4 bytes */

				cg6_setinx(fbc, 0, 1);
				cg6_setfontxy(fbc, rdx, rdx+31, dy);
				PR_LOOPP(dh - 1,
					fbc->l_fbc_font = *s;
					s += soffset);
			}else {
				/* width is 4 bytes or more */
				register rdy = dy;
				register x1 = rdx + 31;
	
				cg6_setinx(fbc, 32, 0);
				PR_LOOPP(dh - 1,
					cg6_setfontxy(fbc, rdx, x1, rdy++);
					PR_LOOPP(w, fbc->l_fbc_font = *s++);
					s += soffset);
			}
			goto done;
		   }

		   /* Pixrect width is not a multiple of 4 bytes. Use word moves.  */
		   {
			register unsigned short *s = (unsigned short*) ss;

			w = (dw >> 4) + ((dw & 15) > 0);
			soffset = (soffset >> 1) - w;
	
			if (--w < 0) {
				/* width is less than 1 word */

				cg6_setinx(fbc, 0, 1);
				cg6_setfontxy(fbc, rdx, rdx+15, dy);
				PR_LOOPP(dh - 1,
					fbc->l_fbc_font = *s << 16;
					s += soffset);
			}else{
				/* width one word or more */
				register rdy = dy;
				register x1 = rdx + 15;

				cg6_setinx(fbc, 16, 0);
				PR_LOOPP(dh - 1,
					cg6_setfontxy(fbc, rdx, x1, rdy++);
					PR_LOOPP(w, fbc->l_fbc_font = *s++ << 16);
					s += soffset);
			}
		   }
		}
	   } 
done:
	/*
	 * Finish off 2c cases by discarding temporary memory pixrect.
	 */
	if (tpr)
		(void) mem_destroy(spr);

	}

	return 0;
}

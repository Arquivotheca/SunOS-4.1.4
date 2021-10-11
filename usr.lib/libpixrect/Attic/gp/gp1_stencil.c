#ifndef lint
static char sccsid[] = "@(#)gp1_stencil.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1985, 1986 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>
#include <pixrect/gp1var.h>
#include <pixrect/memvar.h>

extern short gpmrc_lmasktable[];
extern short gpmrc_rmasktable[];

/*
 * Rasterop involving gp colorbuf.  Normally we are called
 * as the destination via one of the macros in <pixrect/pixrect.h>,
 * but we may also be the source if another implementor wants
 * us to retrieve bits into memory.
 */
gp1_stencil(dst, op, sten, src)
	struct pr_subregion dst;
	int op;
	struct pr_prpos sten, src;
{
	register u_char *bx, *by, *ma;	/* ROOM FOR ONE MORE REGISTER */
	u_char *ma_top;
	register ma_vert;
	register short w, color, linebytes;
	struct cg2fb *fb;
	struct gp1pr *sbd, *dbd;
	struct pr_prpos tem;
	short skew, sizex;

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
		return 0;

	if (MP_NOTMPR(sten.pr) || sten.pr->pr_depth != 1)
		return PIX_ERR;

	tem.pr = 0;

	if (dst.pr->pr_ops == &gp1_ops)
		goto gp1_write;

	if (src.pr->pr_ops != &gp1_ops) 
		return PIX_ERR;

/*
 * Case 1 -- gp1 is the source for the rasterop, but not the destination.
 * This is a rasterop from the color frame buffer to another kind of pixrect.
 * If the destination is not memory, then we will go indirect by allocating
 * a memory temporary, and then asking the destination to rasterop from there
 * into itself.
 */
	sbd = gp1_d(src.pr);
	src.pos.x += sbd->cgpr_offset.x;
	src.pos.y += sbd->cgpr_offset.y;

	/* first sync with the GP */
	if (gp1_sync(sbd->gp_shmem, sbd->ioctl_fd))
		return PIX_ERR;

	if (MP_NOTMPR(dst.pr)) {
		tem.pr = dst.pr;
		tem.pos = dst.pos;
		dst.pr = mem_create(dst.size, CG2_DEPTH);
		dst.pos.x = dst.pos.y = 0;
	}

	fb = gp1_fbfrompr(src.pr);

	fb->ppmask.reg = dbd->cgpr_planes;
	fb->status.reg.ropmode = SRWPIX;
	linebytes = cg2_linebytes( fb, SRWPIX);
	by = cg2_roppixaddr( fb, src.pos.x, src.pos.y);
	ma_top = mprs8_addr(dst);
	ma_vert = mpr_mdlinebytes(dst.pr);
	
	{
	short *stentop, *st, stenvert;
	short i, n, sizey, jump = dst.size.y * linebytes;
	short pumpprimer;

	stentop = mprs_addr(sten);
	stenvert = mpr_mdlinebytes(sten.pr);
	skew = mprs_skew(sten);
	n = (dst.size.x > 15-skew) ? 16-skew : dst.size.x;
	while (dst.size.x > 0) {
		bx = by;
		st = stentop;
		ma = ma_top;
		sizey = dst.size.y;
		while (sizey-- > 0) {
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
			bx -= n;
			ma += ma_vert - n;
			(char *)st += stenvert;
			by += linebytes;
		}
		by -= jump - n;
		dst.size.x -= n;
		ma_top += n;
		stentop++;
		n = (dst.size.x > 15) ? 16 : dst.size.x;
		skew = 0;
	}

	if (tem.pr) {
		int rc = (*tem.pr->pr_ops->pro_rop)
		    (tem, dst.size, PIX_SRC, dst.pr, dst.pos);

		mem_destroy(dst.pr);
		return (rc);
	}

	return (0);
	}
/* End Case 1 */

/*
 * Case 2 -- writing to the sun2 color frame buffer this consists of 4 different
 * cases depending on where the data is coming from:  from nothing, from
 * memory, from another type of pixrect, and from the frame buffer itself.
 */
gp1_write:

	/*
	 * 8 bit deep src is handled by mem_stencil
	 */
	if (src.pr &&
		(src.pr->pr_size.x != 1 || src.pr->pr_size.y != 1)) {
		if (src.pr->pr_depth == 8) 
			return mem_stencil(dst.pr, dst.pos.x, dst.pos.y,
				dst.size.x, dst.size.y,
				op | PIX_DONTCLIP, 
				sten.pr, sten.pos.x, sten.pos.y,
				src.pr, src.pos.x, src.pos.y);
		else if (src.pr->pr_depth != 1)
			return PIX_ERR;
	}

	dbd = gp1_d(dst.pr);
	dst.pos.x += dbd->cgpr_offset.x;
	dst.pos.y += dbd->cgpr_offset.y;
	fb = gp1_fbfrompr(dst.pr);
	color = PIX_OPCOLOR( op);
	op = (op>>1) & 0xF;
	op |= op<<4;
	op = (CG_NOTMASK & CG_DEST) | (CG_MASK & op);

	/* sync with the GP */
	if (gp1_sync(dbd->gp_shmem, dbd->ioctl_fd))
		return PIX_ERR;

	if (src.pr == 0) {
		/*
		 * Case 2a: source pixrect is specified as null.
		 * In this case we interpret the source as color.
		 */
		short *stentop, *st, stenvert, *patreg;
		short stskew, dskew, stshift, stprime;
		union {
			int l;
			short w[2];
		} stfifo;

		stentop = mprs_addr(sten);
		stenvert = mpr_mdlinebytes(sten.pr);
		stskew = mprs_skew(sten);
                dskew = cg2_prskew( dst.pos.x);
		stprime = (stskew > dskew);
		if (stprime)
			stshift = 16 - stskew + dskew;
		else	stshift = dskew - stskew;

		fb->ppmask.reg = dbd->cgpr_planes;
		cg2_setfunction(fb, CG2_ALLROP, op);
		cg2_setshift(fb, CG2_ALLROP, 0, 1);
		fb->ropcontrol[CG2_ALLROP].prime.ropregs.mrc_source2 =
			color | (color<<8);	/* load color in src */

		patreg =(short*)&fb->ropcontrol[CG2_ALLROP].ropregs.mrc_pattern;
		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 =
			gpmrc_lmasktable[dst.pos.x & 15];
		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 =
			gpmrc_rmasktable[(dst.pos.x+dst.size.x-1) & 15];
		fb->status.reg.ropmode = PWRWRD;
		linebytes = cg2_linebytes( fb, PWRWRD);
		w = (dst.size.x + dskew -1)>>4;
		cg2_setwidth( fb, CG2_ALLROP, w, w);

		by = (u_char *)cg2_ropwordaddr( fb, 0, dst.pos.x, dst.pos.y);
		w++;
		while (dst.size.y--) {
			st = stentop;
			bx = by;
			if (stprime) stfifo.w[1] = *st++;
			sizex = w;
			while (sizex-- > 0) {
				stfifo.w[0] = stfifo.w[1];
				stfifo.w[1] = *st++;
				*patreg = (stfifo.l >> stshift);
				*((short*)bx)++ = w;
			}
			by += linebytes;
			(char*)stentop += stenvert;
		}
		return (0);
	}

	if (MP_NOTMPR(src.pr)) {
		/*
		 * Case 2c: Source is some other kind of pixrect, but not
		 * memory.  Ask the other pixrect to read itself into
		 * temporary memory to make the problem easier.
		 */
		tem.pr = mem_create(dst.size, src.pr->pr_depth);
		tem.pos.x = tem.pos.y = 0;
		(*src.pr->pr_ops->pro_rop)(tem, dst.size, PIX_SRC, src);
		src = tem;
	}

	/*
	 * Case 2d: Source is a memory pixrect.  This
	 * case we can handle because the format of memory
	 * pixrects is public knowledge.
	 */
	ma_vert = mpr_mdlinebytes(src.pr);
	if (src.pr->pr_size.x == 1 && src.pr->pr_size.y == 1) {
		/*
		 * Case 2d.1: Source memory pixrect has width = height = 1.
		 * The source is this single pixel.  If the source pixrect
		 * depth is 1 the source value is zero or mapsize-1.
		 */
		short *stentop, *st, stenvert, *patreg;
		short stskew, dskew, stshift, stprime;
		union {
			int l;
			short w[2];
		} stfifo;

		stentop = mprs_addr(sten);
		stenvert = mpr_mdlinebytes(sten.pr);
		stskew = mprs_skew(sten);
                dskew = cg2_prskew( dst.pos.x);
		stprime = (stskew > dskew);
		if (stprime)
			stshift = 16 - stskew + dskew;
		else	stshift = dskew - stskew;

		if (src.pr->pr_depth == 1) {
			ma_top = (u_char*)mprs_addr(src);
			skew = mprs_skew(src);
			color = ( (*(short*)ma_top<<skew) < 0) ?
				color : 0;
		} else {
			ma_top = mprs8_addr(src);
			color = *ma_top;
		}

		fb->ppmask.reg = dbd->cgpr_planes;
		cg2_setshift( fb, CG2_ALLROP, 0, 1);
		fb->ropcontrol[CG2_ALLROP].prime.ropregs.mrc_source2 =
			color | (color<<8);	/* load color in src */

		fb->status.reg.ropmode = PWRWRD;
		linebytes = cg2_linebytes( fb, PWRWRD);
		cg2_setfunction(fb, CG2_ALLROP, op);
		cg2_setshift( fb, CG2_ALLROP, 0, 0);
		patreg =(short*)&fb->ropcontrol[CG2_ALLROP].ropregs.mrc_pattern;

		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 =
			gpmrc_lmasktable[dst.pos.x & 15];
		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 =
			gpmrc_rmasktable[(dst.pos.x+dst.size.x-1) & 0xf];
		w = (dst.size.x + dskew - 1)>>4;
		cg2_setwidth( fb, CG2_ALLROP, w, w);

		w++;
		by = (u_char *)cg2_ropwordaddr( fb, 0, dst.pos.x, dst.pos.y);
		while (dst.size.y--) {
			st = stentop;
			bx = by;
			if (stprime) stfifo.w[1] = *st++;
			sizex = w;
			while (sizex-- > 0) {
				stfifo.w[0] = stfifo.w[1];
				stfifo.w[1] = *st++;
				*patreg = (stfifo.l >> stshift);
				*((short*)bx)++ = color;
			}
			by += linebytes;
			(char*)stentop += stenvert;
		}
	} else {		/* source pixrect is not degenerate */
		/*
		 * Case 2d.2: Source memory pixrect has width > 1 || height > 1.
		 * We copy the source pixrect to the destination.  If the source
		 * pixrect depth = 1 the source value is zero or 255.
		 */
		if (src.pr->pr_depth == 1) {
			short prime, dskew, srcskew;
			register short *mwa;
			short *stentop, *st, stenvert, *patreg;
			short stskew, stshift, stprime;
			union {
				int l;
				short w[2];
			} stfifo;

			stentop = mprs_addr(sten);
			stenvert = mpr_mdlinebytes(sten.pr);
			stskew = mprs_skew(sten);
               		dskew = cg2_prskew( dst.pos.x);
			stprime = (stskew > dskew);
			if (stprime)
				stshift = 16 - stskew + dskew;
			else	stshift = dskew - stskew;

			srcskew = mprs_skew(src);
			ma_top = (u_char*)mprs_addr(src);
							/* set op,planes*/
			fb->ppmask.reg = dbd->cgpr_planes;
			cg2_setfunction(fb, CG2_ALLROP, op);
			fb->status.reg.ropmode = PWWWRD;
			linebytes = cg2_linebytes( fb, PWWWRD);
			patreg = (short*)&fb->ropcontrol[CG2_ALLROP].
				ropregs.mrc_pattern;

			cg2_setshift(fb,CG2_ALLROP,(dskew-srcskew)&0xf, 1);
			prime = (srcskew >= dskew);
			w = (dst.size.x + dskew - 1)>>4;/* set width,opcount */
			cg2_setwidth( fb, CG2_ALLROP, w, w);

							/* set src extract */
			fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 =
				gpmrc_lmasktable[dst.pos.x & 15];
			fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 =
				gpmrc_rmasktable[(dst.pos.x+dst.size.x-1) & 0xf];
		
			by = (u_char*)cg2_ropwordaddr(fb,0,dst.pos.x,dst.pos.y);

			w++;
			while (dst.size.y--) {
				st = stentop;
				mwa = (short*)ma_top;
				bx = by;
				if (stprime) stfifo.w[1] = *st++;
				if (prime) cg2_setrsource(fb,CG2_ALLROP,*mwa++);
				sizex = w;
				while (sizex-- > 0) {
					stfifo.w[0] = stfifo.w[1];
					stfifo.w[1] = *st++;
					*patreg = (stfifo.l >> stshift);
					*((short*)bx)++ = *mwa++;
				}
				by += linebytes;
				(char*)stentop += stenvert;
				(char *)ma_top += ma_vert;
			}
		} 
	}

	/*
	 * Finish off 2c cases by discarding temporary memory pixrect.
	 */
	if (tem.pr)
		mem_destroy(tem.pr);

	return (0);			/* s u c c e s s ... */
}


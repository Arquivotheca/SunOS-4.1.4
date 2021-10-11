#ifndef lint
static char sccsid[] = "@(#)gp1_rop.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1985, 1987 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>
#include <pixrect/gp1reg.h>
#include <pixrect/gp1var.h>
#include <pixrect/gp1cmds.h>
#include <pixrect/memvar.h>

short gpmrc_lmasktable[]={0x0000, 0x8000, 0xC000, 0xE000,
			0xF000, 0xF800, 0xFC00, 0xFE00,
			0xFF00, 0xFF80, 0xFFC0, 0xFFE0,
			0xFFF0, 0xFFF8, 0xFFFC, 0xFFFE  };
short gpmrc_rmasktable[]={0x7FFF, 0x3FFF, 0x1FFF, 0x0FFF,
			0x07FF, 0x03FF, 0x01FF, 0x00FF,
			0x007F, 0x003F, 0x001F, 0x000F,
			0x0007, 0x0003, 0x0001, 0x0000  };

/*
 * Rasterop involving cg2 colorbuf.  Normally we are called
 * as the destination via one of the macros in <pixrect/pixrect.h>,
 * but we may also be the source if another implementor wants
 * us to retrieve bits into memory.
 */
gp1_rop(dst, op, src)
	struct pr_subregion dst;
	struct pr_prpos src;
{
	register u_char *bx, *by, *ma;	/* ROOM FOR ONE MORE REGISTER */
	u_char *ma_top;
	register ma_vert, linebytes;
	register short w, color;
	struct cg2fb *fb;
	struct gp1pr *sbd, *dbd;
	int errs=0;
	int skew;

#ifndef KERNEL
        unsigned int bitvecptr;
        short offset, *bufptr;
	struct pr_prpos tem1, tem2;
#endif !KERNEL

#ifndef KERNEL
	tem1.pr = 0;
#endif !KERNEL

	if (dst.pr->pr_ops == &gp1_ops)
		goto gp1_write;

	if (op & PIX_DONTCLIP)
		op &= ~PIX_DONTCLIP;
	else
		pr_clip(&dst, &src);
	if (dst.size.x <= 0 || dst.size.y <= 0 )
		return (0);
	if (src.pr->pr_ops != &gp1_ops) {
		return (-1);	/* neither src nor dst is cg */
	}

/*
 * Case 1 -- we are the source for the rasterop, but not the destination.
 * This is a rasterop from the color frame buffer to another kind of pixrect.
 * If the destination is not memory, then we will go indirect by allocating
 * a memory temporary, and then asking the destination to rasterop from there
 * into itself.  Also, since rasterop hardware doesn't operate on reads from
 * the frame buffer, if op != SRC we must rop with PIX_SRC to temporary memory
 * then rop with op from memory to memory.
 */
	sbd = gp1_d(src.pr);
	src.pos.x += sbd->cgpr_offset.x;
	src.pos.y += sbd->cgpr_offset.y;

#ifndef KERNEL
	if (MP_NOTMPR( dst.pr)) {
		tem1.pr = dst.pr;
		tem1.pos = dst.pos;
		dst.pr = mem_create(dst.size, CG2_DEPTH);
		dst.pos.x = dst.pos.y = 0;
	}
	tem2.pr = 0;
	if ((op&PIX_SET) != PIX_SRC) {	/* handle non PIX_SRC ops since */
		tem2.pr = dst.pr;	/* hdwre rops don't work on fb reads */
		tem2.pos = dst.pos;
		dst.pr = mem_create(dst.size, CG2_DEPTH);
		dst.pos.x = dst.pos.y = 0;
	}
#endif	!KERNEL
	
	/* first sync with the GP */

#ifdef KERNEL
	if (gp1_kern_sync(sbd->gp_shmem))
		kernsyncrestart(sbd->gp_shmem);
#else
	if(gp1_sync(sbd->gp_shmem, sbd->ioctl_fd))
		return(-1);
#endif KERNEL

	fb = gp1_fbfrompr(src.pr);
	fb->ppmask.reg = 255;
	fb->status.reg.ropmode = SRWPIX;
	linebytes = cg2_linebytes( fb, SRWPIX);
	by = cg2_roppixaddr( fb, src.pos.x, src.pos.y);
	ma_top = mprs8_addr(dst);
	ma_vert = mpr_mdlinebytes(dst.pr);

	w = dst.size.y;
	while (w-- > 0) {
		bx = by;
		ma = ma_top;
		ma_top += ma_vert;
		rop_fastloop(dst.size.x, *ma++ = *bx++)
		by += linebytes;
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
gp1_write:
	dbd = gp1_d(dst.pr);

#ifdef KERNEL
	if (op & PIX_DONTCLIP)
		op &= ~PIX_DONTCLIP;
	else
		pr_clip(&dst, &src);
	if (dst.size.x <= 0 || dst.size.y <= 0 )
		return (0);
	dst.pos.x += dbd->cgpr_offset.x;
	dst.pos.y += dbd->cgpr_offset.y;
	fb = gp1_fbfrompr(dst.pr);
	color = PIX_OPCOLOR( op);
	op = (op>>1)&0xf;
	
	if (gp1_kern_sync(dbd->gp_shmem))
		kernsyncrestart(dbd->gp_shmem);

	if (src.pr == 0) {
		/*
		 * Case 2a: source pixrect is specified as null.
		 * In this case we interpret the source as color.
		 */
		short ropmode, nodst;

		fb->ppmask.reg = dbd->cgpr_planes;
		cg2_setfunction(fb, CG2_ALLROP, op);
		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_pattern = 0;
		fb->ropcontrol[CG2_ALLROP].prime.ropregs.mrc_source2 =
			color | (color<<8);	/* load color in src */

		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 =
			gpmrc_lmasktable[dst.pos.x & 0xF];
		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 =
			gpmrc_rmasktable[(dst.pos.x+dst.size.x-1) & 0xF];

		nodst = (PIXOP_NEEDS_DST(op<<1) == 0);
		if (nodst)
			ropmode = PRRWRD;	/* load dst when necessary */
		else
			ropmode = PWRWRD;	/* but never load src */
		fb->status.reg.ropmode = PWRWRD; /* ld dst for mask */

		linebytes = cg2_linebytes( fb, PWRWRD);

                w = cg2_prskew( dst.pos.x);
		w = (dst.size.x + w - 1)>>4;
		cg2_setwidth( fb, CG2_ALLROP, w, w);
		cg2_setshift( fb, CG2_ALLROP, 0, 1);

		by = (u_char *)cg2_ropwordaddr( fb, 0, dst.pos.x, dst.pos.y);
		switch (w) {
		case 0:
			w = dst.size.y;
			while (w--) {
				*((short*)by) = color;
				by += linebytes;
			}
			break;
		case 1:
			while (dst.size.y--) {
				bx = by;
				*((short*)bx)++ = color;
				*((short*)bx)++ = color;
				by += linebytes;
			}
			break;
		default:
			if (nodst) {
			    cg2_setwidth(fb, CG2_ALLROP, 2, 2);
			    w -= 2;
			    while (dst.size.y--) {
				    bx = by;
				    *((short*)bx)++ = color;
				    *((short*)bx)++ = color;
			    	    fb->status.reg.ropmode = ropmode;
				    rop_fastloop( w, *((short *)bx)++ = color);
				    fb->status.reg.ropmode = PWRWRD;
				    *((short*)bx)++ = color;
				    by += linebytes;
			    }
			} else {
			    while (dst.size.y--) {
				    bx = by;
				    rop_fastloop(w, *((short *)bx)++ = color);
				    *((short*)bx)++ = color;
				    by += linebytes;
			    }
			}
		}
		return (0);
	}

	if (src.pr->pr_ops == &gp1_ops) {
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
		short prime, ropmode, srcskew;
		int dir;

		sbd = gp1_d(src.pr);			/* set src and dir */
		src.pos.x += sbd->cgpr_offset.x;
		src.pos.y += sbd->cgpr_offset.y;
		dir = rop_direction(src.pos, sbd->cgpr_offset,
				dst.pos, dbd->cgpr_offset);

		skew = cg2_prskew( dst.pos.x);		/* words to xfer - 1 */
		w = (dst.size.x + skew - 1)>>4;

		if (rop_isdown(dir)) {			/* adjust direction */
			src.pos.y += dst.size.y -1;
			dst.pos.y += dst.size.y -1;
		}
		fb->ppmask.reg = dbd->cgpr_planes;
		cg2_setfunction(fb, CG2_ALLROP, op);
		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_pattern = 0;
		cg2_setwidth(fb, CG2_ALLROP, w, w);	/* set width,opcount */

		if (PIXOP_NEEDS_DST(op<<1))
			ropmode = PWRWRD;		/* set ropmode */
		else
			ropmode = PRRWRD;
							/* set src extract */
		if (rop_isright(dir) && w) {	/* addresses inc toward left */
			src.pos.x += dst.size.x -1;
			dst.pos.x += dst.size.x -1;
			fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 =
				gpmrc_rmasktable[dst.pos.x & 0xF];
			fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 =
				gpmrc_lmasktable[(dst.pos.x-dst.size.x+1) & 0xF];
			srcskew = 15 - cg2_prskew( src.pos.x);
			skew = 15 - cg2_prskew( dst.pos.x);
			cg2_setshift(fb,CG2_ALLROP,(srcskew - skew)&0xF,0);
			prime = srcskew >= skew;
		} else {		/* addresses increment toward right */
			fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 =
				gpmrc_lmasktable[dst.pos.x & 0xF];
			fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 =
				gpmrc_rmasktable[(dst.pos.x+dst.size.x-1) & 0xF];
			srcskew = cg2_prskew( src.pos.x);
			skew = cg2_prskew( dst.pos.x);
			cg2_setshift(fb,CG2_ALLROP,(skew-srcskew)&0xF,1);
			prime = srcskew >= skew;
		}
							/* src and dst addrs */
		ma_top =(u_char *)cg2_ropwordaddr(fb, 0, src.pos.x, src.pos.y);
		by = (u_char *)cg2_ropwordaddr(fb, 0, dst.pos.x, dst.pos.y);

		fb->status.reg.ropmode = PWRWRD;
		linebytes = cg2_linebytes( fb, PWRWRD);
		if (w) {
		    w--;			/* words to xfer minus two */
		    while (dst.size.y-- > 0) {
			ma = ma_top;
			bx = by;
			if (rop_isright(dir)) {
			    if (prime)
				cg2_setrsource(fb,CG2_ALLROP, *((short*)ma)--);
			    *((short*)bx)-- = *((short*)ma)--;
			    fb->status.reg.ropmode = ropmode;
			    rop_fastloop(w, *((short*)bx)-- = *((short*)ma)--);
			    fb->status.reg.ropmode = PWRWRD;
			    *((short*)bx) = *((short*)ma);
			} else {
			    if (prime)
				cg2_setlsource(fb,CG2_ALLROP, *((short*)ma)++);
			    *((short*)bx)++ = *((short*)ma)++;
			    fb->status.reg.ropmode = ropmode;
			    rop_fastloop(w,*((short*)bx)++ = *((short*)ma)++);
			    fb->status.reg.ropmode = PWRWRD;
			    *((short*)bx) = *((short*)ma);
			}
			if (rop_isdown(dir)) {
				ma_top -= linebytes;
				by -= linebytes;
			} else {
				ma_top += linebytes;
				by += linebytes;
			}
		    }
		} else {
		    fb->status.reg.ropmode = PWRWRD; /* ld dst for mask */
		    w = dst.size.y;
		    while (w-- > 0) {
			ma = ma_top;
			bx = by;
		        if (prime)
				cg2_setlsource(fb,CG2_ALLROP,*((short*)ma)++);
			*((short*)bx) = *((short*)ma);
			if (rop_isdown(dir)) {
				ma_top -= linebytes;
				by -= linebytes;
			} else {
				ma_top += linebytes;
				by += linebytes;
			}
		    }
		}
		return (0);
	}

#else KERNEL

	if (src.pr == 0) {
		/*
		 * Case 2a: source pixrect is specified as null.
		 * In this case we interpret the source as color.
		 */
 
                /* get a block from the ring buffer */
                if ( (offset = gp1_alloc(dbd->gp_shmem, 1,&bitvecptr, dbd->minordev, dbd->ioctl_fd)) == 0) {
			return(-1);
		}

                bufptr = &((short *) dbd->gp_shmem)[offset];
        
                /* write the command:GP1_PRROPNF and the planes mask*/
                *((short *)bufptr)++ = 
			GP1_PRROPNF | (dbd->cgpr_planes & 0xFF);
 
                /* write the color board index*/
                *((short *)bufptr)++ = dbd->cg2_index;
 
                /* write the op */
                *((short *)bufptr)++ = op;
 
                /* write the dst x offset */
                *((short *)bufptr)++ = dbd->cgpr_offset.x;
 
                /* write the dst y offset */
                *((short *)bufptr)++ = dbd->cgpr_offset.y;
 
                /* write the dst pixrect width */
                *((short *)bufptr)++ = (dst.pr)->pr_size.x;
 
                /* write the dst pixrect height */
                *((short *)bufptr)++ = (dst.pr)->pr_size.y;
 
                /* write dst rectangle x0 */
                *((short *)bufptr)++ = dst.pos.x;
  
                /* write dst rectangle y0 */
                *((short *)bufptr)++ = dst.pos.y;
 
                /* write dst rectangle size.x */
                *((short *)bufptr)++ = dst.size.x;
 
                /* write dst rectangle size.y */
                *((short *)bufptr)++ = dst.size.y;

                /* write end of command list and flag to free buffer block*/
                *((short *)bufptr)++ = GP1_EOCL | 0x80;
 
                /* write the bitvecptr*/
                *((unsigned int *)bufptr)++ = bitvecptr;
 
                /* post the command*/
                if(gp1_post(dbd->gp_shmem, offset, dbd->ioctl_fd))
			return(-1);
 
  		return(0);
	}

	if (src.pr->pr_ops == &gp1_ops) {
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
		short prime, ropmode, srcskew;
		int dir;

		sbd = gp1_d(src.pr);			/* set src and dir */

                /* get a block from the ring buffer */
                if ( (offset = gp1_alloc(dbd->gp_shmem, 1,&bitvecptr, dbd->minordev, dbd->ioctl_fd)) == 0)
			return(-1);

		bufptr = &((short *) dbd->gp_shmem)[offset];
 
                /* write the command:GP1_PRROPFF and the planes mask*/
                *((short *)bufptr)++ = GP1_PRROPFF | (dbd->cgpr_planes &
0xFF);
 
                /* write the index of the color board */
                *((short *)bufptr)++ = dbd->cg2_index;
 
                /* write the op */
                *((short *)bufptr)++ = op;
 
                /* write the dst x offset */
                *((short *)bufptr)++ = dbd->cgpr_offset.x;
 
                /* write the dst y offset */
                *((short *)bufptr)++ = dbd->cgpr_offset.y;
 
                /* write the dst pixrect width */
                *((short *)bufptr)++ = (dst.pr)->pr_size.x;
 
                /* write the dst pixrect height */
                *((short *)bufptr)++ = (dst.pr)->pr_size.y;
 
                /* write dst rectangle x0 */
                *((short *)bufptr)++ = dst.pos.x;
  
                /* write dst rectangle y0 */
                *((short *)bufptr)++ = dst.pos.y;
 
                /* write dst rectangle size.x */
                *((short *)bufptr)++ = dst.size.x;
 
                /* write dst rectangle size.y */
                *((short *)bufptr)++ = dst.size.y;

                /* write the src x offset */
                *((short *)bufptr)++ = sbd->cgpr_offset.x;

                /* write the src y offset */
                *((short *)bufptr)++ = sbd->cgpr_offset.y;

                /* write the src pixrect width */
                *((short *)bufptr)++ = (src.pr)->pr_size.x;

                /* write the src pixrect height */
                *((short *)bufptr)++ = (src.pr)->pr_size.y;

                /* write src origin x0 */
                *((short *)bufptr)++ = src.pos.x;

                /* write src origin y0 */
                *((short *)bufptr)++ = src.pos.y;
 
                /* write end of command list and flag to free buffer block*/
                *((short *)bufptr)++ = GP1_EOCL | 0x80;

                /* write the bitvecptr*/
                *((unsigned int *)bufptr)++ = bitvecptr;

                /* post the command*/
                if(gp1_post(dbd->gp_shmem, offset, dbd->ioctl_fd))
			return(-1);

                return(0);
	}


	if (MP_NOTMPR(src.pr)) {
		/*
		 * Case 2c: Source is some other kind of pixrect, but not
		 * memory.  Ask the other pixrect to read itself into
		 * temporary memory to make the problem easier.
		 */
		tem1.pr = mem_create(dst.size, CG2_DEPTH);
		tem1.pos.x = tem1.pos.y = 0;
		errs |= pr_rop(tem1.pr, tem1.pos.x, tem1.pos.y,
			dst.size.x, dst.size.y, 
			PIX_SRC, src.pr, src.pos.x, src.pos.y);
		src = tem1;
	}

	/*
	 * Case 2d: Source is a memory pixrect.  This
	 * case we can handle because the format of memory
	 * pixrects is public knowledge.
	 */

	if (op & PIX_DONTCLIP)
		op &= ~PIX_DONTCLIP;
	else
		pr_clip(&dst, &src);
	if (dst.size.x <= 0 || dst.size.y <= 0 )
		return (0);
	dst.pos.x += dbd->cgpr_offset.x;
	dst.pos.y += dbd->cgpr_offset.y;
	fb = gp1_fbfrompr(dst.pr);
	color = PIX_OPCOLOR( op);
	op = (op>>1)&0xf;
#endif !KERNEL

	ma_vert = mpr_mdlinebytes(src.pr);
	linebytes = cg2_linebytes( fb, PWRWRD);
	if (src.pr->pr_size.x == 1 && src.pr->pr_size.y == 1) {
		/*
		 * Case 2d.1: Source memory pixrect has width = height = 1.
		 * The source is this single pixel.  If the source pixrect
		 * depth is 1 the source value is zero or mapsize-1.
		 */
		if (src.pr->pr_depth == 1) {
			ma_top = (u_char*)mprs_addr(src);
			skew = mprs_skew(src);
			color = ( (*(short*)ma_top<<skew) < 0) ?
				color : 0;
		} else {
			ma_top = mprs8_addr(src);
			color = *ma_top;
		}

		/* first sync with the GP */

#ifdef KERNEL
		if (gp1_kern_sync(dbd->gp_shmem))
			kernsyncrestart(dbd->gp_shmem);
#else
		if (gp1_sync(dbd->gp_shmem, dbd->ioctl_fd))
			goto error;
#endif KERNEL

		fb->ppmask.reg = dbd->cgpr_planes;
		cg2_setfunction(fb, CG2_ALLROP, op);
		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_pattern = 0;
		fb->ropcontrol[CG2_ALLROP].prime.ropregs.mrc_source2 =
			color | (color<<8);	/* load color in src */
		cg2_setshift( fb, CG2_ALLROP, 0, 1);

		fb->status.reg.ropmode = PWRWRD;  /* rasterop mode */

		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 =
			gpmrc_lmasktable[dst.pos.x & 0xF];
		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 =
			gpmrc_rmasktable[(dst.pos.x+dst.size.x-1) & 0xF];
                w = cg2_prskew( dst.pos.x);
		w = (dst.size.x + w - 1)>>4;
		cg2_setwidth( fb, CG2_ALLROP, w, w);

		w++;							 
		by = (u_char *)cg2_ropwordaddr( fb, 0, dst.pos.x, dst.pos.y);
		while (dst.size.y-- > 0) {
			bx = by;
			rop_fastloop( w, *((short *)bx)++ = color);
			by += linebytes;
		}

	} else {		/* source pixrect is not degenerate */
		/*
		 * Case 2d.2: Source memory pixrect has width > 1 || height > 1.
		 * We copy the source pixrect to the destination.  If the source
		 * pixrect depth = 1 the source value is zero or color.
		 */
		if (src.pr->pr_depth == 1) {
			short ropmode, prime, nodst;
			register short *mwa;

			if (color == 0) color = -1;
			skew = mprs_skew(src);
			ma_top = (u_char*)mprs_addr(src);

			/* first sync with the GP */

#ifdef KERNEL
			if (gp1_kern_sync(dbd->gp_shmem))
				kernsyncrestart(dbd->gp_shmem);
#else
			if(gp1_sync(dbd->gp_shmem, dbd->ioctl_fd))
				goto error;
#endif KERNEL

			fb->ppmask.reg = color;
			fb->ropcontrol[CG2_ALLROP].ropregs.mrc_pattern = -1;
			fb->ppmask.reg = ~color;
			fb->ropcontrol[CG2_ALLROP].ropregs.mrc_pattern = 0;
							/* set op,planes*/
			fb->ppmask.reg = dbd->cgpr_planes;
			cg2_setfunction(fb, CG2_ALLROP, 
				op << 4 | (op & 3) << 2 | op & 3);

			nodst = (PIXOP_NEEDS_DST(op<<1) == 0);
			if (nodst)
				ropmode = PRWWRD;/* don't ld dst on wr */
			else	ropmode = PWWWRD;/* always ld src on wr */

			fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 =
				gpmrc_lmasktable[dst.pos.x & 0xF];
			fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 =
				gpmrc_rmasktable[(dst.pos.x+dst.size.x-1) & 0xF];

			w = cg2_prskew( dst.pos.x);
			cg2_setshift(fb,CG2_ALLROP,(w-skew)&0xF, 1);
			prime = (skew >= w);
			w = (dst.size.x + w -1)>>4;	/* set width,opcount */
			cg2_setwidth( fb, CG2_ALLROP, w, w);
			fb->status.reg.ropmode = PWWWRD;
		
			by = (u_char*)cg2_ropwordaddr(fb,0,dst.pos.x,dst.pos.y);
			switch (w) {
			case 0:
			    while (dst.size.y--) {
				    mwa = (short*)ma_top;
				    if (prime)
				        cg2_setrsource(fb,CG2_ALLROP,*mwa++);
				    *((short*)by) = *mwa;
				    by += linebytes;
				    (char *)ma_top += ma_vert;
			    }
			    break;
			case 1:
			    while (dst.size.y--) {
				    mwa = (short*)ma_top;
				    bx = by;
				    if (prime)
				        cg2_setrsource(fb,CG2_ALLROP,*mwa++);
				    *((short*)bx)++ = *mwa++;
				    *((short*)bx)++ = *mwa;
				    by += linebytes;
				    (char *)ma_top += ma_vert;
			    }
			    break;
			default:
			    if (nodst) {
				w -= 2;
				cg2_setwidth( fb, CG2_ALLROP, 2, 2);
				while (dst.size.y--) {
				    mwa = (short*)ma_top;
				    bx = by;
				    if (prime)
					cg2_setrsource(fb,CG2_ALLROP,*mwa++);
				    *((short*)bx)++ = *mwa++;
				    *((short*)bx)++ = *mwa++;
				    fb->status.reg.ropmode = ropmode;
				    rop_fastloop(w, *((short*)bx)++ = *mwa++);
				    fb->status.reg.ropmode = PWWWRD;
				    *((short*)bx)++ = *mwa;
				    by += linebytes;
				    (char *)ma_top += ma_vert;
			        }
			    } else {
				while (dst.size.y--) {
				    mwa = (short*)ma_top;
				    bx = by;
				    if (prime)
					cg2_setrsource(fb,CG2_ALLROP,*mwa++);
				    rop_fastloop(w, *((short*)bx)++ = *mwa++);
				    *((short*)bx)++ = *mwa;
				    by += linebytes;
				    (char *)ma_top += ma_vert;
			        }
			    }
			}
		} else if (src.pr->pr_depth == 8) {
			/*
			 * a raster line of byte pixels is padded to word 
			 * boundaries so that two pixel transfers to the 
			 * hardware can be done with word moves.  The ROPC
			 * masks will trim the odd pixels off the ends.
			 */
			ma_top = mprs8_addr(src);
			by = (u_char*)cg2_roppixaddr(fb,dst.pos.x,dst.pos.y);
			(int)ma = ((int)ma_top & 1);
			(int)bx = ((int)by & 1);
			if (ma != bx)
				goto bytemoves;
			w = dst.size.x + (int)ma;
			w = ((w+(w&1))>>1) -1;	/* wrds per raster line -1 */

			/* first sync with the GP */

#ifdef KERNEL
			if (gp1_kern_sync(dbd->gp_shmem))
				kernsyncrestart(dbd->gp_shmem);
#else
			if(gp1_sync(dbd->gp_shmem, dbd->ioctl_fd))
				goto error;
#endif KERNEL

			fb->ppmask.reg = dbd->cgpr_planes;
			cg2_setfunction(fb, CG2_ALLROP, op);
			fb->ropcontrol[CG2_ALLROP].ropregs.mrc_pattern = 0;
			cg2_setshift( fb, CG2_ALLROP, 0, 1);
			cg2_setwidth( fb, CG2_ALLROP, w, w);

			fb->status.reg.ropmode = SWWPIX;
			linebytes = cg2_linebytes( fb, SWWPIX);

			fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 =
				gpmrc_lmasktable[dst.pos.x & 0xF];
			fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 =
				gpmrc_rmasktable[(dst.pos.x+dst.size.x-1) & 0xF];

			ma_top = (u_char*)((int)ma_top & ~1);	/* word pad */
			by = (u_char*)((int)by & ~1);		/* word pad */
			while( dst.size.y-- > 0) {
			    ma = ma_top;
			    bx = by;
			    fb->ropcontrol[CG2_ALLROP].prime.ropregs.
				mrc_source1 = *((short*)ma)++;	/* prime fifo */
			    rop_fastloop(w, *((short*)bx)++ = *((short*)ma)++);
			    *((short*)bx)++ = w;		/* flush fifo */
			    by += linebytes;
			    ma_top += ma_vert;
			}
		goto finish;
bytemoves:
			w = dst.size.x - 1;
							/* set op,color,planes*/

			/* first sync with the GP */

#ifdef KERNEL
			if (gp1_kern_sync(dbd->gp_shmem))
				kernsyncrestart(dbd->gp_shmem);
#else
			if(gp1_sync(dbd->gp_shmem, dbd->ioctl_fd))
				goto error;
#endif KERNEL

			fb->ppmask.reg = dbd->cgpr_planes;
			cg2_setfunction(fb, CG2_ALLROP, op);
			fb->ropcontrol[CG2_ALLROP].ropregs.mrc_pattern = 0;
			cg2_setshift( fb, CG2_ALLROP, 0, 1);
			cg2_setwidth( fb, CG2_ALLROP, 0, 0);
			fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 = 0;
			fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 = 0;
			
			if (PIXOP_NEEDS_DST(op<<1))
				fb->status.reg.ropmode = SWWPIX;
			else    fb->status.reg.ropmode = SRWPIX;
			linebytes = cg2_linebytes( fb, SWWPIX);

			while( dst.size.y-- > 0) {
			    ma = ma_top;
			    bx = by;
			    fb->ropcontrol[CG2_ALLROP].prime.ropregs.
				mrc_source1 = *ma|(*ma << 8);	/* prime fifo */
			    ma++;
			    rop_fastloop(w, *bx++ = *ma++);
			    *bx++ = w;				/* flush fifo */
			    by += linebytes;
			    ma_top += ma_vert;
			}
		}
		else
			goto error;
	}
	goto finish;
error:
	errs = PIX_ERR;
finish:
#ifndef KERNEL
	/*
	 * Finish off 2c cases by discarding temporary memory pixrect.
	 */
	if (tem1.pr)
		mem_destroy(tem1.pr);
#endif !KERNEL
	return (errs);
}


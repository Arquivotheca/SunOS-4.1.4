#ifndef PROM
#ifndef lint
static	char sccsid[] = "@(#)mem_batch_v2.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Memory batchrop
 *
 * Note the use of several non-portable features:
 *	-- loopd6 which loops over the provided 2nd argument expression
 *	   until register d6 goes from 0 to -1.  Equivalent to:
 *		loopd6(a,b;) --> while((short)d6var-- != 0) b;
 *	-- the use of typecasts to the left of assignops -- for example,
 *		((unsigned short)mask) >>= dskew;
 *	   equivalent is: mask = ((unsigned short)mask) >> dskew;
 * Both of these are to generate better code.  They should be fixed by
 * fixing the C compiler to generate the better code in the portable case.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_util.h>

#ifndef PROM
extern char	pr_reversedst[];
extern struct pixrectops	mem_ops;

int	mem_nobatchrop = 0;
#endif PROM

#ifndef PROM
mem_batchrop(dst, op, src, count)
#else PROM
prom_mem_batchrop(dst, op, src, count)
#endif PROM
	struct pr_prpos dst;
	int op;
	struct pr_prpos *src;
	short count;
{
	register u_short *sp;
	register char *dp;
	register char *handy;
	register sizex, sizey;	/* sizey must be d6 */
	register vert, dskew;
	int errors = 0;
#ifndef PROM
	int clip, limx, limy;
	int oppassed = op;

	if (mem_nobatchrop /* || (op & 14) != 12 */ ) {
#define prp ((struct pixrect *)handy)
		while (--count >= 0) {
			dst.pos.x += src->pos.x;
			dst.pos.y += src->pos.y;
			prp = src->pr;
			if (prp)
				errors |= mem_rop(dst.pr, dst.pos, prp->pr_size,
				    oppassed, prp, 0, 0);
			src++;
		}
		return( errors);
#undef prp
	}
#endif PROM

	/*
	 * Preliminaries: get pixrect data and image
	 * pointers; decide whether clipping.  If not clipping,
	 * normalize op, else compute limits for cursors for later
	 * comparisons.
	 */

#ifndef PROM
	clip = 0;
	if (!(op & PIX_DONTCLIP)) {
		clip = 1;
		limx = dst.pr->pr_size.x;
		limy = dst.pr->pr_size.y;
	}
#endif PROM
	op = (op>>1)&0xf;		/* Kill dontclip, just keep useful */
#ifndef PROM
	/*
	 * If destination is reverse video, invert function.
	 * FIXME: we don't deal with a reverse video source.
	 * Admittedly it's unlikely that anyone will call batchrop with
	 * a device pixrect as source (since we copy the whole pixrect),
	 * but this is a bug.
	 */
	if (mpr_d(dst.pr)->md_flags & MP_REVERSEVIDEO)
		op = pr_reversedst[op];
#endif PROM

	vert = mpr_d(dst.pr)->md_linebytes;
	for (; --count >= 0; src++) {
		dst.pos.x += src->pos.x;
		dst.pos.y += src->pos.y;
#define dprd ((struct mpr_data *)handy)
		dprd = mpr_d(dst.pr);
		dp = (char *)mprd_addr(dprd, dst.pos.x, dst.pos.y);
		dskew = mprd_skew(dprd, dst.pos.x, dst.pos.y);
#undef dprd
#define spr ((struct pixrect *)handy)
		spr = src->pr;
		if (spr == 0)
			continue;
		sizex = spr->pr_size.x;
		sizey = spr->pr_size.y;
#ifndef PROM
		if (spr->pr_ops != &mem_ops)
			goto hard;
#endif PROM
#define sprd ((struct mpr_data *)handy)
		sprd = mpr_d(spr);
#undef spr
#ifndef PROM
		if (((sprd->md_linebytes^2)|sprd->md_offset.x|sprd->md_offset.y)
			!= 0)
			goto hard;
#endif PROM
		sp = (u_short *)sprd->md_image;
#undef sprd

#ifndef PROM
		/*
		 * If clipping (rare case hopefully)
		 * compare cursors against limits.
		 */
		if (clip) {
			if (dst.pos.x + sizex > limx)
				goto hard;
			if (dst.pos.y + sizey > limy)
				sizey = limy - dst.pos.y;
			if (dst.pos.x < 0)
				goto hard;
			if (dst.pos.y < 0) {
				sizey += dst.pos.y;
				sp -= dst.pos.y;
				dp -= pr_product(dst.pos.y, vert);
			}
			if (sizex <= 0 || sizey <= 0)
				continue;
		}
#endif PROM
		switch (op) {
		case (PIX_SRC^PIX_DST)>>1: 		/* xor */
			if (dskew + sizex <= 16) {
/* 				loopd6(xora, *(u_short *)dp ^= *sp++ >> dskew; dp += vert;) */
				sizey--;
				do {
				    *(u_short *)dp ^= *sp++ >> dskew; dp += vert;
				} while (--sizey != -1);
			} else {
				dskew = 16 - dskew;
/* 				loopd6(xorb, *(u_int *)dp ^= *sp++ << dskew; dp += vert;) */
				sizey--;
				do {
				    *(u_int *)dp ^= *sp++ << dskew; dp += vert;
				} while (--sizey != -1);
			}
			break;
		case (PIX_NOT(PIX_DST))>>1:		/* not dst */
			if (dskew + sizex <= 16) {
				register short mask;
				mask = 0x8000;
				sizex -= 1;
				mask >>= sizex;
				((unsigned short)mask) >>= dskew;
/* 				loopd6(notdsta, *(u_short *)dp ^= mask; dp += vert;) */
				sizey--;
				do {
				    *(u_short *)dp ^= mask; dp += vert;
				} while (--sizey != -1);
			} else {
				register long mask;
				mask = 0x80000000;
				sizex -= 1;
				mask >>= sizex;
				((unsigned long)mask) >>= dskew;
/* 				loopd6(notdstb, *(u_int *)dp ^= mask; dp += vert;) */
				sizey--;
				do {
				    *(u_int *)dp ^= mask; dp += vert;
				} while (--sizey != -1);
			}
			break;
			
		case (PIX_CLR)>>1:
			if (dskew + sizex <= 16) {
				register short mask;
				mask = 0x8000;
				sizex -= 1;
				mask >>= sizex;
				((unsigned short)mask) >>= dskew;
				mask = ~mask;
/* 				loopd6(clra, *(u_short *)dp &= mask; dp += vert;) */
				sizey--;
				do {
				    *(u_short *)dp &= mask; dp += vert;
				} while (--sizey != -1);
			} else {
				register long mask;
				mask = 0x80000000;
				sizex -= 1;
				mask >>= sizex;
				((unsigned long)mask) >>= dskew;
				mask = ~mask;
/* 				loopd6(clrb, *(u_int *)dp &= mask; dp += vert;) */
				sizey--;
				do {
				    *(u_int *)dp &= mask; dp += vert;
				} while (--sizey != -1);
			}
			break;
			
		case (PIX_SET)>>1:
			if (dskew + sizex <= 16) {
				register short mask;
				mask = 0x8000;
				sizex -= 1;
				mask >>= sizex;
				((unsigned short)mask) >>= dskew;
/* 				loopd6(seta, *(u_short *)dp |= mask; dp += vert;) */
				sizey--;
				do {
				    *(u_short *)dp |= mask; dp += vert;
				} while (--sizey != -1);
			} else {
				register long mask;
				mask = 0x80000000;
				sizex -= 1;
				mask >>= sizex;
				((unsigned long)mask) >>= dskew;
/* 				loopd6(setb, *(u_int *)dp |= mask; dp += vert;) */
				sizey--;
				do {
				    *(u_int *)dp |= mask; dp += vert;
				} while (--sizey != -1);
			}
			break;
			
		case (PIX_SRC)>>1:
			if (dskew + sizex <= 16) {
				register short mask;
				mask = 0x8000;
				sizex -= 1;
				mask >>= sizex;
				((unsigned short)mask) >>= dskew;
				mask = ~mask;
/* 				loopd6(srca, *(u_short *)dp &= mask; *(u_short *)dp |= *sp++ >> dskew; dp += vert;) */
				sizey--;
				do {
				    *(u_short *)dp &= mask; *(u_short *)dp |= *sp++ >> dskew; dp += vert;
				} while (--sizey != -1);
			} else {
				register long mask;
				mask = 0x80000000;
				sizex -= 1;
				mask >>= sizex;
				((unsigned long)mask) >>= dskew;
				mask = ~mask;
				dskew = 16 - dskew;
/* 				loopd6(srcb, *(u_int *)dp &= mask; *(u_int *)dp |= *sp++ << dskew; dp += vert;) */
				sizey--;
				do {
				    *(u_int *)dp &= mask; *(u_int *)dp |= *sp++ << dskew; dp += vert;
				} while (--sizey != -1);
			}
			break;
			
		case (PIX_NOT(PIX_SRC))>>1:
			if (dskew + sizex <= 16) {
				register short mask;
				mask = 0x8000;
				sizex -= 1;
				mask >>= sizex;
				((unsigned short)mask) >>= dskew;
/* 				loopd6(notsrca, *(u_short *)dp |= mask; *(u_short *)dp ^= *sp++ >> dskew; dp += vert;) */
				sizey--;
				do {
				    *(u_short *)dp |= mask; *(u_short *)dp ^= *sp++ >> dskew; dp += vert;
				} while (--sizey != -1);
			} else {
				register long mask;
				mask = 0x80000000;
				sizex -= 1;
				mask >>= sizex;
				((unsigned long)mask) >>= dskew;
				dskew = 16 - dskew;
/* 				loopd6(notsrcb, *(u_int *)dp |= mask; *(u_int *)dp ^= *sp++ << dskew; dp += vert;) */
				sizey--;
				do {
				    *(u_int *)dp |= mask; *(u_int *)dp ^= *sp++ << dskew; dp += vert;
				} while (--sizey != -1);
			}
			break;
			
		case (PIX_SRC|PIX_DST)>>1:		/* or */
			if (dskew + sizex <= 16) {
/* 				loopd6(ora, *(u_short *)dp |= *sp++ >> dskew; dp += vert;) */
				sizey--;
				do {
				    *(u_short *)dp |= *sp++ >> dskew; dp += vert;
				} while (--sizey != -1);
			} else {
				dskew = 16 - dskew;
/* 				loopd6(orb, *(u_int *)dp |= *sp++ << dskew; dp += vert;) */
				sizey--;
				do {
				    *(u_int *)dp |= *sp++ << dskew; dp += vert;
				} while (--sizey != -1);
			}
			break;

		default:		
#ifdef PROM
			*(short *)1 = 0;	/* Trap out */
#else  PROM
			goto hard;
#endif PROM
		}
		continue;

#ifndef PROM
		/*
		 * Too hard... give up and call complete
		 * rop routine.  Used if source is skewed,
		 * or if operation unimplemented above,
		 * or if source is more than 2 bytes wide,
		 * or if clipping causes us to chop off
		 * in left-x direction.
		 */
hard:
		if (dst.pos.x + sizex <= 0)
			/*
			 * Completely clipped on left...
			 */
			;
		else {
			errors |= mem_rop(dst.pr, dst.pos, src->pr->pr_size,
			    oppassed, src->pr, 0, 0);
		}
		continue;
#endif PROM
	}
	return( errors);
}

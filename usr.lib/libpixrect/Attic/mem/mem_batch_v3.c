#if !defined(lint) && !defined(PROM)
static char sccsid[] = "@(#)mem_batch_v3.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems,  Inc.
 */

/*
 * Memory batchrop
 *
 * This file contains mem_batchrop and prom_mem_batchrop.
 * The PROM version uses older code; it cannot be upgraded until the
 * new code is tested in the PROM environment.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_util.h>

#ifndef PROM

extern char     pr_reversedst[];


#define MEMBATCH(IfClip, IfMask, op, IfReverse)				\
    for (; --count >= 0; src++) {                                       \
        dst.pos.x += src->pos.x;                                        \
        if (src->pos.y != 0) { dp0 += pr_product(src->pos.y, vert);     \
                               dst.pos.y += src->pos.y; }               \
        spr = src->pr;                                                  \
        sizex = spr->pr_size.x;                                         \
        sizey = spr->pr_size.y;                                         \
	if (MP_NOTMPR(spr) || spr->pr_depth != 1)			\
            goto hard;                                                  \
        sprd = mpr_d(spr);                                              \
        if (sprd->md_linebytes != 2 || sprd->md_flags & MP_REVERSEVIDEO) \
            goto hard;                                                  \
        dp = dp0 + (((dskew = xoff0 + dst.pos.x) >> 3) & ~1);           \
        dskew &= 0xF;                                                   \
        sp = (u_short *) sprd->md_image;                                \
        IfClip( if (dst.pos.x + sizex > limx)                           \
                    goto hard;                                          \
                if (dst.pos.y + sizey > limy)                           \
                sizey = limy - dst.pos.y;                               \
                if (dst.pos.x < 0)                                      \
                    goto hard;                                          \
                if (dst.pos.y < 0) {                                    \
                    sizey += dst.pos.y;                                 \
                    sp -= dst.pos.y;                                    \
                    dp -= pr_product(dst.pos.y, vert);                  \
                }                                                       \
                if (sizex <= 0)                                         \
                    continue;                                           \
        ,)                                                              \
        if (--sizey>=0)                                                 \
        if (dskew + sizex <= 16) {                                      \
            IfMask(     register short mask;                            \
                        mask = 0x8000;                                  \
                        sizex -= 1;                                     \
                        mask >>= sizex;                                 \
                        ((unsigned short) mask) >>= dskew;              \
                        IfReverse(mask = ~mask;,),)                     \
            do {                                                        \
                IfMask(*(u_short *) dp IfReverse(&,|)= mask;,)          \
                    *(u_short *) dp op (*sp++ >> dskew);                \
                dp += vert;                                             \
            } while (--sizey != -1);                                    \
        }                                                               \
        else {                                                          \
            IfMask(     register long mask;                             \
                        mask = 0x80000000;                              \
                        sizex -= 1;                                     \
                        mask >>= sizex;                                 \
                        ((unsigned long) mask) >>= dskew;               \
                        IfReverse(mask = ~mask;,),)                     \
            dskew = 16 - dskew;                                         \
            do {                                                        \
                IfMask(*(u_int *) dp IfReverse(&,|)= mask;,)            \
                *(u_int *) dp op (*sp++ << dskew);                      \
                dp += vert;                                             \
            } while (--sizey != -1);                                    \
        }                                                               \
    }

#define MTRUE(a,b) a
#define MFALSE(a,b) b

#define ClippedOp(mask,op,revmask) \
    if(clip) MEMBATCH(MTRUE,mask,op,revmask) \
    else MEMBATCH(MFALSE,mask,op,revmask)

mem_batchrop(dst, op, src, count)
	struct pr_prpos dst;
	int             op;
	struct pr_prpos *src;
	short           count;
{
	register u_short *sp;
	register char  *dp;
	char           *dp0;
	register char  *handy;
	register short  sizex, sizey;
	register        vert, dskew;
	int             dskew0, xoff0;
	int             errors = 0;
	int             clip, limx, limy;
	int             oppassed = op;

	/* If destination is not 1 bit deep, force use of mem_rop */
	if (dst.pr->pr_depth != 1) {
		op = -1;	/* impossible op (I hope) */
		goto restart;	/* skip useless stuff */
	}

	/* If clipping, compute destination limits for later comparisons. */
	clip = 0;
	if (!(op & PIX_DONTCLIP)) {
		clip = 1;
		limx = dst.pr->pr_size.x;
		limy = dst.pr->pr_size.y;
	}
	op = (op >> 1) & 0xf;	/* Kill clipping, color, etc. */

	/*
	 * If destination is reverse video, invert function. 
	 * (We punt to mem_rop for reverse video sources.)
	 */
	if (mpr_d(dst.pr)->md_flags & MP_REVERSEVIDEO)
		op = pr_reversedst[op];

	/* Get destination pixrect data and image pointers. */
#define dprd ((struct mpr_data *)handy)
	dprd = mpr_d(dst.pr);
	vert = dprd->md_linebytes;
	xoff0 = dprd->md_offset.x;
	dp0 = (char *) ((int) dprd->md_image
		+ pr_product(vert, dst.pos.y + dprd->md_offset.y));
#undef dprd

restart:

#define spr ((struct pixrect *)handy)
#define sprd ((struct mpr_data *)handy)

	switch (op) {
	case (PIX_SRC ^ PIX_DST) >> 1:
		ClippedOp(MFALSE, ^=, MTRUE);
		break;
	case PIX_SRC >> 1:
		ClippedOp(MTRUE, |=, MTRUE);
		break;
	case PIX_NOT(PIX_SRC) >> 1:
		ClippedOp(MTRUE, ^=, MFALSE);
		break;
	case (PIX_SRC | PIX_DST) >> 1:
		ClippedOp(MFALSE, |=, MTRUE);
		break;
	case (PIX_NOT(PIX_SRC) & PIX_DST) >> 1:
		ClippedOp(MFALSE, &=~, MTRUE);
		break;
	default:
		for (; --count >= 0; src++) {
			dst.pos.x += src->pos.x;
			dst.pos.y += src->pos.y;
			errors |= mem_rop(dst.pr, dst.pos.x, dst.pos.y, 
				src->pr->pr_size.x, src->pr->pr_size.y,
				oppassed, src->pr, 0, 0);
		}
		break;
	}
	return errors;
	
hard:
	if (dst.pos.x + sizex <= 0)
		/*
		 * Completely clipped on left... 
		 */
		;
	else {
		errors |= mem_rop(dst.pr, dst.pos.x, dst.pos.y, 
			src->pr->pr_size.x, src->pr->pr_size.y,
			oppassed, src->pr, 0, 0);
	}
	src++;
	goto restart;
}

#else PROM

/*
 * Memory batchrop (old version left for prom code - should be merged someday)
 *
 * Note the use of several non-portable features:
 *	-- loopd6 which loops over the provided 2nd argument expression
 *	   until register d6 goes from 0 to -1.  Equivalent to:
 *		loopd6(a,b;) --> while((short)d6var-- != 0) b;
 *	   (loopd6 is no longer used.)
 *	-- the use of typecasts to the left of assignops -- for example,
 *		((unsigned short)mask) >>= dskew;
 *	   equivalent is: mask = ((unsigned short)mask) >> dskew;
 * Both of these are to generate better code.  They should be fixed by
 * fixing the C compiler to generate the better code in the portable case.
 */

prom_mem_batchrop(dst, op, src, count)
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

	/*
	 * Preliminaries: get pixrect data and image
	 * pointers; decide whether clipping.  If not clipping,
	 * normalize op, else compute limits for cursors for later
	 * comparisons.
	 */

	op = (op>>1)&0xf;		/* Kill dontclip, just keep useful */

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
#define sprd ((struct mpr_data *)handy)
		sprd = mpr_d(spr);
#undef spr
		sp = (u_short *)sprd->md_image;
#undef sprd

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
			*(short *)1 = 0;	/* Trap out */
		}
		continue;

	}
	return( errors);
}
#endif ~PROM

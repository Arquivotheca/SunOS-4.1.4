/* @(#)prom_batch.c 1.1 94/10/31 SMI */

/*
 * Copyright (c) 1986 by Sun Microsystems,  Inc.
 */

/*
 * PROM memory batchrop
 */

/*
 * Memory batchrop -- old PROM version
 *
 * Note the use of a non-portable feature:
 *	-- the use of typecasts to the left of assignops -- for example,
 *		((unsigned short)mask) >>= dskew;
 *	   equivalent is: mask = ((unsigned short)mask) >> dskew;
 *
 * This code also does 32 bit accesses on 16 bit boundaries.
 *
 * Ops used are PIX_SRC, PIX_NOT(PIX_SRC), PIX_CLR, PIX_SET, PIX_NOT(PIX_DST)
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_util.h>

prom_mem_batchrop(dst, op, src, count)
	struct pr_prpos dst;
	int op;
	struct pr_prpos *src;
	short count;
{
	register u_short *sp;
	register char *dp;
	register char *handy;
	register sizex, sizey;
	register vert, dskew;
	int errors = 0;

	/*
	 * Preliminaries: get pixrect data and image
	 * pointers; normalize op.
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
		case (PIX_NOT(PIX_DST))>>1:		/* not dst */
			if (dskew + sizex <= 16) {
				register short mask;
				mask = 0x8000;
				sizex -= 1;
				mask >>= sizex;
				((unsigned short)mask) >>= dskew;
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
				sizey--;
				do {
				    *(u_short *)dp &= mask; 
				    *(u_short *)dp |= *sp++ >> dskew; 
				    dp += vert;
				} while (--sizey != -1);
			} else {
				register long mask;
				mask = 0x80000000;
				sizex -= 1;
				mask >>= sizex;
				((unsigned long)mask) >>= dskew;
				mask = ~mask;
				dskew = 16 - dskew;
				sizey--;
				do {
				    *(u_int *)dp &= mask; 
				    *(u_int *)dp |= *sp++ << dskew; 
				    dp += vert;
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
				sizey--;
				do {
				    *(u_short *)dp |= mask; 
				    *(u_short *)dp ^= *sp++ >> dskew; 
				    dp += vert;
				} while (--sizey != -1);
			} else {
				register long mask;
				mask = 0x80000000;
				sizex -= 1;
				mask >>= sizex;
				((unsigned long)mask) >>= dskew;
				dskew = 16 - dskew;
				sizey--;
				do {
				    *(u_int *)dp |= mask; 
				    *(u_int *)dp ^= *sp++ << dskew; 
				    dp += vert;
				} while (--sizey != -1);
			}
			break;
		}
		continue;
	}
	return errors;
}

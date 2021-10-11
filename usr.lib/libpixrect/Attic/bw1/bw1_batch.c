#ifndef lint
static char sccsid[] = "@(#)bw1_batch.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1983, 1987 by Sun Microsystems, Inc.
 */

/*
 * bw1_batch.c: Sun1 Black-and-White batchrop
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/bw1reg.h>
#include <pixrect/bw1var.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>

extern char	pr_reversedst[];

int bw1_nobatchrop;

bw1_batchrop(dst, op, src, count)
	struct pr_prpos dst;
	int op;
	register struct pr_prpos *src;
	register int count;
{
	struct bw1pr *dbd;
	struct bw1fb *fb;
	short *homex, *homey;
	register int clip;
	short *limx, *limy, *dstx, *dsty;
	register struct pixrect *pr;
	register short sizey, sizex, sizey_homey;
	struct mpr_data *md;
	register short *ma;
	register int tem;
	register short *by;
	short *ma_homey, *bx;
	int oppassed = op;
	int errors = 0;

	if (bw1_nobatchrop) {
		while (--count >= 0) {
			dst.pos.x += src->pos.x;
			dst.pos.y += src->pos.y;
			pr = src->pr;
			if (pr)
				errors |= bw1_rop(dst.pr, dst.pos.x, dst.pos.y,
				    pr->pr_size.x, pr->pr_size.y,
				    oppassed, pr, 0, 0);
			src++;
		}
		return( errors);
	}

	/*
	 * Preliminaries: get pixrect data and frame buffer
	 * pointers; decide whether clipping.  If not clipping,
	 * normalize op, else compute limits for cursors for later
	 * comparisons.
	 */
	dbd = bw1_d(dst.pr);
	fb = dbd->bwpr_va;
	homex = bw1_xdst(fb, dbd->bwpr_offset.x);
	homey = bw1_ydst(fb, dbd->bwpr_offset.y);
	if (op & PIX_DONTCLIP) {
		op &= ~PIX_DONTCLIP;
		clip = 0;
	} else {
		clip = 1;
		limx = homex + dst.pr->pr_size.x;
		limy = homey + dst.pr->pr_size.y;
	}
	op = (op>>1)&0xf;		/* AND off the color parameter */

	/*
	 * Compute true operation if screen is reverse video (normal case).
	 * Set the operation into frame buffer hardware.
	 * Initialize x and y destination cursors pointers.
	 */
	if (dbd->bwpr_flags & BW_REVERSEVIDEO)
		op = pr_reversedst[op];
	bw1_setfunction(fb, op);
	bw1_setmask(fb, 0);
	dstx = homex + dst.pos.x;
	dsty = homey + dst.pos.y;

	for (; --count >= 0; src++) {
		/*
		 * Update destination x and y by pre-advance amount.
		 * If no pixrect for this, then skip to next.
		 */
		bw1_touch(*(dstx += src->pos.x));
		dsty += src->pos.y;
		pr = src->pr;
		if (pr == 0)
			continue;
		sizex = pr->pr_size.x;
		sizey = pr->pr_size.y;
		if (MP_NOTMPR(pr))
			goto hard;
		md = mpr_d(pr);
		if (md->md_offset.x || md->md_offset.y)
			goto hard;
		ma = md->md_image;

		/*
		 * Grab sizes and address of image.
		 * If clipping (rare case hopefully)
		 * compare cursors against limits.
		 */
		by = dsty;
		if (clip) {
			if (dstx + sizex > limx)
				sizex = limx - dstx;
			if (dsty + sizey > limy)
				sizey = limy - dsty;
			if (dstx < homex)
				goto hard;
			if (dsty < homey) {
				tem = homey - dsty;
				by += tem;
				ma += pr_product(tem, (md->md_linebytes>>1));
				sizey -= tem;
			}
			if (sizex <= 0 || sizey <= 0)
				continue;
		}

		/*
		 * Fast check for characters <= 16 wide and
		 * stored in aligned words in memory.
		 */
		if (md->md_linebytes == 2) {
			bw1_setwidth(fb, sizex);
			while (--sizey != -1)
				*by++ = *ma++;
			continue;
		}

		/*
		 * Hard case: characters greater than 16 wide.
		 * Do one vertical slice at a time.
		 */
		ma_homey = ma;
		sizey_homey = sizey;
		bx = dstx;
		bw1_setwidth(fb, 16);
		for (;;) {
			ma = ma_homey++; 
			sizey = sizey_homey;
			if (sizex < 16) 
				bw1_setwidth(fb, sizex);
			while (--sizey != -1) {
				*by++ = *ma; 
				ma = (short *) 
					((char *) ma + md->md_linebytes);
			}
			if ((sizex -= 16) <= 0)
				break;
			bw1_touch(*(bx += 16));
			by -= sizey_homey;
		}
		continue;

		/*
		 * Too hard... give up and call complete
		 * rop routine.  Used if source is skewed,
		 * or if clipping causes us to chop off
		 * in left-x direction.
		 */
hard:
		if (dstx + sizex <= homex)
			/*
			 * Completely clipped on left...
			 */
			;
		else {
			errors |= bw1_rop(dst.pr, dstx - homex, dsty - homey,
			    sizex, sizey, oppassed, pr, 0, 0);
			bw1_setfunction(fb, op);
			bw1_setmask(fb, 0);
		}
		continue;
	}
	return( errors);
}

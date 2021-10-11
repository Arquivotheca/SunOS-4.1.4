#ifndef lint
static char sccsid[] = "@(#)gp1_batch.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>
#include <pixrect/gp1var.h>
#include <pixrect/memvar.h>

extern struct pixrectops	mem_ops;

int	gp1_nobatchrop = 0;

extern short gpmrc_lmasktable[];
extern short gpmrc_rmasktable[];

gp1_batchrop(dst, op, src, count)
	struct pr_prpos dst;
	int op;
	register struct pr_prpos *src;
	register int count;
{
	register short sizey;
	register int tem, w, prime, linebytes;
	register short *bx, *leftx, *ma;

	short sizex;
	int clip;
	struct pixrect *pr;
	short *ma_homey;
	short homex, homey, limx, limy, dstx, dsty, by;

	struct gp1pr *dbd;
	struct cg2fb *fb;
	struct mpr_data *md;
	int oppassed = op;
	int errors = 0;
	short color;

	if (gp1_nobatchrop || (src->pr->pr_depth != 1)) {
		while (--count >= 0) {
			dst.pos.x += src->pos.x;
			dst.pos.y += src->pos.y;
			pr = src->pr;
			if (pr)
				errors |= gp1_rop(dst.pr, dst.pos.x, dst.pos.y,
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
	dbd = gp1_d(dst.pr);
	fb = dbd->cgpr_va;
	homex = dbd->cgpr_offset.x;
	homey = dbd->cgpr_offset.y;
	if (op & PIX_DONTCLIP) {
		op &= ~PIX_DONTCLIP;
		clip = 0;
	} else {
		clip = 1;
		limx = homex + dst.pr->pr_size.x;
		limy = homey + dst.pr->pr_size.y;
	}
	color = PIX_OPCOLOR(op);
	if (color == 0) color = -1;
	op = (op>>1) & 0xf;

	/*
	 * Set the operation into frame buffer hardware.
	 * Initialize x and y destination cursors pointers.
	 */
	dstx = homex + dst.pos.x;
	dsty = homey + dst.pos.y;

	/* first sync with the gp */
	if(gp1_sync(dbd->gp_shmem, dbd->ioctl_fd))
		return(-1);

	fb->ppmask.reg = color;			/* set colored text */
	fb->ropcontrol[CG2_ALLROP].ropregs.mrc_pattern = -1;
	fb->ppmask.reg = ~color;
	fb->ropcontrol[CG2_ALLROP].ropregs.mrc_pattern = 0;
	fb->ppmask.reg = dbd->cgpr_planes;
	cg2_setfunction( fb, CG2_ALLROP, op<<4);	/* op & MASK */
	fb->status.reg.ropmode = PWWWRD;
	linebytes = cg2_linebytes( fb, PWWWRD); 

	for (; --count >= 0; src++) {
		/*
		 * Update destination x and y by pre-advance amount.
		 * If no pixrect for this, then skip to next.
		 */
		dsty += src->pos.y;
		dstx += src->pos.x;
		pr = src->pr;
		if (pr == 0)
			continue;
		sizex = pr->pr_size.x;
		sizey = pr->pr_size.y;
		if (pr->pr_ops != &mem_ops)
			goto hard;
		md = mpr_d(pr);
		if (md->md_offset.x || md->md_offset.y)
			goto hard;
		ma = md->md_image;

		/*
		 * Grab sizes and address of image.  If clipping (rare case
		 * hopefully) compare cursors against limits.
		 */
		by = dsty;
		if (clip) {
			if (dstx + sizex > limx)
				sizex = limx - dstx;
			if (dsty + sizey > limy)
				sizey = limy - dsty;
			if (dstx < homex)
				goto hard;
			if (dsty < homey) {	/* works if pr_depth = 1! */
				tem = homey - dsty;
				by += tem;
				ma += pr_product(tem, (md->md_linebytes>>1));
				sizey -= tem;
			}
			if (sizex <= 0 || sizey <= 0)
				continue;
		}

		/*
		 * Hard case: characters greater than 16 wide.
		 */
		ma_homey = ma;

			/* set the ROP chip word width and opcount */

		w = cg2_prskew(dstx);		/* source skew is 0 */
		cg2_setshift(fb,CG2_ALLROP, w & 0xF, 1);
		prime = !w;
		w = (sizex + w - 1) >> 4;
		cg2_setwidth( fb, CG2_ALLROP, w, w);

			/* set the ROP chip end masks */

		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 =
			gpmrc_lmasktable[dstx & 0xf];
		fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 =
			gpmrc_rmasktable[(dstx+sizex-1) & 0xf];

		leftx = cg2_ropwordaddr( fb, 0, dstx, by);
		tem = md->md_linebytes;
		if (w) {
			w++;
			while( sizey--) {
				ma = ma_homey;
				bx = leftx;
				if (prime) cg2_setrsource(fb,CG2_ALLROP,*ma++);
				rop_fastloop(w, *bx++ = *ma++);
				(char *)ma_homey += tem;
				(char *)leftx += linebytes;
			}
		} else {
			bx = leftx;
			ma = ma_homey;
			if (prime) ma++;
			while( sizey--) {
				if (prime) {
					ma--;
					cg2_setrsource(fb,CG2_ALLROP,*ma++);
				}
				*bx = *ma;
				(char *)ma += tem;
				(char *)bx += linebytes;
			}
		}
		continue;

		/*
		 * Too hard... give up and call complete rop routine.  Used
		 * if source is skewed, or if clipping causes us to chop off
		 * in left-x direction.
		 */
hard:
		if (dstx + sizex <= homex)
			/*
			 * Completely clipped on left...
			 */
			;
		else {
			errors |= gp1_rop(dst.pr, dstx - homex, dsty - homey,
			    sizex, sizey, oppassed, pr, 0, 0);
		}
		continue;
	}
	return( errors);
}

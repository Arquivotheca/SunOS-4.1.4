#ifndef lint
static char sccsid [] = "@(#)pr_replrop.c	1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <gp1var.h>

#ifndef pr_product
#define pr_product(x, y)	(x * y)
#endif

#ifndef	REPL_SIZE
#define REPL_SIZE	64	/* width, height of intermediate pixrect */
#endif

/* width of 1-line high buffer */
#define	LINE_SIZE	(REPL_SIZE * REPL_SIZE)

pr_replrop(dpr, dx, dy, dw, dh, op, spr, sx, sy)
	Pixrect *dpr;
	int dx, dy, dw, dh, op;
	register Pixrect *spr;
	int sx, sy;
{
	register int sw, sh;
	register int x, y;
	int errors = 0;
	Pixrect *mpr;
	Pixrect tmp_pr;
	struct mpr_data tmp_prd;
	static struct mpr_data zprd = { 0 };
	short tmp_image[REPL_SIZE][REPL_SIZE/16];

	extern int gp1_rop(), cg6_rop();
	extern struct pixrectops mem_ops;


	if (spr) {
		if ((sw = spr->pr_size.x) <= 0 || 
			(sh = spr->pr_size.y) <= 0)
			return 0;
	}
	else {
		sw = dw; 
		sh = dh;
	}

	/*
	 * Make sx, sy canonical by reducing mod spr size
	 */
	for (x = sw; sx < 0; x += x)
		sx += x;

	if (sx >= sw)
		sx %= sw;

	for (y = sh; sy < 0; y += y)
		sy += y;

	if (sy >= sh)
		sy %= sh;

	/* clip if requested */
	if (!(op & PIX_DONTCLIP)) {
		struct pr_subregion dst;
		struct pr_prpos src;

		dst.pr = dpr;
		dst.pos.x = dx;
		dst.pos.y = dy;
		dst.size.x = dw;
		dst.size.y = dh;

		src.pr = &tmp_pr;
		tmp_pr.pr_size.x = dw + sx;
		tmp_pr.pr_size.y = dh + sy;
		src.pos.x = sx;
		src.pos.y = sy;

		(void) pr_clip(&dst, &src);

		dx = dst.pos.x;
		dy = dst.pos.y;
		dw = dst.size.x;
		dh = dst.size.y;

		/* make sure sx, sy are still canonical */
		if ((sx = src.pos.x) >= sw)
			sx %= sw;

		if ((sy = src.pos.y) >= sh)
			sy %= sh;

		op |= PIX_DONTCLIP;
	}

	if (dw <= 0 || dh <= 0)
		return 0;

	if (dpr->pr_ops->pro_rop == gp1_rop && 
	    gp1_d (dpr)->fbtype == FBTYPE_SUN2COLOR) {
	  if (!gp1_replrop(dpr, dx, dy, dw, dh, op, spr, sx, sy))
	    return 0;
	} else
	  if (dpr->pr_ops->pro_rop == cg6_rop && 
	      !cg6_replrop(dpr, dx, dy, dw, dh, op, spr, sx, sy))
	    return 0;
	  else
	    if (dpr->pr_ops->pro_rop == gp1_rop && 
		gp1_d (dpr)->fbtype == FBTYPE_SUNGP3) {
	      if (!cg12_replrop(dpr, dx, dy, dw, dh, op, spr, sx, sy))
		return 0; 
	    }  
		

	/* make sure sw, sh are no larger than needed */
	if (sw > (x = dw + sx))
		sw = x;

	if (sh > (y = dh + sy))
		sh = y;

	/* 
	 * Build temporary pixrect in case it is needed for either
	 * of two special cases below.  (We don't want to waste time
	 * on a mem_point().)
	 */
	tmp_pr.pr_ops = &mem_ops;
	tmp_pr.pr_depth = 1;
	tmp_pr.pr_data = (caddr_t) &tmp_prd;
	tmp_prd = zprd;
	tmp_prd.md_image = tmp_image[0];
		
	/*
	 * If destination is 1 line high and source is 16 bits
	 * wide, then special case to go fast: replicate
	 * the source in a 1 line high short buffer to be
	 * wide enough, and rop to the destination.
	 *
	 * (It's unlikely that anyone uses this.)
	 */
	if (sw == 16 && dh == 1 && dw <= LINE_SIZE &&
		spr && !(MP_NOTMPR(spr)) && spr->pr_depth == 1) {
		register u_short pat;

		{
			register struct mpr_data *mprd = mpr_d(spr);

			x = sx + mprd->md_offset.x;

			pat = * (short *) ((caddr_t) mprd->md_image +
				pr_product(mprd->md_linebytes, 
					mprd->md_offset.y + sy) +
				(x >> 4 << 1));
			
			pat = (pat | pat << 16) >> (16 - (x & 15));
		}

		{
			register short *image = tmp_image[0];

			x = (dw + 15) >> 4;
			while (--x >= 0)
				*image++ = pat;
		}

		tmp_pr.pr_size.x = LINE_SIZE;
		tmp_pr.pr_size.y = 1;
		tmp_prd.md_linebytes = LINE_SIZE / 8;
		return pr_rop(dpr, dx, dy, dw, dh, op, &tmp_pr, 0, 0);
	}

	/*
	 * If source is too small, blow it up.
	 */
	mpr = 0;

	if ((dw >= REPL_SIZE || dh >= REPL_SIZE) &&
		sw <= REPL_SIZE/2 && sh <= REPL_SIZE/2) {
		register int tw, th;
		register Pixrect *tpr;

		for (tw = sw, x = REPL_SIZE - sw; tw <= x; )
			tw += sw;

		if (tw > (x = dw + sx))
			tw = x;

		for (th = sh, y = REPL_SIZE - sh; th <= y; )
			th += sh;

		if (th > (y = dh + sy))
			th = y;

		if (spr->pr_depth == 1) {
			tpr = &tmp_pr;
			tmp_pr.pr_size.x = REPL_SIZE;
			tmp_pr.pr_size.y = REPL_SIZE;
			tmp_prd.md_linebytes = REPL_SIZE / 8;
		}
		else
			mpr = tpr = mem_create(tw, th, spr->pr_depth);

		if (tpr) {
			for (x = 0; tw - x > 0; x += sw) {
				if (sw > tw - x)
					sw = tw - x;

				errors |= pr_rop(tpr, x, 0, sw, sh,
					PIX_SRC | PIX_DONTCLIP, 
					spr, 0, 0);
			}

			sw = tw;
			spr = tpr;

			for (y = sh; th - y > 0; y += sh) {
				if (sh > th - y)
					sh = th - y;

				errors |= pr_rop(spr, 0, y, sw, sh,
					PIX_SRC | PIX_DONTCLIP, 
					spr, 0, 0);
			}

			sh = th;
		}
	}


	/* 
	 * Perform the replrop.	
	 */
	dw += (x = dx);
	dh += (y = dy);
	sh -= sy;

	/* top left corner */
	errors |= pr_rop(dpr, x, y, sw - sx, sh, op, spr, sx, sy);

	/* top center/right */
	for (x += sw - sx; dw - x >= sw; x += sw)
		errors |= pr_rop(dpr, x, y, sw, sh, op, spr, 0, sy);

	/* top right */
	if (dw - x > 0)
		errors |= pr_rop(dpr, x, dy, dw - x, sh, op, spr, 0, sy);

	/* center/bottom strips */
	y += sh;  sh += sy;
	for ( ; y < dh; y += sh) {
		/* reduce height for bottom strip */
		if (sh > dh - y)
			sh = dh - y;

		/* left */
		x = dx;
		errors |= pr_rop(dpr, x, y, sw - sx, sh, 
			op, spr, sx, 0);

		/* center */
		for (x += sw - sx; dw - x >= sw; x += sw)
			errors |= pr_rop(dpr, x, y, sw, sh, 
				op, spr, 0, 0);

		/* right */
		if (dw - x > 0)
			errors |= pr_rop(dpr, x, y, dw - x, sh, 
				op, spr, 0, 0);
	}

	/* destroy temporary pixrect */
	if (mpr)
		(void) pr_destroy(mpr);

	return errors;
}

/*	mem_batch_v1.c	1.1	94/10/31	*/

/*
 * Memory batchrop
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_util.h>

int	mem_nobatchrop = 0;

mem_batchrop(dst, op, src, count)
	struct pr_prpos dst;
	int op;
	struct pr_prpos *src;
	register int count;
{
	register u_short *sp;
	register char *dp, *dplim;
	register vert, dskew;
	struct pixrect *pr;
	struct mpr_data *dprd, *sprd;
	int clip, limx, limy, sizey, sizex;
	int oppassed = op;

	if (mem_nobatchrop || (op & 14) != 12) {
		while (--count >= 0) {
			dst.pos.x += src->pos.x;
			dst.pos.y += src->pos.y;
			pr = src->pr;
			if (pr)
				mem_rop(dst.pr, dst.pos, pr->pr_size,
				    oppassed, pr, 0, 0);
			src++;
		}
		return;
	}

	/*
	 * Preliminaries: get pixrect data and image
	 * pointers; decide whether clipping.  If not clipping,
	 * normalize op, else compute limits for cursors for later
	 * comparisons.
	 */

	if (op & PIX_DONTCLIP) {
		op &= ~PIX_DONTCLIP;
		clip = 0;
	} else {
		clip = 1;
		limx = dst.pr->pr_size.x;
		limy = dst.pr->pr_size.y;
	}
	op >>= 1;
	dprd  = mpr_d(dst.pr);
	vert = dprd->md_linebytes;
	for (; --count >= 0; src++) {
		dst.pos.x += src->pos.x;
		dst.pos.y += src->pos.y;
		pr = src->pr;
		if (pr == 0)
			continue;
		sizex = pr->pr_size.x;
		sizey = pr->pr_size.y;
		if (pr->pr_ops != &mem_ops)
			goto hard;
		sprd = mpr_d(pr);
		if (sprd->md_linebytes != 2 ||
			sprd->md_offset.x != 0 || sprd->md_offset.y != 0)
			goto hard;
		dp = (char *)mprd_addr(dprd, dst.pos.x, dst.pos.y);
		dskew = mprd_skew(dprd, dst.pos.x, dst.pos.y);
		sp = (u_short *)sprd->md_image;

		/*
		 * If clipping (rare case hopefully)
		 * compare cursors against limits.
		 */
		if (clip) {
			if (dst.pos.x + sizex > limx)
				sizex = limx - dst.pos.x;
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
		dplim = dp + pr_product(sizey, vert);
		if (op == 6) {				/* xor */
			if (dskew + sizex <= 16) {
				do {
					*(u_short *)dp ^= *sp++ >> dskew;
				} while ((dp += vert) < dplim);
			} else {
				register lsh = 16 - dskew;
				do {
					*(u_int *)dp ^= *sp++ << lsh;
				} while ((dp += vert) < dplim);
			}
		} else {				/* or */
			if (dskew + sizex <= 16) {
				do {
					*(u_short *)dp |= *sp++ >> dskew;
				} while ((dp += vert) < dplim);
			} else {
				register lsh = 16 - dskew;
				do {
					*(u_int *)dp |= *sp++ << lsh;
				} while ((dp += vert) < dplim);
			}
		}
		continue;

		/*
		 * Too hard... give up and call complete
		 * rop routine.  Used if source is skewed,
		 * or if operation is other than | or ^,
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
			mem_rop(dst.pr, dst.pos, pr->pr_size,
			    oppassed, pr, 0, 0);
		}
		continue;
	}
}

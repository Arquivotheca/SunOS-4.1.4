#ifndef lint
static	char sccsid[] = "@(#)pf_text.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

/*
 * Rasterop up a text string in a specified Pixfont.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>

struct	pr_size pf_textbatch();

pf_text(prpos, op, pf, str)
	struct pr_prpos prpos;
	int op;
	Pixfont *pf;
	char *str;
{
	register int rmdr = strlen((char *) str), len, errors = 0;
	int prplen;
	struct pr_size pr_size;
	struct pr_prpos prp[81];

	while (rmdr > 0) {
		len = rmdr;
		if (len > 80)
			len = 80;
		prplen = len;
		rmdr -= len;
		pr_size = pf_textbatch(prp, &prplen, pf, str);
		errors |= prs_batchrop(prpos, op, prp, prplen);
		str += len;
		prpos.pos.x += pr_size.x;
		prpos.pos.y += pr_size.y;
	}
	return errors;
}

struct pr_size
pf_textbatch(dst, lenp, pf, str)
	register struct pr_prpos *dst;
	int *lenp;
	Pixfont *pf;
	register char *str;
{
	register int basex = 0, basey = 0;
	register struct pixchar *pc;
	struct pr_size pr_size;
	register int len = *lenp;

	pr_size.x = 0;
	pr_size.y = 0;
	/*
	 * Place each character.  Basex and basey keep track
	 * of the base position change which resulted from the previous
	 * character.
	 */
	while (len > 0) {
		if (*str == 0) {
			*lenp -= len;
			break;
		}
		len--;
		pc = &pf->pf_char[(u_char) *str++];
		if (pc->pc_pr) {
			dst->pr = pc->pc_pr;
			/*
			 * Character begins at its home location.
			 */
			dst->pos.x = basex + pc->pc_home.x;
			dst->pos.y = basey + pc->pc_home.y;
			dst++;
			/*
			 * Next character must specify an offset to return
			 * to the baseline (- home terms) and account for
			 * the width of the character (advance terms).
			 */
			basex = pc->pc_adv.x - pc->pc_home.x;
			basey = pc->pc_adv.y - pc->pc_home.y;
		} else {
			/*
			 * Skip character, but offset next by its advance.
			 */
			(*lenp)--;
			basex += pc->pc_adv.x;
			basey += pc->pc_adv.y;
		}
		/*
		 * Accumulate advances for caller.
		 */
		pr_size.x += pc->pc_adv.x;
		pr_size.y += pc->pc_adv.y;
	}
	return (pr_size);
}

pf_textbound(bound, len, pf, str)
	struct pr_subregion *bound;
	register int len;
	register Pixfont *pf;
	register char *str;
	/*
	 * bound is set to bounding box for str, with origin at left-most
	 *   point on the baseline for the first character.
	 * bound->pos is top-left corner, bound->size.x is width,
	 *   bound->size.y is height.
	 * bound->pr is not modified.
	 * NOTE: pf_textbound must duplicate what pf_text & pf_textbatch do!
	 */
{
	register int basex = 0, basey = 0;
	register struct pixchar *pc;
	int dstposx, dstposy;

	bound->pos.x = bound->pos.y = 0;
	bound->size.x = bound->size.y = 0;
	/*
	 * Place each character.  Basex and basey keep track of the base
	 * position which resulted from the previous character.
	 */
	while (len > 0) {
		len--;
		pc = &pf->pf_char[(u_char) *str++];
		if (pc->pc_pr) {
			/*
			 * Character begins at its home location.
			 */
			dstposx = basex + pc->pc_home.x;
			dstposy = basey + pc->pc_home.y;
			if (dstposx < bound->pos.x)
				bound->pos.x = dstposx;
			if (dstposy < bound->pos.y)
				bound->pos.y = dstposy;
			/*
			 * Character ends where pixrect does
			 */
			dstposx += pc->pc_pr->pr_width;
			dstposy += pc->pc_pr->pr_height;
			if (dstposx > bound->pos.x + bound->size.x)
				bound->size.x = dstposx - bound->pos.x;
			if (dstposy > bound->pos.y + bound->size.y)
				bound->size.y = dstposy - bound->pos.y;
		}
		basex += pc->pc_adv.x;
		basey += pc->pc_adv.y;
	}
}

struct pr_size
pf_textwidth(len, pf, str)
	int len;
	register Pixfont *pf;
	register char *str;
{
	struct pr_size pr_size;
	register struct pixchar *pc;
	register int firstchar = 1;
	u_char ch;

	pr_size.x = 0;
	pr_size.y = 0;
	while (--len >= 0) {
		ch = (u_char) *str++;
		pc = &pf->pf_char[ch];
		if (pc->pc_pr == 0)
			pc = &pf->pf_char[' '];
		if (firstchar) {
			pr_size.y = (pc->pc_pr) ? pc->pc_pr->pr_height :
				pf->pf_char['A'].pc_pr->pr_height;
			firstchar = 0;
		}
		pr_size.x += pc->pc_adv.x;
		pr_size.y += pc->pc_adv.y;
	}
	return (pr_size);
}

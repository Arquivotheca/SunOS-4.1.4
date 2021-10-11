#ifndef lint
static	char sccsid[] = "@(#)pf_ttext.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

/*
 * Rasterop up a transparent background text string in a specified Pixfont.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>

pf_ttext(prpos, op, pf, str)
	struct pr_prpos prpos;
	register int op;
	register Pixfont *pf;
	register char *str;
{
	register int dx = prpos.pos.x, dy = prpos.pos.y;
	register struct pixchar *pc;
	register Pixrect *spr;
	register int errors = 0;

	while (*str != 0) {
		pc = &pf->pf_char[(u_char) *str++];
		if (spr = pc->pc_pr) 
			errors |= pr_stencil(prpos.pr,
				dx + pc->pc_home.x, dy + pc->pc_home.y,
				spr->pr_size.x, spr->pr_size.y,
				op, spr, 0, 0, (Pixrect *) 0, 0, 0);

		dx += pc->pc_adv.x;
		dy += pc->pc_adv.y;
	}

	return errors;
}

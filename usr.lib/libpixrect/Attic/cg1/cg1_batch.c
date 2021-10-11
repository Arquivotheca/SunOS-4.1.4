#ifndef lint
static char sccsid[] = "@(#)cg1_batch.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1985, 1987 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>

cg1_batchrop(dpr, dx, dy, op, src, count)
	register Pixrect *dpr;
	register int dx, dy, op;
	register struct pr_prpos *src;
	register int count;
{
	register Pixrect *spr;
	register int errors = 0;

	while (--count >= 0) {
		spr = src->pr;
		errors |= pr_rop(dpr, 
			dx += src->pos.x, dy += src->pos.y,
			spr->pr_size.x, spr->pr_size.y,
			op, spr, 0, 0);
		src++;
	}

	return errors;
}

#ifndef lint
static char sccsid[] = "@(#)pr_gen_batch.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1989 by Sun Microsystems, Inc.
 */

/*
 * Moved this routine from cg2_batch.c and cg6_batch.c to its
 * own file:  pr_gen_batch.c 
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>


pr_gen_batchrop(dpr, dx, dy, op, src, count)
        register Pixrect *dpr;
        register int dx, dy, op;
        register struct pr_prpos *src;
        register int count;
{
        register Pixrect *spr;
        register int errors = 0;

        while (--count >= 0) {
                dx += src->pos.x;
                dy += src->pos.y;

                if (spr = src->pr)
                        errors |= pr_rop(dpr, dx, dy,
                                spr->pr_size.x, spr->pr_size.y,
                                op, spr, 0, 0);

                src++;
        }

        return errors;
}

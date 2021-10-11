#ifndef lint
static	char sccsid[] = "@(#)mem_prs.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * mem_prs.c: Memory pixrect abstraction
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_util.h>

short * 
_mprs_addr(memprs)
	register struct pr_prpos *memprs;
{
	register struct mpr_data *mpd = mpr_d(memprs->pr);

	return( mprd_addr(mpd,memprs->pos.x,memprs->pos.y));
}

u_char * 
_mprs8_addr(memprs)
	register struct pr_prpos *memprs;
{
	register struct mpr_data *mpd = mpr_d(memprs->pr);

	return( mprd8_addr(mpd,memprs->pos.x,memprs->pos.y,
		memprs->pr->pr_depth));
}

_mprs_skew(memprs)
	register struct pr_prpos *memprs;
{
	register struct mpr_data *mpd = mpr_d(memprs->pr);

	return (mprd_skew(mpd, memprs->pos.x, memprs->pos.y));
}

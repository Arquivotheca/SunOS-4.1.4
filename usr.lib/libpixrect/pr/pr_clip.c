#ifndef lint
static	char sccsid[] = "@(#)pr_clip.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
#include <sys/types.h>
#include <pixrect/pixrect.h>

#define	pr_clip1(pra,size,prb) {					\
	if ((pra)->pos.x<0) {						\
		(prb)->pos.x  -= (pra)->pos.x;				\
		(size)->x += (pra)->pos.x;				\
		(pra)->pos.x   = 0;					\
	}								\
	if ((pra)->pos.y<0) {						\
		(prb)->pos.y  -= (pra)->pos.y;				\
		(size)->y += (pra)->pos.y;				\
		(pra)->pos.y   = 0;					\
	}								\
	if ((pra)->pos.x + (size)->x > (pra)->pr->pr_size.x)		\
		(size)->x = (pra)->pr->pr_size.x - (pra)->pos.x;	\
	if ((pra)->pos.y + (size)->y > (pra)->pr->pr_size.y)		\
		(size)->y = (pra)->pr->pr_size.y - (pra)->pos.y;	\
}

#define	pr_clip2(pra,size) {						\
	if ((pra)->pos.x<0) {						\
		(size)->x += (pra)->pos.x;				\
		(pra)->pos.x = 0;					\
	}								\
	if ((pra)->pos.y<0) {						\
		(size)->y += (pra)->pos.y;				\
		(pra)->pos.y = 0;					\
	}								\
	if ((pra)->pos.x + (size)->x > (pra)->pr->pr_size.x)		\
		(size)->x = (pra)->pr->pr_size.x - (pra)->pos.x;	\
	if ((pra)->pos.y + (size)->y > (pra)->pr->pr_size.y)		\
		(size)->y = (pra)->pr->pr_size.y - (pra)->pos.y;	\
}

pr_clip(dstp, srcp)
	register struct pr_subregion *dstp;
	register struct pr_prpos *srcp;
{

	if (srcp->pr) {
		pr_clip1(dstp, &dstp->size, srcp);
		pr_clip1(srcp, &dstp->size, dstp);
	} else
		pr_clip2(dstp, &dstp->size);
}

#ifndef lint
static char sccsid[] = "@(#)gp1_loadtex.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * gp1_loadtex() -- load 2-dimensional texture into the GP
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/memvar.h>

short *
gp1_loadtex(pr, shmptr)
	register Pixrect *pr;
	register short *shmptr;
{
	register short *image;
	register int w, offset;

	*shmptr++ = pr->pr_depth;
	*shmptr++ = w = pr->pr_size.x;
	*shmptr++ = pr->pr_size.y;

	w = mpr_linebytes(w, pr->pr_depth);
	offset = mpr_d(pr)->md_linebytes - w;
	w = (w >> 1) - 1;

	image = mpr_d(pr)->md_image;

	PR_LOOP(pr->pr_size.y,
		PR_LOOPP(w, *shmptr++ = *image++);
		PTR_INCR(short *, image, offset));

	return shmptr;
}

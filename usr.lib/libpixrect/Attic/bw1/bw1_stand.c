#ifndef lint
static	char sccsid[] = "@(#)bw1_stand.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Sun1 Black-and-White pixrect create and destroy for standalone applications.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/bw1reg.h>
#include <pixrect/bw1var.h>
#include <mon/sunromvec.h>

static	struct	pixrectops bw1_opsstand = {
	bw1_rop,
};

static	struct	bw1pr bw1prstruct =
    { 0, 0, BW_REVERSEVIDEO, 0, 0 };
static	struct	pixrect pixrectstruct =
    { &bw1_opsstand, { 1024, 800 }, 1, (caddr_t)&bw1prstruct };

struct	pixrect *
bw1_createstand(size, depth)
	struct pr_size size;
	int depth;
{

	bw1prstruct.bwpr_va = (struct bw1fb *)(*RomVecPtr->v_FBAddr);
	return (&pixrectstruct);
}

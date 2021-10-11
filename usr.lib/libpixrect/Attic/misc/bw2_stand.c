#ifndef lint
static	char sccsid[] = "@(#)bw2_stand.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Monochrome frame buffer pixrect create and destroy 
 * for standalone applications.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <pixrect/bw2var.h>
#include <mon/sunromvec.h>

static struct pixrectops bw2_ops = {
	mem_rop
};
static struct mpr_data mpr_data[1] = { 
	0, 0, { 0, 0 }, 0, MP_DISPLAY 
};
static struct pixrect bw2_mpr[1] = {
	&bw2_ops, 0, 0, 1, (caddr_t) mpr_data
};


/*
 * Since we don't know if the size passed by the caller is correct
 * or even present, we provide separate functions for the valid
 * frame buffer sizes.  If we could trust the size param, we would
 * only need one create function.
 */

#ifndef	TRUST_CALLER

static struct pixrect *
bw2_create_util(w, h)
register int w, h;
{
	bw2_mpr->pr_size.x = w;
	bw2_mpr->pr_size.y = h;
	mpr_data->md_linebytes = mpr_linebytes(w, 1);
	mpr_data->md_image = (short *) *RomVecPtr->v_FBAddr;

	return bw2_mpr;
}

struct pixrect *
bw2_createstand(size, depth)
	struct pr_size size;
	int depth;
{
	return bw2_create_util(BW2SIZEX, BW2SIZEY);
}

struct pixrect *
bw2square_createstand(size, depth)
	struct pr_size size;
	int depth;
{
	return bw2_create_util(BW2SQUARESIZEX, BW2SQUARESIZEY);
}

struct pixrect *
bw2h_createstand(size, depth)
	struct pr_size size;
	int depth;
{
	return bw2_create_util(BW2HSIZEX, BW2HSIZEY);
}

struct pixrect *
bw2hsquare_createstand(size, depth)
	struct pr_size size;
	int depth;
{
	return bw2_create_util(BW2HSQUARESIZEX, BW2HSQUARESIZEY);
}

#else	TRUST_CALLER

struct pixrect *
bw2_createstand(size, depth)
	struct pr_size size;
	int depth;
{
	bw2_mpr->pr_size = size;
	
	bw2_data->md_linebytes = mpr_linebytes(size.x, 1);
	bw2_data->pr_data)->md_image = (short *) *RomVecPtr->v_FBAddr;

	return bw2_mpr;
}

#endif	TRUST_CALLER

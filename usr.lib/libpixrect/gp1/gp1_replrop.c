#ifndef lint
static char sccsid []= "@(#)gp1_replrop.c 1.1 94/10/31 SMI";
#endif lint

/*
 * Copyright 1986, 1987 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <pixrect/gp1var.h>
#include <pixrect/gp1cmds.h>

gp1_replrop(dpr, dx, dy, dw, dh, op, spr, sx, sy)
	Pixrect *dpr;
	int dx, dy, dw, dh, op;
	register Pixrect *spr;
	int sx, sy;
{
	/* check for null spr, non-memory pixrect spr, bad depth */
	if (!spr || MP_NOTMPR(spr) ||
		spr->pr_depth != 1 && spr->pr_depth != 8)
		return PIX_ERR;

	/* check for non-primary or reverse video spr */
	{
		register struct mpr_data *mprd = mpr_d(spr);

		if (mprd->md_offset.x || mprd->md_offset.y ||
			mprd->md_flags & MP_REVERSEVIDEO)
			return PIX_ERR;
	}

	{
		register struct gp1pr *dmd = gp1_d(dpr);
		int words;
		short offset;
		unsigned int bitvec;
		register short *shmptr;
		extern short *gp1_loadtex();

		if (dmd->cmdver[GP1_PR_ROP_TEX >> 8] == 0 ||
			spr->pr_depth == 8 &&
				dmd->cmdver[GP1_PR_ROP_TEX >> 8] <= 1 ||
			(words = pr_product(mpr_prlinebytes(spr) >> 1, 
				spr->pr_size.y)) > GP1_MAXPRTEXSHORTS)
			return PIX_ERR;

		shmptr = (short *) dmd->gp_shmem;

		if ((offset = gp1_alloc((caddr_t) shmptr, 
			words + 19 + 511 >> 9, &bitvec, 
			dmd->minordev, dmd->ioctl_fd)) == 0) 
			return PIX_ERR;

		shmptr += offset;

		*shmptr++ = GP1_PR_ROP_TEX | dmd->cgpr_planes & 255;
		*shmptr++ = dmd->cg2_index;
		*shmptr++ = op;
		*shmptr++ = dmd->cgpr_offset.x;
		*shmptr++ = dmd->cgpr_offset.y;
		*shmptr++ = dpr->pr_size.x;
		*shmptr++ = dpr->pr_size.y;
		*shmptr++ = dx;
		*shmptr++ = dy;
		*shmptr++ = dw;
		*shmptr++ = dh;
		shmptr = gp1_loadtex(spr, shmptr);
		*shmptr++ = sx;
		*shmptr++ = sy;
		*shmptr++ = GP1_EOCL | GP1_FREEBLKS;
		*shmptr++ = bitvec >> 16;
		*shmptr   = bitvec;

		return gp1_post(dmd->gp_shmem, offset, dmd->ioctl_fd);
	}
}

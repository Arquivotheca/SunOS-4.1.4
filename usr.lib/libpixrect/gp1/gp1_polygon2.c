#ifndef lint
static char sccsid[] = "@(#)gp1_polygon2.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1985, 1986, 1987 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/memvar.h>
#include <pixrect/gp1var.h>
#include <pixrect/gp1cmds.h>

#define MAXVERT	400

gp1_polygon_2(dpr, dx, dy, nbnds, npts, pts, op, spr, sx, sy)
	Pixrect *dpr;
	int dx, dy;
	int nbnds;
	int *npts;
	struct pr_pos *pts;
	int op;
	Pixrect *spr;
	int sx, sy;
{
	register int words = 0;
	register struct gp1pr *dmd = gp1_d(dpr);

	if (spr) {
		register struct mpr_data *mprd = mpr_d(spr);

		/* 
		 * check for non-memory spr, bad depth, 
		 * region, reverse video, oversize
		 */
		if (MP_NOTMPR(spr) ||
			spr->pr_depth != 1 && spr->pr_depth != 8 ||
			dmd->cmdver[GP1_PR_PGON_TEX >> 8] == 0 ||
			spr->pr_depth == 8 && 
				dmd->cmdver[GP1_PR_PGON_TEX >> 8] <= 1 ||
			mprd->md_offset.x || mprd->md_offset.y ||
			mprd->md_flags & MP_REVERSEVIDEO ||
			(words = pr_product(mpr_prlinebytes(spr) >> 1, 
				spr->pr_size.y)) > GP1_MAXPRTEXSHORTS)
			return PIX_ERR;

		/* allow for depth, width, height, sx, sy */
		words += 5;
	}

	{
		int nvert = 0;
		short offset;
		unsigned int bitvec;
		register short *shmptr;
		extern short *gp1_loadtex();

		{
			register int i = nbnds;

			while (--i >= 0)
				if ((nvert += npts[i]) > MAXVERT)
					return PIX_ERR;
		}

		shmptr = (short *) dmd->gp_shmem;

		if ((offset = gp1_alloc((caddr_t) shmptr,
			words + nbnds + (nvert << 1) + 13 + 511 >> 9, 
			&bitvec, dmd->minordev, dmd->ioctl_fd)) == 0) 
			return PIX_ERR;

		shmptr += offset;

		if (spr) {
			*shmptr++ = GP1_PR_PGON_TEX | dmd->cgpr_planes & 255;
			*shmptr++ = dmd->cg2_index;
			shmptr = gp1_loadtex(spr, shmptr);
			*shmptr++ = sx - dmd->cgpr_offset.x;
			*shmptr++ = sy - dmd->cgpr_offset.y;
		} 
		else {
			*shmptr++ = GP1_PR_PGON_SOL | dmd->cgpr_planes & 255;
			*shmptr++ = dmd->cg2_index;
		}

		*shmptr++ = op;
		*shmptr++ = dmd->cgpr_offset.x;
		*shmptr++ = dmd->cgpr_offset.y;
		*shmptr++ = dpr->pr_size.x;
		*shmptr++ = dpr->pr_size.y;
		*shmptr++ = dx;
		*shmptr++ = dy;
		*shmptr++ = nbnds;

		PR_LOOP(nbnds, *shmptr++ = *npts++);

		PR_LOOP(nvert,
			*shmptr++ = pts->x;
			*shmptr++ = pts->y;
			pts++);

		*shmptr++ = GP1_EOCL | GP1_FREEBLKS;
		*shmptr++ = bitvec >> 16;
		*shmptr   = bitvec;

		return gp1_post(dmd->gp_shmem, offset, dmd->ioctl_fd);
	}
}

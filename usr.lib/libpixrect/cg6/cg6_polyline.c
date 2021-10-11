#ifndef lint
static char sccsid[] = "@(#)cg6_polyline.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1988-1989, Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_line.h>
#include <pixrect/cg6var.h>

cg6_polyline(pr, dx, dy, npts, ptlist, mvlist, brush, tex, op)
	Pixrect *pr;
	int dx, dy;
	register int npts;
	register struct pr_pos *ptlist;
	register u_char *mvlist;
	struct pr_brush *brush;
	Pr_texture *tex;
	int op;
{
	register struct fbc *fbc;
	register int r;
	int close;
	register int xfirst, yfirst;
	register int errs = 0;

	if ((brush && brush->width > 1) || tex || --npts <= 0)
		return PIX_ERR;

	{
		register struct cg6pr *prd = cg6_d(pr);

		fbc = prd->cg6_fbc;

		/* set raster op */
		cg6_setregs(fbc,
			prd->mprp.mpr.md_offset.x + dx,
			prd->mprp.mpr.md_offset.y + dy,
			PIXOP_OP(op),
			prd->mprp.planes,
			PIXOP_COLOR(op),
			L_FBC_RASTEROP_PATTERN_ONES,
			L_FBC_RASTEROP_POLYG_OVERLAP
		);

		/* set clip rectangle */
		if (op & PIX_DONTCLIP)
			cg6_clip(fbc, 0, 0, prd->cg6_size.x - 1,
				prd->cg6_size.y - 1);
		else
			cg6_clip(fbc, prd->mprp.mpr.md_offset.x,
				prd->mprp.mpr.md_offset.y,
				prd->mprp.mpr.md_offset.x + pr->pr_size.x - 1,
				prd->mprp.mpr.md_offset.y + pr->pr_size.y - 1);
	}

	/* get close flag */
	switch (mvlist) {
	case POLY_CLOSE:
		mvlist = 0;
		close = 1;
		break;
	case POLY_DONTCLOSE:
		mvlist = 0;
		close = 0;
		break;
	default:
		close = *mvlist++;
		break;
	}

#if FBC_REV0
	if (fbc_rev0(fbc))
		return cg6_polyline0(pr, dx, dy, npts, ptlist,
			mvlist, close, fbc, op);
#endif FBC_REV0

	while (--npts >= 0) {
		fbc->l_fbc_ilineabsy = yfirst = ptlist->y;
		fbc->l_fbc_ilineabsx = xfirst = ptlist->x;
		ptlist++;

		if (!mvlist)
			do {
				fbc->l_fbc_ilineabsy = ptlist->y;
				fbc->l_fbc_ilineabsx = ptlist->x;
				ptlist++;

				cg6_draw_done(fbc, r);
				if (r < 0)
					errs |= mem_vector(pr,
						ptlist[-2].x + dx,
						ptlist[-2].y + dy,
						ptlist[-1].x + dx,
 						ptlist[-1].y + dy,
						op, 0);
			} while (--npts >= 0);
		else if (*mvlist++)
			continue;
		else
			do {
				fbc->l_fbc_ilineabsy = ptlist->y;
				fbc->l_fbc_ilineabsx = ptlist->x;
				ptlist++;

				cg6_draw_done(fbc, r);
				if (r < 0)
					errs |= mem_vector(pr,
						ptlist[-2].x + dx,
						ptlist[-2].y + dy,
						ptlist[-1].x + dx,
 						ptlist[-1].y + dy,
						op, 0);
			} while (--npts >= 0 && *mvlist++ == 0);

		if (close) {
			fbc->l_fbc_ilineabsy = yfirst;
			fbc->l_fbc_ilineabsx = xfirst;

			cg6_draw_done(fbc, r);
			if (r < 0)
				errs |= mem_vector(pr,
					ptlist[-1].x + dx,
					ptlist[-1].y + dy,
					xfirst + dx,
 					yfirst + dy,
					op, 0);
		}
	}

	return errs;
}

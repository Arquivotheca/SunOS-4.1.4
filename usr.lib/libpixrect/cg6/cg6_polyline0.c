#ifndef lint
static char sccsid[] = "@(#)cg6_polyline0.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1988-1989, Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_line.h>
#include <pixrect/cg6var.h>

/* WARNING: non-standard calling sequence; see cg6_polyline() */

#if FBC_REV0

cg6_polyline0(pr, dx, dy, npts, ptlist, mvlist, close, fbc, op)
	Pixrect *pr;
	int dx, dy;
	register int npts;
	register struct pr_pos *ptlist;
	register u_char *mvlist;
	int close;
	register struct fbc *fbc;
	int op;
{
	register int r;
	register int ddx, x0, x1;
	register int xfirst, yfirst;
	register int errs = 0;

	ddx = dx + cg6_d(pr)->mprp.mpr.md_offset.x;

	while (--npts >= 0) {
		fbc->l_fbc_ilineabsy = yfirst = ptlist->y;
		fbc->l_fbc_ilineabsx = xfirst = x1 = ptlist->x;
		x1 += ddx;
		ptlist++;

		if (!mvlist)
			do {
				fbc->l_fbc_ilineabsy = ptlist->y;
				x0 = x1;
				fbc->l_fbc_ilineabsx = x1 = ptlist->x;
				x1 += ddx;
				ptlist++;

				cg6_draw_done0(fbc, x0, x1, r);
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
				x0 = x1;
				fbc->l_fbc_ilineabsx = x1 = ptlist->x;
				x1 += ddx;
				ptlist++;

				cg6_draw_done0(fbc, x0, x1, r);
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
			x0 = x1;
			fbc->l_fbc_ilineabsx = x1 = xfirst;
			x1 += ddx;

			cg6_draw_done0(fbc, x0, x1, r);
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

#endif FBC_REV0

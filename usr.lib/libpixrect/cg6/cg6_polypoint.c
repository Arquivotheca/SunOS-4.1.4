#ifndef lint
static char sccsid[] = "@(#)cg6_polypoint.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1988-1989, Sun Microsystems, Inc.
 */

/*
 * Draw a list of points on the screen
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/cg6var.h>

cg6_polypoint(pr, dx, dy, n, ptlist, op)
	Pixrect *pr;
	int dx, dy;
	int n;
	register struct pr_pos *ptlist;
	int op;
{
	register struct fbc *fbc;
	int errs = 0;

	/* do hardware initialization - rop, color,  & clip */
	{
		register struct cg6pr *prd = cg6_d(pr);
		register int xo, yo;

		fbc = prd->cg6_fbc;
		xo = prd->mprp.mpr.md_offset.x;
		yo = prd->mprp.mpr.md_offset.y;

		/* set raster op */
		cg6_setregs(fbc,
			xo + dx,
			yo + dy,
			PIXOP_OP(op),			/* rop */
			prd->mprp.planes,		/* planemask */
			PIXOP_COLOR(op),		/* fcolor (bcol = 0) */
			L_FBC_RASTEROP_PATTERN_ONES,	/* pattern */
			L_FBC_RASTEROP_POLYG_OVERLAP	/* polygon overlap */
		);

		/* set clip rectangle */
		if (op & PIX_DONTCLIP)
			cg6_clip(fbc, 0, 0,
				prd->cg6_size.x - 1,
 				prd->cg6_size.y - 1);
		else
			cg6_clip(fbc, xo, yo,
				xo + pr->pr_size.x - 1,
 				yo + pr->pr_size.y - 1);
	}

	PR_LOOP(n, 
		register u_int r;

		fbc->l_fbc_ipointabsx = ptlist->x;
		fbc->l_fbc_ipointabsy = ptlist->y;
		cg6_draw_done(fbc, r);
		if fbc_exception(r)
			errs |= mem_polypoint(pr, dx, dy, 1, ptlist, op);
		ptlist++);

	return errs;
}

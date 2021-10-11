#ifndef lint
static char sccsid[] = "@(#)cg6_vec.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1988-1989, Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/cg6var.h>

cg6_vector(pr, x0, y0, x1, y1, op, color)
	register Pixrect *pr;
	int x0, y0, x1, y1;
	int op;
	int color;
{
	register struct cg6pr *dprd = cg6_d(pr);
	register struct fbc *fbc = dprd->cg6_fbc;
	register int x, y, r;

	x = dprd->mprp.mpr.md_offset.x;
	y = dprd->mprp.mpr.md_offset.y;

	/* set raster op */
	cg6_setregs(fbc, 
		x, y,					/* offset */
		PIXOP_OP(op),				/* rop */
		dprd->mprp.planes,			/* planemask */
		(r = PIX_OPCOLOR(op)) ? r : color,	/* fcolor (bcol = 0) */
		L_FBC_RASTEROP_PATTERN_ONES,		/* pattern */
		L_FBC_RASTEROP_POLYG_OVERLAP		/* polygon overlap */
	);

	/* set clip rectangle */
	if (op & PIX_DONTCLIP)
		cg6_clip(fbc, 0, 0,
			dprd->cg6_size.x - 1, dprd->cg6_size.y - 1);
	else
		cg6_clip(fbc, x, y,
			x + pr->pr_size.x - 1, y + pr->pr_size.y - 1);

	/* write y first, CG6 autoincrements after x reg */
	fbc->l_fbc_ilineabsy = y0;
	fbc->l_fbc_ilineabsx = x0;
	fbc->l_fbc_ilineabsy = y1;
	fbc->l_fbc_ilineabsx = x1;

	cg6_draw_done(fbc, r);
	if (fbc_exception(r))
		return mem_vector(pr, x0, y0, x1, y1, op, color);

	return 0;
}

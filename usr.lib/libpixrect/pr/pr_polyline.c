#ifndef lint
static char sccsid [] = "@(#)pr_polyline.c	1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986 Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_line.h>
#include <pixrect/cg2var.h>
#include <pixrect/cg6var.h>
#include <pixrect/gp1var.h>
#include <pixrect/gp1cmds.h>

pr_polyline(pr, dx, dy, npts, ptlist, mvlist, brush, tex, op)
	register Pixrect *pr;
	int dx, dy;
	register int npts;
	register struct pr_pos *ptlist;
	register u_char *mvlist;
	struct pr_brush *brush;
	Pr_texture *tex;
	int op;
{
	register x0, y0, x1, y1;
	register int errs = 0;
	struct fbgattr fb;
	int xfirst, yfirst;
	int close, easy;

	/* need at least 2 points */
	if (--npts <= 0)
		return PIX_ERR;

	/* use LEGO if possible */
	if (pr->pr_ops->pro_rop == cg6_rop &&
		!cg6_polyline(pr, dx, dy, npts + 1, ptlist,
			mvlist, brush, tex, op))
		return 0;
	/* use GP if possible */
	else if (pr->pr_ops->pro_rop == gp1_rop && 
	    gp1_d (pr)->fbtype == FBTYPE_SUN2COLOR &&
		gp1_d(pr)->cmdver[GP1_PR_POLYLINE>>8] > 0 &&
		!gp1_polyline(pr, dx, dy, npts + 1, ptlist, 
			mvlist, brush, tex, op))
		return 0;

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

	/* figure out if pro_line is needed or pr_vector will do */
	easy = !(brush || tex);

	while (--npts >= 0) {
		x0 = ptlist->x + dx;
		y0 = ptlist->y + dy;
		ptlist++;

		if (mvlist && *mvlist++)
			continue;

		x1 = ptlist->x + dx;
		y1 = ptlist->y + dy;
		ptlist++;

		errs |= easy ?
			pr_vector(pr, x0, y0, x1, y1, op, 0) :
			pro_line(pr, x0, y0, x1, y1, brush, tex, op, 0);

		xfirst = x0;
		yfirst = y0;

		if (easy && !mvlist)
			while (--npts >= 0) {
				x0 = x1;
				y0 = y1;

				x1 = ptlist->x + dx;
				y1 = ptlist->y + dy;
				ptlist++;

				errs |= pr_vector(pr, x0, y0, x1, y1, op, 0);
			}
		else
			while (--npts >= 0 && !(mvlist && *mvlist++)) {
				x0 = x1;
				y0 = y1;

				x1 = ptlist->x + dx;
				y1 = ptlist->y + dy;
				ptlist++;

				errs |= easy ?
					pr_vector(pr, x0, y0, x1, y1, op, 0) :
					pro_line(pr, x0, y0, x1, y1, 
						brush, tex, op, 1);
			}

		if (close)
			errs |= easy ?
				pr_vector(pr, x1, y1, xfirst, yfirst, op, 0) :
				pro_line(pr, x1, y1, xfirst, yfirst,
					brush, tex, op, 1);
	}

	return errs;
}

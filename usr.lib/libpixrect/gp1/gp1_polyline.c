#ifndef lint
static char sccsid[] = "@(#)gp1_polyline.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1987 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/pr_line.h>
#include <pixrect/gp1var.h>
#include <pixrect/gp1cmds.h>

/*
 * TODO -- recover from alloc failures, split up large polylines
 */

gp1_polyline(pr, dx, dy, npts, ptlist, mvlist, brush, tex, op)
	Pixrect *pr;
	int dx, dy, npts;
	struct pr_pos *ptlist;
	u_char *mvlist;
	struct pr_brush *brush;
	Pr_texture *tex;
	int op;
{
	register short *shp;
	register struct gp1pr *prd;
	union {
		struct pr_texture_options s;
		short u;
	} options;
	int blocks, offset;
	u_int bitvec;

	/* punt if brush is too wide */
	if (brush && brush->width > GP1_MAX_BRUSH_WIDTH)
		return PIX_ERR;

	blocks = 0;
	options.u = 0;

	if (tex) {
		register short *patp = tex->pattern;

		/* punt if too many pattern segments */
		do {
			if (++blocks > GP1_MAX_PAT_SEGS + 1)
				return PIX_ERR;
		} while (*patp++);

		options.s = tex->options;
	}

	if (mvlist == POLY_CLOSE) {
		options.s.res_close = 1;
		mvlist = 0;
	}
	else if (mvlist) {
		options.s.res_mvlist = 1;
		options.s.res_close = (u_char *) *mvlist == POLY_CLOSE;
		blocks += npts;
	}

	if ((blocks = (blocks + 17 + (npts << 1) + 511) >> 9) > 8) 
		return PIX_ERR;
	prd = gp1_d(pr);

	shp = (short *) prd->gp_shmem;

	if ((offset = gp1_alloc((caddr_t) shp, blocks, &bitvec,
		prd->minordev, prd->ioctl_fd)) == 0)
		return PIX_ERR;
	
	shp += offset;
	
	*shp++ = GP1_PR_POLYLINE | prd->cgpr_planes & 255;
	*shp++ = prd->cg2_index;
	*shp++ = op;
	*shp++ = PIX_OPCOLOR(op);
	*shp++ = prd->cgpr_offset.x;
	*shp++ = prd->cgpr_offset.y;
	*shp++ = pr->pr_size.x;
	*shp++ = pr->pr_size.y;
	*shp++ = dx;
	*shp++ = dy;
	*shp++ = brush ? brush->width : 1;
	
	if (tex) {
		register short *patp = tex->pattern;

		while (*shp++ = *patp++)
			/* nothing */ ;
	
		*shp++ = tex->offset;
	} 
	else 
		*shp++ = 0;

	*shp++ = options.u;
	*shp++ = npts;

	{
		register struct pr_pos *p = ptlist;
		register u_char *m = mvlist;

		if (m) 
			PR_LOOP(npts,
				*shp++ = p->x;
				*shp++ = p->y;
				p++;
				*shp++ = *m++);
		else
			PR_LOOP(npts,
				*shp++ = p->x;
				*shp++ = p->y;
				p++);
	}

	*shp++ = GP1_EOCL | GP1_FREEBLKS;
	*shp++ = bitvec >> 16;
	*shp   = bitvec;

	return gp1_post(prd->gp_shmem, offset, prd->ioctl_fd);
}

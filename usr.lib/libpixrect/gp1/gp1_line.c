#ifndef lint
static char sccsid[] = "@(#)gp1_line.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1987 by Sun Microsystems, Inc.
 */

 
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/gp1var.h>
#include <pixrect/gp1cmds.h>
#include <pixrect/pr_line.h>

gp1_line(pr, x0, y0, x1, y1, brush, tex, op)
	Pixrect *pr;
	int x0, y0, x1, y1;
	struct pr_brush *brush;
	register Pr_texture *tex;
	int op;
{
	register short *shp, *patp;
	register struct gp1pr *prd;
	register int offset;
	u_int bitvec;

	/* punt if brush is too wide */
	if (brush && brush->width > GP1_MAX_BRUSH_WIDTH)
		return PIX_ERR;

	/* punt if too many pattern segments */
	if (tex) 
		for (offset = 0, patp = tex->pattern; *patp; patp++)
			if (++offset > GP1_MAX_PAT_SEGS)
				return PIX_ERR;

	prd = gp1_d(pr);
	shp = (short *) prd->gp_shmem;

	if ((offset = gp1_alloc((caddr_t) shp, 1, &bitvec,
		prd->minordev, prd->ioctl_fd)) == 0)
		return PIX_ERR;

	shp += offset;
    
	*shp++ = GP1_PR_LINE | prd->cgpr_planes & 255;
	*shp++ = prd->cg2_index;
	*shp++ = op;
	*shp++ = PIX_OPCOLOR(op);
	*shp++ = prd->cgpr_offset.x;
	*shp++ = prd->cgpr_offset.y;
	*shp++ = pr->pr_size.x;
	*shp++ = pr->pr_size.y;
	*shp++ = x0;
	*shp++ = y0;
	*shp++ = x1;
	*shp++ = y1;
	*shp++ = brush ? brush->width : 1;

	/* textured vector -- write pattern, starting offset, options */
	if (tex) {
		patp = tex->pattern;

		while (*shp++ = *patp++)
			/* nothing */ ;

		*shp++ = tex->offset;
		*shp++ = * (short *) &tex->options;
	} 
	else
		*shp++ = 0;

	*shp++ = GP1_EOCL | GP1_FREEBLKS;
	*shp++ = bitvec >> 16;
	*shp   = bitvec;

	return gp1_post(prd->gp_shmem, offset, prd->ioctl_fd);
}

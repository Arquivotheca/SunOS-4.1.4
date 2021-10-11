#ifndef lint
static char sccsid[] = "@(#)gp1_vec.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1985, 1987 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/cg9var.h>
#include <pixrect/gp1var.h>
#include <pixrect/gp1cmds.h>


gp1_vector(pr, x0, y0, x1, y1, op, color)
	Pixrect *pr;
	int x0, y0, x1, y1;
	int op, color;
{
	register short *shp;
	register struct gp1pr *prd;
	register int offset;
	u_int bitvec;

	prd = gp1_d(pr);
	if (prd->fbtype == FBTYPE_SUNROP_COLOR) {
	    Pixrect cgpixrect;
	    cgpixrect = *pr;
	    cgpixrect.pr_data = (caddr_t) &(prd->cgpr.cg9pr);
	    return cg9_vector(&cgpixrect, x0, y0, x1, y1, op, color);
	}
	/* GP2/CG2[2/3/5] */
	shp = (short *) prd->gp_shmem;

	if ((offset = gp1_alloc((caddr_t) shp, 1, &bitvec, 
		prd->minordev, prd->ioctl_fd)) == 0)
		return PIX_ERR;

	shp += offset;

	*shp++ = GP1_PRVEC | prd->cgpr_planes & 255;
	*shp++ = prd->cg2_index;
	*shp++ = op;
	*shp++ = color;
	*shp++ = prd->cgpr_offset.x;
	*shp++ = prd->cgpr_offset.y;
	*shp++ = pr->pr_size.x;
	*shp++ = pr->pr_size.y;
	*shp++ = x0;
	*shp++ = y0;
	*shp++ = x1;
	*shp++ = y1;
	*shp++ = GP1_EOCL | GP1_FREEBLKS;
	*shp++ = bitvec >> 16;
	*shp   = bitvec;

	/* post the command*/
	return gp1_post(prd->gp_shmem, offset, prd->ioctl_fd);
}

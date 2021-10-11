#ifndef lint
static char sccsid [] = "@(#)pr_polygon2.c	1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

/*
 *  pr_polygon_2 (dpr, dx, dy, nbnds, npts, vlist, op, spr, sx, sy)
 *  Scan convert a 2-D polygon.  Polygon may have multiple boundaries (holes)
 *  and may be filled with a texture.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/gp1var.h>

/* Plan:  change vector code to draw downwards and eliminate this option. */
#define VECSDRAWTORIGHT


#define swap(a,b,tmp)	((tmp) = (a), (a) = (b), (b) = (tmp))

struct edge {		/* polygon edge data structure */
	short ymn, ymx;		/* lower, upper y of edge */
	short xmn, dx;		/* x start, delta x for edge */
	short errx, erry;	/* error x and y for edge. */
	short error;		/* breshenham err accumulator. */
	struct edge *nxt;
};

static Pixrect *mpr;	/* memory pixrect for band */

pr_polygon_2 (dpr, dx, dy, nbnds, npts, vlist, op, spr, sx, sy)
	struct pixrect *dpr;		/* destination pixrect */
	int dx, dy;			/* dest pixrect offset position */
	int nbnds;			/* number of closed boundaries */
	int npts [];			/* number of points in each boundary */
	struct pr_pos *vlist;		/* list of points in all boundaries */
	int op;				/* pixrect op to use in painting */
	struct pixrect *spr;		/* source pixrect */
	int sx, sy;			/* source pixrect offset position */
{
/*
 * Fill a polygon which may contain holes.  The polygon is defined
 * by nbnds boundaries.  Each boundary is a list of 2-D points,
 * where npts[i] is the number of points in the ith boundary.  This
 * routine joins the last point and the first point to close each
 * boundary.  See Foley and Van Dam "Fundamentals Of Interactive
 * Computer Graphics" pg 459 for algorithm description.  Pixels on
 * the boundary are painted if at the left edge but not painted if
 * at the right edge.  Thus polygons fit together with no overlap
 * of pixels and no gaps between their edges.
 *
 * Note: Very thin polygons and zero width bridges will disappear,
 * since we are not doing any antialiasing.
 *
 * Polygons are scanned downwards.  Edge stepping code is designed to
 * match code used in vector drawing.  Unlike vector code, it is not
 * necessary for special bresenham error balancing if edge is clipped
 * since clipping occurs in pr_rop which paints scanlines.
 *
 * Octants are the four octants below the horizontal and are numbered
 * clockwise.
 *
 * We no longer clip to the bounds of a source pixrect.
 * Patterns (non-null source pixrects) are replicated in the X direction 
 * until they are wide enough that a rop can apply entire scanlines
 * to the destination pixrect.
 */

	struct edge aet[1];		/* active edge table */
	char *malloc();
	int gp1_rop();
	int cg6_rop();
	struct pixrect *mem_create();

struct edge	*et;		/* edge table. */
int	soffset, spr_height;	/* needed only for textured polygons (spr!=0) */
/* The scanline loop moves sy & uses the above, but that doesn't matter. */

	/* check for illegal size spr */
	if (spr && (spr->pr_size.x <= 0 || spr->pr_size.y <= 0))
		return PIX_ERR;

/*
 * Use the LEGO if applicable.  If LEGO routine fails, soldier on in software.
 * If LEGO succeeds, return 0.
 */
    if (dpr->pr_ops->pro_rop == cg6_rop) {
	if (!cg6_polygon_2 (dpr, dx, dy, nbnds, npts, vlist, op, spr, sx, sy))
	    return 0;
    }else
/*
 * Use the GP if applicable.  If GP routine fails, soldier on in software.
 * If GP succeeds, return 0.
 */
    if (dpr->pr_ops->pro_rop == gp1_rop && 
	    gp1_d (dpr)->fbtype == FBTYPE_SUN2COLOR &&
	    !gp1_polygon_2 (dpr, dx, dy, nbnds, npts, vlist, op, spr, sx, sy))
	    return 0;

   {register int		i, x1, x2;
    register struct edge	*pet;
    int				n;
    struct pr_pos		*vp;		/* vertex pointer. */

    /*
     *	Set up the edge table.
     *	Also glean overall minimums and maximums for textured polygons.
     */
	x1 = 1; /* Use register for total number of edges (+ first is dummy) */
	for (i = 0; i < nbnds; i++)		/* alloc edge table */
		x1 += npts [i];
	et = (struct edge *) malloc ((unsigned) x1 * sizeof (*et));
	if (!et) {
		return PIX_ERR;
	}

	pet = et;
	pet->nxt = &et [1];		/* dummy 1st edge points to nxt */
	x1 = x2 = dx + vlist->x;
	for (n = 0; n < nbnds; n++) {	/* for each boundary of polygon */
                int startx =  dx + vlist->x;

                if (startx < x1)
			x1 = startx;
                else if (startx > x2)
			x2 = startx;
		vp = vlist;		/* save vertex list start */
		if (npts [n] < 3) {
			free ((char *) et);
			return PIX_ERR;
		}
		for (i = 1; i < npts [n]; i++) { /* put boundary in edge tbl. */
			int endx =  dx + (vlist+1)->x;
			if(endx < x1) x1 = endx;
			else if(endx > x2) x2 = endx;
			if (vlist->y != (vlist+1)->y) {	/* skip horiz edges */
				pet++;		/* put edge in edge table */
				pet->xmn = dx + vlist->x;	/* start x */
				pet->dx =  endx;		/* end x */
				pet->ymn = dy + vlist->y;	/* start y */
				pet->ymx = dy + (vlist+1)->y;	/* end y */
				pet->nxt = pet + 1;
			}
			vlist++;
		}
		/* Add edge from last boundary point to first boundary point */
		if (vlist->y != vp->y) {  	/* Skip horiz edges */
			pet++;			/* Wrap around to first point */
			pet->xmn = dx + vlist->x;	/* start x */
			pet->ymn = dy + vlist->y;	/* start y */
			pet->dx = vp->x + dx;		/* end x */
			pet->ymx = vp->y + dy;		/* end y */
			pet->nxt = pet+1;
		}
		vlist++;
	}
	pet->nxt = 0;
	/* x1 & x2 are min and max X coords of vertices on dpr */

    /*
     *  For textured polygons, need a "band" as high and deep as spr
     *  and as wide as the portion of the polygon within dpr.
     */
	if (spr) {
	    int width_needed, minx, maxx;
	    /*
	     * Determine needed size for spr so each scanline can merely rop
	     */

	    /* Using pixrect bounds even if PIX_DONTCLIP.  Slower without it. */
		/* (WRONG!!!  Should be fixed to obey DONTCLIP.) */
	    minx = (x1 < 0)? 0 : x1;
	    maxx = (x2 >= dpr->pr_size.x)? dpr-> pr_size.x-1 : x2;
	    /* x1 & x2 have values which are no longer needed */
	    if( (minx >= dpr->pr_size.x) || (maxx < 0) ) {
		free ((char *) et); /* No portion of this figure is within dpr */
		return 0; /* Need not observe PIX_DONTCLIP, faster not to */
	    }
	    width_needed = maxx - minx;		/* width  =  last - first + 1 */
	    /* might want to subract 1 from maxx, as well */
	    spr_height = spr->pr_size.y;

	    /* spr big enough minx & maxx are within aligned source pixrect? */
	    if(spr->pr_size.x < width_needed)
		goto spr_inadequate;
	    /* Move sx within source pixrect, if it isn't already */
	    x1 = sx; /* Two loops do a mod operation, fast if not needed */
		/* (But really slow if it is needed!) */
	    x2 = spr->pr_size.x;
	    while (x1 >= x2)
		x1 -= x2;
	    /* And this loop is correct for negative sx; % may not be */
	    while (x1 < 0)
		x1 += x2;
	    sx = x1; /* save sx moved within source pixrect */

	    /* spr big enough minx & maxx are within aligned source pixrect? */
	    if( ((dx - x1) <= minx) && (maxx <= dx - x1 + x2) ) {
		    /* spr should be used as the band, mpr not needed */
		    soffset = dx - sx; /* dpr X coord - soffset = spr X coord */
	    } else {
spr_inadequate:
		/*
		 *  spr too small or not correctly aligned.  Build mpr band.
		 */
		if (mpr && mpr->pr_depth == spr->pr_depth
			&& mpr->pr_size.x >= width_needed
			&& mpr->pr_size.y >= spr_height)
		    { }	/* mpr is already adequate */
		else {
		    if (mpr) 
		    	(void) pr_destroy(mpr);
		    if( (mpr = mem_create(width_needed, spr_height,
			spr->pr_depth)) == (struct pixrect *) 0) {
			free ((char *) et);
			return PIX_ERR;
		    }
		}

		/*
		 * Replicate spr into mpr in X,
		 * aligning mpr's 0 with dpr's minx using dx and sx.
		 */
		x1 = sx - dx + minx;
		x2 = spr->pr_size.x;
                while (x1 >= x2)
		    x1 -= x2;
                while (x1 < 0)
		    x1 += x2;
		/* x1 is the offset within spr that maps onto mpr x==0 */
		(void) pr_rop(mpr, 0, 0, x2 - x1, spr_height,
			PIX_SRC, spr, x1, 0);
		if (x1) /* write the left portion of spr into mpr, if any */
		    (void) pr_rop(mpr, x2 - x1, 0, x1, spr_height, PIX_SRC, spr, 0, 0);
		/* Keep aligned with both spr width and word boundry */
		while (x2 < width_needed) { /* final rop is clipped by pr_rop */
		    (void) pr_rop(mpr, x2, 0, x2, spr_height, PIX_SRC, mpr, 0, 0);
		    /* move dest. by width & write 2 times width next time */
		    x2 <<= 1; /* Next time, write twice the width */
		}

		soffset = minx; /* dpr X coord - soffset = spr X coord */
		spr = mpr;
	    } /* end of preparing spr band in mpr */
	} /* end of preparing spr band for textured polygons */
    } /* end of set up the edge table. */

    {register struct edge	*p1, *p2, *pet, *paet;
     int			cury;
     int			temp;
    /*
     *  Initialize the edge values: max y, min y, min x and delta x.
     */
	p1 = et[0].nxt;
	while (p1) {		/* For all edges y min must be lowest. */
		if (p1->ymn > p1->ymx) {
			swap (p1->xmn, p1->dx, temp);
			swap (p1->ymn, p1->ymx, temp);
		}
		p1->erry = p1->ymx - p1->ymn;
		p1->errx = p1->dx - p1->xmn;	/* dx initially stores xmax */
		if (0 <= p1->errx) {			/* octants 0 and 1. */
			p1->dx = p1->errx / p1->erry;
			if (p1->erry < p1->errx) {		/* octant 0. */
				p1->error = -(p1->errx >> 1)
				    + (p1->dx / 2) * p1->erry;
				if (p1->error <= 0) {
					p1->error += p1->erry;
					p1->xmn++;
				}
				p1->error -= p1->errx;
				p1->xmn += p1->dx / 2;
			} else {				/* octant 1. */
				p1->error = -(p1->erry >> 1);
			}
		} else {				/* octants 2 and 3. */
			p1->dx = -((-p1->errx) / p1->erry);
			if (p1->erry < -p1->errx) {		/* octant 3. */
				p1->error = -(-p1->errx >> 1)
				    + ((-p1->dx) / 2) * p1->erry;
				/* -(- necessary since >> is not symmetric. */
				if (p1->error <= 0) {
					p1->error += p1->erry;
					p1->xmn--;
				}
				p1->error += p1->errx;
				p1->xmn += p1->dx / 2 + 1;
#ifdef VECSDRAWTORIGHT
    p1->error= p1->errx + p1->dx * p1->erry - p1->error;
    if (p1->erry & 1) p1->error++;
#endif
			} else {				/* octant 2. */
				p1->error = -(p1->erry >> 1);
#ifdef VECSDRAWTORIGHT
    if (p1->error <= -p1->erry) {
	p1->xmn--;
	p1->error += p1->erry;
    }
#endif
			}
		}
		p1 = p1->nxt;
	}

    /*
     *  Sort edge table on y, and on x within a y bucket.
     *  Bubble sort for now.
     */
	p1 = et[0].nxt;		/* p1 points to the edge list */
	if (!p1) {		/* If edge list is empty, polygon disappears. */
		free ((char *) et);
		return 0;
	}

	do {
		temp = 0;
		pet = et;
		p1 = pet->nxt;
		while (p1->nxt) {
			if (p1->ymn > p1->nxt->ymn) {
				paet = p1->nxt;		    /* Swap to sort y */
				p1->nxt = p1->nxt->nxt;
				paet->nxt = pet->nxt;
				pet->nxt = paet;
				temp = 1;
			} else if (p1->ymn == p1->nxt->ymn) { /* Sort on > x*/
				if (p1->xmn > p1->nxt->xmn) {
					paet = p1->nxt;	    /* Swap to sort x */
					p1->nxt = p1->nxt->nxt;
					paet->nxt = pet->nxt;
					pet->nxt = paet;
					temp = 1;
				}
			}
			pet = pet->nxt;
			p1 = pet->nxt;
		}
	} while (temp);

    /*
     *  Ready to draw the segments
     */

	pet = et [0].nxt;
	paet = aet;			/* paet points to Active Edge Table */
	cury = pet->ymn;		/* Set ypixel to 1st non-empty bucket */
	paet->nxt = 0;			/* Init aet to empty */
	if (spr) {			/* If old mpr is wider than needed... */
	    op &= ~PIX_DONTCLIP;	/* must clip to avoid using mpr trash */
	    sy += cury - dy;		/* Give sy starting point within spr */
	    while (sy >= spr_height)
		sy -= spr_height;
	    while (sy < 0)
		sy += spr_height;
	}
	do {				/* Repeat til aet & et are empty */
		/*
		 * Get current edges out of edge table (et) and put in active
		 * edge table (aet).
		 */
		p1 = paet;
		while (pet && cury >= pet->ymn) {
			while ( p1->nxt && (p1->nxt->xmn < pet->xmn))
				p1 = p1->nxt;
			p2 = p1->nxt;
			p1->nxt = pet;
			pet = pet->nxt;
			p1->nxt->nxt = p2;
		}
		
		/*
		 * Paint the current (active) scanline segments.  We are
		 * guaranteed that the edges come in pairs.
		 */

		p1 = paet->nxt;
		/* Don't rop on scanlines outside clipping bound */
		if( (cury >= 0) && (cury < dpr->pr_size.y) ) {
		    /* Need not observe PIX_DONTCLIP, and faster not to do so */
		    while (p1) {
			p2 = p1->nxt;
			/* old replrop fix: x1 = p1->xmn; if(x1 < 0)x1=0; */
			/* width  =  last - first + 1 */
			(void) pr_rop (dpr, p1->xmn, cury, p2->xmn - p1->xmn, 1,
				op, spr, p1->xmn - soffset, sy);
			p1 = p2->nxt;
		    }
		}

		cury++;		/* Prepare for next scan line. */
		if (++sy >= spr_height)
			sy -= spr_height;

		/*
		 * Remove active edges when cury >= ymax
		 */

		p1 = paet;
		while (p1->nxt)
			if (cury >= p1->nxt->ymx) {
				p1->nxt = p1->nxt->nxt;
			} else p1 = p1->nxt;

		/*
		 * Update x values in Active Edge Table.
		 */

		p1 = paet;
		while (p1->nxt) {
			p1 = p1->nxt;
			if (p1->errx >= 0) {		/* octants 0 and 1. */
				if (p1->erry < p1->errx) {	/* octant 0. */
					p1->xmn += p1->dx;
					p1->error += p1->erry * p1->dx;
					if (p1->error <= 0) {
						p1->error += p1->erry -p1->errx;
						p1->xmn++;
					} else {
						p1->error -= p1->errx;
					}
				} else {			/* octant 1. */
					p1->error += p1->errx;
					if (p1->error > 0) {
						p1->error -= p1->erry;
						p1->xmn++;
					}
				}
			} else {		/* octants 2 and 3. */
				if (p1->erry < -p1->errx) {	/* Octant 3. */
					p1->xmn += p1->dx;
#ifdef VECSDRAWTORIGHT
					p1->error -= p1->errx;
					if (p1->error > 0) {
						p1->xmn--;
						p1->error -= p1->erry;
					}
					p1->error += p1->erry * p1->dx;
#else
					p1->error -= p1->erry * p1->dx;
					if (p1->error <= 0) {
						p1->error += p1->erry;
						p1->xmn--;
					}
					p1->error += p1->errx;
#endif
			    } else {			/* Octant 2. */
#ifdef VECSDRAWTORIGHT
					p1->error += p1->errx;
					if (p1->error <= -p1->erry) {
						p1->error += p1->erry;
						p1->xmn--;
					}
#else
					p1->error -= p1->errx;
					if (p1->error > 0) {
						p1->error -= p1->erry;
						p1->xmn--;
					}
#endif
				}
			}
		}

		/* Resort on > xmn because previous step
		 * may have crossed edges.
		 */

		p1 = paet;
		if (p1->nxt) {
			do {
				p1 = paet;
				p2 = p1->nxt;
				temp = 0;
				while (p2->nxt)
					if (p2->xmn > p2->nxt->xmn) {
						p1->nxt = p2->nxt;
						p2->nxt = p2->nxt->nxt;
						p1->nxt->nxt = p2;
						p1 = p1->nxt;
						temp = 1;
					} else {
						p1 = p1->nxt;
						p2 = p1->nxt;
					}
			} while (temp);
		}
	} while (paet->nxt || pet);
    }
    free ((char *) et);
    return 0;
} /* pr_polygon2 */

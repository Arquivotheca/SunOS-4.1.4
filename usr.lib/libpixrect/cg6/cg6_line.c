#ifndef lint
static char sccsid[] = "@(#)cg6_line.c 1.1 94/10/31 SMI";
#endif
 
/*
 * Copyright 1988-1989, Sun Microsystems, Inc
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_line.h>
#include <pixrect/cg6var.h>

/* XXX  Are odd widths handled properly? */

#define CG6_VEC(fbc, x0, y0, x1, y1, r) _STMT( \
	(fbc)->l_fbc_ilineabsy = (y0); \
	(fbc)->l_fbc_ilineabsx = (x0); \
	(fbc)->l_fbc_ilineabsy = (y1); \
	(fbc)->l_fbc_ilineabsx = (x1); \
	if (fbc_rev0(fbc)) \
		cg6_draw_done0((fbc), (x0), (x1), (r)); \
	else \
		cg6_draw_done((fbc), (r)); \
)

#define CG6_BOX(fbc, x0, y0, x1, y1, x2, y2, x3, y3, r) _STMT( \
	(fbc)->l_fbc_iquadabsy = (y0); \
	(fbc)->l_fbc_iquadabsx = (x0); \
	(fbc)->l_fbc_iquadabsy = (y1); \
	(fbc)->l_fbc_iquadabsx = (x1); \
	(fbc)->l_fbc_iquadabsy = (y2); \
	(fbc)->l_fbc_iquadabsx = (x2); \
	(fbc)->l_fbc_iquadabsy = (y3); \
	(fbc)->l_fbc_iquadabsx = (x3); \
	cg6_draw_done((fbc), (r)); \
)

#define CG6_RBOX(fbc, x0, y0, x2, y2, r) _STMT( \
	(fbc)->l_fbc_irectabsy = (y0); \
	(fbc)->l_fbc_irectabsx = (x0); \
	(fbc)->l_fbc_irectabsy = (y2); \
	(fbc)->l_fbc_irectabsx = (x2); \
	cg6_draw_done((fbc), (r)); \
)

#define	SWAP(a, b, t)	((t) = (b), (b) = (a), (a) = (t))


cg6_line(pr, x0, y0, x1, y1, brush, tex, op)
	register Pixrect *pr;
	register int x0, y0, x1, y1;
	struct pr_brush *brush;
	Pr_texture *tex;
	int op;
{
    	register struct fbc *fbc;
	register int r;
	register int w;

    	if (tex || (brush && brush->width == 2))
		return PIX_ERR;

    	{
		register int x, y;
        	register struct cg6pr *dprd = cg6_d(pr);

    		fbc = dprd->cg6_fbc;

		x = dprd->mprp.mpr.md_offset.x;
		y = dprd->mprp.mpr.md_offset.y;
	
    		/* set raster op */
    		cg6_setregs(fbc, 
			x, y,				/* offset */
			PIXOP_OP(op),			/* rop */
			dprd->mprp.planes,		/* planemask */
			PIXOP_COLOR(op),		/* fcolor (bcol = 0) */
			L_FBC_RASTEROP_PATTERN_ONES,	/* pattern */
			L_FBC_RASTEROP_POLYG_OVERLAP	/* polygon overlap */
    		);

		/* set clip rectangle */
		if (op & PIX_DONTCLIP)
			cg6_clip(fbc, 0, 0,
				dprd->cg6_size.x - 1,
				dprd->cg6_size.y - 1);
		else
			cg6_clip(fbc, x, y, 
				x + pr->pr_size.x - 1,
				y + pr->pr_size.y - 1);
    	}

    	/* draw thin vectors with pr_vector */
    	if (!brush || (w = brush->width) <= 1) {
		CG6_VEC(fbc, x0, y0, x1, y1, r);
		goto done;
    	}

	/* vertical rectangle */
	if (x0 == x1) {
		CG6_RBOX(fbc, x0 - (w>>1), y0, x0 + ((w-1)>>1), y1, r);
		goto done;
	}

	/* force vector to go left to right */
	if (x0 > x1) {
		SWAP(x0, x1, r);
		SWAP(y0, y1, r);
	}

	/* horizontal rectangle */
	if (y0 == y1) {
		CG6_RBOX(fbc, x0, y0 - (w>>1), x1, y0 + ((w-1)>>1), r);
		goto done;
	}

	{
		/*  dx_r => dx to right
		    dx_l => dx to left
		    dy_u => dy up
		    dy_d => dy down
		*/
		register int dx_r, dy_u, dx_l, dy_d; 
		register int vecl, offset;
		/*
		 * Find vector length, calculate distance of
		 * rectangle endpoints from vector endpoints.
		 */
		dx_r = y1 - y0;  
		dy_u = x1 - x0;

		w--;

		vecl = pr_fat_sqrt(dx_r * dx_r + dy_u * dy_u);
		w <<= 1;
		dx_r = ((w * dx_r)/vecl);
		dy_u = ((w * dy_u)/vecl);  
		
		offset = dx_r & 3;   /* calculate remainder */
		dx_r >>= 2;
		
		if (offset == 3)     /* calculate x offsets */ 
		  dx_r++;
		if (offset == 2)
		  dx_l = (dx_r + 1);
		else 
		  dx_l = dx_r;
		offset = dy_u & 3;   /* calculate remainder */
		dy_u >>= 2;
		
		if (offset == 3)     /* calculate y offsets */
		  dy_u++;
		if (offset == 2) 
		  dy_d = (dy_u + 1);
		else 
		  dy_d = dy_u;

                CG6_BOX(fbc,
                        x0 + dx_r, y0 - dy_u,
                        x0 - dx_l, y0 + dy_d,
                        x1 - dx_l, y1 + dy_d,
                        x1 + dx_r, y1 - dy_u,
                        r);
	}
	

done:
	return fbc_exception(r) ? PIX_ERR : 0;
}

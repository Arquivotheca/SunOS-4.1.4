#ifndef lint
static char sccsid[] = "@(#)pr_line.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1989 Sun Microsystems, Inc.
 */
 


#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_line.h>
#include <pixrect/gp1var.h>
#include <pixrect/gp1cmds.h>

extern int cg6_rop();

#define abs(i)		((i) < 0 ? -(i) : (i))

/*
 * pro_line is called by either pr_polyline or by the user calling pr_line,
 * and doing a macro substitution to pro_line.  If it is called by pr_polyline,
 * the poly argument is usually a 1 (except for the first vector), while if
 * it is being called by the user, the field is always zero.  Regardless,
 * the poly bitfield of the tex struct is filled with this value.
 */

pro_line(pr, x0, y0, x1, y1, brush, tex, op, poly)
    register Pixrect *pr;
    int x0, y0, x1, y1;
    register struct pr_brush *brush;
    register Pr_texture *tex;
    int op, poly;
{
    register short majax, minax, error;
    register int dxprime, dyprime;
    short p0x, p0y, p1x, p1y, dx, dy, poslope;
    int diag_sq, seg_error, old_error, vecl;
    int maj_count=0, maj_sq=1, min_sq=1, min_count=0;
    int pr_errs = 0;

/*
 * If the machine has a gp, an we weren't called from polyline, pass this 
 * command down to the gp, if the microcode supports textured & fat lines. 
 * If the number of segments is greater than 16, gp1_line will return a
 * value of 1, and the pixrect code will handle the vector, else the gp 
 * code will return a zero, and we will return.
 */    
    if (!poly &&
    	pr->pr_ops->pro_rop == gp1_rop && 
	gp1_d (pr)->fbtype == FBTYPE_SUN2COLOR &&
	gp1_d(pr)->cmdver[GP1_PR_POLYLINE>>8] > 0 &&
	!gp1_line(pr, x0, y0, x1, y1, brush, tex, op)) {
		return 0;
    } else 
    /*
     * If this is a CG6, give it a try
     */
    if (pr->pr_ops->pro_rop == cg6_rop &&
	!cg6_line(pr, x0, y0, x1, y1, brush, tex, op)) {
		return (0);
    }

    /* draw thin vectors with pr_texvec or pr_vector */
    if (!brush || brush->width <= 1) {
	if (tex) {
	    tex->options.res_poly = poly;
	    return pr_texvec(pr, x0, y0, x1, y1, tex, op);
	} else
	    return pr_vector(pr, x0, y0, x1, y1, op, 0);
    }
    
    dx = x1 - x0;
    dy = y1 - y0;
    vecl = pr_fat_sqrt((unsigned) (dx*dx + dy*dy));
    if (vecl == 0) {				/* is a fat point */
	if (tex)
	    return pr_texvec(pr, x0, y0, x1, y1, tex, op);
	else
	    return pr_vector(pr, x0, y0, x1, y1, op, 0);
    }	
    
    if (tex) {					/* is a fat textured vector */
	tex->options.res_poly = poly;
	tex->options.res_fat = 0;		/* set after first vector */
        poslope = ((dx>0)^(dy>0)) ? 0 : 1;
	dyprime = (brush->width * abs(dx)/(vecl))>>1;
	dxprime = (brush->width * abs(dy)/(vecl))>>1; 
	if (dxprime > dyprime) {		/* y is major axis */
            minax=2*dyprime;	
            majax=2*dxprime;
            error=2*dyprime - dxprime;
            p0x=x0-dxprime; p1x=x1-dxprime;
	    if (poslope) {
		p0y=y0+dyprime; p1y=y1+dyprime;
	    } else {
		p0y=y0-dyprime; p1y=y1-dyprime;
	    }
 	    diag_sq = brush->width * brush->width;
	    seg_error = diag_sq - (maj_sq + min_sq);
	    do {
		pr_errs |= pr_texvec (pr, p0x, p0y, p1x, p1y, tex, op);
		tex->options.res_fat = 1;	/* CHANGE LATER */
                p0x++; p1x++;
		maj_count++;
		maj_sq += maj_count + maj_count + 1;
		error += minax;
		if (error > 0) {
		    pr_errs |= pr_texvec (pr, p0x, p0y, p1x, p1y, tex, op);
                    if (poslope) {
			p0y--; p1y--;
		    } else {
			p0y++; p1y++;
		    }
		    min_count++;
		    min_sq += min_count + min_count + 1;
		    error -= majax;
		}
		old_error = seg_error;
		seg_error = diag_sq - (maj_sq + min_sq);
	    } while ((abs(seg_error)) <= (abs(old_error)));  
        } else {				/* x is major axis */
            minax=2*dxprime;
            majax=2*dyprime;
            error=2*dxprime - dyprime;
            p0y=y0-dyprime; p1y=y1-dyprime;
	    if (poslope) {
		p0x=x0+dxprime; p1x=x1+dxprime;
	    } else {
		p0x=x0-dxprime; p1x=x1-dxprime;
	    }
	    diag_sq = brush->width * brush->width;
	    seg_error = diag_sq - (maj_sq + min_sq);
	    do {
		pr_errs |= pr_texvec (pr, p0x, p0y, p1x, p1y, tex, op);
 		tex->options.res_fat = 1;	/* CHANGE LATER */
		p0y++; p1y++;
		maj_count++;
		maj_sq += maj_count + maj_count + 1;
		error += minax;
		if (error > 0) {
		    pr_errs |= pr_texvec (pr, p0x, p0y, p1x, p1y, tex, op);
                    if (poslope) {
			p0x--; p1x--;
		    } else {
			p0x++; p1x++;
		    }
		    min_count++;
		    min_sq += min_count + min_count + 1;
		    error -= majax;
		}
		old_error = seg_error;
		seg_error = diag_sq - (maj_sq + min_sq);
	    } while ((abs(seg_error)) <= (abs(old_error)));  
	}
	tex->options.res_fat = 0;
    
    } else {					/* is a fat solid vector */
        poslope = ((dx>0)^(dy>0)) ? 0 : 1;
	dyprime = (brush->width * abs(dx)/(vecl))>>1;
	dxprime = (brush->width * abs(dy)/(vecl))>>1; 
	if (dxprime > dyprime) {		/* y is major axis */
            minax=2*dyprime;	
            majax=2*dxprime;
            error=2*dyprime - dxprime;
            p0x=x0-dxprime; p1x=x1-dxprime;
	    if (poslope) {
		p0y=y0+dyprime; p1y=y1+dyprime;
	    } else {
		p0y=y0-dyprime; p1y=y1-dyprime;
	    }
 	    diag_sq = brush->width * brush->width;
	    seg_error = diag_sq - (maj_sq + min_sq);
	    do {
		pr_errs |= pr_vector (pr, p0x, p0y, p1x, p1y, op, 0);
                p0x++; p1x++;
		maj_count++;
		maj_sq += maj_count + maj_count + 1;
		error += minax;
		if (error > 0) {
		    pr_errs |= pr_vector (pr, p0x, p0y, p1x, p1y, op, 0);
                    if (poslope) {
			p0y--; p1y--;
		    } else {
			p0y++; p1y++;
		    }
		    min_count++;
		    min_sq += min_count + min_count + 1;
		    error -= majax;
		}
		old_error = seg_error;
		seg_error = diag_sq - (maj_sq + min_sq);
	    } while ((abs(seg_error)) <= (abs(old_error)));  
        } else {				/* x is major axis */
            minax=2*dxprime;
            majax=2*dyprime;
            error=2*dxprime - dyprime;
            p0y=y0-dyprime; p1y=y1-dyprime;
	    if (poslope) {
		p0x=x0+dxprime; p1x=x1+dxprime;
	    } else {
		p0x=x0-dxprime; p1x=x1-dxprime;
	    }
	    diag_sq = brush->width * brush->width;
	    seg_error = diag_sq - (maj_sq + min_sq);
	    do {
		pr_errs |= pr_vector (pr, p0x, p0y, p1x, p1y, op, 0);
		p0y++; p1y++;
		maj_count++;
		maj_sq += maj_count + maj_count + 1;
		error += minax;
		if (error > 0) {
		    pr_errs |= pr_vector (pr, p0x, p0y, p1x, p1y, op, 0);
                    if (poslope) {
			p0x--; p1x--;
		    } else {
			p0x++; p1x++;
		    }
		    min_count++;
		    min_sq += min_count + min_count + 1;
		    error -= majax;
		}
		old_error = seg_error;
		seg_error = diag_sq - (maj_sq + min_sq);
	    } while ((abs(seg_error)) <= (abs(old_error)));  
	}
    }
    return pr_errs;
} /* pro_line */



int
pr_fat_sqrt(n) 
    register unsigned n;
{
    register unsigned q,r,x2,x;
    unsigned t;

    if (n > 0xFFFE0000) return 0xFFFF;	/* algorithm doesn't cover this case*/
    if (n == 0xFFFE0000) return 0xFFFE;	/* or this case */
    if (n < 2) return n;		/* or this case */
    t = x = n;
    while (t>>=2) x>>=1;
    x++;
    for(;;) {
	q = n/x;
	r = n%x;
	if (x <= q) {
		x2 = x+2;
		if (q < x2 || q==x2 && r==0) break;
	}
	x = (x + q)>>1;
    }
    return x;
} /* pr_fat_sqrt */

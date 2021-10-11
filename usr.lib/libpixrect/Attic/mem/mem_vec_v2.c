#ifndef lint
static char sccsid[] = "@(#)mem_vec_v2.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Memory pixrect vector
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>

#define swap(a,b,t)	{(t) = (a); (a) = (b); (b) = (t);}

static int  optable[4] =
{
    0, 1, -1, 2
};
extern char pr_reversedst[];

mem_vector (dst, pos0, pos1, op, color)
struct pixrect *dst;
struct pr_pos   pos0, pos1;
int     op, color;
{
    register    d7_dx, d6_dy, d5_count, d4_initerror, d3_skew;

 /* d7,d6,d5,d4,d3 */
    register short *a5_adr, *a4_vert, *a3_enderror, *a2_dxclip;
 /* a5,a4,a3,a2 */
    int     clip, reflect = 0, col;
    short   i0, i1, i2;
    struct mpr_data *dmd;
    struct pr_size  size;
    struct pr_pos   tempos, start;
    short   depth1;

    depth1 = (dst -> pr_depth == 1);
    size = dst -> pr_size;
    dmd = mpr_d (dst);
    clip = !(op & PIX_DONTCLIP);
    col = PIX_OPCOLOR (op);
    if (col)
	color = col;
    op = (op >> 1) & 0xf;
    if (dmd -> md_flags & MP_REVERSEVIDEO)
	op = (pr_reversedst[op]);
/*    if (depth1)
    { */
	if (color)
	    op >>= 2;
	op = optable[op & 3];
	if (op < 0)
	    return (0);
/*     } */

    d7_dx = pos1.x - pos0.x;
    d6_dy = pos1.y - pos0.y;

 /* 
  * Handle length 1 or 2 vectors by just drawing endpoints.
  */
    if ((u_int) (d7_dx + 1) <= 2 && (u_int) (d6_dy + 1) <= 2)
    {
      if (depth1)
      {
	if (clip == 0 ||
		(u_int) pos0.x < size.x && (u_int) pos0.y < size.y)
	{
	    a5_adr = mprd_addr (dmd, pos0.x, pos0.y);
	    d3_skew = 0x8000 >> mprd_skew (dmd, pos0.x, pos0.y);
	    switch (op)
	    {
		case 0: 
		    *a5_adr &= ~d3_skew;
		    break;
		case 1: 
		    *a5_adr ^= d3_skew;
		    break;
		case 2: 
		    *a5_adr |= d3_skew;
		    break;
	    }
	}
	if ((d7_dx != 0 || d6_dy != 0) &&
		(clip == 0 ||
		    (u_int) pos1.x < size.x && (u_int) pos1.y < size.y))
	{
	    a5_adr = mprd_addr (dmd, pos1.x, pos1.y);
	    d3_skew = 0x8000 >> mprd_skew (dmd, pos1.x, pos1.y);
	    switch (op)
	    {
		case 0: 
		    *a5_adr &= ~d3_skew;
		    break;
		case 1: 
		    *a5_adr ^= d3_skew;
		    break;
		case 2: 
		    *a5_adr |= d3_skew;
		    break;
	    }
	}
      } else
      /* 8 bit */
      {
        register u_char *col_adr;

	if (clip == 0 ||
		(u_int) pos0.x < size.x && (u_int) pos0.y < size.y)
        {
	    col_adr = mprd8_addr (dmd, pos0.x, pos0.y, dst -> pr_depth);
	    switch (op)
	    {
		case 0: 
		    *col_adr = 0; /* to jive with longer vectors */
		    break;
		case 1: 
		    *col_adr ^= color;
		    break;
		case 2: 
		    *col_adr = color;
		    break;
	    }
	}
	if ((d7_dx != 0 || d6_dy != 0) &&
		(clip == 0 ||
		    (u_int) pos1.x < size.x && (u_int) pos1.y < size.y))
	{
	    col_adr = mprd8_addr (dmd, pos1.x, pos1.y, dst -> pr_depth);
	    switch (op)
	    {
		case 0: 
		    *col_adr = 0;
		    break;
		case 1: 
		    *col_adr ^= color;
		    break;
		case 2: 
		    *col_adr = color;
		    break;
	    }
	}
      }
      return (0);
    }

 /* 
  * Need to normalize the vector so that the following
  * algorithm can limit the number of cases to be considered.
  * We can always interchange the points in x, so that
  * pos0 is to the left of pos1.
  */
    if (d7_dx < 0)
    {
    /* force vector to scan to right */
	d7_dx = -d7_dx;
	d6_dy = -d6_dy;
	swap (pos0, pos1, tempos);
    }

 /* 
  * The clipping routine needs to work with a vector which
  * increases in y from pos0.y to pos1.y.  We want to change
  * pos0.y and pos1.y so that there is an increase in y
  * from pos0.y to pos1.y without affecting the clipping
  * and bounds checking that we will do.  We accomplish
  * this by reflecting the vector around the horizontal
  * centerline of dst, and remember that we did this by
  * incrementing reflect by 2.  The reflection will be undone
  * before the vector is drawn.
  */
    if (d6_dy < 0)
    {
	d6_dy = -d6_dy;
	pos0.y = (size.y - 1) - pos0.y;
	pos1.y = (size.y - 1) - pos1.y; 
/*	pos0.y = (size.y) - pos0.y;
	pos1.y = (size.y) - pos1.y; */
	reflect += 2;
    }

 /* 
  * Can now do bounds check, since the vector increasing in
  * x and y can check easily if it has no chance of intersecting
  * the destination rectangle: if the vector ends before the
  * beginning of the target or begins after the end!
  */
    if (clip &&
	    (pos1.y < 0 || pos0.y >= size.y || pos1.x < 0 || pos0.x >= size.x))
	return (0);

 /* 
  * If vector is horizontal, use fast algorithm.
  */
    if (d6_dy == 0)
    {
	if (clip)
	{
	    if (pos0.x < 0)
		pos0.x = 0;
	    if (pos1.x >= size.x)
		pos1.x = size.x - 1;
	    d7_dx = pos1.x - pos0.x;
	}
	if (depth1)
	{
	    a5_adr = mprd_addr (dmd, pos0.x, pos0.y);
	    d3_skew = mprd_skew (dmd, pos0.x, pos0.y);
	    switch (op)
	    {
		case 0: 
 			d7_dx++; 
		    if(d3_skew) {
		    d5_count = ((d7_dx-(16-d3_skew)) >> 4) - 1;
		    }
		    else {
		    d5_count = ((d7_dx) >> 4) - 1;
		    }
		    if (d5_count > -1)
		    {		/* stuff in the middle */
/* 			d7_dx++; */
			if (d3_skew)
			{
			d6_dy = 0xffff0000 >> d3_skew;
/* 			*a5_adr++ = (*a5_adr & d6_dy) | (~d6_dy & ~(*a5_adr)); */
			*a5_adr &= d6_dy;
			*a5_adr++;
			}
			do
			{
/* 			    *a5_adr++ = ~(*a5_adr) & 0xffff; */
			    *a5_adr++ &= ~(*a5_adr);
			} while (--d5_count != -1);
			d4_initerror = ((d7_dx - (16 - d3_skew)) & 15);
			if (d4_initerror) {
			d6_dy = 0xffff0000 >> d4_initerror;
/* 			*a5_adr = (*a5_adr & ~d6_dy) | (d6_dy & ~(*a5_adr)); */
			*a5_adr &= ~d6_dy;
			}
		    }
		    else
		    {		/* one write */
			if ((d7_dx + d3_skew) >> 4)
			{
/* 			d7_dx++; */
			    d6_dy = 0xffff0000 >> d3_skew;
			    *a5_adr &= d6_dy;
			    a5_adr++;
/* 			    d6_dy = 0xffff0000 >> ((d7_dx + d3_skew) & 15); */
/* 			    *a5_adr &= d6_dy; */
			    d6_dy = 0xffff0000 >> (d7_dx - (16 - d3_skew));
			    *a5_adr &= ~d6_dy; 
			}
			else
			{
/*  			d7_dx++;  */
			    d6_dy = 0xffff0000 >> d3_skew;
 			    d6_dy |= 0x0000ffff >> (d7_dx + d3_skew);
 			    *a5_adr &= d6_dy; 
/* 			    d6_dy |= 0x00007fff >> ((d3_skew + d7_dx) & 15); */
/* 			    *a5_adr &= d6_dy; */
			}
		    }
		    break;
		case 1: 
 			d7_dx++; 
		    if(d3_skew) {
		    d5_count = ((d7_dx-(16-d3_skew)) >> 4) - 1;
		    }
		    else {
		    d5_count = ((d7_dx) >> 4) - 1;
		    }
		    if (d5_count > -1)
		    {		/* stuff in the middle */
/* 			d7_dx++; */
			if (d3_skew)
			{
			d6_dy = 0xffff0000 >> d3_skew;
			*a5_adr++ = (*a5_adr & d6_dy) | (~d6_dy ^ *a5_adr);
			}
			do
			{
			    *a5_adr++ ^= 0xffff;
			} while (--d5_count != -1);
			d4_initerror = ((d7_dx - (16 - d3_skew)) & 15);
			if (d4_initerror) {
			d6_dy = 0xffff0000 >> d4_initerror;
/* 			*a5_adr = (*a5_adr & d6_dy) | (~d6_dy ^ *a5_adr); */
			*a5_adr ^= d6_dy;
			}
		    }
		    else
		    {		/* one write */
			if ((d7_dx + d3_skew) >> 4)
			{
/* 			d7_dx++; */
			    d6_dy = 0xffff0000 >> d3_skew;
			    *a5_adr ^= ~d6_dy;
			    a5_adr++;
/* 			    d6_dy = 0xffff0000 >> ((d7_dx + d3_skew) & 15); */
/* 			    *a5_adr ^= ~d6_dy; */
			    d6_dy = 0xffff0000 >> (d7_dx - (16 - d3_skew));
			    *a5_adr ^= d6_dy; 
			}
			else
			{
/*  			d7_dx++;  */
			    d6_dy = 0xffff0000 >> d3_skew;
 			    d6_dy |= 0x0000ffff >> (d7_dx + d3_skew);
 			    *a5_adr ^= ~d6_dy; 
			}
		    }
		    break;
		case 2:
			d7_dx++;
		    if(d3_skew) {
		    d5_count = ((d7_dx-(16-d3_skew)) >> 4) - 1;
		    }
		    else {
		    d5_count = ((d7_dx) >> 4) - 1;
		    }
		    if (d5_count > -1)
		    {		/* stuff in the middle */
			if (d3_skew)
			{
			    d6_dy = 0xffff0000 >> d3_skew;
			    *a5_adr++ = (*a5_adr & d6_dy) | (~d6_dy | *a5_adr);
/* 			    d5_count = ((d7_dx - (16 - d3_skew)) >> 4) - 1; */
			}
			do
			{
			    *a5_adr++ |= 0xffff;
			} while (--d5_count != -1);
			d4_initerror = ((d7_dx - (16 - d3_skew)) & 15);
			if (d4_initerror) {
			d6_dy = 0xffff0000 >> d4_initerror;
			*a5_adr |= d6_dy;
			}
		    }
		    else
		    {		/* one write */
			if ((d7_dx + d3_skew) >> 4)
			{
/* 			    d7_dx++; */
			    d6_dy = 0xffff0000 >> d3_skew;
			    *a5_adr |= ~d6_dy;
			    a5_adr++;
/*			    d6_dy = 0xffff0000 >> ((d7_dx + d3_skew) & 15);
			    *a5_adr |= ~d6_dy; */
			    d6_dy = 0xffff0000 >> (d7_dx - (16 - d3_skew));
			    *a5_adr |= d6_dy; 
			}
			else
			{
/*  			    d7_dx++;  */
			    d6_dy = 0xffff0000 >> d3_skew;
 			    d6_dy |= 0x0000ffff >> (d7_dx + d3_skew);
 			    *a5_adr |= ~d6_dy; 
			}
		    }
		    break;
	    }
	}
	else
	{			/* depth8 code */ 
	    register u_char *col_adr;
	    if((pos0.x == pos1.x)   && (pos0.y == pos1.y)) {
		/* trap zero length vectors */
		return (0);
	    }
/* 	    col_adr =  (short*) mprd8_addr (dmd, pos0.x, pos0.y, dst -> pr_depth); */
	    col_adr =  mprd8_addr (dmd, pos0.x, pos0.y, dst -> pr_depth);
	    switch (op)
	    {
		case 0: 
/* 		    color = ~color; */
		    do
		    {
/*			*col_adr &= color;
			*col_adr++; */
			*col_adr++ = 0;
		    } while (--d7_dx);
		    break;
		case 1: 
		    do
		    {
			*col_adr ^= color;
			*col_adr++;
		    } while (--d7_dx);
		    break;
		case 2: 
		    do
		    {
			*col_adr = color;
			*col_adr++;
		    } while (--d7_dx);
		    break;

	    }
	}
	return (0);
    }

 /* 
  * If vector is vertical, use fast algorithm.
  */
    if (d7_dx == 0)
    {
	if (clip)
	{
	    if (pos0.y < 0)
		pos0.y = 0;
	    if (pos1.y >= size.y)
		pos1.y = size.y - 1;
	    d6_dy = pos1.y - pos0.y;
	}
	if (reflect & 2)	
	    pos0.y = size.y - pos1.y-1; 
/*  	    if(d6_dy) d6_dy--;  */
	if (depth1)
	{
	    a5_adr = mprd_addr (dmd, pos0.x, pos0.y);
	    d3_skew = mprd_skew (dmd, pos0.x, pos0.y);
	    d5_count = dmd -> md_linebytes >> 1;
	    switch (op)
	    {
		case 0: 
		    d7_dx = 0xffff0000 >> d3_skew;
		    d7_dx |= 0x0000ffff >> (d3_skew + 1);
/* 		    d7_dx = ~d7_dx; */
		    do
		    {
/* 			*a5_adr = ((*a5_adr) & (~d7_dx)); */
			*a5_adr &= d7_dx;
			a5_adr += d5_count;
		    } while (--d6_dy != -1);
		    break;
		case 1: 
		    d7_dx = 0xffff0000 >> d3_skew;
		    d7_dx |= 0x0000ffff >> (d3_skew + 1);
		    d4_initerror = ~d7_dx;
		    do
		    {
			*a5_adr = (*a5_adr & d7_dx) | (*a5_adr ^ d4_initerror);
			a5_adr += d5_count;
		    } while (--d6_dy != -1);
		    break;
		case 2: 
		    d7_dx = 0xffff0000 >> d3_skew;
		    d7_dx |= 0x0000ffff >> (d3_skew + 1);
		    d4_initerror = ~d7_dx;
		    do
		    {
			*a5_adr = (*a5_adr & d7_dx) | (*a5_adr | d4_initerror);
			a5_adr += d5_count;
		    } while (--d6_dy != -1);
		    break;
	    }
	}
	else
	{			/* compile and run depth8 code */
	    register u_char *col_adr;
	    col_adr = mprd8_addr (dmd, pos0.x, pos0.y, dst -> pr_depth);
	    d5_count =  dmd -> md_linebytes;
	    switch (op)
	    {
		case 0: 
/* 		    color = ~color; */
		    do
		    {
/* 			*col_adr &= color; */
			*col_adr = 0;
			col_adr += d5_count;
		    } while (--d6_dy != -1);
		    break;
		case 1: 
		    do
		    {
			*col_adr ^= color;
			col_adr += d5_count;
		    } while (--d6_dy != -1);
		    break;
		case 2: 
		    do
		    {
			*col_adr = color;
			col_adr += d5_count;
		    } while (--d6_dy != -1);
		    break;
	    }
	}
	return (0);
    }

 /* 
  * One more reflection: we want to assume that d7_dx >= d6_dy.
  * So if this is not true, we reflect the vector around
  * the diagonal line x = y and remember that we did
  * this by adding 1 to reflect.
  */
    if (d7_dx < d6_dy)
    {
	swap (pos0.x, pos0.y, d5_count);
	swap (pos1.x, pos1.y, d5_count);
	swap (d7_dx, d6_dy, d5_count);
	swap (size.x, size.y, d5_count);
	reflect += 1;
    }

    d4_initerror = -(d7_dx >> 1);/* error at pos0 */
    start = pos0;
    if (!clip)
    {
	goto clipdone;
    }

 /* 
  * Begin hard part of clipping.
  *
  * We have insured that we are clipping on a vector
  * which has dx > 0, dy > 0 and dx >= dy.  The error is
  * the vertical distance from the true line to the approximation
  * in units where each pixel is dx by dx.  Moving one
  * to the right (increasing x by 1) subtracts dy from
  * the error.  Moving one pixel down (increasing y by 1)
  * adds dx to the error.
  *
  * Bresenham functions by restoring the error to the
  * range (-dx,0] whenever the error leaves it.  The
  * algorithm increases x and increases y when
  * it needs to constrain the error.
  */

 /* 
  * Clip left end to yield start of visible vector.
  * If the starting x coordinate is negative, we
  * must advance to the first x coordinate which will
  * be drawn (x=0).  As we advance (-start.x) units in the
  * x direction, the error increases by (-start.x)*dy.
  * This means that the error when x=0 will be
  *	-(dx/2)+(-start.x)*dy
  * For each y advance in this range, the error is reduced
  * by dx, and should be in the range (-dx,0] at the y
  * value at x=0.  Thus to compute the increment in y we
  * should take
  *	(-(dx/2)+(-start.x)*dy+(dx-1))/dx
  * where the numerator represents the length of the interval
  *	[-dx+1,-(dx/2)+(-start.x)*dy]
  * The number of dx steps by which the error can be reduced
  * and stay in this interval is the y increment which would
  * result if the vector were drawn over this interval.
  */
    if (start.x < 0)
    {
	d4_initerror += pr_product (-start.x, d6_dy);
	start.x = 0;
	d5_count = (d4_initerror + (d7_dx - 1)) / d7_dx;
	start.y += d5_count;
	d4_initerror -= pr_product (d5_count, d7_dx);
    }

 /* 
  * After having advanced x to be at least 0, advance
  * y to be in range.  If y is already too large (and can
  * only get larger!), just give up.  Otherwise, if start.y < 0,
  * we need to compute the value of x at which y is first 0.
  * In advancing y to be zero, the error decreases by (-start.y)*dx,
  * in the y steps.
  *
  * Immediately after an advance in y the error is in the range
  * (-dx,-dx+dy].  This can be seen by noting that what last
  * happened was that the error was in the range (-dy,0],
  * the error became positive by adding dy, to be in the range (0,dy],
  * and we subtracted dx to get into the range (-dx,-dx+dy].
  *
  * Thus we need to advance x to cause the error to change
  * to be at most (-dx+dy), or, in steps of dy, at most:
  *	((-dx+dy)-error)/dy
  * which is the number of dy steps in the interval [error,-dx+dy].
  */
    if (start.y >= size.y)
	return (0);
    if (start.y < 0)
    {
    /* skip to dst top edge */
	d4_initerror += pr_product (start.y, d7_dx);
	start.y = 0;
	d5_count = ((-d7_dx + d6_dy) - d4_initerror) / d6_dy;
	start.x += d5_count;
	if (start.x >= size.x)
	    return (0);
    }

 /* 
  * Now clip right end.
  *
  * If the last x position is outside the rectangle,
  * then clip the vector back to within the rectangle
  * at x=size.x-1.  The corresponding y value has d4_initerror
  *	-(dx/2)+((size.x-1)-pos0.x)*dy
  * We need an error in the range (-dx,0], so compute
  * the corresponding number of steps in y from the number
  * of dy's in the interval:
  *	(-dx,-(dx/2)+((size.x-1)-pos0.x)*dy]
  * which is:
  *	[-dx+1,-(dx/2)+((size.x-1)-pos0.x)*dy]
  * or:
  *	(-(dx/2)+((size.x-1)-pos0.x)*dy+dx-1)/dx
  * Note that:
  *	dx - (dx/2) != (dx/2)
  * (consider dx odd), so we can't simplify much further.
  */
    if (pos1.x >= size.x)
    {
	pos1.x = size.x - 1;
	pos1.y = pos0.y +
	    (-(d7_dx >> 1) + pr_product ((size.x - 1) - pos0.x, d6_dy) +
		d7_dx - 1) / d7_dx;
    }
 /* 
  * If the last y position is outside the rectangle, then
  * clip it back at y=size.y-1.  The corresponding x value
  * has error
  *	-(dx/2)-((size.y-1)-pos0.y)*dx
  * We want the last x value with this y value, which
  * will have an error in the range (-dy,0] (so that increasing
  * x one more time will make the error > 0.)  Thus
  * the amount of error allocatable to dy steps is from
  * the length of the interval:
  *	[-(dx/2)-((size.y-1)-pos0.y)*dx,0]
  * that is,
  *	(0-(-(dx/2)-((size.y-1)-pos0.y)*dx))/dy
  * or:
  *	((dx/2)+((size.y-1)-pos0.y)*dx)/dy
  */
    if (pos1.y >= size.y)
    {
	pos1.y = size.y - 1;
	pos1.x = pos0.x +
	    ((d7_dx >> 1) + pr_product ((size.y - 1) - pos0.y, d7_dx)) / d6_dy;
    }

clipdone: 
 /* 
  * Now have to set up for the Bresenham routines.
  */

    d7_dx = pos1.x - start.x;
    d6_dy = pos1.y - start.y;
	if ((d7_dx == 0 && d6_dy == 0))
	{
	if (depth1)
	{
/*	if (reflect & 2)
	{
 	    start.x = ((size.x - 1) - start.x); 
 	    start.y = ((size.y - 1) - start.y); 
	} */
	    if(reflect & 1) {
	    a5_adr = mprd_addr (dmd, start.y, start.x);
	    d3_skew = 0x8000 >> mprd_skew (dmd, start.y, start.x);
	    }
	    else {
	    a5_adr = mprd_addr (dmd, start.x, start.y);
	    d3_skew = 0x8000 >> mprd_skew (dmd, start.x, start.y);
	    }		
	    switch (op)
	    {
		case 0: 
		    *a5_adr &= ~d3_skew;
		    break;
		case 1: 
		    *a5_adr ^= d3_skew;
		    break;
		case 2: 
		    *a5_adr |= d3_skew;
		    break;
	    }
	    return(0);
	}
	}
    a4_vert = &i1;
    if (reflect & 1)
    {
    /* major axis is y */
	if (reflect & 2)
	{
	/* unreflect major axis (misnamed x) */
 	    start.x = ((size.x - 1) - start.x); 
/* 	    start.x = ((size.x) - start.x); */
/* 	    a4_vert = (short *) - ((int) a4_vert); */
	}
	if (depth1)
	{
	    a5_adr = mprd_addr (dmd, start.y, start.x);
	    d3_skew = mprd_skew (dmd, start.y, start.x);
/*  			(*bres[op])();  */
	    d3_skew = 0x8000 >> mprd_skew (dmd, start.y, start.x);
	    switch (op)
	    {
		case 0: 
		    d4_initerror = d7_dx / 2;
		    if (reflect & 2)
			*a4_vert =  -dmd -> md_linebytes  >> 1;
		    else
			*a4_vert =  dmd -> md_linebytes  >> 1;
/* 		    *a5_adr &= ~d3_skew; */
		    d5_count = d7_dx;
		    do
		    {
			if (d4_initerror > 0)
			{	/* move in y */
			    *a5_adr &= ~d3_skew;
			    a5_adr += *a4_vert;
			    d4_initerror -= d6_dy;
			    d7_dx--;
			}
			else
			{	/* move in x */
			    d4_initerror += d5_count;
			    if (d3_skew & 1)
			    {
				a5_adr++;
				d3_skew = 0x8000;
			    }
			    else
			    {
				d3_skew >>= 1;
			    }
			}
		    }
		    while (d7_dx != -1);
		    break;
		case 1: 
		    d4_initerror = d7_dx / 2;
		    if (reflect & 2)
			*a4_vert =  -dmd -> md_linebytes  >> 1;
		    else
			*a4_vert =  dmd -> md_linebytes  >> 1;
/* 		    *a5_adr ^= d3_skew; */
		    d5_count = d7_dx;
		    do
		    {
			if (d4_initerror > 0)
			{	/* move in y */
			    *a5_adr ^= d3_skew;
			    a5_adr += *a4_vert;
			    d4_initerror -= d6_dy;
			    d7_dx--;
			}
			else
			{	/* move in x */
			    d4_initerror += d5_count;
			    if (d3_skew & 1)
			    {
				a5_adr++;
				d3_skew = 0x8000;
			    }
			    else
			    {
				d3_skew >>= 1;
			    }
			}
		    }
		    while (d7_dx != -1);
		    break;
		case 2: 
		    d4_initerror = d7_dx / 2;
		    if (reflect & 2)
			*a4_vert =  -dmd -> md_linebytes >> 1;
		    else
			*a4_vert =  dmd -> md_linebytes >> 1;
/* 		    *a5_adr |= d3_skew; */
		    d5_count = d7_dx;
/* 		    while (d7_dx) */
/* 		    while (d7_dx) */
		    do
		    {
			if (d4_initerror > 0)
			{	/* move in y */
			    *a5_adr |= d3_skew;
			    a5_adr += *a4_vert;
			    d4_initerror -= d6_dy;
			    d7_dx--;
			}
			else
			{	/* move in x */
			    d4_initerror += d5_count;
			    if (d3_skew & 1)
			    {
				a5_adr++;
				d3_skew = 0x8000;
			    }
			    else
			    {
				d3_skew >>= 1;
			    }
			}
		    }
		    while (d7_dx != -1);
		    break;
	    }
	}
	else
	{			/* compile and run depth8 code */
	    register u_char *col_adr;
	    col_adr = mprd8_addr (dmd, start.y, start.x, dst -> pr_depth);
	    switch (op)
	    {
		case 0: 
		    d4_initerror = d7_dx / 2;
		    if (reflect & 2)
			*a4_vert =  -dmd -> md_linebytes;
		    else
			*a4_vert =  dmd -> md_linebytes;
/* 		    *col_adr &= ~color; */
		    d5_count = d7_dx;
		    do
		    {
			if (d4_initerror > 0)
			{	/* move in y */
			    *col_adr = 0;
			    col_adr += *a4_vert;
			    d4_initerror -= d6_dy;
			    d7_dx--;
			}
			else
			{	/* move in x */
			    d4_initerror += d5_count;
				col_adr++;
			}
		    }
		    while (d7_dx != -1);
		    break;
		case 1: 
		    d4_initerror = d7_dx / 2;
		    if (reflect & 2)
			*a4_vert =  -dmd -> md_linebytes;
		    else
			*a4_vert =  dmd -> md_linebytes;
/* 		    *col_adr ^= color; */
		    d5_count = d7_dx;
		    do
		    {
			if (d4_initerror > 0)
			{	/* move in y */
			    *col_adr ^= color;
			    col_adr += *a4_vert;
			    d4_initerror -= d6_dy;
			    d7_dx--;
			}
			else
			{	/* move in x */
			    d4_initerror += d5_count;
				col_adr++;
			}
		    }
		    while (d7_dx != -1);
		    break;
		case 2: 
		    d4_initerror = d7_dx / 2;
		    if (reflect & 2)
			*a4_vert =  -dmd -> md_linebytes;
		    else
			*a4_vert =  dmd -> md_linebytes;
/* 		    *col_adr = color; */
		    d5_count = d7_dx;
		    do
		    {
			if (d4_initerror > 0)
			{	/* move in y */
			    *col_adr = color;
			    col_adr += *a4_vert;
			    d4_initerror -= d6_dy;
			    d7_dx--;
			}
			else
			{	/* move in x */
			    d4_initerror += d5_count;
				col_adr++;
			}
		    }
		    while (d7_dx != -1);
		    break;
	    }
	}
    }
    else
    {
    /* major axis is x */
	if (reflect & 2)
	{
	/* unreflect minor axis */
 	    start.y = ((size.y - 1) - start.y); 
/* 	    start.y = ((size.y) - start.y); */
/* 	    a4_vert = (short *) - ((int) a4_vert); */
	}
	if (depth1)
	{
	    a5_adr = mprd_addr (dmd, start.x, start.y);
	    d3_skew = mprd_skew (dmd, start.x, start.y);
	    d3_skew = 0x8000 >> d3_skew;
	    switch (op)
	    {
		case 0: 
		    d4_initerror = d7_dx / 2;
		    if (reflect & 2)
			*a4_vert =  -dmd -> md_linebytes >> 1;
		    else
			*a4_vert =  dmd -> md_linebytes >> 1;
/* 		    *a5_adr &= ~d3_skew; */
		    d5_count = d7_dx;
		    do
		    {
			if (d4_initerror < 0)
			{	/* move in y */
			    a5_adr += *a4_vert;
			    d4_initerror += d5_count;
			}
			else
			{	/* move in x */
			    *a5_adr &= ~d3_skew;
			    d4_initerror -= d6_dy;
			    if (d3_skew & 1)
			    {
				a5_adr++;
				d3_skew = 0x8000;
			    }
			    else
			    {
				d3_skew >>= 1;
			    }
			    d7_dx--;
			}
		    }
		    while (d7_dx != -1);
		    break;
		case 1: 
		    d4_initerror = d7_dx / 2;
		    if (reflect & 2)
			*a4_vert =  -dmd -> md_linebytes >> 1;
		    else
			*a4_vert =  dmd -> md_linebytes >> 1;
/*  		    *a5_adr ^= d3_skew;  */ /* change */
		    d5_count = d7_dx;
		    do
		    {
			if (d4_initerror < 0)
			{	/* move in y */
			    a5_adr += *a4_vert;
			    d4_initerror += d5_count;
			}
			else
			{	/* move in x */
			    *a5_adr ^= d3_skew;
			    d4_initerror -= d6_dy;
			    if (d3_skew & 1)
			    {
				a5_adr++;
				d3_skew = 0x8000;
			    }
			    else
			    {
				d3_skew >>= 1;
			    }
			    d7_dx--;
			}
		    }
		    while (d7_dx != -1);
		    break;
		case 2: 
		    d4_initerror = d7_dx / 2;
		    if (reflect & 2)
			*a4_vert =  -dmd -> md_linebytes >> 1;
		    else
			*a4_vert =  dmd -> md_linebytes >> 1;
/* 		    *a5_adr |= d3_skew; */
		    d5_count = d7_dx;
		    do 
		    {
			if (d4_initerror < 0)
			{	/* move in y */
			    a5_adr += *a4_vert;
			    d4_initerror += d5_count;
			}
			else
			{	/* move in x */
			    *a5_adr |= d3_skew;
			    d4_initerror -= d6_dy;
			    if (d3_skew & 1)
			    {
				a5_adr++;
				d3_skew = 0x8000;
			    }
			    else
			    {
				d3_skew >>= 1;
			    }
			    d7_dx--;
			}
		    }
		    while (d7_dx != -1);
		    break;
	    }
	}
	else
	{			/* compile and run depth8 code */
	    register u_char *col_adr;
	    col_adr = mprd8_addr (dmd, start.x, start.y, dst -> pr_depth);
	    switch (op)
	    {
		case 0: 
		    d4_initerror = d7_dx / 2;
		    if (reflect & 2)
			*a4_vert =  -dmd -> md_linebytes;
		    else
			*a4_vert =  dmd -> md_linebytes;
/* 		    *col_adr &= ~color; */
		    d5_count = d7_dx;
		    do
		    {
			if (d4_initerror < 0)
			{	/* move in y */
			    col_adr += *a4_vert;
			    d4_initerror += d5_count;
			}
			else
			{	/* move in x */
/* 			    *col_adr &= ~color; */
			    *col_adr = 0;
			    d4_initerror -= d6_dy;
				col_adr++;
			    d7_dx--;
			}
		    }
		    while (d7_dx != -1);
		    break;
		case 1: 
		    d4_initerror = d7_dx / 2;
		    if (reflect & 2)
			*a4_vert =  -dmd -> md_linebytes;
		    else
			*a4_vert =  dmd -> md_linebytes;
/* 		    *col_adr ^= color; */
		    d5_count = d7_dx;
		    do
		    {
			if (d4_initerror < 0)
			{	/* move in y */
			    col_adr += *a4_vert;
			    d4_initerror += d5_count;
			}
			else
			{	/* move in x */
			    *col_adr ^= color;
			    d4_initerror -= d6_dy;
				col_adr++;
			    d7_dx--;
			}
		    }
		    while (d7_dx != -1);
		    break;
		case 2: 
		    d4_initerror = d7_dx / 2;
		    if (reflect & 2)
			*a4_vert =  -dmd -> md_linebytes;
		    else
			*a4_vert =  dmd -> md_linebytes;
		    *col_adr = color;
		    d5_count = d7_dx;
		    do
		    {
			if (d4_initerror < 0)
			{	/* move in y */
			    col_adr += *a4_vert;
			    d4_initerror += d5_count;
			}
			else
			{	/* move in x */
			    *col_adr = color;
			    d4_initerror -= d6_dy;
				col_adr++;
			    d7_dx--;
			}
		    }
		    while (d7_dx != -1);
		    break;
	    }
	}
    }
    return (0);
}

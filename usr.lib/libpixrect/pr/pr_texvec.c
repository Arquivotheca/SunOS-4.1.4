#ifndef lint
static char sccsid[] = "@(#)pr_texvec.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1990 Sun Microsystems, Inc.
 */
 

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_line.h>
#include "pr_texmacs.h"

/* largest delta we can handle without overflow in clip code */
#define	MAXDELTA	(32767)

#define swap(a,b,t)	do { (t) = (a); (a) = (b); (b) = (t); } while (0)
#define abs(i)		((i) < 0 ? -(i) : (i))

/* XXX mallocated segment array, never freed */
#define MaxSegs         256
static short *segarray;

pr_texvec(dst, x0, y0, x1, y1, tex, op)
    Pixrect *dst;
    int x0, y0, x1, y1;
    Pr_texture *tex;
    int op;
{
    register dx, dy, count, initerror;
    int clip, tempos, start_x, start_y, vert, npts;
    int reflect = 0, clip_off = 0;
    static unsigned ptlist_size;
    struct pr_size 		size;
    static struct pr_pos 	*ptlist;

    if (!segarray &&
        !(segarray = (short *) malloc(MaxSegs * sizeof (short))))
        return PIX_ERR;
    
    size = dst->pr_size;
	
    tex->options.res_right = 1;
    tex->options.res_cliprt = 0;
    dx = x1 - x0;
    dy = y1 - y0;
    vert = ((dx > 0)^(dy > 0)) ? -1 : 1;
 
    /*
     * Need to normalize the vector so that the following
     * algorithm can limit the number of cases to be considered.
     * We can always interchange the points in x, so that
     * pos0 is to the left of pos1.
     */
    if (dx < 0) {
	/* force vector to scan to right */
	dx = -dx;
	dy = -dy;
	swap(x0, x1, tempos);
	swap(y0, y1, tempos);
	tex->options.res_right = 0;
    }

    /*
     * The clipping routine needs to work with a vector which
     * increases in y from y0 to y1.  We want to change
     * y0 and y1 so that there is an increase in y
     * from y0 to y1 without affecting the clipping
     * and bounds checking that we will do.  We accomplish
     * this by reflecting the vector around the horizontal
     * centerline of dst, and remember that we did this by
     * incrementing reflect by 2.  The reflection will be undone
     * before the vector is drawn.
     */
    if (dy < 0) {
	dy = -dy;
	y0 = (size.y-1) - y0;
	y1 = (size.y-1) - y1;
	if (!dx) 
	    tex->options.res_right = 0;
	reflect += 2;
    }
    /*
     * Can now do bounds check, since the vector increasing in
     * x and y can check easily if it has no chance of intersecting
     * the destination rectangle: if the vector ends before the
     * beginning of the target or begins after the end!
     */
    if (op & PIX_DONTCLIP)
    	clip = 0;
    else {
	if (y1 < 0 || y0 >= size.y || x1 < 0 || x0 >= size.x) {
	    if (dx < dy) 
	    	swap(dx, dy, count);

	    clip_offset(dx, dy, tex);

	    return 0;
	}
	clip = 1;
	op |= PIX_DONTCLIP;
    }

    /*
     * If vector is horizontal, use fast algorithm.
     */
    if (dy == 0) {
	if (clip) {
	    if (x0 < 0) {
		clip_off = -x0;
		x0 = 0;
	    }
	    if (x1 >= size.x) {
		x1 = size.x - 1;
		tex->options.res_cliprt = 1;
	    }
	}
	count = x1 - x0 + 1;

	MALLOC_PTLIST(ptlist, ptlist_size, count); 

	npts = bres_horizontal(count, dx, x0, y0, 
	    clip_off, tex, ptlist);
	return pr_polypoint(dst, 0, 0, npts, ptlist, op);
    }

    /*
     * If vector is vertical, use fast algorithm.
     */
    if (dx == 0) {
	if (clip) {
	    if (y0 < 0) {
		if (tex->options.res_right)
		    clip_off = -y0;
		else 
		    tex->options.res_cliprt = 1;
		y0 = 0;
	    }
	    if (y1 >= size.y) {
		if (!(tex->options.res_right))
		    clip_off += y1 - size.y + 1;
		else
		    tex->options.res_cliprt = 1;
		y1 = size.y - 1;
	    }
	}
	count = y1 - y0 + 1;

	MALLOC_PTLIST(ptlist, ptlist_size, count); 
		   	
	if (reflect & 2) 	/* undo reflection, startpoint only */
	    y0 = size.y - 1 - y1;
	npts = bres_vert(count, dy, 1, x0, y0, 
	    clip_off, tex, ptlist);
	return pr_polypoint(dst, 0, 0, npts, ptlist, op);
    }

    /*
     * One more reflection: we want to assume that dx >= dy.
     * So if this is not true, we reflect the vector around
     * the diagonal line x = y and remember that we did
     * this by adding 1 to reflect.
     */
    if (dx < dy) {
	swap(x0, y0, count);
	swap(x1, y1, count);
	swap(dx, dy, count);
	swap(size.x, size.y, count);
	reflect += 1;
    }
    initerror = -(dx>>1);		/* error at pos0 */
    start_x = x0;
    start_y = y0;
    if (!clip) 
	goto clipdone;

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
 	 * If dx or dy are too large, scale to avoid overflow in
	 * multiplications below.
 	 */
	while (dx > MAXDELTA || dy > MAXDELTA) {
		dx >>= 1;
		dy >>= 1;
	}

    /*
     * Clip left end to yield start of visible vector.
     * If the starting x coordinate is negative, we
     * must advance to the first x coordinate which will
     * be drawn (x=0).  As we advance (-start_x) units in the
     * x direction, the error increases by (-start_x)*dy.
     * This means that the error when x=0 will be
     *	-(dx/2)+(-start_x)*dy
     * For each y advance in this range, the error is reduced
     * by dx, and should be in the range (-dx,0] at the y
     * value at x=0.  Thus to compute the increment in y we
     * should take
     *	(-(dx/2)+(-start_x)*dy+(dx-1))/dx
     * where the numerator represents the length of the interval
     *	[-dx+1,-(dx/2)+(-start_x)*dy]
     * The number of dx steps by which the error can be reduced
     * and stay in this interval is the y increment which would
     * result if the vector were drawn over this interval.
     */
    if (start_x < 0) {
	initerror += (-start_x * dy);
	clip_off = -start_x;
	start_x = 0;
	count = (initerror + (dx-1)) / dx;
	start_y += count;
	initerror -= (count * dx);
    }

    /*
     * After having advanced x to be at least 0, advance
     * y to be in range.  If y is already too large (and can
     * only get larger!), just give up.  Otherwise, if start_y < 0,
     * we need to compute the value of x at which y is first 0.
     * In advancing y to be zero, the error decreases by (-start_y)*dx,
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
    if (start_y >= size.y) {
	clip_offset(dx, dy, tex);
	return (0);
    }
    if (start_y < 0) {
	/* skip to dst top edge */
	initerror += (start_y * dx);
	start_y = 0;
	count = ((-dx+dy)-initerror)/dy;
	start_x += count;
	clip_off += count;
	initerror += (count * dy);
	if (start_x >= size.x) {
	    clip_offset(dx, dy, tex);
	    return (0);
	}
    }

    /*
     * Now clip right end.
     *
     * If the last x position is outside the rectangle,
     * then clip the vector back to within the rectangle
     * at x=size.x-1.  The corresponding y value has initerror
     *	-(dx/2)+((size.x-1)-x0)*dy
     * We need an error in the range (-dx,0], so compute
     * the corresponding number of steps in y from the number
     * of dy's in the interval:
     *	(-dx,-(dx/2)+((size.x-1)-x0)*dy]
     * which is:
     *	[-dx+1,-(dx/2)+((size.x-1)-x0)*dy]
     * or:
     *	(-(dx/2)+((size.x-1)-x0)*dy+dx-1)/dx
     * Note that:
     *	dx - (dx/2) != (dx/2)
     * (consider dx odd), so we can't simplify much further.
     */
    if (x1 >= size.x) {
	x1 = size.x - 1;
	y1 = y0+ (-(dx>>1)+ (((size.x-1)-x0) * dy)+ dx-1)/dx;
	tex->options.res_cliprt = 1;
    } 
    /*
     * If the last y position is outside the rectangle, then
     * clip it back at y=size.y-1.  The corresponding x value
     * has error
     *	-(dx/2)-((size.y-1)-y0)*dx
     * We want the last x value with this y value, which
     * will have an error in the range (-dy,0] (so that increasing
     * x one more time will make the error > 0.)  Thus
     * the amount of error allocatable to dy steps is from
     * the length of the interval:
     *	[-(dx/2)-((size.y-1)-y0)*dx,0]
     * that is,
     *	(0-(-(dx/2)-((size.y-1)-y0)*dx))/dy
     * or:
     *	((dx/2)+((size.y-1)-y0)*dx)/dy
     */
    if (y1 >= size.y) {
	y1 = size.y - 1;
	x1 = x0 + ((dx>>1) + (((size.y-1)-y0) * dx))/dy;
	tex->options.res_cliprt = 1;
    }

clipdone:
    /*
     * Now have to set up for the Bresenham routines.
     */

    count = x1 - start_x + 1;
	
    MALLOC_PTLIST(ptlist, ptlist_size, count); 
		   	
    if (reflect & 1) {
	/* major axis is y */
	if (reflect & 2) {
	    /* unreflect major axis (misnamed x) */
	    start_x = ((size.x-1) - start_x);
	}
	npts = bres_majy(dx, dy, count, initerror, vert, 
	    start_y, start_x, clip_off, tex, ptlist);
    } else {
	/* major axis is x */
	if (reflect & 2) {
	    /* unreflect minor axis */
		start_y = ((size.y-1) - start_y);
	    }
	npts = bres_majx(dx, dy, count, initerror, vert, 
	    start_x, start_y, clip_off, tex, ptlist);
    }

    return pr_polypoint(dst, 0, 0, npts, ptlist, op);
}


/* 
 * This routine calculates and sets up the offset for vectors.  If the
 * vectors were meant to be drawn left to right, or if the computed offset
 * is equal to the pattern length, we set the offset to the length of
 * the first segment of the pattern and return.  If the vector was meant
 * to be drawn from left to right, we calculate an offset into the pattern
 * so that we would get concentric circles when drawing multiple vectors 
 * from the origin.  Seg holds the number of the segment that we are in, 
 * and offset contains an offset into that segment.
 */

static
pattern_offset (segment,numsegs,seg,offset,majax,pat_length,start_off,
	clip_off,tex)
    int clip_off;
    register short *segment;
    int numsegs;
    register majax, pat_length, start_off;
    register int *offset, *seg;
    Pr_texture 		*tex;
{
    int i, mod, init_off;
/* 
 * First we handle the code for balanced lines, and the different starting 
 * offsets for the left and right sides.  For line balancing, we take
 * the modulus of the count and pattern length and divide this by 2 so 
 * that the offset is the same from either end.  But, since the pattern is 
 * even (has an even number of segments) we cannot truly center it, so we
 * add in the length of the first segment divided by 2.
 */ 
    init_off = start_off;
    mod = ((majax + 1) % pat_length) + start_off;
    if (!(tex->options.res_right)) {		/* for left side */
	tex->res_polyoff = mod + 1;
	start_off = clip_off + pat_length - mod;
	if (tex->options.balanced)
	    start_off += ((mod-init_off)>>1) - (segment[numsegs-1] >> 1);
	while (start_off < 0)
	    start_off += pat_length;
    } else {
	tex->res_polyoff = mod - 1; 
	start_off += clip_off;			/* for right side */
	if (tex->options.balanced)
	    start_off += pat_length - ((mod-init_off)>>1) + (*segment>>1);
    }
    while (tex->res_polyoff >= pat_length)
	tex->res_polyoff -= pat_length;
    while (start_off >= pat_length)
	start_off -= pat_length;
    if (start_off == 0) {			/* no offset necessary */
	*offset = *segment;
	*seg = 0;
	return;
    }
/*
 * When we reach here we know that the starting offset is less than the total
 * pattern length, so we want to figure out where in the patttern we should 
 * start.  Offset holds the count into a segment, and seg holds the segment
 * number.
 */
    for (i = 0; (*offset = *segment++); i++) {
	if (start_off <= *offset) {
	    *offset -= start_off;
	    break;
	} else
	    start_off -= *offset;
    }
    if (*offset)				/* offset is not zero */
	*seg = i;
    else {					/* offset is zero */
	*offset = *segment;
	*seg = ++i;
    }
} /* pattern_offset */

	    
static
IncSetPattern(majax,minax,segment,pat_length,numsegs,start_off,tex)
    register majax, minax;
    register int *pat_length, *start_off;
    register short *segment;
    register Pr_texture		*tex;
    int numsegs;
/* 
 * This routine computes the actual pattern length for non-horizontal
 * and non-vertical vectors, using pythagoreans theorem.  When the error
 * for the current point is greater than the error for the last point, the
 * routine falls out of its loop.  For the case when the major axis is
 * the x axis, then x squared begins as 1, and after that, the
 * algorithm follows the formula (x + 1)**2 = x**2 + 2x + 1.
 */
{
    int diag_sq, maj_sq = 1, min_sq = 1, maj_count = 0, min_count = 0; 
    int not_begin = 0, maj_count_old = 0, i, seg_count = 0;
    register error, seg_error = 0, old_error;
     
    error = -(majax >> 1);		/* done for clipping reasons */
    for (i = 0; i < numsegs; i++) {
    	seg_count += *segment;
    	diag_sq = pr_product(seg_count, seg_count);
	old_error = diag_sq - (maj_sq + min_sq);
	while ((abs(seg_error)) <= (abs(old_error))) { 
	    maj_count++;
	    maj_sq += maj_count + maj_count + 1;
	    error += minax;
	    if (error > 0) {
		min_count++;
		min_sq += min_count + min_count + 1;
		error -= majax;
	    }
	    if (not_begin)
		old_error = seg_error;
	    seg_error = diag_sq - (maj_sq + min_sq);
	    not_begin = 1;
   	} 
	*segment++ = maj_count - maj_count_old;
	maj_count_old = maj_count;
	not_begin = seg_error = 0;
    }
    *pat_length = maj_count;
    *start_off = tex->res_fatoff = ((*start_off) * (*pat_length) +
	(tex->res_oldpatln>>1)) / tex->res_oldpatln;
} /* IncSetPattern */
	    

static
clip_offset(majax, minax, tex)
    register majax, minax;
    register Pr_texture		*tex;
{     
    int start_off, mod, pat_length, i, j;
    int numsegs;
/*
 * Fill in the segment array and compute the pattern_length for vectors
 * on both the left and right halves, since it could be used for balancing
 * or for offsets into the pattern for vectors on the left half.  It isn't
 * worth the time to do the tests that would be needed to determine if 
 * absolutely necessary.
 */
    if (!tex->options.res_fat) {	/* is NOT fat vector */
	{				
	    register short *s, *p;
	
	    pat_length = numsegs = 0;
	    for (s = segarray, p = tex->pattern; (*p); numsegs++) {
		*s++ = *p;
		pat_length += *p++;
	    }
	} /* end block */
	if (tex->options.res_poly) {
	    start_off = tex->res_fatoff = tex->res_polyoff;
	} else {
	    start_off = tex->res_fatoff = tex->offset;
	    tex->res_oldpatln = pat_length;
	}
/* 
 * Angle correct if not horizontal or vertical & givenpattern bit is clear.
 */
	if ((!tex->options.givenpattern) && (majax && minax)) {
	    IncSetPattern(majax, minax, segarray, &pat_length, numsegs, 
		&start_off, tex);
	}
	if (!tex->options.res_right) {	/* reverse pattern in array */
	    REVERSE_PATTERN(segarray, ptlist, numsegs, i, j);
	}
/*
 * For fat vectors who are initially totally outside the clipping region,
 * and then enter the drawing window, we store a pointer to the angle
 * corrected pattern, in addition to storing the number of segments in
 * the pattern.
 */
	tex->res_patfat = segarray;
	tex->res_numsegs = numsegs;
    } else {				/* is fat vector */
	start_off = tex->res_fatoff;
	pat_length = tex->res_oldpatln;
    }
/* 
 * Now we compute the necessary offset parameters, with code lifted from the
 * pattern_offset routine
 */    
    mod = ((majax + 1) % pat_length) + start_off;
    if (tex->options.res_right)		/* for right side */
	tex->res_polyoff = mod - 1;	
    else 				/* for left side */
	tex->res_polyoff = mod + 1;
    while (tex->res_polyoff >= pat_length)
	tex->res_polyoff -= pat_length;
    tex->res_oldpatln = pat_length;
} /* clip_offset */    

#define nxt(k,np) ((k+1)%np)
#define prev(k,np) ((k-1+np)%np)
#define even(n) (!(n & 1))
#define isgap(n) (n & 1)
#define min(m,n) ((m)<(n)? (m): (n))

static int hl, hr, hrsave;
static int maxdash, mindash, maxgap, mingap;

extern void bal_dash();
extern void fil_dash();
extern void clip_offset1();

static int
bres_horizontal(count,majax,x,y,clip_off,tex,ptlist)
register count, majax;
register Pr_texture   *tex;
register struct pr_pos  *ptlist;
int clip_off, x, y; 
{
  int i, j, start_off, offset, seg, npts = 0, pat_length;
  int numsegs;
  register short *segment, *segptr;
  short auxp[20];
  short lauxp[20];
  int auxnp;
  int lauxnp;
  int bestc;
  int lcount, mcount, rcount;

/*
 * Fill in the p array and compute the pattern_length for vectors
 * on both the left and right halves, since it could be used for balancing
 * or for offsets into the pattern for vectors on the left half.  It isn't
 * worth the time to do the tests that would be needed to determine if 
 * absolutely necessary.
 */
  
  if (!tex->options.res_fat) {  /* is NOT fat vector */
    {       
      register short *_s, *_p;
    
      pat_length = numsegs = 0;
      for (_s = segarray, _p = tex->pattern; (*_p); numsegs++) {
      *_s++ = *_p;
      pat_length += *_p++;
      }
    } /* end block */

    /* reverse pattern in array */
    if (!tex->options.res_right && !tex->options.balanced) { 
      REVERSE_PATTERN(segarray, ptlist, numsegs, i, j);
    }
    if (tex->options.res_poly) 
      start_off = tex->res_fatoff = tex->res_polyoff;
    else {
      start_off = tex->res_fatoff = tex->offset;
      tex->res_oldpatln = pat_length;
    }
    segment = tex->res_patfat = segarray;
    tex->res_numsegs = numsegs;
  } 
  else { /* is fat vector */
    start_off = tex->res_fatoff;
    pat_length = tex->res_oldpatln; 
    numsegs = tex->res_numsegs; 
    segment = tex->res_patfat;
  }
/*
 * Account for the case when you have a single segment in your pattern.
 * Line will appear solid.
 */ 
  if (numsegs == 1)
    tex->options.res_right = 1;

  if (tex->options.balanced) {
    bal_dash(majax+1,numsegs,segment,&bestc,&auxnp,auxp,
      &seg,&offset,&lcount,&rcount);
    mcount = majax+1-lcount-rcount;
  }
  else {
    pattern_offset(segment,numsegs,&seg,&offset,majax,pat_length,
      start_off,clip_off,tex);
    bestc = -1;
  }
  segptr = &segment[seg+1];	     
  tex->res_oldpatln = pat_length;


/* 
 * Modified bresenham for horizontals. 
 */
  if (tex->options.balanced) {
    short* lsegptr;
    int loffset;
    int lseg;
    int rseg;
    short* rsegptr;
    int roffset;

    for (i=0; i<auxnp; i++) {
      lauxp[i] = auxp[i];
    }
    lauxnp = auxnp;

    if (rcount > 0 && lcount > rcount) {
      (lauxp[0])++;
      if (lauxp[0] > 1.6 * maxdash && lauxp[0] < 4) { /* ugly */
        if (lauxnp == 1) { 
          if (seg & 1) { /* first regular portion is gap */
            lauxp[0]--;
            lauxnp = 2;
            lauxp[1] = 1;
          }
        }
        else {
          lauxp[0]--;
          lauxp[1]++;
        }
      }
    }

    /* determine clip_off */
    lseg = 0;
    loffset = lauxp[lseg];
    lsegptr = &lauxp[(lseg+1)%lauxnp];
    rseg = auxnp-1;
    roffset = auxp[rseg];
    rsegptr = &auxp[(rseg-1+auxnp)%auxnp];

    (void) clip_offset1(clip_off,majax,count,numsegs,segment,
      lauxnp,lauxp,auxnp,auxp,&lcount,&mcount,&rcount,
      &lseg,&loffset,&seg,&offset,&rseg,&roffset);

    lsegptr = &lauxp[(lseg+1)%lauxnp];
    segptr = &segment[(seg+1)%numsegs];
    rsegptr = &auxp[(rseg-1+auxnp)%auxnp];

    /* test point */
        
    while (lcount > 0) {
      for ( ; ((lcount > 0) && (loffset > 0)); --lcount, --loffset) {
        if (!(lseg & 1))
        FILL_PR_POS(npts, ptlist, x, y);
        x++;
      }
      if (lcount == 0) break;
      lseg++;
      if (lseg >= lauxnp) {
        lseg = 0;  
        lsegptr = lauxp;
      }
      loffset = *lsegptr++;
    }

    while (mcount > 0) {
      for ( ; ((mcount > 0) && (offset > 0)); --mcount, --offset) {
        if (!(seg & 1))
        FILL_PR_POS(npts, ptlist, x, y);
        x++;
      }
      seg++;
      if (seg >= numsegs) {
        seg = 0;  
        segptr = segment;
      }
      offset = *segptr++;
    }

    while (rcount > 0) {
      for ( ; ((rcount > 0) && (roffset > 0)); --rcount, --roffset) {
        if (!(rseg & 1))
          FILL_PR_POS(npts, ptlist, x, y);
        x++;
      }
      if (rcount == 0) break;
      rseg--;
      if (rseg < 0) {
        rseg = auxnp-1;  
        rsegptr = &auxp[auxnp-1];
      }
      roffset = *rsegptr--;
    }
    return npts;
  }
  if (tex->options.res_right) { /* right half */
    if ((tex->options.startpoint) && (!clip_off)) {
      FILL_PR_POS(npts, ptlist, x, y);
      DRAW_HORZ();
    }
    if (count == 0)
      return npts;
    if ((tex->options.endpoint) && (!tex->options.res_cliprt))
      --count;
    while (count > 0) {
      for ( ; ((count > 0) && (offset > 0)); --count, --offset) {
        if (!(seg & 1))
        FILL_PR_POS(npts, ptlist, x, y);
        x++;
      }
      seg++;
      if (seg >= numsegs) {
        seg= 0;   
        segptr = segment;
      }
      offset = *segptr++;
    }
    if ((tex->options.endpoint) && (!tex->options.res_cliprt))
      FILL_PR_POS(npts, ptlist, x, y);
  } 
  else {        /* left half */
    if ((tex->options.endpoint) && (!clip_off)) {
      FILL_PR_POS(npts, ptlist, x, y);
      DRAW_HORZ();
    }
    if ((tex->options.startpoint) && (!tex->options.res_cliprt))
      --count;
    while (count > 0) {
      for ( ; ((count > 0) && (offset > 0)); --count, --offset) {
        if (seg & 1)
        FILL_PR_POS(npts, ptlist, x, y);
        x++;
      }
      seg++;
      if (seg >= numsegs) {
        seg= 0;   
        segptr = segment;
      }
      offset = *segptr++;
    }
    if ((tex->options.startpoint) && (!tex->options.res_cliprt))
      FILL_PR_POS(npts, ptlist, x, y);
  }
  return(npts);
} /* bres_horizontal */


static int
bres_vert (count,majax,vert,x,y,clip_off,tex,ptlist)
register count, majax, vert;
register Pr_texture		*tex;
register struct pr_pos	*ptlist;
int clip_off, x, y; 
{
  int i, j, start_off, offset, seg, npts = 0, pat_length;
  int numsegs;
  register short *segment, *segptr;
  short auxp[20];
  short lauxp[20];
  int auxnp;
  int lauxnp;
  int bestc;
  int lcount, mcount, rcount;

/*
 * Fill in the segment array and compute the pattern_length for vectors
 * on both the left and right halves, since it could be used for balancing
 * or for offsets into the pattern for vectors on the left half.  It isn't
 * worth the time to do the tests that would be needed to determine if 
 * absolutely necessary.
 */
  if (!tex->options.res_fat) {	/* is NOT fat vector */		
    {
      register short *s, *p;
      
      pat_length = numsegs = 0;
      for (s = segarray, p = tex->pattern; (*p); numsegs++) {
        *s++ = *p;
        pat_length += *p++;
      }
    } /* end block */
    if (tex->options.res_poly)
      start_off = tex->res_fatoff = tex->res_polyoff;
    else {
      start_off = tex->res_fatoff = tex->offset;
      tex->res_oldpatln = pat_length;
    }

    /* reverse pattern in array */
    if (!tex->options.res_right && !tex->options.balanced) {
      REVERSE_PATTERN(segarray, ptlist, numsegs, i, j);
    }
    segment = tex->res_patfat = segarray;
    tex->res_numsegs = numsegs;
  } 
  else { /* is fat vector */
    start_off = tex->res_fatoff;
    pat_length = tex->res_oldpatln;	
    numsegs = tex->res_numsegs;	
    segment = tex->res_patfat;
  }
/*
 * Account for the case when you have a single segment in your pattern.
 * Line will appear solid.
 */	
  if (numsegs == 1)
      tex->options.res_right = 1;
  
  if (tex->options.balanced) {
    bal_dash(majax+1,numsegs,segment,&bestc,&auxnp,auxp,
      &seg,&offset,&lcount,&rcount);
    mcount = majax+1-lcount-rcount;
  }
  else {
    pattern_offset(segment,numsegs,&seg,&offset,majax,pat_length,
      start_off, clip_off, tex);
    bestc = -1;
  }
  segptr = &segment[seg+1];
  tex->res_oldpatln = pat_length;
/*
 * Modified bresenham for verticals.
 */

  if (tex->options.balanced) {
    short* lsegptr;
    int loffset;
    int lseg;
    int rseg;
    short* rsegptr;
    int roffset;

    for (i=0; i<auxnp; i++) {
      lauxp[i] = auxp[i];
    }
    lauxnp = auxnp;
    loffset = lauxp[0];

    if (rcount > 0 && lcount > rcount) {
      (lauxp[0])++;

      if (lauxp[0] > 1.6 * maxdash && lauxp[0] < 4) {
        if (lauxnp == 1) { 
          if (seg & 1) { /* first reg is gap */
            lauxp[0]--;
            lauxnp = 2;
            lauxp[1] = 1;
          }
        }
        else {
          lauxp[0]--;
          lauxp[1]++;
        }
      }
    }
    
    
    /* determine clip_off */
    lseg = 0;
    loffset = lauxp[lseg];
    lsegptr = &lauxp[(lseg+1)%lauxnp];
    rseg = auxnp-1;
    roffset = auxp[rseg];
    rsegptr = &auxp[(rseg-1+auxnp)%auxnp];

    (void) clip_offset1(clip_off,majax,count,numsegs,segment,
      lauxnp,lauxp,auxnp,auxp,&lcount,&mcount,&rcount,
      &lseg,&loffset,&seg,&offset,&rseg,&roffset);

    lsegptr = &lauxp[(lseg+1)%lauxnp];
    segptr = &segment[(seg+1)%numsegs];
    rsegptr = &auxp[(rseg-1+auxnp)%auxnp];


    while (lcount > 0) {
      for ( ; ((lcount > 0) && (loffset > 0)); --lcount, --loffset) {
        if (!(lseg & 1))
          FILL_PR_POS(npts, ptlist, x, y);
        y += vert;
      }
      if (lcount == 0) break;
      lseg++;
      if (lseg >= lauxnp) {
        lseg = 0;  
        lsegptr = lauxp;
      }
      loffset = *lsegptr++;
    }

    while (mcount > 0) {
      for ( ; ((mcount > 0) && (offset > 0)); --mcount, --offset) {
        if (!(seg & 1))
          FILL_PR_POS(npts, ptlist, x, y);
        y += vert;
      }
      seg++;
      if (seg >= numsegs) {
        seg = 0;  
        segptr = segment;
      }
      offset = *segptr++;
    }

    while (rcount > 0) {
      for ( ; ((rcount > 0) && (roffset > 0)); --rcount, --roffset) {
        if (!(rseg & 1))
          FILL_PR_POS(npts, ptlist, x, y);
        y += vert;
      }
      if (rcount == 0) break;
      rseg--;
      if (rseg < 0) {
        rseg = auxnp-1;  
        rsegptr = &auxp[auxnp-1];
      }
      roffset = *rsegptr--;
    }
    return npts;
  }

  if (tex->options.res_right) {
    if ((tex->options.startpoint) && (!clip_off)) { 
      FILL_PR_POS(npts, ptlist, x, y);
      DRAW_VERT();
    }
    if ((tex->options.endpoint) && (!tex->options.res_cliprt))	
      --count;	
    while (count > 0) {
      for ( ; ((count > 0) && (offset > 0)); --count, --offset) {
      if (!(seg & 1))
        FILL_PR_POS(npts, ptlist, x, y);
      y += vert;
      }
      seg++;
      if (seg >= numsegs) {
        seg = 0;		
        segptr = segment;
      }
      offset = *segptr++;
    } /* end bresenham */
    if ((tex->options.endpoint) && (!tex->options.res_cliprt))
      FILL_PR_POS(npts, ptlist, x, y);
  } 
  else {
    if ((tex->options.endpoint) && (!clip_off)) {
        FILL_PR_POS(npts, ptlist, x, y); 
        DRAW_VERT();
    }
    if ((tex->options.startpoint) && (!tex->options.res_cliprt))
        --count;
    while (count > 0) {
      for ( ; ((count > 0) && (offset > 0)); --count, --offset) {
      if (seg & 1)
        FILL_PR_POS(npts, ptlist, x, y);
      y += vert;
      }
      seg++;
      if (seg >= numsegs) {
        seg = 0;
        segptr = segment;		
      }
      offset = *segptr++;
    } 	/* end bresenham */
    if ((tex->options.startpoint) && (!tex->options.res_cliprt))
      FILL_PR_POS(npts, ptlist, x, y);
  }
  return(npts);
} /* bres_vert */


static int
bres_majx(majax,minax,count,error,vert,x,y,clip_off,tex,ptlist)
register majax, minax, count, error, vert;
register Pr_texture   *tex;
register struct pr_pos  *ptlist;
int clip_off, x, y;
{
  int i, j, start_off, offset, seg, npts = 0, pat_length;
  int numsegs;
  register short *segment, *segptr;
  short auxp[20];
  short lauxp[20];
  int auxnp; 
  int lauxnp;
  int bestc;
  int lcount, mcount, rcount;


  if (!tex->options.res_fat) { /* is NOT fat vector */
    { 
      register short *s, *p;
  
      numsegs = pat_length = 0;
      for (s = segarray, p = tex->pattern; (*p); numsegs++) {
        *s++ = *p;
        pat_length += *p++;
      }
    } /* end block */
  
    if (tex->options.res_poly)
      start_off = tex->res_fatoff = tex->res_polyoff;
    else {
      start_off = tex->res_fatoff = tex->offset;
      tex->res_oldpatln = pat_length;
    }
    if (!tex->options.givenpattern) {
      IncSetPattern(majax, minax, segarray, &pat_length, numsegs, 
          &start_off, tex);
    }
    /* 
   * Now we reverse the pattern if we were supposed to draw from right to
   * left.  
   */
    if (!tex->options.res_right && !tex->options.balanced) {
      REVERSE_PATTERN(segarray, ptlist, numsegs, i, j);
    }
    segment = tex->res_patfat = segarray;
    tex->res_numsegs = numsegs;
  }
  else {        /* is fat vector */
    /*
   * Restore angle-corrected offset, since don't need to continually angle 
   * correct for a fat vector & restore patln in case this is the first 
   * visible vector of the fat vector.
   */
    start_off = tex->res_fatoff;
    pat_length = tex->res_oldpatln;
    numsegs = tex->res_numsegs;
    segment = tex->res_patfat;
  }
  
  /*
   * Account for the case when you have a single segment in your pattern.
   * Line will appear solid.
   */
  if (numsegs == 1)
  tex->options.res_right = 1;
  
  if (tex->options.balanced) {
    bal_dash(majax+1,numsegs,segment,&bestc,&auxnp,auxp,
      &seg,&offset,&lcount,&rcount);
    mcount = majax+1-lcount-rcount;
  }
  else {
    pattern_offset(segment, numsegs, &seg, &offset, majax, pat_length, 
      start_off, clip_off, tex);
    bestc = -1;
  }
  segptr = &segment[seg+1];
  tex->res_oldpatln = pat_length;
  /* 
   * Modified bresenham loop, where we account for the fact that if we were
   * meant to go right to left, we are drawing the pattern backwards, 
   * therefore we draw the even segments for the right half, and the odd
   * segments for the left half.
   */
  if (tex->options.balanced) {
    short* lsegptr;
    int loffset;
    int lseg;
    int rseg;
    short* rsegptr;
    int roffset;

    for (i=0; i<auxnp; i++) {
      lauxp[i] = auxp[i];
    }
    lauxnp = auxnp;
    loffset = lauxp[0];

    if (rcount > 0 && lcount > rcount) {
      (lauxp[0])++;

      if (lauxp[0] > 1.6 * maxdash && lauxp[0] < 4) {
        if (lauxnp == 1) { 
          if (seg & 1) { /* first reg is gap */
            lauxp[0]--;
            lauxnp = 2;
            lauxp[1] = 1;
          }
        }
        else {
          lauxp[0]--;
          lauxp[1]++;
        }
      }
    }
    
    /* determine clip_off */
    lseg = 0;
    loffset = lauxp[lseg];
    lsegptr = &lauxp[(lseg+1)%lauxnp];
    rseg = auxnp-1;
    roffset = auxp[rseg];
    rsegptr = &auxp[(rseg-1+auxnp)%auxnp];

    (void) clip_offset1(clip_off,majax,count,numsegs,segment,
      lauxnp,lauxp,auxnp,auxp,&lcount,&mcount,&rcount,
      &lseg,&loffset,&seg,&offset,&rseg,&roffset);

    lsegptr = &lauxp[(lseg+1)%lauxnp];
    segptr = &segment[(seg+1)%numsegs];
    rsegptr = &auxp[(rseg-1+auxnp)%auxnp];


    while (lcount > 0) {
      for ( ; ((lcount > 0) && (loffset > 0)); --lcount, --loffset) {
        if (!(lseg & 1))
        FILL_PR_POS(npts, ptlist, x, y);
        x++;
        error += minax;
        if (error > 0) {
          error -= majax;
          y += vert;
        }
      }
      if (lcount == 0) break;
      lseg++;
      if (lseg >= lauxnp) {
        lseg = 0;  
        lsegptr = lauxp;
      }
      loffset = *lsegptr++;
    }

    while (mcount > 0) {
      for ( ; ((mcount > 0) && (offset > 0)); --mcount, --offset) {
        if (!(seg & 1))
        FILL_PR_POS(npts, ptlist, x, y);
        x++;
        error += minax;
        if (error > 0) {
          error -= majax;
          y += vert;
        }
      }
      seg++;
      if (seg >= numsegs) {
        seg = 0;  
        segptr = segment;
      }
      offset = *segptr++;
    }

    while (rcount > 0) {
      for ( ; ((rcount > 0) && (roffset > 0)); --rcount, --roffset) {
        if (!(rseg & 1))
          FILL_PR_POS(npts, ptlist, x, y);
        x++;
        error += minax;
        if (error > 0) {
          error -= majax;
          y += vert;
        }
      }
      if (rcount == 0) break;
      rseg--;
      if (rseg < 0) {
        rseg = auxnp-1;  
        rsegptr = &auxp[auxnp-1];
      }
      roffset = *rsegptr--;
    }
    return npts;
  }

  if (tex->options.res_right) { /* right half */
    if ((tex->options.startpoint) && (!clip_off)) {
      FILL_PR_POS(npts, ptlist, x, y);
      DRAW_MAJ_X();
    }
    if ((tex->options.endpoint) && (!tex->options.res_cliprt))
      --count;
    while (count > 0) {   /* for each segment. */
      for (; ((offset > 0) && (count > 0)); count--, offset--) {
        if (!(seg & 1))
          FILL_PR_POS(npts, ptlist, x, y);
        x++;
        error += minax;
        if (error > 0) {
          error -= majax;
          y += vert;
        }
      }         /* for each pixel in segment. */
      seg++;
      if (seg >= numsegs) {
        seg = 0;
        segptr = segment;
      }
      offset = *segptr++;
    }
    if ((tex->options.endpoint) && (!tex->options.res_cliprt))
      FILL_PR_POS(npts, ptlist, x, y);
  }
  else {        /* left half */
    if ((tex->options.endpoint) && (!clip_off)) {
      FILL_PR_POS(npts, ptlist, x, y);
      DRAW_MAJ_X();
    }
    if ((tex->options.startpoint) && (!tex->options.res_cliprt))
      --count;
    while (count > 0) {   /* for each segment. */
      for (; ((offset > 0) && (count > 0)); count--, offset--) {
        if (seg & 1)
          FILL_PR_POS(npts, ptlist, x, y);
        x++;
        error += minax;
        if (error > 0) {
          error -= majax;
          y += vert;
        }
      }
      seg++;
      if (seg >= numsegs) {
        seg = 0;
        segptr = segment;
      }
      offset = *segptr++;
    }
    if ((tex->options.startpoint) && (!tex->options.res_cliprt))
      FILL_PR_POS(npts, ptlist, x, y);
  }   /* end bresenham loop */
  return(npts);
} /* bres_majx */


static int
bres_majy (majax,minax,count,error,vert,x,y,clip_off,tex,ptlist)
register majax, minax, count, error, vert;
register Pr_texture		*tex;
register struct pr_pos	*ptlist;
int clip_off, x, y; 
{
  int i, j, start_off, offset, seg, npts = 0, pat_length;
  int numsegs;
  register short *segment, *segptr;
  short auxp[20];
  short lauxp[20];
  int auxnp;
  int lauxnp;
  int bestc;
  int lcount, mcount, rcount;
  
  if (!tex->options.res_fat) {	/* is NOT fat vector */		
    {
      register short *s, *p;

      pat_length = numsegs = 0;
      for (s = segarray, p = tex->pattern; (*p); numsegs++) {
        *s++ = *p;
    	pat_length += *p++;
      }
    } /* end block */

    if (tex->options.res_poly)
      start_off = tex->res_fatoff = tex->res_polyoff;
    else {
      start_off = tex->res_fatoff = tex->offset;
      tex->res_oldpatln = pat_length;
    }

    if (!tex->options.givenpattern) {
      IncSetPattern(majax, minax, segarray, &pat_length, numsegs, 
      &start_off, tex);
    }

    /* reverse pattern in array */
    if (!tex->options.res_right && !tex->options.balanced) {	
      REVERSE_PATTERN(segarray, ptlist, numsegs, i, j);
    }
    segment = tex->res_patfat = segarray;
    tex->res_numsegs = numsegs;
  }
  else { /* is fat vector */
/*
 * Restore angle-corrected offset, since don't need to continually angle 
 * correct for a fat vector & restore patln in case this is the first 
 * visible vector of the fat vector.
 */
    start_off = tex->res_fatoff;	
    pat_length = tex->res_oldpatln;	
    numsegs = tex->res_numsegs;	
    segment = tex->res_patfat;
  }
/*
 * Account for the case when you have a single segment in your pattern.
 * Line will appear solid.
 */	
  if (numsegs == 1)
    tex->options.res_right = 1;
    
  if (tex->options.balanced) {
    bal_dash(majax+1,numsegs,segment,&bestc,&auxnp,auxp,
      &seg,&offset,&lcount,&rcount);
    mcount = majax+1-lcount-rcount;
  }
  else {
    pattern_offset(segment, numsegs, &seg, &offset, majax, pat_length, 
      start_off, clip_off, tex);
    bestc = -1;
  }
  segptr = &segment[seg+1];
  tex->res_oldpatln = pat_length;
/* 
 * Modified bresenham loop, where we account for the fact that if we were
 * meant to go right to left, we are drawing the pattern backwards, 
 * therefore we draw the even segments for the right half, and the odd
 * segments for the left half.
 */
  if (tex->options.balanced) {
    short* lsegptr;
    int loffset;
    int lseg;
    int rseg;
    short* rsegptr;
    int roffset;

    for (i=0; i<auxnp; i++) {
      lauxp[i] = auxp[i];
    }
    lauxnp = auxnp;
    loffset = lauxp[0];

    if (rcount > 0 && lcount > rcount) {
      (lauxp[0])++;

      if (lauxp[0] > 1.6 * maxdash && lauxp[0] < 4) {
        if (lauxnp == 1) { 
          if (seg & 1) { /* first reg is gap */
            lauxp[0]--;
            lauxnp = 2;
            lauxp[1] = 1;
          }
        }
        else {
          lauxp[0]--;
          lauxp[1]++;
        }
      }
    }
    
    /* determine clip_off */
    lseg = 0;
    loffset = lauxp[lseg];
    lsegptr = &lauxp[(lseg+1)%lauxnp];
    rseg = auxnp-1;
    roffset = auxp[rseg];
    rsegptr = &auxp[(rseg-1+auxnp)%auxnp];

    (void) clip_offset1(clip_off,majax,count,numsegs,segment,
      lauxnp,lauxp,auxnp,auxp,&lcount,&mcount,&rcount,
      &lseg,&loffset,&seg,&offset,&rseg,&roffset);

    lsegptr = &lauxp[(lseg+1)%lauxnp];
    segptr = &segment[(seg+1)%numsegs];
    rsegptr = &auxp[(rseg-1+auxnp)%auxnp];


    while (lcount > 0) {
      for ( ; ((lcount > 0) && (loffset > 0)); --lcount, --loffset) {
        if (!(lseg & 1))
          FILL_PR_POS(npts, ptlist, x, y);

        y += vert;
        error += minax;
        if (error > 0) {
          error -= majax;
          x++;
        }

      }
      if (lcount == 0) break;
      lseg++;
      if (lseg >= lauxnp) {
        lseg = 0;  
        lsegptr = lauxp;
      }
      loffset = *lsegptr++;
    }

    while (mcount > 0) {
      for ( ; ((mcount > 0) && (offset > 0)); --mcount, --offset) {
        if (!(seg & 1))
        FILL_PR_POS(npts, ptlist, x, y);
        y += vert;
        error += minax;
        if (error > 0) {
          error -= majax;
          x++;
        }
      }
      seg++;
      if (seg >= numsegs) {
        seg = 0;  
        segptr = segment;
      }
      offset = *segptr++;
    }

    while (rcount > 0) {
      for ( ; ((rcount > 0) && (roffset > 0)); --rcount, --roffset) {
        if (!(rseg & 1))
          FILL_PR_POS(npts, ptlist, x, y);
        y += vert;
        error += minax;
        if (error > 0) {
          error -= majax;
          x++;
        }
      }
      if (rcount == 0) break;
      rseg--;
      if (rseg < 0) {
        rseg = auxnp-1;  
        rsegptr = &auxp[auxnp-1];
      }
      roffset = *rsegptr--;
    }
    return npts;
  }

  if (tex->options.res_right) {        /* right half */
    if ((tex->options.startpoint) && (!clip_off)) {
      FILL_PR_POS(npts, ptlist, x, y);
      DRAW_MAJ_Y();
    }
    if ((tex->options.endpoint) && (!tex->options.res_cliprt))
      --count;        
    while (count > 0) {                /* for each segment. */
      for (; ((offset > 0) && (count > 0)); --count, --offset) {
        if (!(seg & 1))
          FILL_PR_POS(npts, ptlist, x, y);
        y += vert;
        error += minax;
        if (error > 0) {
          error -= majax;
          x++;
        }
      }                                 /* for each pixel in segment. */
      seg++;
      if (seg >= numsegs) {
        seg = 0;                
        segptr = segment;
      }
      offset = *segptr++;
    } 
    if ((tex->options.endpoint) && (!tex->options.res_cliprt))
      FILL_PR_POS(npts, ptlist, x, y);
  }
  else {                                /* left half */
    if ((tex->options.endpoint) && (!clip_off)) {        
      FILL_PR_POS(npts, ptlist, x, y);
      DRAW_MAJ_Y();
    }
    if ((tex->options.startpoint) && (!tex->options.res_cliprt))
      --count;
    while (count > 0) {                /* for each segment. */
      for (; ((offset > 0) && (count > 0)); --count, --offset) {
        if (seg & 1)
          FILL_PR_POS(npts, ptlist, x, y);
        y += vert;
        error += minax;
        if (error > 0) {
          error -= majax;
          x++;
        }
      } 
      seg++;
      if (seg >= numsegs) {
        seg = 0;        
        segptr = segment;
      }
      offset = *segptr++;
    } 
    if ((tex->options.startpoint) && (!tex->options.res_cliprt))
      FILL_PR_POS(npts, ptlist, x, y);
  } /* end bresenham loop */
  return(npts);
} /* bres_majy */


void
bal_dash(len,np,p,bestc,auxnp,auxp,seg,offset,lcount,rcount)
int len, np;
short *p;
int* bestc;
int* auxnp;
short* auxp;
int* seg;
int* offset;
int* lcount;
int* rcount;
{
  int i, j;
  int maxi, mini;
  int maxc, minc;
  int alen, blen, clen, dlen, elen, flen;
  int c;
  int bal;
  int center[20];
  int nport;
  int nfold;
  int nc; /* number of centers */
  int rem; 
  int score[20];
  int scoremax;
  int flg[20];
  int unit;
  int h;
  int flag;
  int fflag;

  maxgap = 0;
  maxdash = 0;
  mingap = 999;
  mindash = 999;
  for (i=0; i<np; i += 2) {
    if (maxdash < p[i]) { /* find max dash */
      maxdash = p[i];
      maxi = i;
    }
    if (mindash > p[i]) { /*find min dash */
      mindash = p[i];
      mini = i;
    }
    if (maxgap < p[i+1]) { /* find max dash */
      maxgap = p[i+1];
      maxc = i+1;
    }
    if (mingap > p[i+1]) { /* find min gap */
      mingap = p[i+1];
      minc = i+1;
    }
  }

  /* decide minimum *.....* length */
  alen = mingap + 2*min(mindash,2);
  blen = mingap + 2*min(maxdash,2);
  clen = mingap + 2*min(p[prev(minc,np)],p[nxt(minc,np)]);
  dlen = maxgap + 2*min(mindash,2);
  elen = maxgap + 2*min(maxdash,2);
  flen = maxgap + 2*min(p[prev(maxc,np)],p[nxt(maxc,np)]);

  /* 1.3 is a magic number */
  /* 8 <7 7> => [********] */
  /* 3 <1 5> => [* *] */
  if (len <= alen) {
    if (len <= 1.3*maxdash) { /* draw a solid line */
      *lcount = len;
      *rcount = 0;
      auxp[0] = len;
      *auxnp = 1;
      *bestc = -1;
      return;
    }
    else { /* draw a *....* line w/ adaptive gap */
      *lcount = len;
      *rcount = 0;
      auxp[0] = auxp[2] = min(2,maxdash);
      auxp[1] = len -2*auxp[0];
      *auxnp = 3;
      *bestc = -1;
      return;
    }
  }
  
  if (len >= blen && len <= clen) { /* draw *..* line w/ fixed gap */
    *lcount = len;
    *rcount = 0;
    auxp[2] = (len-mingap)/2;
    if (!((len-mingap) & 1))  /* even */
      auxp[0] = auxp[2];
    else
      auxp[0] = auxp[2] + 1;
    auxp[1] = mingap;
    *auxnp = 3;
    *bestc = -1;
    return;
  }

  if (len < dlen) {
    *lcount = len;
    *rcount = 0;
    if (len <= 1.3*maxdash) { /* draw a solid line */
      auxp[0] = len;
      *auxnp = 1;
      *bestc = -1;
      return;
    }
    else { /* draw a *..* line w/ adaptive gap */
      auxp[0] = auxp[2] = min(2,maxdash);
      auxp[1] = len - 2*auxp[0];
      *auxnp = 3;
      *bestc = -1;
      return;
    }
  }
  
  if (len >= elen && len <= flen) { /* draw *..* line w/ fixed gap */
    *lcount = len;
    *rcount = 0;
    auxp[2] = (len-maxgap)/2;
    if (even(len-maxgap))  /* even */
      auxp[0] = auxp[2];
    else
      auxp[0] = auxp[2] + 1;
    auxp[1] = maxgap;
    *auxnp = 2;
    *bestc = -1;
    return;
  }

  c = 0;
  for (i=0; i<np; i++) {
    bal = 1;
    for (j=1; j<np/2+1; j++) {
      if (p[(i+j)%np] != p[(i-j+np)%np]) {
        bal = 0;
        break;
      }
    }
    if (bal == 1) {
      center[c++] = i;
    }
  }

  nc = c; /* no of portion can be used as center */
  if (nc == 0) { /* impossible to balance */
    nc = 1;
    center[0] = maxi;
  }

  /* get the remainder */
  unit = 0;
  for (i=0; i<np; i++) unit += p[i];
  scoremax = -9999;
  for (c=0; c<nc; c++) {
    if (len <= p[center[c]]) {
      if (center[c] & 1) { /* gap */
        score[c] = -9999;
      }
      else { /* dash */
        score[c] = 0;
        flg[c] = 0;
      }
    }
    else {
      h = (len-p[center[c]])/2 + p[center[c]];
      rem = h % unit;
      nfold = h / unit; /* no of fold */
      nport = 0;
      for (i=center[c]; rem>0; i=(i+1)%np) {
        rem -= p[i]; 
        nport += 1;
      }
      nport += nfold * np;

      if (nport == 1) { /* special case */
        flg[c] = 0;
        score[c] = 0;
      }  
      else { /* more than 1 portion */
        flag = 0;
        score[c] = heur(rem,(i-1+np)%np,nport,np,p,&flag);
        flg[c] = flag;
      }
    }  
    if (score[c] > scoremax) {
      scoremax = score[c];
      *bestc = center[c];
      fflag = flg[c];
    }
  } /* end for (c) loop */

  if ((len <= p[*bestc]) || (len - p[*bestc] <= 1)) {
    *lcount = len;
    *rcount = 0;
    auxp[0] = len;
    *auxnp = 1;
    *bestc = -1;
    return;
  }
  
  do_bal(len,*bestc,fflag,np,p,auxnp,auxp,seg,offset,lcount,rcount);
}


static int 
heur(rem,i,nport,np,p,flag)
int rem;
int i;
int nport;
int np;
short* p;
int* flag;
{
  int sc, shksc, strsc, mersc;
  int endp;
  int strchlen, strratio;
  int xtra;

  sc = -999;
  shksc = -999;
  strsc = -999;
  mersc = -999;

  /* exactly match */
  if (rem == 0) {
    if (isgap(i)) { /* match gap */
      /* get the score for shrinking gap */

      if (p[i] > (0.5*mingap+min(2,maxdash))) { /* shrink gap */
        endp = i;
        shksc = -40*mingap/(p[i]-2);
      }

      /* get the score for stretching segment */
      strchlen = p[i];
      endp = prev(i,np);
  
      if (p[endp]+strchlen > 1.3*maxdash) { /* serious stretch */
        strsc = -60*strchlen/p[endp];
      }
      else { /* minor stretch */
        strsc = -20*strchlen/p[endp];
      }

      if (shksc > strsc) { /* shrink gap */
        sc = shksc;
        *flag = 2;
      }
      else { /* stretch segment */
        sc = strsc;
        *flag = 1;
      }

      return sc;
    }
    else { /* match segment */

      endp = i;

      if (p[endp] == 1) { /* end segment == 1 */
        if (maxdash == 1) { /* since the max dash is 1, O.K. */
          sc = 999;
          *flag = 0;
        }
        else { /* evaluate shrink and stretch */
          /* get the score for shrinking gap */
          if (p[prev(i,np)] > (0.5*mingap+2)) { /* shrink gap */
            shksc = -40*mingap/(p[i]);
          }
          strratio = (p[prev(endp,np)]+p[endp])/p[(endp-2+np)%np];
          if (nport>2 && strratio < 0.3) { /* can be stretched */
            strsc = -40*strratio; /* penalty for stretch */
          }
          else { /* serious penalty */
            strsc = -60*strratio; /* avoid short end dash */
          }
          if (shksc > strsc) { /* shrink gap */
            sc = shksc;
            *flag = 2;
          }
          else { /* stretch segment */
            sc = strsc;
            *flag = 1;
          }
        }
      }
      else { /* match segment and segment > 1 */
        sc = 999;
        *flag = 0;
      }
    }
    return sc;
  }
  else if (rem < 0) { /* not exactly match */
    if (isgap(i)) { /* end is gap */
      xtra = rem+p[i]; /* extra portion */
      if (xtra > (0.5*mingap+min(2,maxdash))) { /*  */
        shksc = -40*mingap/xtra;
      }
      strchlen = xtra; /* stretch length */
      endp = prev(i,np);
      if (p[endp]+strchlen > 1.3*maxdash) { /* serious penalty */
        strsc = -20*strchlen/p[endp]-60*strchlen/maxdash;
      }
      else { /* minor penalty */
        strsc = -20*strchlen/p[endp];
      }
      if (p[(i-2+np)%np]+p[prev(i,np)]+xtra < 1.6*maxgap && nport > 2) { 
        /* merge two gaps together */
        mersc = -80*(p[endp]+xtra)/p[prev(endp,np)];
      }
      if (shksc > strsc) { 
        if (shksc > mersc) {
          sc = shksc;
          *flag = 2;
        }
        else {
          sc = mersc;
          *flag = 3;
        }
      }
      else {
        if (strsc > mersc) {
          sc = strsc;
          *flag = 1;
        }
        else {
          sc = mersc;
          *flag = 3;
        }
      }

      return sc;
    }
    else { /* end is segment */
      /* can be stay, stretch, shrink, but not merge */
      endp = i;
      xtra = p[endp] + rem;
         
      if (xtra == 1) {
        strratio = (p[prev(endp,np)]+1)/p[(endp-2+np)%np];
        if (nport>1 && strratio < 0.3) { /* minor penalty */
          strsc = -30*(p[prev(endp,np)]+1)/p[(endp-2+np)%np];
        }
        else { /* severe penalty */
          strsc = -200*strratio; /* avoid end segment == 1 */
        }
        /* try shrink gap */
        if (p[prev(i,np)]+1 > 0.5*mingap+min(2,maxdash)) { 
          shksc = -40*mingap/(p[prev(i,np)]+1);
        }
      }
      else { /* xtra > 1 */
        sc = (5*xtra/p[prev(endp,np)]);
        *flag = 0;
      }
      if (sc > 0) {
        *flag = 0;
      }
      else {
        if (shksc > strsc) {
          sc = shksc;
          *flag = 2;
        }
        else {
          sc = strsc;
          *flag = 1;
        }
      }
    }
    return sc;
  }
  return sc;
}


static int
do_bal(len,bestc,fillflg,np,p,auxnp,auxp,seg,offset,lcount,rcount)
int len, bestc, fillflg;
int np;
short* p;
short* auxp;
int* auxnp;
int* seg;
int* offset;
int* lcount;
int* rcount;
{
  int k, kl;
  int endlen;
  int i;
  int symmetry;
  int noseg;

  *auxnp = 0; /* always initialize it */
  hrsave = hr = (len-p[bestc])/2 + p[bestc]; /* right half */
  hl = len - hr; /* left half */
  if (even(len) ^ even(p[bestc]))
    symmetry = 0;
  else
    symmetry = 1;
  
  noseg = 0;
  kl = k = bestc; /* kl is left side starting segment */
  for (i=0; hr > 0; i++) {
    if (hr < p[k]) 
      break;
    else {
      hr -= p[k];
      k = (k+1)%np;
      kl = (kl-1+np)%np;
      noseg++;
    }
  }
  if (hr == 0) {
    k = (k-1+np)%np;
    kl = (kl+1)%np;
    noseg--;
  }

  /* decide no of segment (ns) */

  if (hr == 0) { /* exactly match */
    if (isgap(k)) { /* match gap, need to stretch */
      /* .......*******....... */
      /* (i-3)  (i-2)  (i-1) */
      /*                k */
      /* hr == 0 */
      *rcount = p[k];
      *lcount = (symmetry)? (*rcount): (*rcount+1);
      if (fillflg == 1) { /* stretch */
        auxp[0] = p[k];
        *auxnp = 1;
        *seg = (kl+1)%np;
        *offset = p[*seg];
      }
      else if (fillflg == 2) { /* don't stretch, use mindash as end instead */
        auxp[0] = mindash;
        auxp[1] = p[k] - mindash;
        *auxnp = 2;
        *seg = (kl+1)%np;
        *offset = p[*seg];
      }
    }
    else { /* match segment */
      if (fillflg == 0) {
        *rcount = p[k];
        *lcount = (symmetry)? (*rcount): (*rcount+1);
        auxp[0] = p[k];
        *auxnp = 1;
        *seg = (kl+1)%np;
        *offset = p[*seg];
      }
      else if (fillflg == 1) { /* stretch for special end treatment */
        auxp[0] = p[k] + p[prev(k,np)];
        *auxnp = 1;
        *rcount = auxp[0];
        *lcount = (symmetry)? (*rcount): (*rcount+1);
        *seg = (kl+2)%np;
        *offset = p[*seg];
      }
      else if (fillflg == 2) { /* shrink for special end treatment */
        int trim;

        trim = 2-p[k];
        auxp[0] = 2;
        auxp[1] = p[prev(k,np)] - trim;
        *rcount = auxp[0]+auxp[1];
        *lcount = (symmetry)? (*rcount): (*rcount+1);
        *auxnp = 2;
        *seg = (kl+2)%np;
        *offset = p[*seg];
      }
    }
  }
  else { /* nomatch, */
    if (isgap(k)) { /* gap cut off */
      /* ......*****...... */
      /* (i-2) (i-1) i */
      /*             k */
      /* hr > 0 */
      if (fillflg == 1) { /* allow stretch */
        *rcount = hr;
        *lcount = (symmetry)? (*rcount): (*rcount+1);
        auxp[0] = hr;
        *auxnp = 1;
        *seg = (kl+1)%np;
        *offset = p[*seg];
      }
      else if (fillflg == 2) { /* shrink */
        *rcount = hr;
        *lcount = (symmetry)? (*rcount): (*rcount+1);
        endlen = min(2,maxdash);
        auxp[1] = hr - endlen;
        auxp[0] = endlen;
        *auxnp = 2;
        *seg = (kl+1)%np;
        *offset = p[*seg];
      }
      else if (fillflg == 3) { /* merge two gaps together */
        if (noseg > 2) {
          auxp[1] = p[(k-2+np)%np] + p[(k-1+np)%np] + hr - mindash;
          auxp[0] = mindash;
          *auxnp = 2;
          *rcount = auxp[0] + auxp[1];
          *lcount = (symmetry)? (*rcount): (*rcount+1);
          *seg = (kl+3)%np;
          *offset = p[*seg];
        }
        else {
          auxp[0] = mindash; /*(symmetry)? (mindash): (mindash+1);*/
          auxp[2] = mindash;
          auxp[1] = len - auxp[0] - auxp[2];
          *auxnp = 3;
          *rcount = 0;
          *lcount = len;
        }
      }
    }
    else { /* nomatch, end dash */
      if (fillflg == 0) { /* regular cut */
        auxp[0] = hr;
        *rcount = hr;
        *lcount = (symmetry)? (*rcount): (*rcount+1);
        *auxnp = 1;
        *seg = (kl+1)%np;
        *offset = p[*seg];
      }
      else if (fillflg == 1) { /* special end treatment */
        /* merge (i-1) and (i) into (i-2) segment */
        auxp[0] = p[prev(k,np)] + hr;
        *auxnp = 1;
        *rcount = auxp[0];
        *lcount = (symmetry)? (*rcount): (*rcount+1);
        *seg = (kl+2)%np;
        *offset = p[*seg];
      }
      else if (fillflg == 2) {
        auxp[0] = mindash;
        auxp[1] = p[(k-1+np)%np] - (mindash - hr);
        *auxnp = 2;
        *rcount = auxp[0]+auxp[1];
        *lcount = (symmetry)? (*rcount): (*rcount+1);
        *seg = (kl+2)%np;
        *offset = p[*seg];
      }
    }
  }

}


static void
clip_offset1(clip_off,majax,count,numsegs,segment,lauxnp,lauxp,auxnp,auxp,
  lcount,mcount,rcount,lseg,loffset,seg,offset,rseg,roffset)
int clip_off;
int majax;
int count;
int numsegs;
short segment[];
int auxnp;
short auxp[];
int lauxnp;
short lauxp[];
int* lcount;
int* mcount;
int* rcount;
int* lseg;
int* loffset;
int* seg;
int* offset;
int* rseg;
int* roffset;
{
  if (clip_off == 0 && count < majax+1) { /* right clipping */
    if (count <= *lcount ) {
      *lcount = count;
      *mcount = *rcount = 0;
    }
    else if (count <= *lcount+*mcount) {
      *mcount = count - *lcount;
      *rcount = 0;
    }
    else {
      *rcount = count - *lcount - *mcount;
    }
  }
  else { /* left clipping */
    if (clip_off <= *lcount) { /* within left */
      *lcount -= clip_off;
      if (*lcount) {
        for (; clip_off>0; (*lseg)++, *loffset=lauxp[*lseg]) {
          clip_off -= *loffset;
        }
        if (clip_off < 0) {
          *loffset = -clip_off;
          (*lseg)--;
        }
      }
    }
    else if (clip_off <= *lcount + *mcount) { /* within middle */
      *mcount += *lcount-clip_off;
      clip_off -= *lcount;
      *lcount = 0;
      if (*mcount) {
        for (; clip_off>0; *seg=(*seg+1)%numsegs, *offset=segment[*seg]) {
          clip_off -= *offset;
        }
        if (clip_off < 0) {
          *offset = -clip_off;
          (*seg)--;
        }
      }
    }
    else { /* within right */
      *rcount += *lcount + *mcount-clip_off;
      clip_off -= (*lcount + *mcount);
      *lcount = *mcount = 0;
      if (*rcount) {
        for (; clip_off>0; (*rseg)--, *roffset=auxp[*rseg]) {
          clip_off -= *roffset;
        }
        if (clip_off < 0) {
          *roffset = -clip_off;
          (*rseg)++;
        }
      }
    } /* end if */
  }
}       


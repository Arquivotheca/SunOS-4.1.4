/*	@(#)pr_texmacs.h 1.1 94/10/31 SMI	*/

/* 
 * Copyright 1986 by Sun Microsystems, Inc.
 */

 
#ifndef pr_texmacs_h_DEFINED
#define pr_texmacs_h_DEFINED 

extern char *malloc(), *realloc();
 
/*
 * These are macros to be used for the textured vector code.
 */

#define MALLOC_PTLIST(ptlist, ptlist_size, d5_count) 		\
	if (ptlist_size) {					\
	    if (ptlist_size <= d5_count) {			\
		ptlist_size = d5_count + 1;			\
		ptlist = (struct pr_pos *) realloc((char *) ptlist, \
		    ptlist_size * sizeof(struct pr_pos));	\
	    }							\
	} else {		/* malloc new block */		\
	    ptlist_size = d5_count + 1;				\
	    ptlist = (struct pr_pos *) malloc(ptlist_size * \
		sizeof(struct pr_pos));				\
	}
    
#define REVERSE_PATTERN(segarray, ptlist, numsegs, i, j) 	\
	{ /* begin block */					\
	    register short *s1, *s2;				\
	    short oneseg;					\
								\
	    for (i=0, j=(numsegs-1), s1=segarray, s2= &segarray[j]; \
		    (i < j); i++, --j) { 			\
		oneseg = *s1;					\
		*s1++ = *s2;					\
		*s2-- = oneseg;					\
	    }							\
	} /* end block */

#define FILL_PR_POS(npts, ptlist, x, y) {			\
	ptlist->x = x;						\
	ptlist->y = y;						\
	ptlist++;						\
	npts++;							\
    }


#define DRAW_MAJ_X() {						\
	x++;							\
	error += minax;						\
	if (error > 0) {					\
	    error -= majax;					\
	    y += vert;		   				\
	}							\
	--count;						\
	--offset;						\
	if (!offset) {						\
	    seg++;						\
	    if (seg >= numsegs)	{				\
		seg = 0;					\
		segptr = segment;				\
	    }							\
	    offset = *segptr++;					\
	}							\
    }
	
#define DRAW_MAJ_Y() {						\
	y += vert;						\
	error += minax;						\
	if (error > 0) {					\
	    error -= majax;					\
	    x++;	   					\
	}							\
	--count;						\
	--offset;						\
	if (!offset) {						\
	    seg++;						\
	    if (seg >= numsegs)	{				\
		seg = 0;					\
		segptr = segment;				\
	    }							\
	    offset = *segptr++;					\
	}							\
    }
	
#define DRAW_VERT() {						\
	y += vert;						\
	--count;						\
	--offset;						\
	if (!offset) {						\
	    seg++;						\
	    if (seg >= numsegs)	{				\
		seg = 0;					\
		segptr = segment;				\
	    }							\
	    offset = *segptr++;					\
	}							\
    }
    
    
#define DRAW_HORZ() {						\
	x++;							\
	--count;						\
	--offset;						\
	if (!offset) {						\
	    seg++;						\
	    if (seg >= numsegs)	{				\
		seg = 0;					\
		segptr = segment;				\
	    }							\
	    offset = *segptr++;					\
	}							\
    }

#endif pr_texmacs_h_DEFINED 

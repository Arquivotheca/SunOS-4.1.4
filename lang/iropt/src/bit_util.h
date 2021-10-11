/*	@(#)bit_util.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __bit_util__
#define __bit_util__

/*
 * SET_PTR -- points to a descriptor for a bit-matrix
 * representation of an array of sets
 */

/* typedef struct set_description *SET_PTR; */

struct set_description {
	int bit_rowsize;
	int word_rowsize;
	int nrows;
	SET bits;
}; /* SET_PTR */ 

/*
 * operations on sets and arrays of sets
 */
extern BOOLEAN test_bit(/*set, bitno*/);
extern void    set_bit(/*set, bitno*/);
extern void    reset_bit(/*set, bitno*/);

extern BOOLEAN bit_test(/*set_ptr, rowno, bitno*/);
extern void    bit_set(/*set_ptr, rowno, bitno*/);
extern void    bit_reset(/*set_ptr, rowno, bitno*/);

#define ROW_ADDR(set, blockno) \
	((set)->bits + ((blockno)*(set)->word_rowsize)) /* &set[blockno] */
#define NROWS(set) ((set)->nrows)	    /* # rows in an array of sets */
#define SET_SIZE(set) ((set)->word_rowsize) /* size of a set, in words */
#define SET_CARD(set) (set)->bit_rowsize    /* # elements in a set */

#endif

#ifndef lint
static	char sccsid[] = "@(#)bit_util.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

# if defined(sparc) || defined(i386)
 
#include "iropt.h"
#include "bit_util.h"


void
bit_reset( set_ptr, rowno, bitno)
SET_PTR set_ptr;
int rowno, bitno;
{
register unsigned bit_index, word_index, bit_offset;

		bit_index = (rowno*set_ptr->bit_rowsize)+bitno;
		word_index = bit_index / BPW;
		bit_offset = (bit_index % BPW);
		*(set_ptr->bits + word_index) &= ~(1<< bit_offset);
}

void
bit_set( set_ptr, rowno, bitno)
SET_PTR set_ptr;
int rowno, bitno;
{
register unsigned bit_index, word_index, bit_offset;

		bit_index = (rowno*set_ptr->bit_rowsize)+bitno;
		word_index = bit_index / BPW;
		bit_offset = (bit_index % BPW);
		*(set_ptr->bits + word_index) |= (1<< bit_offset);
}

BOOLEAN
bit_test( set_ptr, rowno, bitno)
SET_PTR set_ptr;
int rowno, bitno;
{
register unsigned bit_index, word_index, bit_offset;

		bit_index = (rowno*set_ptr->bit_rowsize)+bitno;
		word_index = bit_index / BPW;
		bit_offset = (bit_index % BPW);
		return (BOOLEAN)((set_ptr->bits[word_index]>>bit_offset)&1);
}

BOOLEAN
test_bit( set, bitno ) register SET set; register int bitno;
{
	register int word_index, bit_offset;

	word_index = (unsigned)bitno / BPW;
	bit_offset = (unsigned)bitno % BPW;
	return (BOOLEAN)((set[word_index]>>bit_offset)&1);
}

void
set_bit( set, bitno ) register SET set; register int bitno;
{
	register int word_index, bit_offset;

	word_index = (unsigned)bitno / BPW;
	bit_offset = (unsigned)bitno % BPW;
	set[word_index] |= (1<<bit_offset);
}

void
reset_bit( set, bitno ) register SET set; register int bitno;
{
	register int word_index, bit_offset;

	word_index = (unsigned)bitno / BPW;
	bit_offset = (unsigned)bitno % BPW;
	set[word_index] &= ~(1<<bit_offset);
}

# endif

/*	@(#)bitmacros.h	1.1	10/31/94	*/

/* this macro converts a bit# (numbered R-to-L, 0-oringined) to a bit mask */
#define NUM_TO_MASK(bit_no)		(1 << (bit_no))

/* macros for setting/clearing bits in bit-vectors, by mask */
#define SET_BITS(bitvect, mask)		( (bitvect) |=   (mask)  )
#define CLR_BITS(bitvect, mask)		( (bitvect) &= (~(mask)) )
#define SET_BIT(bitvect, mask)		SET_BITS(bitvect, mask)
#define CLR_BIT(bitvect, mask)		CLR_BITS(bitvect, mask)

/* these macros set/clear a bit in a bit-vector bit its bit# */
#define SET_BIT_NUM(bitvect, bit_no)	SET_BIT((bitvect), NUM_TO_MASK(bit_no))
#define CLR_BIT_NUM(bitvect, bit_no)	CLR_BIT((bitvect), NUM_TO_MASK(bit_no))

/* this macro tests to see if a single bit	*/
/* (or any one of multiple bits) is on, by mask */
#define TST_BIT(bitvect, mask)		( ((bitvect)&(mask)) != 0 )
#define BIT_IS_ON(bitvect, mask)	TST_BIT(bitvect,mask)
#define BIT_IS_OFF(bitvect, mask)	(!TST_BIT(bitvect,mask))

/* these macro tests to see if multiple bits are all on(or off), by mask */
#define TST_BITS(bitvect, mask)		( ((bitvect)&(mask)) == (mask) )
#define BITS_ARE_ON(bitvect, mask)	TST_BITS(bitvect,mask)
#define BITS_ARE_OFF(bitvect, mask)	( ((bitvect)&(mask)) ==   0    )

/* this macro tests to see if a bit is on, by bit# */
#define TST_BIT_NUM(bitvect, bit_no)	TST_BIT(bitvect, NUM_TO_MASK(bit_no))

	.data
	.asciz	"@(#)bit_util.s 1.1 94/10/31 Copyr 1984 Sun Micro"
	.even
	.text

|	Copyright (c) 1984 by Sun Microsystems, Inc.

#ifdef PROFILE
#define MCOUNT       lea 277$,a0;\
		 .data; 277$: .long 0; .text;\
					  jsr mcount
#else
#define MCOUNT
#endif

	.globl	_bit_reset
_bit_reset:
	MCOUNT
	movl	sp@(4),a0
	movl	a0@(12),a1	| set_ptr->bits->a1
	movw	a0@(2),d0	| set_ptr->word_rowsize->d0
	mulu	sp@(10),d0	| rowno*word_rowsize -> d0
	addl	sp@(12),d0	| bit_index ->d0
	movl	d0,d1
	andl	#-8,d1
	subl	d1,d0		| bit remainder (0-7) ->d0
	lsrl	#3,d1		| byte_index -> d1
	bclr	d0,a1@(0,d1:L)
	rts

	.globl	_bit_set
_bit_set:
	MCOUNT
	movl	sp@(4),a0
	movl	a0@(12),a1	| set_ptr->bits->a1
	movw	a0@(2),d0	| set_ptr->word_rowsize->d0
	mulu	sp@(10),d0	| rowno*word_rowsize -> d0
	addl	sp@(12),d0	| bit_index ->d0
	movl	d0,d1
	andl	#-8,d1
	subl	d1,d0		| bit remainder (0-7) ->d0
	lsrl	#3,d1		| byte_index -> d1
	bset	d0,a1@(0,d1:L)
	rts

	.globl	_bit_test
_bit_test:
	MCOUNT
	movl	sp@(4),a0
	movl	a0@(12),a1	| set_ptr->bits->a1
	movw	a0@(2),d0	| set_ptr->word_rowsize->d0
	mulu	sp@(10),d0	| rowno*word_rowsize -> d0
	addl	sp@(12),d0	| bit_index ->d0
	movl	d0,d1
	andl	#-8,d1
	subl	d1,d0		| bit remainder (0-7) ->d0
	lsrl	#3,d1		| byte_index -> d1
	btst	d0,a1@(0,d1:L)
	jeq		L0
	moveq	#1,d0
	rts
L0:	
	moveq	#0,d0
	rts

|BOOLEAN
|test_bit( set, bitno ) register SET set; register int bitno;
|{
|	register int word_index, bit_offset;
|
|	word_index = (unsigned)bitno / BPW;
|	bit_offset = (unsigned)bitno % BPW;
|	if( set[word_index] & (1<<bit_offset) )
|		return( TRUE );
|	else
|		return( FALSE );
|}

	.globl _test_bit
_test_bit:
	MCOUNT
	movl	sp@(4),a1		| set == a1
	movl	sp@(8),d0		| bitno == d0
	movl	d0,d1			| bitno == d0 == d1
	andl	#-8,d1			| 
	subl	d1,d0			| bit remainder == d0
	lsrl	#3,d1			| byte_index == d1
	btst	d0,a1@(0,d1:L)
	jeq	L10
	moveq	#1,d0
	rts
L10:
	moveq	#0,d0
	rts

|void
|set_bit( set, bitno ) register SET set; register int bitno;
|{
|	register int word_index, bit_offset;
|
|	word_index = (unsigned)bitno / BPW;
|	bit_offset = (unsigned)bitno % BPW;
|	set[word_index] |= (1<<bit_offset);
|}

	.globl	_set_bit
_set_bit:
	MCOUNT
	movl	sp@(4),a1		| set == a1
	movl	sp@(8),d0		| bitno == d0
	movl	d0,d1			| bitno == d0 == d1
	andl	#-8,d1			| 
	subl	d1,d0			| bit remainder == d0
	lsrl	#3,d1			| byte_index == d1
	bset	d0,a1@(0,d1:L)
	rts

|void
|reset_bit( set, bitno ) register SET set; register int bitno;
|{
|	register int word_index, bit_offset;
|
|	word_index = (unsigned)bitno / BPW;
|	bit_offset = (unsigned)bitno % BPW;
|	set[word_index] &= ~(1<<bit_offset);
|}

	.globl	_reset_bit
_reset_bit:
	MCOUNT
	movl	sp@(4),a1		| set == a1
	movl	sp@(8),d0		| bitno == d0
	movl	d0,d1			| bitno == d0 == d1
	andl	#-8,d1			| 
	subl	d1,d0			| bit remainder == d0
	lsrl	#3,d1			| byte_index == d1
	bclr	d0,a1@(0,d1:L)
	rts

|
|_ckalloca:
|	This atrocity isn't worth porting to sunrise,
|	so we've made it uncallable.
|
	MCOUNT
	movl    sp@,a0		| save return addr
	movl    sp@(4),d0	| align on longword boundary
	addql   #3,d0
	asrl	#2,d0
	lea	sp@(8),sp
	cmpl	#0xffff,d0
	jgt	L2		| allocate more than 2**16 words
	subql	#1,d0
L1:	clrl	sp@-
	dbra	d0,L1
	jra	L4		| done
L2:
	clrl	sp@-
	subql	#1,d0
	jgt	L2
L4:
	movl    sp,d0
	lea    	sp@(-8),sp
	jmp     a0@

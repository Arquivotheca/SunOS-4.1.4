/*
 *	.seg	"data"
 *	.asciz	"@(#)bcmp.s 1.1 94/10/31"
 *	Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <sun4/asm_linkage.h>

	.seg	"text"
	.align	4

/*
 * bcmp(s1, s2, len)
 * compare string s1 to string s2 for len bytes
 * return 0 if equal, 1 otherwise
 */
	ENTRY(bcmp)
	cmp	%o2, 7
	ble,a	cmpbyt			! for small counts go do bytes
	sub	%o1, %o0, %o1		

	andcc	%o0, 3, %o3		! is s1 aligned?
	bz,a	iss2			! if so go check s2
	andcc	%o1, 3, %o4		! is s2 aligned?
	cmp	%o3, 2
	be	algn2
	cmp	%o3, 3

algn1:	ldub	[%o0], %o4		! cmp one byte
	inc	%o0
	ldub	[%o1], %o5
	inc	%o1
	dec	%o2
	be	algn3
	cmp	%o4, %o5
	be	algn2
	nop
	b,a	noteq

algn2:	lduh	[%o0], %o4
	inc	2, %o0
	ldub	[%o1], %o5
	inc	1, %o1
	srl	%o4, 8, %o3
	cmp	%o3, %o5
	be,a	1f
	ldub	[%o1], %o5
	b,a	noteq
1:	inc	%o1
	dec	2, %o2
	and	%o4, 0xff, %o4
	cmp	%o4, %o5
algn3:	be	iss2
	andcc	%o1, 3, %o4		! is s2 aligned?
	b,a	noteq

cmpbyt:	b	bytcmp
	deccc	%o2
1:	ldub	[%o0 + %o1], %o5	! byte compare loop
	inc	%o0
	cmp	%o4, %o5
	be,a	bytcmp
	deccc	%o2			! delay slot, compare count (len)
	b,a	noteq
bytcmp:	bge,a	1b
	ldub	[%o0], %o4
cmpeq:	retl				! strings compare equal
	clr	%o0

noteq:	retl				! strings aren't equal
	mov	1, %o0

iss2:	andn	%o2, 3, %o3		! count of aligned bytes
	and	%o2, 3, %o2		! remaining bytes
	bz	w4cmp			! if s2 word aligned, compare words
	cmp	%o4, 2
	be	w2cmp			! s2 half aligned
	cmp	%o4, 1		

w3cmp:	ldub	[%o1], %g1		! read a byte to align for word reads
	inc	1, %o1 
	be	w1cmp			! aligned to 1 or 3 bytes
	sll	%g1, 24, %o5

	sub	%o1, %o0, %o1
2:	ld	[%o0 + %o1], %g1
	ld	[%o0], %o4
	inc	4, %o0
	srl	%g1, 8, %g2		! merge with the other half
	or	%g2, %o5, %o5
	cmp	%o4, %o5
	bne	noteq
	deccc	4, %o3
	bnz	2b
	sll	%g1, 24, %o5
	sub	%o1, 1, %o1		! used 3 bytes of the last word read
	b	bytcmp
	deccc	%o2

w1cmp:	lduh	[%o1], %g1		! read 3 bytes to word align
	inc	2, %o1
	sll	%g1, 8, %g2
	or	%o5, %g2, %o5

	sub	%o1, %o0, %o1
3:	ld	[%o0 + %o1], %g1
	ld	[%o0], %o4
	inc	4, %o0
	srl	%g1, 24, %g2		! merge with the other half
	or	%g2, %o5, %o5
	cmp	%o4, %o5
	bne	noteq
	deccc	4, %o3
	bnz	3b
	sll	%g1, 8, %o5
	sub	%o1, 3, %o1		! used 1 byte of the last word read
	b	bytcmp
	deccc	%o2
	
w2cmp:	lduh	[%o1], %g1		! read a halfword to align s2
	inc	2, %o1	
	sll	%g1, 16, %o5
	sub	%o1, %o0, %o1
4:	ld	[%o0 + %o1], %g1	! read a word from s2
	ld	[%o0], %o4		! read a word from s1
	inc	4, %o0
	srl	%g1, 16, %g2		! merge with the other half
	or	%g2, %o5, %o5
	cmp	%o4, %o5
	bne	noteq
	deccc	4, %o3
	bnz	4b
	sll	%g1, 16, %o5
	sub	%o1, 2, %o1		! only used half of the last read word
	b	bytcmp
	deccc	%o2

w4cmp:	sub	%o1, %o0, %o1
	ld	[%o0 + %o1], %o5
5:	ld	[%o0], %o4
	inc	4, %o0
	cmp	%o4, %o5
	bne	noteq
	deccc	4, %o3
	bnz,a	5b
	ld	[%o0 + %o1], %o5
	b	bytcmp			! compare remaining bytes, if any
	deccc	%o2

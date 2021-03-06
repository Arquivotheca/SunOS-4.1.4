!	
!	"@(#)crt.s.sv 1.1 94/10/31 SMI"
!	was originally crt.s 1.11 in the sparc directory
!	Copyright (c) 1988 by Sun Microsystems, Inc.

	.seg	"text"
	.align	4

#include <machine/asm_linkage.h>
#include <machine/trap.h>
#include <machine/psl.h>

/*
 * C run time subroutines.
 */

/*
 * procedure to perform a 32 by 32 multiply.
 * pass the multiplier into %o0, and the multiplicand into %o1
 * the least significant 32 bits of the result will be returned in %o0,
 * and the most significant in %o1
 *
 * This code has an optimization built in for short (less than 13 bit)
 * multipliers. Short multipliers require 26 or 27 instruction cycles, and
 * long ones require 47 to 51 instruction cycles.  For two positive numbers,
 * the most common case, a long multiply takes 47 instruction cycles.
 *
 * This code indicates that overflow has occured, by leaving the Z condition
 * code clear. The following call sequence would be used if you wish to
 * deal with overflow:
 *
 *		call	.mul
 *		nop		( or set up last parameter here )
 *		bnz	overflow_code	(or tnz to overflow handler)
 */
	RTENTRY(.mul)
	mov	%o0, %y			! multiplier to Y register
	andncc	%o0, 0xfff, %g0		! mask out lower 12 bits
	be	mul_shortway		! can do it the short way
	andcc	%g0, %g0, %o4		! zero the partial product
					! and clear N and V conditions
	!
	! long multiply
	!
	mulscc	%o4, %o1, %o4		! first iteration of 33
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4		! 32nd iteration
	mulscc	%o4, %g0, %o4		! last iteration only shifts
	!
	! if %o0 (multiplier) was negative, the result is
	!	(%o0 * %o1) + %o1 * (2**32)
	! we fix that here
	!
	tst	%o0
	rd	%y, %o0
	bge	1f
	tst	%o0			! for when we check for overflow

	sub	%o4, %o1, %o4		! bit 33 and up of the product are in
					! %o4, so we don't have to shift %o1
	!
	! We haven't overflowed if:
	!	low-order bits are positive and high-order bits are 0
	!	low-order bits are negative and high-order bits are -1
	!
	! if you are not interested in detecting overflow,
	! replace the following code with:
	!
	!	1:	jmp	%o7+8
	!		mov	%o4, %o1
	!
1:
	bge	2f			! if low-order bits were positive.
	addcc	%o4, %g0, %o1		! return most sig. bits of prod and set
					! Z appropriately (for positive product)
	jmp	%o7+8
	subcc	%o4, -1, %g0		! set Z if high order bits are -1 (for
					! negative product)
2:
	jmp	%o7+8			! leaf routine return
	nop
	!
	! short multiply
	!
mul_shortway:
	mulscc	%o4, %o1, %o4		! first iteration of 13
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4		! 12th iteration
	mulscc	%o4, %g0, %o4		! last iteration only shifts

	rd	%y, %o5
	sll	%o4, 12, %o0		! left shift middle bits by 12 bits
	srl	%o5, 20, %o5		! right shift low bits by 20 bits
	!
	! We haven't overflowed if:
	!	low-order bits are positive and high-order bits are 0
	!	low-order bits are negative and high-order bits are -1
	!
	! if you are not interested in detecting overflow,
	! replace the following code with:
	!
	!		or	%o5, %o4, %o0
	!		jmp	%o7+8
	!		mov	%o4, %o1
	!
	orcc	%o5, %o0, %o0		! merge for true product
	bge	3f			! if low-order bits were positive.
	sra	%o4, 20, %o1		! right shift high bits by 20 bits
					! and put into  %o1
	jmp	%o7+8
	subcc	%o1, -1, %g0		! set Z if high order bits are -1 (for
					! negative product)
3:
	jmp	%o7+8			! leaf routine return
	addcc	%o1, %g0, %g0		! set Z if high order bits are 0

/*
 * procedure to perform a 32 by 32 unsigned multiply.
 * pass the multiplier into %o0, and the multiplicand into %o1
 * the least significant 32 bits of the result will be returned in %o0,
 * and the most significant in %o1
 *
 * This code has an optimization built in for short (less than 13 bit)
 * multiplies. Short multiplies require 25 instruction cycles, and long ones
 * require 46 or 48 instruction cycles.
 *
 * This code indicates that overflow has occured, by leaving the Z condition
 * code clear. The following call sequence would be used if you wish to
 * deal with overflow:
 *
 *		call	.umul
 *		nop		( or set up last parameter here )
 *		bnz	overflow_code	(or tnz to overflow handler)
 */
	RTENTRY(.umul)
	or	%o0, %o1, %o4		! logical or of multiplier
					! and multiplcand
	mov	%o0, %y			! multiplier to Y register
	andncc	%o4, 0xfff, %o5		! mask out lower 12 bits
	be	umul_shortway		! can do it the short way
	andcc	%g0, %g0, %o4		! zero the partial product
					! and clear N and V conditions
	!
	! long multiply
	!
	mulscc	%o4, %o1, %o4		! first iteration of 33
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4		! 32nd iteration
	mulscc	%o4, %g0, %o4		! last iteration only shifts
	!
	! Normally, with the shifty-add approach, if both numbers are positive,
	! you get the correct result.  With 32-bit twos-complement numbers,
	! -x can be represented as ((2 - (x/(2**32)) mod 2) * 2**32.  To avoid
	! a lot of 2**32's, we can just move the radix point up to be just
	! to the left of the sign bit.  So:
	!
	!	 x *  y	= (xy) mod 2
	!	-x *  y	= (2 - x) mod 2 * y = (2y - xy) mod 2
	!	 x * -y	= x * (2 - y) mod 2 = (2x - xy) mod 2
	!	-x * -y = (2 - x) * (2 - y) = (4 - 2x - 2y + xy) mod 2
	!
	! For signed multiplies, we subtract (2**32) * x from the partial
	! product to fix this problem for negative multipliers (see multiply.s)
	! Because of the way the shift into the partial product is calculated
	! (N xor V), this term is automatically removed for the multiplicand,
	! so we don't have to adjust.
	!
	! But for unsigned multiplies, the high order bit wasn't a sign bit,
	! and the correction is wrong.  So for unsigned multiplies where the
	! high order bit is one, we end up with xy - (2**32) * y.  To fix it
	! we add y * (2**32).
	!
	tst	%o1
	bge	1f
	nop
	add	%o4, %o0, %o4
1:
	rd	%y, %o0			! return least sig. bits of prod
	jmp	%o7+8			! leaf routine return
	addcc	%o4, %g0, %o1		! delay slot; return high bits and set
					! zero bit appropriately
	!
	! short multiply
	!
umul_shortway:
	mulscc	%o4, %o1, %o4		! first iteration of 13
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4		! 12th iteration
	mulscc	%o4, %g0, %o4		! last iteration only shifts

	rd	%y, %o5
	sll	%o4, 12, %o4		! left shift partial product
					! by 12 bits
	srl	%o5, 20, %o5		! right shift product 20 bits
	or	%o5, %o4, %o0		! merge for true product
	!
	! The delay instruction moves zero into %o1,
	! sets the zero condition code, and clears the other conditions.
	! This is the equivalent result to a long umultiply which doesn't
	! overflow
	!
	jmp	%o7+8			! leaf routine return
	addcc	%g0, %g0, %o1

/*
 * divison/remainder
 *
 * Input is:
 *	dividend -- the thing being divided
 * divisor  -- how many ways to divide
 * Important parameters:
 *	N -- how many bits per iteration we try to get
 *		as our current guess:
 *	WORDSIZE -- how many bits altogether we're talking about:
 *		obviously:
 * A derived constant:
 *	TOPBITS -- how many bits are in the top "decade" of a number:
 *
 * Important variables are:
 *	Q -- the partial quotient under development -- initally 0
 *	R -- the remainder so far -- initially == the dividend
 *	ITER -- number of iterations of the main division loop will
 *		be required. Equal to CEIL( lg2(quotient)/4 )
 *		Note that this is log_base_(2^4) of the quotient.
 *	V -- the current comparand -- initially divisor*2^(ITER*4-1)
 * Cost:
 *	current estimate for non-large dividend is
 *		CEIL( lg2(quotient) / 4 ) x ( 10 + 74/2 ) + C
 *	a large dividend is one greater than 2^(31-4 ) and takes a
 *	different path, as the upper bits of the quotient must be developed
 *	one bit at a time.
 */
	RTENTRY(.udiv)		! UNSIGNED DIVIDE
	b	divide
	mov	0,%g1		! result always positive

	RTENTRY(.div)		! SIGNED DIVIDE
	orcc	%o1,%o0,%g0 ! are either %o0 or %o1 negative
	bge	divide		! if not, skip this junk
	xor	%o1,%o0,%g1 ! record sign of result in sign of %g1
		tst	%o1
		bge	2f
		tst	%o0
	!	%o1 < 0
		bge	divide
		neg	%o1
	2:
	!	%o0 < 0
		neg	%o0
	!	FALL THROUGH

divide:
!	compute size of quotient, scale comparand
	orcc	%o1,%g0,%o5	! movcc	%o1,%o5
	te	ST_DIV0		! if %o1 = 0
	mov	%o0,%o3
	cmp	%o3,%o5
	blu	got_result ! if %o3<%o5 already, there's no point in continuing
	mov	0,%o2
	sethi	%hi(1<<(32-4 -1)),%g2
	cmp	%o3,%g2
	blu	not_really_big
	mov	0,%o4
	!
	! here, the %o0 is >= 2^(31-4) or so. We must be careful here, as
	! our usual 4-at-a-shot divide step will cause overflow and havoc. The
	! total number of bits in the result here is 4*%o4+%g3, where %g3 <= 4.
	! compute %o4, in an unorthodox manner: know we need to Shift %o5 into
	!	the top decade: so don't even bother to compare to %o3.
	1:
		cmp	%o5,%g2
		bgeu	3f
		mov	1,%g3
		sll	%o5,4,%o5
		b	1b
		inc	%o4
	! now compute %g3
	2:	addcc	%o5,%o5,%o5
		bcc	not_too_big ! bcc	not_too_big
		add	%g3,1,%g3
			!
			! here if the %o1 overflowed when Shifting
			! this means that %o3 has the high-order bit set
			! restore %o5 and subtract from %o3
			sll	%g2,4 ,%g2 ! high order bit
			srl	%o5,1,%o5 ! rest of %o5
			add	%o5,%g2,%o5
			b	do_single_div
			sub	%g3,1,%g3
	not_too_big:
	3:	cmp	%o5,%o3
		blu	2b
		nop
		be	do_single_div
		nop
	! %o5 > %o3: went too far: back up 1 step
	!	srl	%o5,1,%o5
	!	dec	%g3
	! do single-bit divide steps
	!
	! we have to be careful here. We know that %o3 >= %o5, so we can do the
	! first divide step without thinking. BUT, the others are conditional,
	! and are only done if %o3 >= 0. Because both %o3 and %o5 may have the high-
	! order bit set in the first step, just falling into the regular
	! division loop will mess up the first time around.
	! So we unroll slightly...
	do_single_div:
		deccc	%g3
		bl	end_regular_divide
		nop
		sub	%o3,%o5,%o3
		mov	1,%o2
		b,a	end_single_divloop
	single_divloop:
		sll	%o2,1,%o2
		bl	1f
		srl	%o5,1,%o5
		! %o3 >= 0
		sub	%o3,%o5,%o3
		b	2f
		inc	%o2
	1:	! %o3 < 0
		add	%o3,%o5,%o3
		dec	%o2
	2:
	end_single_divloop:
		deccc	%g3
		bge	single_divloop
		tst	%o3
		b,a	end_regular_divide

not_really_big:
1:
	sll	%o5,4,%o5
	cmp	%o5,%o3
	bleu	1b
	inccc	%o4
	be	got_result
	dec	%o4
do_regular_divide:

!	do the main division iteration
	tst	%o3
!	fall through into divide loop
divloop:
	sll	%o2,4,%o2
		!depth 1, accumulated bits 0
	bl	L.1.16
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
			!depth 2, accumulated bits 1
	bl	L.2.17
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
			!depth 3, accumulated bits 3
	bl	L.3.19
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
			!depth 4, accumulated bits 7
	bl	L.4.23
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
		b	9f
		add	%o2, (7*2+1), %o2

L.4.23:	! remainder is negative
	addcc	%o3,%o5,%o3
		b	9f
		add	%o2, (7*2-1), %o2

L.3.19:	! remainder is negative
	addcc	%o3,%o5,%o3
			!depth 4, accumulated bits 5
	bl	L.4.21
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
		b	9f
		add	%o2, (5*2+1), %o2

L.4.21:	! remainder is negative
	addcc	%o3,%o5,%o3
		b	9f
		add	%o2, (5*2-1), %o2

L.2.17:	! remainder is negative
	addcc	%o3,%o5,%o3
			!depth 3, accumulated bits 1
	bl	L.3.17
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
			!depth 4, accumulated bits 3
	bl	L.4.19
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
		b	9f
		add	%o2, (3*2+1), %o2

L.4.19:	! remainder is negative
	addcc	%o3,%o5,%o3
		b	9f
		add	%o2, (3*2-1), %o2

L.3.17:	! remainder is negative
	addcc	%o3,%o5,%o3
			!depth 4, accumulated bits 1
	bl	L.4.17
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
		b	9f
		add	%o2, (1*2+1), %o2

L.4.17:	! remainder is negative
	addcc	%o3,%o5,%o3
		b	9f
		add	%o2, (1*2-1), %o2

L.1.16:	! remainder is negative
	addcc	%o3,%o5,%o3
			!depth 2, accumulated bits -1
	bl	L.2.15
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
			!depth 3, accumulated bits -1
	bl	L.3.15
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
			!depth 4, accumulated bits -1
	bl	L.4.15
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
		b	9f
		add	%o2, (-1*2+1), %o2

L.4.15:	! remainder is negative
	addcc	%o3,%o5,%o3
		b	9f
		add	%o2, (-1*2-1), %o2

L.3.15:	! remainder is negative
	addcc	%o3,%o5,%o3
			!depth 4, accumulated bits -3
	bl	L.4.13
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
		b	9f
		add	%o2, (-3*2+1), %o2

L.4.13:	! remainder is negative
	addcc	%o3,%o5,%o3
		b	9f
		add	%o2, (-3*2-1), %o2

L.2.15:	! remainder is negative
	addcc	%o3,%o5,%o3
			!depth 3, accumulated bits -3
	bl	L.3.13
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
			!depth 4, accumulated bits -5
	bl	L.4.11
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
		b	9f
		add	%o2, (-5*2+1), %o2

L.4.11:	! remainder is negative
	addcc	%o3,%o5,%o3
		b	9f
		add	%o2, (-5*2-1), %o2

L.3.13:	! remainder is negative
	addcc	%o3,%o5,%o3
			!depth 4, accumulated bits -7
	bl	L.4.9
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
		b	9f
		add	%o2, (-7*2+1), %o2

L.4.9:	! remainder is negative
	addcc	%o3,%o5,%o3
		b	9f
		add	%o2, (-7*2-1), %o2
	9:

end_regular_divide:
	deccc	%o4
	bge	divloop
	tst	%o3
	bl,a	got_result
	dec	%o2

got_result:
	tst	%g1
	bl,a	1f
	neg	%o2	! quotient  <- -%o2
1:
	retl
	mov	%o2,%o0	! quotient  <-  %o2


	RTENTRY(.urem)		! UNSIGNED REMAINDER
	b	rem
	mov	0,%g1		! result always positive

	RTENTRY(.rem)		! SIGNED REMAINDER
	orcc	%o1,%o0,%g0 ! are either %o0 or %o1 negative
	bge	rem		! if not, skip this junk
	mov	%o0,%g1	! record sign of result in sign of %g1
		tst	%o1
		bge	2f
		tst	%o0
	!	%o1 < 0
		bge	rem
		neg	%o1
	2:
	!	%o0 < 0
		neg	%o0
	!	FALL THROUGH

rem:
!	compute size of quotient, scale comparand
	orcc	%o1,%g0,%o5	! movcc	%o1,%o5
	te	ST_DIV0		! if %o1 = 0
	mov	%o0,%o3
	cmp	%o3,%o5
	blu	rgot_result ! if %o3<%o5 already, there's no point in continuing
	mov	0,%o2
	sethi	%hi(1<<(32-4 -1)),%g2
	cmp	%o3,%g2
	blu	rnot_really_big
	mov	0,%o4
	!
	! here, the %o0 is >= 2^(31-4) or so. We must be careful here, as
	! our usual 4-at-a-shot divide step will cause overflow and havoc. The
	! total number of bits in the result here is 4*%o4+%g3, where %g3 <= 4.
	! compute %o4, in an unorthodox manner: know we need to Shift %o5 into
	!	the top decade: so don't even bother to compare to %o3.
	1:
		cmp	%o5,%g2
		bgeu	3f
		mov	1,%g3
		sll	%o5,4,%o5
		b	1b
		inc	%o4
	! now compute %g3
	2:	addcc	%o5,%o5,%o5
		bcc	rnot_too_big ! bcc	rnot_too_big
		add	%g3,1,%g3
			!
			! here if the %o1 overflowed when Shifting
			! this means that %o3 has the high-order bit set
			! restore %o5 and subtract from %o3
			sll	%g2,4 ,%g2 ! high order bit
			srl	%o5,1,%o5 ! rest of %o5
			add	%o5,%g2,%o5
			b	do_single_rem
			sub	%g3,1,%g3
	rnot_too_big:
	3:	cmp	%o5,%o3
		blu	2b
		nop
		be	do_single_rem
		nop
	! %o5 > %o3: went too far: back up 1 step
	!	srl	%o5,1,%o5
	!	dec	%g3
	! do single-bit divide steps
	!
	! we have to be careful here. We know that %o3 >= %o5, so we can do the
	! first divide step without thinking. BUT, the others are conditional,
	! and are only done if %o3 >= 0. Because both %o3 and %o5 may have the high-
	! order bit set in the first step, just falling into the regular
	! division loop will mess up the first time around.
	! So we unroll slightly...
	do_single_rem:
		deccc	%g3
		bl	end_regular_remainder
		nop
		sub	%o3,%o5,%o3
		mov	1,%o2
		b,a	end_single_remloop
	single_remloop:
		sll	%o2,1,%o2
		bl	1f
		srl	%o5,1,%o5
		! %o3 >= 0
		sub	%o3,%o5,%o3
		b	2f
		inc	%o2
	1:	! %o3 < 0
		add	%o3,%o5,%o3
		dec	%o2
	2:
	end_single_remloop:
		deccc	%g3
		bge	single_remloop
		tst	%o3
		b,a	end_regular_remainder

rnot_really_big:
1:
	sll	%o5,4,%o5
	cmp	%o5,%o3
	bleu	1b
	inccc	%o4
	be	rgot_result
	dec	%o4
do_regular_remainder:

!	do the main division iteration
	tst	%o3
!	fall through into divide loop
remloop:
	sll	%o2,4,%o2
		!depth 1, accumulated bits 0
	bl	Lr.1.16
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
			!depth 2, accumulated bits 1
	bl	Lr.2.17
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
			!depth 3, accumulated bits 3
	bl	Lr.3.19
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
			!depth 4, accumulated bits 7
	bl	Lr.4.23
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
		b	9f
		add	%o2, (7*2+1), %o2

Lr.4.23:	! remainder is negative
	addcc	%o3,%o5,%o3
		b	9f
		add	%o2, (7*2-1), %o2

Lr.3.19:	! remainder is negative
	addcc	%o3,%o5,%o3
			!depth 4, accumulated bits 5
	bl	Lr.4.21
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
		b	9f
		add	%o2, (5*2+1), %o2

Lr.4.21:	! remainder is negative
	addcc	%o3,%o5,%o3
		b	9f
		add	%o2, (5*2-1), %o2

Lr.2.17:	! remainder is negative
	addcc	%o3,%o5,%o3
			!depth 3, accumulated bits 1
	bl	Lr.3.17
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
			!depth 4, accumulated bits 3
	bl	Lr.4.19
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
		b	9f
		add	%o2, (3*2+1), %o2

Lr.4.19:	! remainder is negative
	addcc	%o3,%o5,%o3
		b	9f
		add	%o2, (3*2-1), %o2

Lr.3.17:	! remainder is negative
	addcc	%o3,%o5,%o3
			!depth 4, accumulated bits 1
	bl	Lr.4.17
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
		b	9f
		add	%o2, (1*2+1), %o2

Lr.4.17:	! remainder is negative
	addcc	%o3,%o5,%o3
		b	9f
		add	%o2, (1*2-1), %o2

Lr.1.16:	! remainder is negative
	addcc	%o3,%o5,%o3
			!depth 2, accumulated bits -1
	bl	Lr.2.15
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
			!depth 3, accumulated bits -1
	bl	Lr.3.15
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
			!depth 4, accumulated bits -1
	bl	Lr.4.15
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
		b	9f
		add	%o2, (-1*2+1), %o2

Lr.4.15:	! remainder is negative
	addcc	%o3,%o5,%o3
		b	9f
		add	%o2, (-1*2-1), %o2

Lr.3.15:	! remainder is negative
	addcc	%o3,%o5,%o3
			!depth 4, accumulated bits -3
	bl	Lr.4.13
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
		b	9f
		add	%o2, (-3*2+1), %o2

Lr.4.13:	! remainder is negative
	addcc	%o3,%o5,%o3
		b	9f
		add	%o2, (-3*2-1), %o2

Lr.2.15:	! remainder is negative
	addcc	%o3,%o5,%o3
			!depth 3, accumulated bits -3
	bl	Lr.3.13
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
			!depth 4, accumulated bits -5
	bl	Lr.4.11
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
		b	9f
		add	%o2, (-5*2+1), %o2

Lr.4.11:	! remainder is negative
	addcc	%o3,%o5,%o3
		b	9f
		add	%o2, (-5*2-1), %o2

Lr.3.13:	! remainder is negative
	addcc	%o3,%o5,%o3
			!depth 4, accumulated bits -7
	bl	Lr.4.9
	srl	%o5,1,%o5
	! remainder is positive
	subcc	%o3,%o5,%o3
		b	9f
		add	%o2, (-7*2+1), %o2

Lr.4.9:	! remainder is negative
	addcc	%o3,%o5,%o3
		b	9f
		add	%o2, (-7*2-1), %o2
	9:

end_regular_remainder:
	deccc	%o4
	bge	remloop
	tst	%o3
	bl,a	rgot_result
	add	%o3,%o1,%o3

rgot_result:
	tst	%g1
	bl,a	1f
	neg	%o3	! remainder <- -%o3
1:
	retl
	mov	%o3,%o0	! remainder <-  %o3

/*
 * Structure return
 */
#define UNIMP		0
#define MASK		0x00000fff
#define STRUCT_VAL_OFF	(16*4)

	RTENTRY(.stret4)
	RTENTRY(.stret8)
	!
	! see if key matches: if not, structure value not expected,
	! so just return
	!
	ld	[%i7 + 8], %o3
	and	%o1, MASK, %o4
	sethi	%hi(UNIMP), %o5
	or	%o4, %o5, %o5
	cmp	%o5, %o3
	be,a	0f
	ld	[%fp + STRUCT_VAL_OFF], %i0	! set expected return value
	ret
	restore
0:						! copy the struct
	subcc	%o1, 4, %o1
	ld	[%o0 + %o1], %o4
	bg	0b
	st	%o4, [%i0 + %o1]		! delay slot
	add	%i7, 0x4, %i7			! bump return address
	ret
	restore

	RTENTRY(.stret2)
	!
	! see if key matches: if not, structure value not expected,
	! so just return
	!
	ld	[%i7 + 8], %o3
	and	%o1, MASK, %o4
	sethi	%hi(UNIMP), %o5
	or	%o4, %o5, %o5
	cmp	%o5, %o3
	be,a	0f
	ld	[%fp + STRUCT_VAL_OFF], %i0	! set expected return value
	ret
	restore
0:						! copy the struct
	subcc	%o1, 2, %o1
	lduh	[%o0 + %o1], %o4
	bg	0b
	sth	%o4, [%i0 + %o1]		! delay slot
	add	%i7, 0x4, %i7			! bump return address
	ret
	restore

/*
 * integer multiply  __ip_umul, __ip_mul, __ip_umulcc, __ip_mulcc
 * input: 	%i0 = rs1
 *		%i1 = rs2 or simm13
 *		%i2 = address for rd
 *		%i3 = address for Y-register
 *		%i4 = address for psr (or icc)
 * 
 * perform a (signed or unsigned) multiplication of a 32 bit multiplier 
 * %i0 and a 32 bit multiplicand %i1 resulting in a 64-bit product. The
 * low order 32-bits of the multiply are placed in [%i2], and the
 * upper 32-bits of the multiply are placed in [%i3]. The condition
 * codes are set as follows:
 * unsigned:  	N	set if product<31> is set
 *		Z	set if product equals zero
 *		V	set if product <63:32> is not zero
 *		C	always cleared
 * signed:  	N	set if product<31> is set
 *		Z	set if product equals zero
 *		V	set if product <63:32> is not product<31>
 *		C	always cleared
 *
 * Traps: (none)
 */
	ENTRY(_ip_umul)
	save	%sp,-96,%sp
	mov	%i0,%o0
	call	.umul
	mov	%i1,%o1
	st	%o0,[%i2]
	st	%o1,[%i3]
	mov	1, %i0		! success
	ret
	restore

	ENTRY(_ip_mul)
	save	%sp,-96,%sp
	mov	%i0,%o0
	call	.mul
	mov	%i1,%o1
	st	%o0,[%i2]
	st	%o1,[%i3]
	mov	1, %i0		! success
	ret
	restore

	ENTRY(_ip_umulcc)
	save	%sp,-96,%sp
	mov	%i0,%o0
	call	.umul
	mov	%i1,%o1
	st	%o0,[%i2]
	st	%o1,[%i3]

unsigned_ccret:
	mov	0,%l0		! clear NZVC
	mov	0,%l2
unsigned_ccret1:
	srl	%o0,31,%l1	! %l1 = product<31>
	sll	%l1,3,%l0	! N = product<31>
	or	%l2,%l0,%l0	! set V bit if set in %l2 earlier
	tst	%o0
	be,a	1f
	or	%l0,0x4,%l0	! set Z if product[31:0] = 0
1:
	b	3f		! use common code in _ip_mulcc
	sll	%l0,20,%l0	! shift to the icc position in psr

	ENTRY(_ip_mulcc)
	save	%sp,-96,%sp
	mov	%i0,%o0
	call	.mul
	mov	%i1,%o1
	st	%o0,[%i2]
	st	%o1,[%i3]

signed_ccret:
	mov	0,%l0		! clear NZVC
	mov	0,%l2
signed_ccret1:
	srl	%o0,31,%l1	! %l1 = result<31>
	sll	%l1,3,%l0	! N = result<31>
	or	%l2,%l0,%l0	! set V bit if set in %l2 earlier
	tst	%o0
	be,a	1f
	or	%l0,0x4,%l0	! set Z if result[31:0] = 0
1:
	sll	%l0,20,%l0	! shift to the icc position in psr
3:
	ld	[%i4],%l1
	set	0x00f00000,%l2
	andn	%l1,%l2,%l1	! clear icc field
	or	%l1,%l0,%l1	! put in icc
	st	%l1,[%i4]	! store to psr
	mov	1, %i0		! success
	ret
	restore

/*
 * integer divide  __ip_udiv, __ip_div, __ip_udivcc, __ip_divcc
 * input: 	%i0 = rs1			-- lower dividend
 *		%i1 = rs2 or simm13		-- divisor
 *		%i2 = address for rd		-- quotient (lower 32-bits)
 *		%i3 = address for Y-register	-- upper dividend
 *		%i4 = address for psr (or icc)
 *
 * When return, %i1 will also contain the upper 32-bits quotient
 * 
 * perform a (signed or unsigned) division of a 64 bit dividend (lower
 * 32-bits in %i0 and upper 32-bits in [%i3]) and a 32 bit divisor %i1 
 * resulting in a 32-bit quotient [%i2]. Overflow is set if the quotient
 * cannot be represented in 32-bits. Here we will put the upper 32-bits
 * quotient in %i1 when return.
 * The condition codes are set as follows:
 * 	  	N	set if MSB of quotient is set
 *		Z	set if quotient equals zero
 *		V	set if division overflow
 *		C	always cleared
 *
 * Traps: division by zero
 */
	ENTRY(_ip_udiv)
	tst	%o1
	bnz,a	1f
	save	%sp,-96,%sp
	b,a	div_by_zero
1:	mov	%i0,%o0
	ld	[%i3],%o1
	call	long_udiv
	mov	%i1,%o2
/* At this point, %o0 ls quotient
 *                %o1 ms quotient
 *                %o2 remainder
 * We want to adjust the quotient as defined by the sparc arch.
 */
        tst     %o1     ! Is result > 2^32-1
        bz,a    divout  ! no, so it has not overflow
        mov     %o1,%i1
        clr     %o0     ! overflow - set result to 0xffffffff
        not     %o0
	b 	divout
	mov	%o1,%i1

	ENTRY(_ip_div)
	tst	%o1
	bnz,a	1f
	save	%sp,-96,%sp
	b,a	div_by_zero
1:	mov	%i0,%o0
	ld	[%i3],%o1
	call	long_div
	mov	%i1,%o2

	tst	%o1		!Is positive
	bge,a	pos_check_overflow
	nop
				! negative
	mov	0xffffffff, %l1
	cmp	%l1, %o1
	bne,a	neg_overflow	! negative overflow 
	nop
	set	0x80000000, %l1 ! -2^31
	tst	%o0
	bneg,a	divout
	clr	%i1
neg_overflow:
	set	0x80000000, %l1
	mov	%l1, %o0
	b	divout
	clr	%i1
	

pos_check_overflow:
	tst	%o1
	bnz	pos_overflow 	! overflow
	nop
				! check if greater than 0x7ffffff
	set	0x7fffffff, %l1 ! 2^31
	cmp	%o0,%l1
	bleu,a 	divout		! not overflow
	mov 	%o1,%i1
pos_overflow:
	set	0x7fffffff, %o0 ! return 0x7ffffff
	b	divout
	clr 	%i1


divout:
	!st	%o0,[%i2]
	mov	%o0, %o1
	call	_suword
	mov	%i2, %o0
	tst	%o0
	bz	2f	
	mov	1, %i0		! success
	mov	-1, %i0		! failure
2:	
	ret
	restore

	ENTRY(_ip_udivcc)
	tst	%o1
	bnz,a	1f
	save	%sp,-96,%sp
	b,a	div_by_zero
1:	mov	%i0,%o0
	ld	[%i3],%o1
	call	long_udiv
	mov	%i1,%o2
/* At this point, %o0 ls quotient
 *                %o1 ms quotient
 *                %o2 remainder
 * We want to adjust the quotient as defined by the sparc arch.
 */
	clr 	%l2	! clear NZVC before hand
        tst     %o1     ! Is result > 2^32-1
        bz,a    udivout  ! no, so it has not overflow
        mov     %o1,%i1
        clr     %o0     ! overflow - set result to 0xffffffff
        not     %o0
	or	%l2, 0x2,%l2	!set V bit
	
	mov	%o1,%i1
udivout:
	!st	%o0,[%i2]
	mov	%o0, %l3	! save quotient
	mov	%o0, %o1
	call	_suword
	mov	%i2, %o0
	
	mov	%o0, %l4	! move result from suword 
	mov 	%l3, %o0	! restore quotent for signed_ccret1
	tst	%l4
	bz	unsigned_ccret1
	mov	1, %i0		! success
	mov	-1, %i0		! failure
	ret
	restore

	ENTRY(_ip_divcc)
	tst	%o1
	bnz,a	1f
	save	%sp,-96,%sp
	b,a	div_by_zero
1:	mov	%i0,%o0
	ld	[%i3],%o1
	call	long_div
	mov	%i1,%o2
	clr	%l2
 	tst     %o1             !Is positive 
        bge,a   ccpos_check_overflow 
	nop
				! negative
	mov	0xffffffff, %l1
	cmp	%l1, %o1
	bne,a	ccneg_overflow	! negative overflow 
	nop
        set     0x80000000, %l1 ! -2^31 
	tst	%o0
        bneg,a  ccdivout
        clr     %i1 
ccneg_overflow:
	or	%l2, 0x2,%l2	!set V bit
	set	0x80000000, %l1
        mov     %l1, %o0
        b       ccdivout
        clr     %i1 
         
 
ccpos_check_overflow: 
	tst	%o1
	bnz	ccpos_overflow
	nop
        set     0x7fffffff, %l1 ! 2^31 
        cmp     %o0,%l1
        bleu,a   ccdivout          ! not overflow
        mov     %o1,%i1 
ccpos_overflow:
	or	%l2, 0x2,%l2	!set V bit
        set     0x7fffffff, %o0 ! return 0x7ffffff 
        b       ccdivout 
        clr     %i1
	
	!mov	%o1,%i1

	!st	%o0,[%i2]
ccdivout:
	mov	%o0, %l3	! save quotient
	mov	%o0, %o1
	call	_suword
	mov	%i2, %o0
	mov	%o0, %l4	! move result from suword 
	mov 	%l3, %o0	! restore quotent for signed_ccret1
	tst	%l4
	bz	signed_ccret1
	mov	1, %i0		! success
	mov	-1, %i0		! failure
	ret
	restore

div_by_zero:
	retl
	mov	-2, %o0		! dvivide by zero detected, see trap.c

/* 
 * long_udiv(L,H,D)	unsigned 64/32 bits divided
 * long_div (L,H,D)	  signed 64/32 bits divided
 * int L,H,D;
 *
 * Input: 
 *	L = %i0 -- least 32 bits of dividend 
 *	H = %i1 -- most  32 bits of dividend 
 *	D = %i2 -- divisor
 * Output:
 *	%i0 -- least 32 bits of quotient
 *	%i1 -- most  32 bits of quotient
 *	%i2 -- remainder
 *
 * local register usage:
 *	%i0	least 32 bits dividend
 * 	%i1	most  32 bits dividend
 *	%i2	divisor
 *	%i3	&QL
 *	%i4	&QH
 *	%i5	&R
 *	%l0	least 32 bits quotient
 *	%l1	most  32 bits quotient
 *	%l2	remainder
 *	%l3	sign
 *	%l4	counter
 *
 * undocumented usage of .udiv:
 *	.udiv return quotient in o0, remainder in o3; if o3 is
 *	negative, then replace o3 by o3 + divisor to get the correct
 *	remainder
 */
long_udiv:
	save	%sp,-0x100,%sp
	b	ldivide
	mov	0,%l3		! sign is positive

long_div:
	save	%sp,-0x100,%sp
	orcc	%i2,%i1,%g0	! dividend or divisor negative ?
	bge	ldivide		! if not, skip this
	xor	%i2,%i1,%l3	! record sign of result
	tst	%i2
	bge	2f
	tst	%i1
    ! divisor < 0
	bge	ldivide
	neg	%i2
2:
    ! dividend < 0
	subcc	%g0,%i0,%i0
	b	ldivide
	subx	%g0,%i1,%i1
	

ldivide:
	cmp	%i2,%i1		! divisor 
	bgu,a	leastquo	! if divisor > dividendh then goto leastquo
	mov	0,%l1		! and set quotienth = 0.
    ! quotienth is non-zero
	mov	%i1,%o0
	call	.udiv		! quotienth = dividendh/divisor
	mov	%i2,%o1
	mov	%o0,%l1		! store result in quotienth
	tst	%o3
	bge,a	leastquo
	mov	%o3,%i1		! %o2 is the remainder, set %i1 = %o2
	add	%o3,%i2,%i1
leastquo:
    ! computing quotientl, the least sig. 32 bits of the quotient	
	tst	%i1		! if 0, then quotientl=dividendl/divisor
	bne,a	1f
	mov	0,%l0		! initialize quotientl
	mov	%i0,%o0
	call	.udiv
	mov	%i2,%o1
	mov	%o0,%i0
	tst	%o3
	bge,a	2f
	mov	%o3,%i2
	add	%o3,%i2,%i2
2:
	ba	endldivide
	mov	%l1,%i1
1:
	mov	32,%l4		! initialize counter
	addcc	%i0,%i0,%i0
loop:
	addxcc	%i1,%i1,%i1	! dividend << 1
	bcs	2f
	add	%l0,%l0,%l0	! quo << 1
	cmp	%i1,%i2
	bcs,a	1f		! if dividend < divisor skip inc quotient
	subcc	%l4,1,%l4	! counter -= 1
2:
	sub	%i1,%i2,%i1	! dividendh -= divisor
	inc	%l0		! quo += 1 when divisor < dividendh 
	subcc	%l4,1,%l4	! counter -= 1
1:
	bg,a	loop
	addcc	%i0,%i0,%i0
	mov	%i1,%i2		! remainder in %i2
	mov	%l1,%i1		! most  32 bits of quotient in %i1
	mov	%l0,%i0		! least 32 bits of quotient in %i0
endldivide:
	tst	%l3		! check sign
	bge	1f
	nop
	neg	%i2
	subcc	%g0,%i0,%i0
	subx	%g0,%i1,%i1
1:
    ! store result to QL,QH and R
! mask off 
!	st	%i0,[%i3]
!	st	%i1,[%i4]
!	st	%i2,[%i5]
	ret
	restore


/*
 * Multiply 'has to' run fast on machines that don't have hardware
 * multiply so when the unimplemented instruction trap is taken we 
 * jump here and do a quick check to see if it is a multiply.
 * If so, do it inline and return, otherwise backoff and
 * go through the generic trap handler.
 * We arrive here from the trap vector with:
#ifndef	sun4m
 * %l0 = psr, %l3 = wim, %l6 = nwindows-1
#else	sun4m
 * %l0 = psr, %l1 = pc, %l2 = npc
 * The SCB no longer has room to set up %l3 and %l6,
 * so if we want either we must load them ourselves.
#endif	sun4m
 */
	.global	multiply_check
multiply_check:
#ifdef	sun4m
	mov	%wim, %l3
#endif
	/* check if we have a register window */
	/* if not, we loose; fall back to slow way */
	mov	0x01, %l5		! CWM = 0x01 << CWP
	sll	%l5, %l0, %l5
	btst	%l5, %l3		! compare WIM and CWM
	bnz	sys_trap		! must do inline overflow, abort
	mov	T_UNIMP_INSTR, %l4	! trap number for sys_trap

	ld	[%l1], %l4		! get illegal instruction
	srl	%l4, 30, %l7		! check op field of instruction
	cmp	%l7, 2			! must be format 3, op field = 2
	bne,a	sys_trap
	mov	T_UNIMP_INSTR, %l4	! trap number for sys_trap

	set	mulchksave, %l7		! save area for scratch registers
	std	%g2, [%l7]
	st	%g1, [%l7+8]

	srl	%l4, 13, %l3		! check if using immediate
	andcc	%l3, 1, %l3
	bz	1f			! if zero get register
					! for second operand
	sll	%l4, 19, %o1		! get the signed immediate
	b	2f			! for second operand rs2
	sra	%o1, 19, %o1
1:
	call	getrealreg		! get src register rs2
	and	%l4, 0x1f, %o0
	mov	%o0, %o1
2:
	srl	%l4, 14, %o0		! get src register rs1
	call	getrealreg
	and	%o0, 0x1f, %o0

	/* at this point o0 = rs1 and o1 = rs2 or simm13 */
	srl	%l4, 19, %l5		! check op3 field of instruction
	and	%l5, 0x3f, %l5
	cmp	%l5, 0xa		! is this a umul instruction?

	bne	5f
	cmp	%l5, 0xb		! is this a ssmul instruction

	/* umul */
	call	.umul
	nop
	mov	%o1, %y
	srl	%l4, 25, %o2
	call	putrealreg
	and	%o2, 0x1f, %o2
	b,a	mulchkdone
5:
	bne	5f
	cmp	%l5, 0x1a		! is this a umulcc instruction?

	/* smul */
	call	.mul
	nop
	mov	%o1, %y
	srl	%l4, 25, %o2
	call	putrealreg
	and	%o2, 0x1f, %o2
	b,a	mulchkdone
5:
	bne	5f
	cmp	%l5, 0x1b		! is this a smulcc instruction?

	/* umulcc */
	call	.umul
	nop
	mov	%o1, %y
	srl	%l4, 25, %o2
	call	putrealreg
	and	%o2, 0x1f, %o2

	/* unsigned set condition codes */
7:	srl	%o0, 31, %o4		! PSR_N = product<31>
	sll	%o4, 3, %o4
	tst	%o0			! is result 0
	bz,a	1f
	or	%o4, 0x4, %o4		! set Z if product[31:0] = 0
1:	tst	%o1			! is there overflow
	sll	%o4, 20, %o4		! shift to the icc position in psr
	set	PSR_ICC, %o3
	andn	%l0, %o3, %l0		! clear condition codes
	or	%l0, %o4, %l0		! put in icc
	b,a	mulchkdone
5:
	bne	5f			! the rest of the checks are for divides
	tst	%o1			! check second operand (divisor) == zero

	/* smulcc */
	call	.mul
	nop
	mov	%o1, %y
	srl	%l4, 25, %o2
	call	putrealreg
	and	%o2, 0x1f, %o2

	/* signed set condition codes */
8:	srl	%o0, 31, %o3		! PSR_N = product<31>
	sll	%o3, 3, %o4
	tst	%o0			! is product[31:0] = 0
	bz,a	1f
	or	%o4, 0x4, %o4		! set Z if product = 0
1:	sll	%o4, 20, %o4		! shift to the icc position in psr
	set	PSR_ICC, %o3
	andn	%l0, %o3, %l0		! clear condition codes
	or	%l0, %o4, %l0		! put in icc
	b,a	mulchkdone
5:
	set	mulchksave, %l7
	ldd	[%l7], %g2 
	ld	[%l7 + 8], %g1 
	b	sys_trap		! couldn't simulate quickly
	mov	T_UNIMP_INSTR, %l4	! trap number

mulchkdone:
	set	mulchksave, %l7
	ldd	[%l7], %g2 
	ld	[%l7 + 8], %g1 
	/*
	 * we never enabled traps, we only call leaf routines,
	 * so  we should have a window to return to
	 */
	mov	%l0, %psr		! restore condition codes
	nop				! psr delay
	jmp	%l2			! skip faulted instruction
	rett	%l2+4

/* return the register designated by %o0 */
getrealreg: 
	cmp	%o0, 15
	set	rtable, %g1
	ble	1f
	sll	%o0, 2, %g2

	restore
	jmp	%g1 + %g2		! execute instruction pair
	b	2f
	nop				! never executed
2:	save
	retl
	mov	%g1, %o0
1:
	set	mulchksave, %l7
	jmp	%g1 + %g2		! execute instruction pair
	retl
	nop
rtable:
	mov	%g0, %o0
	ld	[%l7 + 8], %o0		! these have been modified
	ld	[%l7], %o0		! get the saved copy
	mov	%g3, %o0		! note g3 is not used until later
	mov	%g4, %o0
	mov	%g5, %o0
	mov	%g6, %o0
	mov	%g7, %o0
	mov	%i0, %o0
	mov	%i1, %o0
	mov	%i2, %o0
	mov	%i3, %o0
	mov	%i4, %o0
	mov	%i5, %o0
	mov	%i6, %o0
	mov	%i7, %o0
	mov	%l0, %g1
	mov	%l1, %g1
	mov	%l2, %g1
	mov	%l3, %g1
	mov	%l4, %g1
	mov	%l5, %g1
	mov	%l6, %g1
	mov	%l7, %g1
	mov	%i0, %g1
	mov	%i1, %g1
	mov	%i2, %g1
	mov	%i3, %g1
	mov	%i4, %g1
	mov	%i5, %g1
	mov	%i6, %g1
	mov	%i7, %g1

/* write the register designated in %o1 with the contents of %o0 */
putrealreg: 
	cmp	%o2, 15
	set	wtable, %g2
	ble	1f
	sll	%o2, 2, %g3

	mov	%o0, %g1
	restore
	jmp	%g2 + %g3		! execute instruction pair
	b	2f
	nop				! never executed
2:	save
	retl
	nop
1:
	set	mulchksave, %l7
	jmp	%g2 + %g3		! execute instruction pair
	retl
	nop
wtable:
	mov	%o0, %g0 
	st	%o0, [%l7 + 8]
	st	%o0, [%l7]
	st	%o0, [%l7 + 4]
	mov	%o0, %g4
	mov	%o0, %g5
	mov	%o0, %g6
	mov	%o0, %g7
	mov	%o0, %i0
	mov	%o0, %i1
	mov	%o0, %i2
	mov	%o0, %i3
	mov	%o0, %i4
	mov	%o0, %i5
	mov	%o0, %i6
	mov	%o0, %i7
	mov	%g1, %l0
	mov	%g1, %l1
	mov	%g1, %l2
	mov	%g1, %l3
	mov	%g1, %l4
	mov	%g1, %l5
	mov	%g1, %l6
	mov	%g1, %l7
	mov	%g1, %i0
	mov	%g1, %i1
	mov	%g1, %i2
	mov	%g1, %i3
	mov	%g1, %i4
	mov	%g1, %i5
	mov	%g1, %i6
	mov	%g1, %i7

! save area for scratch register used for mulchk
	.seg	"data"
	.align	8
mulchksave:			
	.word	0	!g2
	.word	0	!g3
	.word	0	!g1



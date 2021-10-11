!	.seg	"data"
!	.asciz	"@(#)ptr_call.s 1.1 94/10/31 Copyr 1986 Sun Micro"
	.seg	"text"

/*
 * Indirect procedure call.
 *	just jump to whatever's in %g1
 */
	.global	.ptr_call
.ptr_call:
	jmp	%g1
	nop

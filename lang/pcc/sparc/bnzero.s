	.seg	"data"
	.asciz	"@(#)bnzero.s 1.1 94/10/31 Copyr 1987 Sun Micro"
	.align	4
	.text
!
! Copyright (c) 1987 by Sun Microsystems, Inc.
!

!
! bnzero(var, size) char *var; unsigned long size;
!
! Initialize block of memory to a regular, nonzero bit
! pattern. Useful for troubleshooting uninitialized variable
! bugs that lint can't detect.
!
	.seg	"text"
	.global	bnzero
bnzero:
	tst	%o1		!  if (size == 0)
	be	Done		!	return;
	andcc	%o0,3,%g0	!  if (!((unsigned)var&3)
	bne	MisalignedCase	!
	andcc	%o1,3,%g0	!    && !((unsigned)size&3)) {
	bne	MisalignedCase	! 
				!	/* aligned case - word at a time */
	set	0xaa55aa55,%o2	!	filler = 0xaa55aa55;
	sra	%o1,2,%o1	!	size /= sizeof(long);
LY1:				!
	st	%o2,[%o0]	!	do {
	deccc	%o1		!	    *((long*)var)++ = filler;
	bne	LY1		!	} while (--size != 0);
	inc	4,%o0		!
	retl			!
	nop			!  } else {
MisalignedCase:			!	/* misaligned case - byte at a time */
	mov	0xaa,%o2	! 	filler = 0xaa;
LY2:				!
	stb	%o2,[%o0]	!	do {
	inc	%o0		!	    *var++ = filler;
	deccc	%o1		!	    filler = ~filler;
	bne	LY2		!	} while (--size != 0);
	xor	%o2,-1,%o2	!
Done:
	retl			!  }
	nop

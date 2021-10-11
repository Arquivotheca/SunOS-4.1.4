!
!       "@(#)cerror.s 1.1 94/10/31"
!       Copyright (c) 1986 by Sun Microsystems, Inc.
!
! private, modified, version of cerror
!
! The idea is that every time an error occurs in a system call
! I want a special function "syserr" called.  This function will
! either print a message and exit or do nothing depending on
! defaults and use of "onsyserr".
!

	.seg	"text"

#include <machine/asm_linkage.h>
	.global cerror,_errno,__mycerror
cerror:
	save	%sp, -(SA(MINFRAME)), %sp
	sethi   %hi(_errno), %g1
	st      %i0, [%g1 + %lo(_errno)]
	call 	_syserr,0
	nop
	mov	-1, %i0
	ret
	restore
__mycerror: ! just to get this file loaded
	.word 0


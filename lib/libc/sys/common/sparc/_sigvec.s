!
!	"@(#)_sigvec.s 1.1 94/10/31"
!       Copyright (c) 1986 by Sun Microsystems, Inc.
!
	.seg	"text"

#include "SYS.h"

	.global	__sigvec
__sigvec:
	mov	SYS_sigvec, %g1
	t	0
	CERROR(o5);
	nop
	RET

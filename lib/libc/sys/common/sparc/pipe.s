!
!	"@(#)pipe.s 1.1 94/10/31"
!       Copyright (c) 1986 by Sun Microsystems, Inc.
!
	.seg	"text"

#include "SYS.h"


	ENTRY(pipe)
	mov	%o0, %o2		! save ptr to array
	mov	SYS_pipe, %g1
	t	0
	CERROR(o5);
	st	%o0, [%o2]
	st	%o1, [%o2 + 4]
	retl
	clr	%o0

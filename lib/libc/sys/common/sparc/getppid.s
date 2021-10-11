!
!	"@(#)getppid.s 1.1 94/10/31"
!       Copyright (c) 1986 by Sun Microsystems, Inc.
!
	.seg	"text"

#include "SYS.h"

	PSEUDO(getppid,getpid)
	retl			/* ppid = getppid(); */
	mov	%o1, %o0

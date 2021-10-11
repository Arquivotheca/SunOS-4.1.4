!	.seg	"data"
!	.asciz	"@(#)abs.s 1.1 94/10/31 Copyr 1987 Sun Micro"
	.seg	"text"

#include <sun4/asm_linkage.h>

	ENTRY(abs)
	tst	%o0
	bl,a	1f
	neg	%o0
1:
	retl
	nop

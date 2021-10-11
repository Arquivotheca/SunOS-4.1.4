/*
 *	.seg	"data"
 *	.asciz	"@(#)getvbr.s 1.1 94/10/31 SMI"
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#include <machine/asm_linkage.h>

/*
 * Get vector base register
 */
	ENTRY(getvbr1)
	retl
	mov	%tbr, %o0		! read trap base register

	.data
	.asciz	"@(#)cg2_bres.S 1.1 94/10/31 Copyr 1984 Sun Micro"
	.even
	.text

/*
 *	Copyright (c) 1984 by Sun Microsystems, Inc.
 */

	.text
	.globl	_cg2_bres_majx
	.globl	_cg2_bres_majy
	.globl	_cg2_vertln

majax = d7
minax = d6
count = d5
error = d4
color = d3
adr = a5
vert = a4

minor2:	addl	vert,adr
_cg2_bres_majx:		| vectors with x as major axis
major2:	movb	color,adr@+
	addw	minax,error
	dbgt	count,major2
	subw	majax,error
	tstw	count
	dblt	count,minor2
	rts


minor5: addql	#1,adr
_cg2_bres_majy:		| vectors with y as major axis
major5:	movb	color,adr@
	addl	vert,adr
	addw	minax,error
	dbgt	count,major5
	subw	majax,error
	tstw	count
	dblt	count,minor5
	rts


dy = d6		| input params for _cg2_vertln
adr = a5
vert = a4

_cg2_vertln:		| vertical vectors
1$:	movb	color,adr@
	addl	vert,adr
	dbf	dy,1$
	rts

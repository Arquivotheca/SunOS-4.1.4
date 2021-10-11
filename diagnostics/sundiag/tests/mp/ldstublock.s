/*
 *       @(#)ldstublock.s 1.1 94/10/31 Copyright(c) Sun Microsystems, Inc.
 */
	.seg	"text"
	.global	_mplock
_mplock:ldstub	[%o0],%o1
	tst	%o1
	be	out
	nop
loop:	ldub	[%o0],%o1
	tst	%o1
	be	skip
	nop
	b	loop
	nop
skip:	ba,a	_mplock
	nop
out:	retl
	nop

	.global	_mpunlk
_mpunlk:
	stb	%g0,[%o0]
	retl
	nop


|
|     @(#)stubs.s 1.1 94/10/31 Copyright (c) 1985 by Sun Microsystems, Inc.
|

	.globl	_nullsys, _xxboot, _xxprobe, _ttboot
_nullsys:
_xxprobe:
_xxboot:
_ttboot:
	movl	#-1,d0
	rts

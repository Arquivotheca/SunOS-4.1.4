	.data
	.asciz "@(#)gp1_sync.S 1.1 94/10/31 SMI";
	.even
	.text

| Copyright 1985 by Sun Microsystems, Inc.

| gp1_sync(shmem, fd)
| 	caddr_t shmem
|	int fd
|
| returns 0 if sync was successful before timeout
| otherwise, ask kernel to rescue the gp and return 1

	.text
	.globl	_gp1_sync
_gp1_sync:
	movl	sp@(4),a0
	movw	a0@,d0
	subqw	#1,d0

#ifdef mc68010
timeout = 2000000
#else
timeout = 8000000
#endif
	movl	#timeout,d1	| about an 8 second timeout
_gp1_sync_loop:
	subl	#1,d1
	beq	_gp1_sync_timeout
	cmpw	a0@(2),d0
	bpl	_gp1_sync_loop
	moveq	#0,d0
	rts

_gp1_sync_timeout:
	pea	0
	movl	sp@(12),sp@-
	jbsr	_gp1_kern_gp_restart
	addqw	#8,sp
	moveq	#1,d0
	rts
	.data

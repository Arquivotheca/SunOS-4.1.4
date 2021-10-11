	.data
	.asciz "@(#)gp1_kern_sync.s 1.1 94/10/31 SMI";
	.even
	.text

| Copyright 1985 by Sun Microsystems, Inc.

| gp1_kern_sync(shmem)
| 	caddr_t shmem
|
| returns 0 if sync was successful before timeout
| returns 1 otherwise
| this routine should only be called from the kernel

	.text
	.globl	_gp1_kern_sync
_gp1_kern_sync:
	movl	sp@(4),a0
	movw	a0@,d0
	subqw	#1,d0

#ifdef mc68010
timeout = 2000000
#else
timeout = 8000000
#endif
	movl	#timeout,d1
_gp1_kern_sync_loop:
	subl	#1,d1
	beq	_gp1_kern_sync_timeout
	cmpw	a0@(2),d0
	bpl	_gp1_kern_sync_loop

	moveq	#0,d0
	rts
_gp1_kern_sync_timeout:

	moveq	#1,d0
	rts
	.data

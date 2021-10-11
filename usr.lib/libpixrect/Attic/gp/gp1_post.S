	.data
	.asciz "@(#)gp1_post.S 1.1 94/10/31 SMI";
	.even
	.text

| Copyright 1985 by Sun Microsystems, Inc.

|gp1_post(shmem, offset, fd)
|	caddr_t shmem;
|	short offset;
|	int fd;
|
|	{
|	register short *shmem = (short *) gp1_shmem;
|	register short *postslot;
|
|	asm("_gp1_post_loop:	bset #0,a5@(15)");
|	asm("	bne _gp1_post_loop");
|	postslot = &shmem[(*shmem & 0xFF) + 8];
|	while (*postslot)
|		;
|	(*shmem)++;
|	*postslot = addr;
|	asm("clrb a5@(15)");
|	}

	.text
	.globl	_gp1_post
_gp1_post:
	movl	sp@(4),a0

#ifdef mc68010
timeout1 = 2000000
#else
timeout1 = 8000000
#endif
	movl	#timeout1,d1		| timeout loop for getting semaphore
_gp1_post_loop:
	subl	#1,d1
	beq	_gp1_post_timeout
	bset	#0,a0@(15)
	jne	_gp1_post_loop

	movw	a0@,d0
	andl	#0xff,d0
	addql	#8,d0
	asll	#1,d0
	movl	a0,a1
	addl	d0,a1

#ifdef mc68010
timeout2 = 300000
#else
timeout2 = 1200000
#endif
	movl	#timeout2,d1		| timeout loop for posting slot
L15:	
	subl	#1,d1
	beq	_gp1_post_t2
	tstw	a1@
	jne	L15

	movw	sp@(10),a1@
	addqw	#1,a0@
	clrb	a0@(15)
	moveq	#0,d0
	rts

_gp1_post_t2:				| error return if we had semaphore
	clrb	a0@(15)

_gp1_post_timeout:			| error return if we didn't
	pea	0
	movl	sp@(16),sp@-
	jbsr	_gp1_kern_gp_restart
	addqw	#8,sp
	moveq	#1,d0
	rts
	.data

	.data
	.asciz "@(#)gp1_alloc.S 1.1 94/10/31 SMI";
	.even
	.text

| Copyright 1985 by Sun Microsystems, Inc.

| gp1_alloc (gp1_shmem, nblocks, bitvector, mindev, fd)
|	caddr_t gp1_shmem; int nblocks; int *bitvector; int mindev, fd;

n = d7
mask = d7
index = d6
mindevblk = d5
fd = d4
bitvec = d1
bitpos = d0
LF13 = 24
LS13 = 0x00f8

	.text
	.globl	_gp1_alloc

_gp1_alloc:
	link	a6,#-LF13
	moveml	#LS13,sp@

	movl	a6@(24),fd
	movl	a6@(12),n
	cmpl	#1,n
	jne	multblks

oneblk:	movl	a6@(8),a0

#ifdef mc68010
timeout1 = 1500000
#else
timeout1 = 6000000
#endif
	movl	#timeout1,d3	| about a 5 second timeout to get semaphore
1$:	
	subl	#1,d3
	beq	notfound1
	bset	#0,a0@(5)	| wait until gain exclusive right to allocate
	bne	1$

#ifdef mc68010 
timeout2 = 500000
#else 
timeout2 = 2000000
#endif
	movl	#timeout2,d3	| about a 10 second timeout to allocate
2$:
	subl	#1,d3
	beq	notfound1a

	movl	a0@(10),d6	| compute bit vector representing 1K blocks
	movl	a0@(6),bitvec	| available for allocation
	eorl	d6,bitvec
	notl	bitvec

	moveq	#0x3F,mask
	moveq	#31,bitpos
	roll	#6,bitvec
	movl	bitvec,index
	andw	mask,index
	jne	found1
	moveq	#25,bitpos
	roll	#6,bitvec
	movl	bitvec,index
	andw	mask,index
	jne	found1
	moveq	#19,bitpos
	roll	#6,bitvec
	movl	bitvec,index
	andw	mask,index
	jne	found1
	moveq	#13,bitpos
	roll	#6,bitvec
	movl	bitvec,index
	andw	mask,index
	jne	found1
	jra 2$

found1:	subb	pc@(table1-.-2:b,index:w),bitpos
	moveq	#0,bitvec
	bset	bitpos,bitvec
	movw	bitpos,mindevblk
	movl	a0,a1
	addw	#528,a1
	movb	a6@(23),a1@(0,mindevblk:W)	| record the minor device
	jra	fndret

table1:	.byte	0,5,4,4,3,3,3,3
	.byte	2,2,2,2,2,2,2,2
	.byte	1,1,1,1,1,1,1,1
	.byte	1,1,1,1,1,1,1,1
	.byte	0,0,0,0,0,0,0,0
	.byte	0,0,0,0,0,0,0,0
	.byte	0,0,0,0,0,0,0,0
tablem:	.byte	0,0,0,0,0,0,0,0
	.long	find2,find3,find4,find5
	.long	find6,find7,find8

multblks:
	cmpl	#8,n
	jhi	L00

	movl	a6@(8),a0

1$:	bset	#0,a0@(5)	| wait until gain exclusive right to allocate
	bne	1$

	movw	n,index		| compute label to jump to in loop below
	addw	index,index
	addw	index,index
	movl	pc@(tablem-.-2:b,index:w),a1

	movl	a0@(10),d6	| compute bit vector representing 1K blocks
	movl	a0@(6),bitvec	| available for allocation
	eorl	d6,bitvec
	notl	bitvec		| now 1's represent available blocks
	bset	#5,bitvec	| bits 0-7 are now 0's, but set
	bset	#6,bitvec	| bits 5 and 6 to insure termination

	moveq	#31,bitpos	| start at bit 30 (after next subtract)

restart:
	subqw	#1,bitpos
	btst	bitpos,bitvec
	jeq	restart
	subqw	#1,bitpos
	btst	bitpos,bitvec
	jeq	restart
	cmpw	#5,bitpos
	jeq	notfoundm
	jmp	a1@
find8:	subqw	#1,bitpos
	btst	bitpos,bitvec
	jeq	restart
find7:	subqw	#1,bitpos
	btst	bitpos,bitvec
	jeq	restart
find6:	subqw	#1,bitpos
	btst	bitpos,bitvec
	jeq	restart
find5:	subqw	#1,bitpos
	btst	bitpos,bitvec
	jeq	restart
find4:	subqw	#1,bitpos
	btst	bitpos,bitvec
	jeq	restart
find3:	subqw	#1,bitpos
	btst	bitpos,bitvec
	jeq	restart
find2:
found:	moveq	#0,d6
	bset	bitpos,d6
	movw	bitpos,mindevblk
	addw	n,bitpos
	moveq	#0,bitvec
	bset	bitpos,bitvec
	subl	d6,bitvec
	subqw	#1,bitpos
	movl	a0,a1
	addl	#528,a1
	addw	mindevblk,a1
recordmindev:			| record the minor device in all allocated blocks
	movb	a6@(23),a1@+
	subqw	#1,n
	jeq	fndret
	jra	recordmindev

fndret:	eorl	bitvec,a0@(6)
	clrb	a0@(5)		| clear test and set flag
	movl	a6@(16),a0	| bitvecptr
	movl	bitvec,a0@
	negw	bitpos
	addw	#31,bitpos
	moveq	#9,d6
	asll	d6,bitpos

Lret:	moveml	a6@(-LF13),#LS13
	unlk	a6
	rts

notfound1a:
	clrb	a0@(5)		| reset semaphore before exitting
notfound1:
	pea	1
	movl	fd, sp@-
	jbsr	_gp1_kern_gp_restart
	addqw	#8,sp
        moveq   #0,d0           | error return
	jra	Lret
 
notfoundm:
	clrb	a0@(5)		| clear test and set flag
	moveq	#0,d0		| error return
	jra	Lret

L00:	moveq	#0,d0		| return value
	jra	Lret

	.data

/* @(#)gp1_shmem.IL 1.1 94/10/31 SMI */

/*
 * Copyright 1987 by Sun Microsystems,  Inc.
 */

/* GP inline functions */

/* nominal 1 second timeout constant */
#ifdef mc68010
#define	GP1_TIMECONST		( 250 * 1000)
#endif
#ifdef sparc
#define	GP1_TIMECONST		(2000 * 1000)
#endif
#ifndef GP1_TIMECONST
#define	GP1_TIMECONST		(1000 * 1000)
#endif

#if 0
/*
 * gp1_lock -- acquire GP semaphore
 */
static
_gp1_lock(sem)
	register u_char *sem;
{
	register u_char semval;
	register long count = GP1_TIMECONST * 8;

	do {
		semval = *sem;
		*sem = 1;
		if (semval == 0)
			return 0;
	} while (--count >= 0);

	return -1;
}
#endif 0

#ifdef mc68000
/*
 * a0 = sem
 * d0 = semval
 * d1 = count
 */
	.inline	__gp1_lock,4
	movl	sp@+, a0
	movl	#(GP1_TIMECONST * 8), d1
	moveq	#0, d0
1:	bset	#0, a0@
	beq	2f
	subql	#1, d1
	bge	1b
	notl	d0
2:
	.end
#endif mc68000

#ifdef sparc
/*
 * %o0 = sem
 * %o1 = semval
 * %o2 = count
 */
	.inline	__gp1_lock,4
	sethi	%hi(GP1_TIMECONST * 8), %o2
	ldstub	[%o0], %o1
1:	tst	%o1
	bz,a	2f
	mov	0, %o0
	deccc	%o2
	bge,a	1b
	ldstub	[%o0], %o1
	mov	-1, %o0
2:
	.end
#endif

#ifndef lint
static char sccsid[] = "@(#)gp1_shmem.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1987 by Sun Microsystems,  Inc.
 */

/*
 * GP shared memory operations
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/gp1reg.h>

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

/* acquire/release GP semaphore */
#define	gp1_lock(addr)		(_gp1_lock(addr))
#define	gp1_unlock(addr)	(*(addr) = 0)

/*
 * gp1_lock -- acquire GP semaphore
 *
 * This will normally be replaced by an inline function which uses
 * an uninterruptable instruction.
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

/*
 * gp1_alloc -- allocate a command block
 */
gp1_alloc(shmem, numblk, bitvec, minordev, fd)
	caddr_t shmem;
	int numblk;
	u_int *bitvec;
	int minordev, fd;
{
	register struct gp1_shmem *shp = (struct gp1_shmem *) shmem;
	register int block;

	if (gp1_lock(&shp->alloc_sem)) {
		if (fd >= 0 && !gp1_kern_gp_restart(fd, 0))
			return gp1_alloc((caddr_t) shp, numblk, bitvec,
				minordev, -1);
		return 0;
	}

	if (numblk == 1) {
		register short hosth, hostl, free;
		long count;

		block = 0;
		count = GP1_TIMECONST * 10 / 10;

		hosth = shp->host_alloc_h;
		hostl = shp->host_alloc_l;

		do {
			free = ~(shp->gp_alloc_h ^ hosth);

			if (free) {
				do
					block++;
				while ((free <<= 1) >= 0);

				free = 0x8000 >> block;
				shp->block_owners[GP1_OWNER_INDEX(block)] = 
					minordev;
				shp->host_alloc_h = hosth ^ free;
				gp1_unlock(&shp->alloc_sem);
				*bitvec = (u_short) free << 16;

				return block << 9;
			}

			free = ~(shp->gp_alloc_l ^ hostl);

			if (free) {
				free >>= 1;
				do
					block++;
				while ((free <<= 1) >= 0);

				free = 0x10000 >> block;
				block += 15;
				shp->block_owners[GP1_OWNER_INDEX(block)] = 
					minordev;
				shp->host_alloc_l = hostl ^ free;
				gp1_unlock(&shp->alloc_sem);
				*bitvec = (u_short) free;

				return block << 9;
			}
		} while (--count >= 0);
	}
	else if (numblk > 1 && numblk <= 8) {
		register u_int want, free;

		free = ~(IF68000(
			* (u_int *) &shp->host_alloc_h ^ 
			* (u_int *) &shp->gp_alloc_h,
			(shp->host_alloc_h ^ shp->gp_alloc_h) << 16 | 
			(shp->host_alloc_l ^ shp->gp_alloc_l)));

		want = 0xff00 >> 8 - numblk & 0xff00;
		block = 24 - numblk;

		do {
			if ((free & want) == want) {
				u_char *ownp;

				ownp = shp->block_owners + 
					GP1_OWNER_INDEX(block);
				PR_LOOP(numblk, *ownp-- = minordev);

				IF68000(* (u_int *) &shp->host_alloc_h ^= want,
					shp->host_alloc_h ^= want >> 16;
					shp->host_alloc_l ^= want);
				gp1_unlock(&shp->alloc_sem);
				*bitvec = want;

				return block << 9;
			}

			want <<= 1;
		} while (--block > 0);
	}

	gp1_unlock(&shp->alloc_sem);

	if (numblk == 1 && 
		fd >= 0 && !gp1_kern_gp_restart(fd, 1))
		return gp1_alloc((caddr_t) shp, numblk, bitvec,
			minordev, -1);

	return 0;
}

/*
 * gp1_post -- post a command
 */
gp1_post(shmem, offset, fd)
	caddr_t shmem;
	int offset;
	int fd;
{
	register struct gp1_shmem *shp = (struct gp1_shmem *) shmem;

	if (gp1_lock(&shp->post_sem) == 0) {
		register u_short *postslot;
		register long count = GP1_TIMECONST * 2;

		postslot = 
			&shp->post_buf[shp->host_count & GP1_POST_SLOTS - 1];

		do {
			if (*postslot == 0) {
				*postslot = offset;
				shp->host_count++;
				gp1_unlock(&shp->post_sem);
				return 0;
			}
		} while (--count >= 0);

		gp1_unlock(&shp->post_sem);
	}

	if (fd >= 0 && !gp1_kern_gp_restart(fd, 0))
		return gp1_post((caddr_t) shp, offset, -1);

	return -1;
}

/*
 * gp1_wait0 -- wait for a shared memory word to become zero
 */
gp1_wait0(addr, fd)
	register short *addr;
	int fd;
{
	register long count = GP1_TIMECONST * 5;

	do {
		if (*addr == 0)
			return 0;
	} while (--count >= 0);

	(void) gp1_kern_gp_restart(fd, 0);
	return -1;
}

/* 
 * gp1_sync -- wait for the GP to finish executing all pending commands 
 */
gp1_sync(shmem, fd)
	caddr_t shmem;
	int fd;
{
	register short *shp, host, diff;
	register long count;

	shp = (short *) &((struct gp1_shmem *) shmem)->host_count;
	host = *shp++;	/* point to gp_count */
	count = GP1_TIMECONST * 8;

	do {
		diff = *shp - host;
		if (diff >= 0)
			return 0;
	} while (--count >= 0);

	if (fd >= 0 && !gp1_kern_gp_restart(fd, 0))
		return gp1_sync(shmem, -1);

	return -1;
}

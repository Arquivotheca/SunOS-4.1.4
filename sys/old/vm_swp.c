/*	@(#)vm_swp.c 1.1 94/10/31 SMI; from UCB 4.41 83/05/21	*/

#include <machine/pte.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/seg.h>
#include <sys/vm.h>
#include <sys/trace.h>
#include <sys/map.h>
#include <sys/uio.h>
#include <sys/vnode.h>

/*
 * Swap IO headers -
 * They contain the necessary information for the swap I/O.
 * At any given time, a swap header can be in three
 * different lists. When free it is in the free list, 
 * when allocated and the I/O queued, it is on the swap 
 * device list, and finally, if the operation was a dirty 
 * page push, when the I/O completes, it is inserted 
 * in a list of cleaned pages to be processed by the pageout daemon.
 */
struct	buf *swbuf;

/*
 * swap I/O -
 *
 * If the flag indicates a dirty page push initiated
 * by the pageout daemon, we map the page into the i th
 * virtual page of process 2 (the daemon itself) where i is
 * the index of the swap header that has been allocated.
 * We simply initialize the header and queue the I/O but
 * do not wait for completion. When the I/O completes,
 * iodone() will link the header to a list of cleaned
 * pages to be processed by the pageout daemon.
 */
swap(p, dblkno, addr, nbytes, rdflg, flag, vp, pfcent)
	struct proc *p;
	swblk_t dblkno;
	caddr_t addr;
	int nbytes, rdflg, flag;
	struct vnode *vp;
	u_int pfcent;
{
	register struct buf *bp;
	register u_int c;
	int p2dp;
	register struct pte *dpte, *vpte;
	int s;
	extern swdone();
	int error = 0;

	s = spl6();
	while (bswlist.av_forw == NULL) {
		bswlist.b_flags |= B_WANTED;
		(void) sleep((caddr_t)&bswlist, PSWP+1);
	}
	bp = bswlist.av_forw;
	bswlist.av_forw = bp->av_forw;
	(void) splx(s);

	bp->b_flags = B_BUSY | B_PHYS | rdflg | flag;
	if ((bp->b_flags & (B_DIRTY|B_PGIN)) == 0)
		if (rdflg == B_READ)
			sum.v_pswpin += btoc(nbytes);
		else
			sum.v_pswpout += btoc(nbytes);
	bp->b_proc = p;
	if (flag & B_DIRTY) {
		p2dp = ((bp - swbuf) * CLSIZE) * KLMAX;
		dpte = dptopte(&proc[2], p2dp);
		vpte = vtopte(p, btop(addr));
		for (c = 0; c < nbytes; c += NBPG) {
			if (vpte->pg_pfnum == 0 || vpte->pg_fod)
				panic("swap bad pte");
			/*
			 * Mapping of addr+c is replaced by dpte. 
			 * Since we have flushed these pages in pageout,
			 * no flush is needed here.
			 */
			*dpte++ = *vpte++;
		}
		bp->b_un.b_addr = (caddr_t)ctob(dptov(&proc[2], p2dp));
		bp->b_flags |= B_CALL;
		bp->b_iodone = swdone;
		bp->b_pfcent = pfcent;
	} else
		bp->b_un.b_addr = addr;
	while (nbytes > 0) {
		bp->b_bcount = nbytes;
		bp->b_blkno = dblkno;
		bp->b_dev = vp->v_rdev;
		bsetvp(bp, vp);
		minphys(bp);
		c = bp->b_bcount;
#ifdef TRACE
		trace(TR_SWAPIO, vp, bp->b_blkno);
#endif
		physstrat(bp, vp->v_op->vn_strategy, PSWP);
		if (flag & B_DIRTY) {
			if (c < nbytes)
				panic("big push");
			return (0);
		}
		bp->b_un.b_addr += c;
		bp->b_flags &= ~B_DONE;
		if (bp->b_flags & B_ERROR) {
			if ((flag & (B_UAREA|B_PAGET)) || rdflg == B_WRITE)
				panic("hard IO err in swap");
			swkill(p, "swap: i/o error");
			error = 1;
		}
		nbytes -= c;
		dblkno += btodb(c);
	}
	if (flag & B_DIRTY) {
		/*
		 * Mapping from i-th virtual page of process 2 to physical
		 * page is not valid anymore.
		 */
		register caddr_t vaddr = (caddr_t)dptov(&proc[2], p2dp);
		caddr_t evaddr;

		for (evaddr = vaddr + nbytes; vaddr < evaddr; vaddr += NBPG)
			vac_pageflush(vaddr);
	}
	s = spl6();
	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS|B_PAGET|B_UAREA|B_DIRTY);
	bp->av_forw = bswlist.av_forw;
	bswlist.av_forw = bp;
	if (bswlist.b_flags & B_WANTED) {
		bswlist.b_flags &= ~B_WANTED;
		wakeup((caddr_t)&bswlist);
		wakeup((caddr_t)&proc[2]);
	}
	(void) splx(s);
	return (error);
}

/*
 * Put a buffer on the clean list after I/O is done.
 * Called from biodone.
 */
swdone(bp)
	register struct buf *bp;
{
	register int s;

	if (bp->b_flags & B_ERROR)
		panic("IO err in push");
	s = spl6();
	bp->av_forw = bclnlist;
	cnt.v_pgout++;
	cnt.v_pgpgout += bp->b_bcount / NBPG;
	bclnlist = bp;
	if (bswlist.b_flags & B_WANTED)
		wakeup((caddr_t)&proc[2]);
	(void) splx(s);
}

/*
 * rout is the name of the routine where we ran into problems.
 */
swkill(p, rout)
	struct proc *p;
	char *rout;
{
	printf("pid %d: killed due to swap problems in %s\n",
		p->p_pid, rout);
	uprintf("sorry, pid %d was killed due to swap problems in %s\n",
		p->p_pid, rout);
	/*
	 * To be sure no looping (e.g. in vmsched trying to
	 * swap out) mark process locked in core (as though
	 * done by user) after killing it so noone will try
	 * to swap it out.
	 */
	psignal(p, SIGKILL);
	p->p_flag |= SULOCK;
}

/*
 * Raw I/O. The arguments are
 *	The strategy routine for the device
 *	A buffer, which will always be a special buffer
 *	  header owned exclusively by the device for this purpose
 *	The device number
 *	Read/write flag
 * Essentially all the work is computing physical addresses and
 * validating them.
 * If the user has the proper access privilidges, the process is
 * marked 'delayed unlock' and the pages involved in the I/O are
 * faulted and locked. After the completion of the I/O, the above pages
 * are unlocked.
 */
physio(strat, bp, dev, rw, mincnt, uio)
	int (*strat)(); 
	register struct buf *bp;
	dev_t dev;
	int rw;
	void (*mincnt)();
	struct uio *uio;
{
	register struct iovec *iov = uio->uio_iov;
	register int c;
	char *a;
	int s, error = 0;

nextiov:
	if (uio->uio_iovcnt == 0)
		return (0);
	if (useracc(iov->iov_base, (u_int)iov->iov_len,
	    rw==B_READ?B_WRITE:B_READ) == NULL)
		return (EFAULT);
	s = spl6();
	while (bp->b_flags&B_BUSY) {
		bp->b_flags |= B_WANTED;
		(void) sleep((caddr_t)bp, PRIBIO+1);
	}
	(void) splx(s);
	bp->b_error = 0;
	bp->b_proc = u.u_procp;
	bp->b_un.b_addr = iov->iov_base;
	while (iov->iov_len > 0) {
		bp->b_flags = B_BUSY | B_PHYS | rw;
		bp->b_dev = dev;
		bp->b_blkno = btodb(uio->uio_offset);
		bp->b_bcount = iov->iov_len;
		(*mincnt)(bp);
		c = bp->b_bcount;
		u.u_procp->p_flag |= SPHYSIO;
		vslock(a = bp->b_un.b_addr, c);
		physstrat(bp, strat, PRIBIO);
		(void) spl6();
		vsunlock(a, c, rw);
		u.u_procp->p_flag &= ~SPHYSIO;
		if (bp->b_flags&B_WANTED)
			wakeup((caddr_t)bp);
		(void) splx(s);
		c -= bp->b_resid;
		bp->b_un.b_addr += c;
		iov->iov_len -= c;
		uio->uio_resid -= c;
		uio->uio_offset += c;
		/* temp kludge for tape drives */
		if (bp->b_resid || (bp->b_flags&B_ERROR))
			break;
	}
	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS);
	error = geterror(bp);
	/* temp kludge for tape drives */
	if (bp->b_resid || error)
		return (error);
	uio->uio_iov++;
	uio->uio_iovcnt--;
	goto nextiov;
}

#define	MAXPHYS	(63 * 1024)	/* don't increase beyond NDMAXIO */

void
minphys(bp)
	struct buf *bp;
{
	if (bp->b_bcount > MAXPHYS)
		bp->b_bcount = MAXPHYS;
}

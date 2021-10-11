/*	@(#)swap_vnodeops.c	1.1	94/10/31	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Vnode operations for the swap vnode.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/mman.h>
#include <sys/bootconf.h>

#include <machine/mmu.h>
#include <machine/vm_hat.h>
#include <vm/anon.h>
#include <vm/swap.h>
#include <vm/page.h>
#include <vm/pvn.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_map.h>
#include <vm/seg_vn.h>

int swapvpdebug = 1;

/*
 * The system itself only uses swap_pageio and swap_blksize.
 * Through the /dev/drum interface we can only get at swap_rdwr.
 * All the others "should never be called".
 */

static	 int swap_rdwr();
static	 int swap_pageio();
static	 int swap_blksize();
static	 int swap_panic();

struct vnodeops swap_vnodeops = {
	swap_panic,	/* open */
	swap_panic,	/* close */
	swap_rdwr,
	swap_panic,	/* ioctl */
	swap_panic,	/* select */
	swap_panic,	/* getattr */
	swap_panic,	/* setattr */
	swap_panic,	/* access */
	swap_panic,	/* lookup */
	swap_panic,	/* create */
	swap_panic,	/* remove */
	swap_panic,	/* link */
	swap_panic,	/* rename */
	swap_panic,	/* mkdir */
	swap_panic,	/* rmdir */
	swap_panic,	/* readdir */
	swap_panic,	/* symlink */
	swap_panic,	/* readlink */
	swap_panic,	/* fsync */
	swap_panic,	/* inactive */
	swap_panic,	/* lockctl */
	swap_panic,	/* fid */
	swap_pageio,
	swap_blksize,
	swap_panic,	/* map */
};

/*
 * Read or write the swap vnode.
 */
/*ARGSUSED*/
static int
swap_rdwr(vp, uiop, rw, ioflag, cred)
	struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	register addr_t base;
	register u_int off;
	register int n, on;
	u_int flags;
	u_int asize;
	int err = 0;

	if (rw != UIO_READ && rw != UIO_WRITE)
		panic("swap_rdwr");
	if (uiop->uio_resid == 0)
		return (0);
	if ((uiop->uio_offset < 0 || (uiop->uio_offset + uiop->uio_resid) < 0))
		return (EINVAL);

	asize = anoninfo.ani_max<<PAGESHIFT;
	do {
		off = uiop->uio_offset & ~(MAXBSIZE - 1);
		on = uiop->uio_offset & (MAXBSIZE - 1);
		n = MIN(MAXBSIZE - on, uiop->uio_resid);

		if (rw == UIO_READ) {
			if (asize) {
				int diff = asize - uiop->uio_offset;

				if (diff <= 0)
					return (0);
				if (diff < n)
					n = diff;
			}
		}

		/*
		 * Check to see if we can not read in the page and just bzero
		 * the memory.  We can do this if we are going to rewrite
		 * the entire mapping or if we are going to write to end
		 * of the device from the beginning of the mapping.
		 */
		if ((rw == UIO_WRITE) && (on == 0) && ((n == MAXBSIZE) ||
		    ((off + n) == asize))) {
			/*
			 * XXX - need to allocate private zfod pages,
			 * then RENAME to the destination vp.
			 */
		}

		base = segmap_getmap(segkmap, vp, off);

		err = uiomove(base + on, n, rw, uiop);

		if (rw == UIO_WRITE && err == 0) {
			/*
			 * Force write back for synchronous write cases
			 */
			if (ioflag & IO_SYNC)
				flags = SM_SYNC;
			else
				flags = 0;
		} else
			flags = 0;

		if (segmap_release(segkmap, base, flags))
			err = EIO;

	} while (err == 0 && uiop->uio_resid > 0 && n != 0);

	return (err);
}

/*
 * Read or write physical page frames given by [vp, offset].
 */
/*ARGSUSED*/
static int
swap_pageio(vp, offset, pp, len, flags)
	struct vnode *vp;
	register u_int offset;
	struct page *pp;
	u_int len;
	int flags;
{
	register struct swapinfo *sip = &swapinfo;

	do {
		if (offset < sip->si_size) {
			if (offset + len > sip->si_size)
				panic("swap_pageio: req crosses devs");
			return (VOP_PAGEIO(sip->si_vp, offset, pp, len, flags));
		}
		offset -= sip->si_size;
	} while (sip = sip->si_next);
	panic("swap_pageio");
	/*NOTREACHED*/
}

/*ARGSUSED*/
static int
swap_blksize(vp, offset, offsetp, sizep, noffsetp, nsizep)
	struct vnode *vp;
	register u_int offset;
	register u_int *offsetp, *sizep;
	u_int *noffsetp, *nsizep;
{
	register struct swapinfo *sip = &swapinfo;
	register u_int soff = 0;
	int err;

	do {
		if (offset < sip->si_size) {
			err = VOP_BLKSIZE(sip->si_vp, offset, offsetp, sizep,
			    noffsetp, nsizep);
			if (err)
				return (err);
			/* check that request isn't past si_size */
			if (offsetp) {
				if (*offsetp >= sip->si_size)
					*sizep = 0;
				else if (*offsetp + *sizep > sip->si_size)
					*sizep = sip->si_size - *offsetp;
				*offsetp += soff;
			}
			if (noffsetp) {
				if (*noffsetp >= sip->si_size)
					*nsizep = 0;
				else if (*noffsetp + *nsizep > sip->si_size)
					*nsizep = sip->si_size - *noffsetp;
				*noffsetp += soff;
			}
if (swapvpdebug) {
	if (offsetp)
		if (offset < *offsetp || offset >= *offsetp + *sizep) {
			printf("offset 0x%x *offsetp 0x%x *sizep 0x%x\n",
				offset, *offsetp, *sizep);
			call_debug("swap_vnodeops");
		}
}
			return (0);
		}
		offset -= sip->si_size;
		soff += sip->si_size;
	} while (sip = sip->si_next);
	/* EOF on /dev/drum */
	if (sizep)
		*sizep = 0;
	if (nsizep)
		*nsizep = 0;
	return (0);
}

static int
swap_panic()
{

	panic("swap: bad vnode op");
	/*NOTREACHED*/
}

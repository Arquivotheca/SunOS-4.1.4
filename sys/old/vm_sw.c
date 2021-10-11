/*	@(#)vm_sw.c 1.1 94/10/31 SMI; from UCB 4.18 83/05/18	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/map.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/pathname.h>
#include <sys/bootconf.h>

struct	buf rswbuf;
/*
 * Indirect driver for multi-controller paging.
 */
swstrategy(bp)
	register struct buf *bp;
{
	int sz, off, seg;
	struct vnode *vp;

#ifdef GENERIC
	/*
	 * A mini-root gets copied into the front of the swap
	 * and we run over top of the swap area just long
	 * enough for us to do a mkfs and restor of the real
	 * root (sure beats rewriting standalone restor).
	 *
	 * Why stop there?  Setup is now going to run window  
	 * system or curses programs from the mini-root.
	 */
#define	MINIROOTSIZE	9216	/* 4.5 Meg */
	if (rootvp == swaptab[0].bo_vp)
		bp->b_blkno += MINIROOTSIZE;
#endif
	sz = howmany(bp->b_bcount, DEV_BSIZE);
	if (bp->b_blkno+sz > nswap) {
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	if (nswdev > 1) {
		off = bp->b_blkno % dmmax;
		if (off+sz > dmmax) {
			bp->b_flags |= B_ERROR;
			iodone(bp);
			return;
		}
		seg = bp->b_blkno / dmmax;
		vp = swaptab[seg % nswdev].bo_vp;
		seg /= nswdev;
		bp->b_blkno = seg*dmmax + off;
	} else {
		vp = swaptab[0].bo_vp;
	}
	bp->b_dev = vp->v_rdev;
	bsetvp(bp, vp);
	VOP_STRATEGY(bp);
}

swread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (physio(swstrategy, &rswbuf, dev, B_READ, minphys, uio));
}

swwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (physio(swstrategy, &rswbuf, dev, B_WRITE, minphys, uio));
}

/*
 * System call swapon(name) enables swapping on a file name,
 * which must be in the swdevsw.  Return EBUSY
 * if already swapping on this file.
 */
swapon()
{
	register struct a {
		char	*name;
	} *uap;
	register struct bootobj *sp;
	struct pathname pn;

	if (!suser())
		return;
	uap = (struct a *)u.u_ap;
	pn_alloc(&pn);
	if (u.u_error = pn_get(uap->name, UIO_USERSPACE, &pn)) {
		return;
	}
	/*
	 * Search starting at second table entry,
	 * since first (primary swap area) is freed at boot.
	 */
	u.u_error = ENODEV;
	for (sp = &swaptab[1]; sp < &swaptab[Nswaptab]; sp++) {
		if ((sp->bo_flags & BO_VALID) == 0) {
			continue;
		}
		if (strcmp(pn.pn_path, sp->bo_name) == 0) {
			if (sp->bo_flags & BO_BUSY) {
				u.u_error = EBUSY;
				break;
			}
			swfree(sp - swaptab);
			u.u_error = 0;
			break;
		}
	}
	pn_free(&pn);
	return;
}

/*
 * Swfree(index) frees the index'th portion of the swap map.
 * Each of the nswdev devices provides 1/nswdev'th of the swap
 * space, which is laid out with blocks of dmmax pages circularly
 * among the devices.
 */
swfree(index)
	int index;
{
	register swblk_t vsbase;
	register long blk;
	struct vnode *vp;
	register swblk_t dvbase;
	register int nblks;

	vp = swaptab[index].bo_vp;
	if (vp == NULL) {
		panic("swfree: null bo_vp");
	}
	VOP_OPEN(&vp, FREAD|FWRITE, u.u_cred);
	swaptab[index].bo_flags |= BO_BUSY;
	nblks = swaptab[index].bo_size;
	for (dvbase = 0; dvbase < nblks; dvbase += dmmax) {
		blk = nblks - dvbase;
		if ((vsbase = index*dmmax + dvbase*nswdev) >= nswap)
			panic("swfree");
		if (blk > dmmax)
			blk = dmmax;
		if (vsbase == 0) {
			/*
			 * Can't free a block starting at 0 in the swapmap
			 * but need some space for argmap so use 1/2 this
			 * hunk which needs special treatment anyways.
			 */
			if (argvp) {
				VN_RELE(argvp);
			}
			argvp = swaptab[0].bo_vp;
			VN_HOLD(argvp);
			rminit(argmap, (long)(blk/2-ctod(CLSIZE)),
			    (long)ctod(CLSIZE), "argmap", ARGMAPSIZE);
			/*
			 * First of all chunks... initialize the swapmap
			 * the second half of the hunk.
			 */
			rminit(swapmap, (long)(blk/2), (long)(blk/2),
			    "swap", nswapmap);
		} else
			rmfree(swapmap, blk, vsbase);
	}
}

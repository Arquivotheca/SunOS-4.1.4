#ifndef lint
static  char sccsid[] = "@(#)bdev_vnodeops.c 1.1 94/10/31";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/bootconf.h>


/*
 * Convert a block dev into a vnode pointer suitable for bio.
 */

struct dev_vnode {
	struct vnode dv_vnode;
	struct dev_vnode *dv_link;
} *dev_vnode_headp;

bdev_badop()
{

	panic("bdev_badop");
}

/*ARGSUSED*/
int
bdev_open(vpp, flag, cred)
	struct vnode **vpp;
	int flag;
	struct ucred *cred;
{

	return ((*bdevsw[major((*vpp)->v_rdev)].d_open)((*vpp)->v_rdev, flag));
}

/*ARGSUSED*/
int
bdev_close(vp, flag, cred)
	struct vnode *vp;
	int flag;
	struct ucred *cred;
{

	/*
	 * On last close of a block device (that isn't mounted)
	 * we must invalidate any in core blocks, so that
	 * we can, for instance, change floppy disks.
	 */
	bflush(vp);
	binval(vp);
	return ((*bdevsw[major(vp->v_rdev)].d_close)(vp->v_rdev, flag));
}

/*
 * For now, the only value we actually return is size.
 */
/*ARGSUSED*/
int
bdev_getattr(vp, vap, cred)
	struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	int (*size)();

	bzero((caddr_t) vap, sizeof (struct vattr));
	size = bdevsw[major(vp->v_rdev)].d_psize;
	if (size) {
		vap->va_size = (*size)(vp->v_rdev) * DEV_BSIZE;
	}
	return (0);
}

/*ARGSUSED*/
int
bdev_inactive(vp)
	struct vnode *vp;
{

	/* could free the vnode here */
	return (0);
}

int
bdev_strategy(bp)
	struct buf *bp;
{

	(*bdevsw[major(bp->b_vp->v_rdev)].d_strategy)(bp);
	return (0);
}

int
bdev_dump(vp, addr, bn, count)
	struct vnode *vp;
	caddr_t addr;
	int bn;
	int count;
{

	return ((*bdevsw[major(vp->v_rdev)].d_dump)
	    (vp->v_rdev, addr, bn, count));
}

struct vnodeops dev_vnode_ops = {
	bdev_open,
	bdev_close,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_getattr,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_inactive,
	bdev_badop,
	bdev_strategy,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_badop,
	bdev_dump,
};

/*
 * Convert a block device into a special purpose vnode for bio
 */
struct vnode *
bdevvp(dev)
	dev_t dev;
{
	register struct dev_vnode *dvp;
	register struct dev_vnode *endvp;

	endvp = (struct dev_vnode *)0;
	for (dvp = dev_vnode_headp; dvp; dvp = dvp->dv_link) {
		if (dvp->dv_vnode.v_rdev == dev) {
			VN_HOLD(&dvp->dv_vnode);
			return (&dvp->dv_vnode);
		}
		endvp = dvp;
	}
	dvp = (struct dev_vnode *)
		kmem_alloc((u_int)sizeof (struct dev_vnode));
	bzero((caddr_t)dvp, sizeof (struct dev_vnode));
	dvp->dv_vnode.v_count = 1;
	dvp->dv_vnode.v_op = &dev_vnode_ops;
	dvp->dv_vnode.v_rdev = dev;
	if (endvp != (struct dev_vnode *)0) {
		endvp->dv_link = dvp;
	} else {
		dev_vnode_headp = dvp;
	}
	return (&dvp->dv_vnode);
}

/* ARGSUSED */
int
bdev_mountroot(vfsp, vpp, name)
	struct vfs *vfsp;
	struct vnode **vpp;
	char *name;
{
	return (EINVAL);
}

/*ARGSUSED*/
int
bdev_swapvp(vfsp, vpp, name)
	struct vfs *vfsp;
	struct vnode **vpp;
	char *name;
{
	extern char *strcpy();
	extern dev_t getblockdev();

	if (*name == '\0') {
		/*
		 * No swap name specified, use root dev partition "b"
		 * if it is a block device, otherwise fail.
		 * XXX should look through device list or something here
		 * if root is not local.
		 */
		if (rootvp->v_op == &dev_vnode_ops) {
			*vpp = bdevvp(makedev(major(rootvp->v_rdev), 1));
			(void) strcpy(name, rootfs.bo_name);
			name[3] = 'b';
			return (0);
		} else {
			dev_t dev;

			if (dev = getblockdev("swap", name)) {
				*vpp = bdevvp(makedev(major(dev), 1));
				name[3] = 'b';
				return (0);
			}
			return (ENODEV);
		}
	} else {
		dev_t dev;

		if (dev = getblockdev("swap", name)) {
			*vpp = bdevvp(dev);
			return (0);
		}
		return (ENODEV);
	}
}

struct vfsops bdev_vfsops = {
	bdev_badop,		/* mount */
	bdev_badop,		/* unmount */
	bdev_badop,		/* root */
	bdev_badop,		/* statfs */
	bdev_badop,		/* sync */
	bdev_badop,		/* vget */
	bdev_mountroot,		/* mountroot */
	bdev_swapvp,		/* swapvp */
};

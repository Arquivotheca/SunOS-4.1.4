/*	@(#)vm_drum.c 1.1 94/10/31 SMI; from UCB 4.9 83/05/18	*/

#include <machine/pte.h>

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/text.h>
#include <sys/map.h>
#include <sys/vm.h>
#include <sys/cmap.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/systm.h>

/*
 * Expand the swap area for both the data and stack segments.
 * If space is not available for both, retract and return 0.
 */
swpexpand(ds, ss, dmp, smp)
	size_t ds, ss;
	register struct dmap *dmp, *smp;
{
	register struct dmap *tmp;
	register int ts;
	size_t ods;

	/*
	 * If dmap isn't growing, do smap first.
	 * This avoids anomalies if smap will try to grow and
	 * fail, which otherwise would shrink ds without expanding
	 * ss, a rather curious side effect!
	 */
	if (dmp->dm_alloc > ds) {
		tmp = dmp; ts = ds;
		dmp = smp; ds = ss;
		smp = tmp; ss = ts;
	}
	ods = dmp->dm_size;
	if (vsexpand(ds, dmp, 0) == 0)
		goto bad;
	if (vsexpand(ss, smp, 0) == 0) {
		(void) vsexpand(ods, dmp, 1);
		goto bad;
	}
	return (1);

bad:
	u.u_error = ENOMEM;
	return (0);
}

/*
 * Expand or contract the virtual swap segment mapped
 * by the argument diskmap so as to just allow the given size.
 *
 * FOR NOW CANT RELEASE UNLESS SHRINKING TO ZERO, SINCE PAGEOUTS MAY
 * BE IN PROGRESS... TYPICALLY NEVER SHRINK ANYWAYS, SO DOESNT MATTER MUCH
 */
vsexpand(vssize, dmp, canshrink)
	register size_t vssize;
	register struct dmap *dmp;
{
	register long blk = dmmin;
	register int vsbase = 0;
	register swblk_t *ip = dmp->dm_map;
	size_t oldsize = dmp->dm_size;
	size_t oldalloc = dmp->dm_alloc;

	vssize = ctod(vssize);
	while (vsbase < oldalloc || vsbase < vssize) {
		if (ip - dmp->dm_map >= NDMAP)
			panic("vmdrum NDMAP");
		if (vsbase >= oldalloc) {
			*ip = rmalloc(swapmap, blk);
			if (*ip == 0) {
				dmp->dm_size = vsbase;
				if (vsexpand(dtoc(oldsize), dmp, 1) == 0)
					panic("vsexpand");
				return (0);
			}
			dmp->dm_alloc += blk;
		} else if (vssize == 0 ||
		    vsbase >= vssize && canshrink) {
#ifdef sun
			/*
			 * If this is a data map, make sure we don't
			 * free swap space for pages in the hole
			 * (whose swap space was already freed in mmap).
			 */
			if (u.u_hole.uh_last != 0 && dmp == &u.u_dmap)
				vsswapfree(vsbase, blk, *ip);
			else
#endif
				rmfree(swapmap, blk, *ip);
			*ip = 0;
			dmp->dm_alloc -= blk;
		}
		vsbase += blk;
		if (blk < dmmax)
			blk *= 2;
		ip++;
	}
	dmp->dm_size = vssize;
	return (1);
}

#ifdef sun
/*
 * Free swap space for the data segment, being careful
 * not to free space in the hole.
 */
vsswapfree(vsbase, blk, swbase)
	int vsbase;
	long blk;
	swblk_t swbase;
{
	int vsbegin = dtoc(vsbase);
	int vsend = dtoc(vsbase + blk) - CLSIZE;
	long b;

	if (vsbegin >= u.u_hole.uh_first) {
		if (vsbegin <= u.u_hole.uh_last) {
			if (vsend > u.u_hole.uh_last) {
				b = ctod(u.u_hole.uh_last - vsbegin + CLSIZE);
				rmfree(swapmap, (long)(blk - b),
				    (long)(swbase + b));
			}
		} else {
			rmfree(swapmap, blk, (long)swbase);
		}
	} else {
		if (vsend < u.u_hole.uh_first) {
			rmfree(swapmap, blk, swbase);
		} else if (vsend <= u.u_hole.uh_last) {
			b = ctod(u.u_hole.uh_first - vsbegin);
			rmfree(swapmap, b, swbase);
		} else {
			b = ctod(u.u_hole.uh_first - vsbegin);
			rmfree(swapmap, b, swbase);
			b = ctod(u.u_hole.uh_last - vsbegin + CLSIZE);
			rmfree(swapmap, (long)(blk - b), (long)(swbase + b));
		}
	}
}
#endif

/*
 * Allocate swap space for a text segment,
 * in chunks of at most dmtext pages.
 */
vsxalloc(xp)
	struct text *xp;
{
	register long blk;
	register swblk_t *dp;
	swblk_t vsbase;

	if (ctod(xp->x_size) > NXDAD * dmtext)
		return (0);
	dp = xp->x_daddr;
	for (vsbase = 0; vsbase < ctod(xp->x_size); vsbase += dmtext) {
		blk = ctod(xp->x_size) - vsbase;
		if (blk > dmtext)
			blk = dmtext;
		if ((*dp++ = rmalloc(swapmap, blk)) == 0) {
			vsxfree(xp, (long)dtoc(vsbase));
			return (0);
		}
	}
	if (xp->x_flag & XPAGV) {
		xp->x_ptdaddr =
		    rmalloc(swapmap, (long)ctod(clrnd(ctopt(xp->x_size))));
		if (xp->x_ptdaddr == 0) {
			vsxfree(xp, (long)xp->x_size);
			return (0);
		}
	}
	return (1);
}

/*
 * Free the swap space of a text segment which
 * has been allocated ts pages.
 */
vsxfree(xp, ts)
	struct text *xp;
	long ts;
{
	register long blk;
	register swblk_t *dp;
	swblk_t vsbase;

	ts = ctod(ts);
	dp = xp->x_daddr;
	for (vsbase = 0; vsbase < ts; vsbase += dmtext) {
		blk = ts - vsbase;
		if (blk > dmtext)
			blk = dmtext;
		rmfree(swapmap, blk, *dp);
		*dp++ = 0;
	}
	if ((xp->x_flag&XPAGV) && xp->x_ptdaddr) {
		rmfree(swapmap, (long)ctod(clrnd(ctopt(xp->x_size))),
		    xp->x_ptdaddr);
		xp->x_ptdaddr = 0;
	}
}

/*
 * Swap a segment of virtual memory to disk,
 * by locating the contiguous dirty pte's
 * and calling vschunk with each chunk.
 */
vsswap(p, pte, type, vsbase, vscount, dmp)
	struct proc *p;
	register struct pte *pte;
	int type;
	register int vsbase, vscount;
	struct dmap *dmp;
{
	register int size = 0;

	if (vscount % CLSIZE)
		panic("vsswap");
	for (;;) {
		if (vscount == 0 || !dirtycl(pte)) {
			if (size) {
				vschunk(p, vsbase, size, type, dmp);
				vsbase += size;
				size = 0;
			}
			if (vscount == 0)
				return;
			vsbase += CLSIZE;
			if (pte->pg_fod == 0 && pte->pg_pfnum)
				if (type == CTEXT)
					p->p_textp->x_rssize -= vmemfree(pte, CLSIZE);
				else
					p->p_rssize -= vmemfree(pte, CLSIZE);
		} else {
			size += CLSIZE;
			mwait(pte->pg_pfnum);
		}
		vscount -= CLSIZE;
		if (type == CSTACK)
			pte -= CLSIZE;
		else
			pte += CLSIZE;
	}
}

vschunk(p, base, size, type, dmp)
	register struct proc *p;
	register int base, size;
	int type;
	struct dmap *dmp;
{
	register struct pte *pte;
	struct dblock db;
	unsigned v;

	base = ctod(base);
	size = ctod(size);
	if (type == CTEXT) {
		while (size > 0) {
			db.db_size = dmtext - base % dmtext;
			if (db.db_size > size)
				db.db_size = size;
			(void) swap(p,
			    (swblk_t)(p->p_textp->x_daddr[base / dmtext]
				+ base % dmtext),
			    (caddr_t)ptob(tptov(p, dtoc(base))),
			    (int)dtob(db.db_size),
			    B_WRITE, 0, swapdev_vp, (u_int)0);
			pte = tptopte(p, dtoc(base));
			p->p_textp->x_rssize -=
			    vmemfree(pte, (int)dtoc(db.db_size));
			base += db.db_size;
			size -= db.db_size;
		}
		return;
	}
	do {
		vstodb(base, size, dmp, &db, type == CSTACK);
		v = type==CSTACK ?
		    sptov(p, dtoc(base+db.db_size)-1) :
		    dptov(p, dtoc(base));
		(void) swap(p, db.db_base, ptob(v), (int)dtob(db.db_size),
		    B_WRITE, 0, swapdev_vp, (u_int)0);
		pte = type==CSTACK ?
		    sptopte(p, dtoc(base+db.db_size)-1) :
		    dptopte(p, dtoc(base));
		p->p_rssize -= vmemfree(pte, (int)dtoc(db.db_size));
		base += db.db_size;
		size -= db.db_size;
	} while (size != 0);
}

/*
 * Given a base/size pair in virtual swap area,
 * return a physical base/size pair which is the
 * (largest) initial, physically contiguous block.
 */
vstodb(vsbase, vssize, dmp, dbp, rev)
	register int vsbase, vssize;
	struct dmap *dmp;
	register struct dblock *dbp;
{
	register int blk = dmmin;
	register swblk_t *ip = dmp->dm_map;

	if (vsbase < 0 || vssize < 0 || vsbase + vssize > dmp->dm_size)
		panic("vstodb");
	while (vsbase >= blk) {
		vsbase -= blk;
		if (blk < dmmax)
			blk *= 2;
		ip++;
	}
	if (*ip + blk > nswap)
		panic("vstodb *ip");
	dbp->db_size = imin(vssize, blk - vsbase);
	dbp->db_base = *ip + (rev ? blk - (vsbase + dbp->db_size) : vsbase);
}

/*
 * Convert a virtual page number 
 * to its corresponding disk block number.
 * Used in pagein/pageout to initiate single page transfers.
 */
swblk_t
vtod(p, v, dmap, smap)
	register struct proc *p;
	unsigned v;
	struct dmap *dmap, *smap;
{
	struct dblock db;
	int tp;

	if (isatsv(p, v)) {
		tp = ctod(vtotp(p, v));
		return (p->p_textp->x_daddr[tp/dmtext] + tp%dmtext);
	}
	if (isassv(p, v))
		vstodb(ctod(vtosp(p, v)), ctod(1), smap, &db, 1);
	else
		vstodb(ctod(vtodp(p, v)), ctod(1), dmap, &db, 0);
	return (db.db_base);
}

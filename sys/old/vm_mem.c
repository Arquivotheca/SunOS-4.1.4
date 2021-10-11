/*	@(#)vm_mem.c 1.1 94/10/31 SMI; from UCB 6.2 83/11/18	*/

#include <machine/pte.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cmap.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/text.h>
#include <sys/vm.h>
#include <sys/file.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/trace.h>
#include <sys/map.h>
#include <sys/kernel.h>
#include <ufs/mount.h>

/*
 * Allocate memory, and always succeed
 * by jolting page-out daemon
 * so as to obtain page frames.
 * To be used in conjunction with vmemfree().
 */
vmemall(pte, size, p, type)
	register struct pte *pte;
	int size;
	struct proc *p;
{
	register int m;

	if (size <= 0 || size > maxmem)
		panic("vmemall size");
	while (size > 0) {
		if (freemem < desfree)
			outofmem();
		while (freemem == 0)
			(void) sleep((caddr_t)&freemem, PSWP+2);
		m = imin(size, freemem);
		(void) memall(pte, m, p, type);
		size -= m;
		pte += m;
	}
	if (freemem < desfree)
		outofmem();
	/*
	 * Always succeeds, but return success for
	 * vgetu and vgetpt (e.g.) which call either
	 * memall or vmemall depending on context.
	 */
	return (1);
}

/*
 * Free valid and reclaimable page frames belonging to the
 * count pages starting at pte.  If a page is valid
 * or reclaimable and locked (but not a system page), then
 * we simply mark the page as c_gone and let the pageout
 * daemon free the page when it is through with it.
 * If a page is reclaimable, and already in the free list, then
 * we mark the page as c_gone, and (of course) don't free it.
 *
 * Determines the largest contiguous cluster of
 * valid pages and frees them in one call to memfree.
 */
vmemfree(pte, count)
	register struct pte *pte;
	register int count;
{
	register struct cmap *c;
	register struct pte *spte;
	register int j;
	int size, pcnt, fileno;

	if (count % CLSIZE)
		panic("vmemfree");
	for (size = 0, pcnt = 0; count > 0; pte += CLSIZE, count -= CLSIZE) {
		if (pte->pg_fod == 0 && pte->pg_pfnum) {
			c = &cmap[pgtocm(pte->pg_pfnum)];
			pcnt += CLSIZE;
			if (c->c_lock && c->c_type != CSYS) {
				for_CLSIZE(j)
					*(int *)(pte+j) &= PG_PROT;
				c->c_gone = 1;
				goto free;
			}
			if (c->c_free) {
				pcnt -= CLSIZE;
				for_CLSIZE(j)
					*(int *)(pte+j) &= PG_PROT;
				if (c->c_type == CTEXT)
					distpte(&text[c->c_ndx], (int)c->c_page, pte);
				c->c_gone = 1;
				goto free;
			}
			if (size == 0)
				spte = pte;
			size += CLSIZE;
			continue;
		}
#ifdef sun
		/* Don't do anything with mapped ptes */
		if (pte->pg_fod && pte->pg_v)
			goto free;
#endif
		if (pte->pg_fod) {
			fileno = ((struct fpte *)pte)->pg_fileno;
			if (fileno < NOFILE)
				panic("vmemfree vread");
			for_CLSIZE(j)
				*(int *)(pte+j) &= PG_PROT;
		}
free:
		if (size) {
			memfree(spte, size, 1);
			size = 0;
		}
	}
	if (size)
		memfree(spte, size, 1);
	return (pcnt);
}

/*
 * Unlink a page frame from the free list -
 *
 * Performed if the page being reclaimed
 * is in the free list.
 */
munlink(pf)
	unsigned pf;
{
	register int next, prev;

	next = cmap[pgtocm(pf)].c_next;
	prev = cmap[pgtocm(pf)].c_prev;
	cmap[prev].c_next = next;
	cmap[next].c_prev = prev;
	cmap[pgtocm(pf)].c_free = 0;
	if (freemem < minfree)
		outofmem();
	freemem -= CLSIZE;
}

/*
 * Allocate memory -
 *
 * The free list appears as a doubly linked list
 * in the core map with cmap[0] serving as a header.
 */
memall(pte, size, p, type)
	register struct pte *pte;
	int size;
	struct proc *p;
{
	register struct cmap *c;
	register struct pte *rpte;
	register struct proc *rp;
	int i, j, next, curpos;
	unsigned pf;
	struct cmap *c1, *c2;
	int s;

	if (size % CLSIZE)
		panic("memall");
	s = splimp();
	if (size > freemem) {
		(void) splx(s);
		return (0);
	}
	trace(TR_MALL, size, u.u_procp->p_pid);
	for (i = size; i > 0; i -= CLSIZE) {
		curpos = cmap[CMHEAD].c_next;
		c = &cmap[curpos];
		freemem -= CLSIZE;
		next = c->c_next;
		cmap[CMHEAD].c_next = next;
		cmap[next].c_prev = CMHEAD;
		if (c->c_free == 0)
			panic("dup mem alloc");
		if (cmtopg(curpos) > maxfree)
			panic("bad mem alloc");
		if (c->c_gone == 0 && c->c_type != CSYS) {
			if (c->c_type == CTEXT)
				rp = text[c->c_ndx].x_caddr;
			else
				rp = &proc[c->c_ndx];
			while (rp->p_flag & SNOVM)
				rp = rp->p_xlink;
			switch (c->c_type) {

			case CTEXT:
				rpte = tptopte(rp, c->c_page);
				break;

			case CDATA:
				rpte = dptopte(rp, c->c_page);
				break;

			case CSTACK:
				rpte = sptopte(rp, c->c_page);
				break;
			}
			zapcl(rpte, pg_pfnum) = 0;
			if (c->c_type == CTEXT)
				distpte(&text[c->c_ndx], (int)c->c_page, rpte);
		}
		switch (type) {

		case CSYS:
			c->c_ndx = p->p_ndx;
			break;

		case CTEXT:
			c->c_page = vtotp(p, ptetov(p, pte));
			c->c_ndx = p->p_textp - &text[0];
			break;

		case CDATA:
			c->c_page = vtodp(p, ptetov(p, pte));
			c->c_ndx = p->p_ndx;
			break;

		case CSTACK:
			c->c_page = vtosp(p, ptetov(p, pte));
			c->c_ndx = p->p_ndx;
			break;
		}
		if (c->c_blkno) {
			/*
			 * This is very like munhash(), except
			 * that we really don't want to bother
			 * to calculate a vp to pass to it.
			 */
			j = CMHASH(c->c_blkno);
			c1 = &cmap[cmhash[j]];
			if (c1 == c)
				cmhash[j] = c1->c_hlink;
			else {
				for (;;) {
					if (c1 == ecmap)
						panic("memall ecmap");
					c2 = c1;
					c1 = &cmap[c2->c_hlink];
					if (c1 == c)
						break;
				}
				c2->c_hlink = c1->c_hlink;
			}
#if lint || !vax
			if (mfind(c->c_vp, (daddr_t)(u_long)c->c_blkno))
#else
			/* avoid VAX C compiler sign-extension bug */
			if (mfind(c->c_vp, c->c_blkno))
#endif
				panic("memall mfind");
			VN_RELE(c1->c_vp);
			c1->c_vp = (struct vnode *)0;
			c1->c_blkno = 0;
			c1->c_hlink = 0;
		}
		pf = cmtopg(curpos);
		for_CLSIZE(j)
			*(int *)pte++ = pf++;
		c->c_free = 0;
		c->c_gone = 0;
		if (c->c_intrans || c->c_want)
			panic("memall intrans|want");
		c->c_lock = 1;
		c->c_type = type;
	}
	(void) splx(s);
	return (size);
}

/*
 * Free memory -
 *
 * The page frames being returned are inserted
 * to the head/tail of the free list depending
 * on whether there is any possible future use of them.
 *
 * If the freemem count had been zero,
 * the processes sleeping for memory
 * are awakened.
 */
memfree(pte, size, detach)
	register struct pte *pte;
	register int size;
{
	register int i, j, prev, next;
	register struct cmap *c;
	int s;
	
	if (size % CLSIZE)
		panic("memfree");
	if (freemem < CLSIZE * KLMAX)
		wakeup((caddr_t)&freemem);
	while (size > 0) {
		size -= CLSIZE;
		i = pte->pg_pfnum;
		if (i < firstfree || i > maxfree)
			panic("bad mem free");
		i = pgtocm(i);
		c = &cmap[i];
		if (c->c_free)
			panic("dup mem free");
		if (detach && c->c_type != CSYS) {
			for_CLSIZE(j)
				*(int *)(pte+j) &= PG_PROT;
			c->c_gone = 1;
		}
		s = splimp();
		if (detach && c->c_blkno == 0) {
			next = cmap[CMHEAD].c_next;
			cmap[next].c_prev = i;
			c->c_prev = CMHEAD;
			c->c_next = next;
			cmap[CMHEAD].c_next = i;
		} else {
			prev = cmap[CMHEAD].c_prev;
			cmap[prev].c_next = i;
			c->c_next = CMHEAD;
			c->c_prev = prev;
			cmap[CMHEAD].c_prev = i;
		}
		c->c_free = 1;
		freemem += CLSIZE;
		(void) splx(s);
		pte += CLSIZE;
	}
}

/*
 * Allocate wired-down (non-paged) pages in kernel virtual memory.
 */
caddr_t
wmemall(pmemall, n)
	int (*pmemall)(), n;
{
	register int npg;
	register caddr_t va;
	register int a;

	npg = clrnd(btoc(n));
	a = rmalloc(kernelmap, (long)npg);
	if (a == 0)
		return (0);
	if ((*pmemall)(&Usrptmap[a], npg, &proc[0], CSYS) == 0) {
		rmfree(kernelmap, (long)npg, (long)a);
		return (0);
	}
	va = (caddr_t) kmxtob(a);
	vmaccess(&Usrptmap[a], va, npg);
	return (va);
}

/*
 * Allocate wired-down (non-paged) pages in kernel virtual memory.
 * (and clear them)
 */
caddr_t
zmemall(pmemall, n)
	int (*pmemall)(), n;
{
	register int npg;
	register caddr_t va;
	register int a;

	npg = clrnd(btoc(n));
	a = rmalloc(kernelmap, (long)npg);
	if (a == 0)
		return (0);
	if ((*pmemall)(&Usrptmap[a], npg, &proc[0], CSYS) == 0) {
		rmfree(kernelmap, (long)npg, (long)a);
		return (0);
	}
	va = (caddr_t) kmxtob(a);
	vmaccess(&Usrptmap[a], va, npg);
	bzero(va, (u_int)(npg * NBPG));
	return (va);
}

wmemfree(va, n)
	caddr_t va;
	int n;
{
	register int a, npg;

	a = btokmx((struct pte *) va);
	npg = clrnd(btoc(n));
	(void) memfree(&Usrptmap[a], npg, 0);
#ifdef sun
	mapout(&Usrptmap[a], npg);
#endif
	rmfree(kernelmap, (long)npg, (long)a);
}

/*
 * Enter clist block c on the hash chains.
 * It contains file system block bn from vnode vp.
 */
mhash(c, vp, bn)
	register struct cmap *c;
	struct vnode *vp;
	daddr_t bn;
{
	register int i = CMHASH(bn);

	c->c_hlink = cmhash[i];
	cmhash[i] = c - cmap;
	c->c_blkno = bn;
	c->c_vp = vp;
	VN_HOLD(vp);
}

/*
 * Pull the clist entry of <vp,bn> off the hash chains.
 * We have checked before calling (using mfind) that the
 * entry really needs to be unhashed, so panic if we can't
 * find it (can't happen).
 */
munhash(vp, bn)
	register struct vnode *vp;
	daddr_t bn;
{
	register int i = CMHASH(bn);
	register struct cmap *c1, *c2;
	int si = splimp();

	c1 = &cmap[cmhash[i]];
	if (c1 == ecmap)
		panic("munhash");
	if (c1->c_blkno == bn && c1->c_vp == vp)
		cmhash[i] = c1->c_hlink;
	else {
		for (;;) {
			c2 = c1;
			c1 = &cmap[c2->c_hlink];
			if (c1 == ecmap)
				panic("munhash");
			if (c1->c_blkno == bn && c1->c_vp == vp)
				break;
		}
		c2->c_hlink = c1->c_hlink;
	}
	if (mfind(vp, bn))
		panic("munhash mfind");
	VN_RELE(vp);
	c1->c_vp = (struct vnode *)0;
	c1->c_blkno = 0;
	c1->c_hlink = 0;
	(void) splx(si);
}

/*
 * Look for block bn of vnode vp in the free pool.
 * Currently it should not be possible to find it unless it is
 * c_free and c_gone, although this may later not be true.
 * (This is because active texts are locked against file system
 * writes by the system.)
 */
struct cmap *
mfind(vp, bn)
	register struct vnode *vp;
	daddr_t bn;
{
	register struct cmap *c1 = &cmap[cmhash[CMHASH(bn)]];
	int si = splimp();

	while (c1 != ecmap) {
		if (c1->c_blkno == bn && c1->c_vp == vp)
			return (c1);
		c1 = &cmap[c1->c_hlink];
	}
	(void) splx(si);
	return ((struct cmap *)0);
}

/*
 * Purge blocks from vnode vp from incore cache
 */
mpurge(vp)
	register struct vnode *vp;
{
	register struct cmap *c1, *c2;
	register int i;
	int j = 0;
	int si = splimp();

	for (i = 0; i < CMHSIZ; i++) {
more:
		c1 = &cmap[cmhash[i]];
		if (c1 == ecmap)
			continue;
		if (c1->c_vp == vp)
			cmhash[i] = c1->c_hlink;
		else {
			for (;;) {
				c2 = c1;
				c1 = &cmap[c1->c_hlink];
				if (c1 == ecmap)
					goto cont;
				if (c1->c_vp == vp)
					break;
			}
			c2->c_hlink = c1->c_hlink;
		}
		j++;
		c1->c_vp = (struct vnode *)0;
		c1->c_blkno = 0;
		c1->c_hlink = 0;
		goto more;
cont:
		;
	}
	(void) splx(si);
	while(j--)
		VN_RELE(vp);
}

/*
 * Purge blocks from filesystem vfsp from incore cache
 * before umount().
 */
mpurgevfs(vfsp)
	register struct vfs *vfsp;
{
	register struct cmap *c1, *c2;
	register int i;
	int si = splimp();

more:
	for (i = 0; i < CMHSIZ; i++) {
		c1 = &cmap[cmhash[i]];
		if (c1 == ecmap)
			continue;
		if (c1->c_vp->v_vfsp == vfsp)
			cmhash[i] = c1->c_hlink;
		else {
			for (;;) {
				c2 = c1;
				c1 = &cmap[c1->c_hlink];
				if (c1 == ecmap)
					goto cont;
				if (c1->c_vp->v_vfsp == vfsp)
					break;
			}
			c2->c_hlink = c1->c_hlink;
		}
		(void) splx(si);
		VN_RELE(c1->c_vp);
		si = splimp();
		c1->c_vp = (struct vnode *)0;
		c1->c_blkno = 0;
		c1->c_hlink = 0;
		goto more;
cont:
		;
	}
	(void) splx(si);
}

/*
 * Initialize core map.
 */
meminit(first, last)
	int first, last;
{
	register int i;

	firstfree = clrnd(first);
	maxfree = clrnd(last - (CLSIZE - 1));
	freemem = 0;
	cmap[CMHEAD].c_prev = CMHEAD;
	cmap[CMHEAD].c_next = CMHEAD;
	cmap[CMHEAD].c_type = CSYS;
	ecmx = ecmap - cmap;
	for (i = 0; i < CMHSIZ; i++)
		cmhash[i] = ecmx;
	hand = 0;
}

/*
 * Add pages [first, last) to core map free list.
 * Assumes that memory is allocated with no
 * overlapping allocation ranges.
 */
memialloc(first, last)
	int first, last;	/* range of page frame number */
{
	register int i;
	register struct cmap *c;
	register int fc, lc;

	fc = pgtocm(first);
	lc = pgtocm(last - (CLSIZE - 1)) - 1;
	if (lc > ecmx)
		lc = ecmx;
	for (i = fc; i <= lc; i++) {
		c = &cmap[i];
		c->c_free = 1;
		c->c_gone = 1;
		c->c_type = CSYS;
		c->c_vp = (struct vnode *)0;
		c->c_blkno = 0;
		c->c_prev = i - 1;
		c->c_next = i + 1;
		freemem += CLSIZE;
	}
	i = cmap[CMHEAD].c_prev;
	cmap[i].c_next = fc;
	cmap[fc].c_prev = i;
	cmap[lc].c_next = CMHEAD;
	cmap[CMHEAD].c_prev = lc;
	avefree = freemem;
}

/*
 * Wait for frame pf to become unlocked
 * if it is currently locked.
 *
 * THIS ROUTINE SHOULD TAKE A CMAP STRUCTURE AS ARGUMENT.
 */
mwait(pf)
	unsigned pf;
{

	mlock(pf);
	munlock(pf);
}

/*
 * Lock a page frame.
 *
 * THIS ROUTINE SHOULD TAKE A CMAP STRUCTURE AS ARGUMENT.
 */
mlock(pf)
	unsigned pf;
{
	register struct cmap *c = &cmap[pgtocm(pf)];

	while (c->c_lock) {
		c->c_want = 1;
		(void) sleep((caddr_t)c, PSWP+1);
	}
	c->c_lock = 1;
}

/*
 * Unlock a page frame.
 *
 * THIS ROUTINE SHOULD TAKE A CMAP STRUCTURE AS ARGUMENT.
 */
munlock(pf)
	unsigned pf;
{
	register struct cmap *c = &cmap[pgtocm(pf)];

	if (c->c_lock == 0)
		panic("dup page unlock");
	if (c->c_want)
		wakeup((caddr_t)c);
	c->c_lock = 0;
	c->c_want = 0;
}

/* 
 * Lock a virtual segment.
 *
 * For each cluster of pages, if the cluster is not valid,
 * touch it to fault it in, otherwise just lock page frame.
 * Called from physio to ensure that the pages 
 * participating in raw i/o are valid and locked.
 */
vslock(base, count)
	caddr_t base;
{
	register unsigned v;
	register int npf;
	register struct pte *pte;
	register struct cmap *c;

	v = btop(base);
	pte = vtopte(u.u_procp, v);
	npf = btoc(count + ((int)base & CLOFSET));
	while (npf > 0) {
		if (pte->pg_v) {
#ifdef sun
			if (!pte->pg_fod) {	/* mmap'ed page */
#endif
				c = &cmap[pgtocm(pte->pg_pfnum)];
				if (c->c_lock) {
					/*
					 * Page may not be valid by the time
					 * we get the lock, look again.
					 */
					mwait(pte->pg_pfnum);
					continue;
				}
				mlock(pte->pg_pfnum);
#ifdef sun
			}
#endif
		} else
			pagein(ctob(v), 1);	/* return it locked */
		pte += CLSIZE;
		v += CLSIZE;
		npf -= CLSIZE;
	}
}

/* 
 * Unlock a virtual segment.
 */
vsunlock(base, count, rw)
	caddr_t base;
{
	register struct pte *pte;
	register int npf;

	pte = vtopte(u.u_procp, btop(base));
	npf = btoc(count + ((int)base & CLOFSET));
	while (npf > 0) {
#ifdef sun
		if (!pte->pg_fod)	/* mmap'ed page */
#endif
			munlock(pte->pg_pfnum);
		if (rw == B_READ)	/* Reading from device writes memory */
			pte->pg_m = 1;
#ifdef sun
		pte->pg_r = 1;		/* In any event, it was referenced */
#endif
		pte += CLSIZE;
		npf -= CLSIZE;
	}
}

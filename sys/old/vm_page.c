/*	@(#)vm_page.c 1.1 94/10/31 SMI; from UCB 4.38 83/07/10	*/

#include <machine/reg.h>
#include <machine/pte.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/text.h>
#include <sys/cmap.h>
#include <sys/vm.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/file.h>
#include <sys/trace.h>
#ifdef sun
#include <sys/map.h>
#include <sys/kernel.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#endif

int	nohash = 0;
/*
 * Handle a page fault.
 *
 * Basic outline
 *	If page is allocated, but just not valid:
 *		Wait if intransit, else just revalidate
 *		Done
 *	Compute <vp,bn> from which page operation would take place
 *	If page is text page, and filling from file system or swap space:
 *		If in free list cache, reattach it and then done
 *	Allocate memory for page in
 *		If block here, restart because we could have swapped, etc.
 *	Lock process from swapping for duration
 *	Update pte's to reflect that page is intransit.
 *	If page is zero fill on demand:
 *		Clear pages and flush free list cache of stale cacheing
 *		for this swap page (e.g. before initializing again due
 *		to 407/410 exec).
 *	If page is fill from file and in buffer cache:
 *		Copy the page from the buffer cache.
 *	If not a fill on demand:
 *		Determine swap address and cluster to page in
 *	Do the swap to bring the page in
 *	Instrument the pagein
 *	After swap validate the required new page
 *	Leave prepaged pages reclaimable (not valid)
 *	Update shared copies of text page tables
 *	Complete bookkeeping on pages brought in:
 *		No longer intransit
 *		Hash text pages into core hash structure
 *		Unlock pages (modulo raw i/o requirements)
 *		Flush translation buffer
 *	Process pagein is done
 */
#ifdef TRACE
#define	pgtrace(e)	trace(e,v,u.u_procp->p_pid)
#else
#define	pgtrace(e)
#endif

int	preptofree = 1;		/* send pre-paged pages to free list */

pagein(virtaddr, dlyu)
	unsigned virtaddr;
	int dlyu;
{
	register struct proc *p;
	register struct pte *pte;
	register u_int v;
	struct vnode *vp;
	unsigned pf;
	int type, fileno;
	struct pte opte;
	register int i;
	int klsize;
	unsigned vsave;
	struct cmap *c;
	int j;
	daddr_t bn, bncache, bnswap;
	int si, sk;
	int swerror = 0;
#ifdef PGINPROF
#include <vax/mtpr.h>
	int otime, olbolt, oicr, s;
	long a;

	s = spl6();
	otime = time, olbolt = lbolt, oicr = mfpr(ICR);
#endif
	cnt.v_faults++;
	/*
	 * Classify faulted page into a segment and get a pte
	 * for the faulted page.
	 */
	vsave = v = clbase(btop(virtaddr));
	p = u.u_procp;
	if (isatsv(p, v)) {
		type = CTEXT;
		pte = tptopte(p, vtotp(p, v));
	} else if (isadsv(p, v)) {
		type = CDATA;
		pte = dptopte(p, vtodp(p, v));
	} else {
		type = CSTACK;
		pte = sptopte(p, vtosp(p, v));
	}
	if (pte->pg_v)
#ifdef sun
		return;			/* happens on sun mmapped pages */
#else
		panic("pagein");
#endif

	/*
	 * If page is reclaimable, reclaim it.
	 * If page is text and intransit, sleep while it is intransit,
	 * If it is valid after the sleep, we are done.
	 * Otherwise we have to start checking again, since page could
	 * even be reclaimable now (we may have swapped for a long time).
	 */
restart:
	if (pte->pg_fod == 0 && pte->pg_pfnum) {
		if (type == CTEXT && cmap[pgtocm(pte->pg_pfnum)].c_intrans) {
			pgtrace(TR_INTRANS);
			(void) sleep((caddr_t)p->p_textp, PSWP+1);
			pgtrace(TR_EINTRANS);
			pte = vtopte(p, v);
			if (pte->pg_v) {
valid:
				if (dlyu) {
					c = &cmap[pgtocm(pte->pg_pfnum)];
					if (c->c_lock) {
						c->c_want = 1;
						(void) sleep((caddr_t)c, PSWP+1);
						goto restart;
					}
					c->c_lock = 1;
				}
				newptes(pte, v, CLSIZE);
				cnt.v_intrans++;
				return;
			}
			goto restart;
		}
		/*
		 * If page is in the free list, then take
		 * it back into the resident set, updating
		 * the size recorded for the resident set.
		 */
		si = splimp();
		if (cmap[pgtocm(pte->pg_pfnum)].c_free) {
			pgtrace(TR_FRECLAIM);
			munlink(pte->pg_pfnum);
			cnt.v_pgfrec++;
			if (type == CTEXT)
				p->p_textp->x_rssize += CLSIZE;
			else
				p->p_rssize += CLSIZE;
		} else
			pgtrace(TR_RECLAIM);
		(void) splx(si);
		pte->pg_v = 1;
		if (anycl(pte, pg_m))
			pte->pg_m = 1;
		distcl(pte);
		if (type == CTEXT)
			distpte(p->p_textp, vtotp(p, v), pte);
		u.u_ru.ru_minflt++;
		cnt.v_pgrec++;
		if (dlyu) {
			c = &cmap[pgtocm(pte->pg_pfnum)];
			if (c->c_lock) {
				c->c_want = 1;
				(void) sleep((caddr_t)c, PSWP+1);
				goto restart;
			}
			c->c_lock = 1;
		}
		newptes(pte, v, CLSIZE);
#ifdef PGINPROF
		a = vmtime(otime, olbolt, oicr);
		rectime += a;
		if (a >= 0)
			vmfltmon(rmon, a, rmonmin, rres, NRMON);
		(void) splx(s);
#endif
		return;
	}
#ifdef PGINPROF
	(void) splx(s);
#endif
	/*
	 * <vp,bn> is where data comes from/goes to.
	 * <vp,bncache> is where data is cached from/to.
	 * <swapdev_vp,bnswap> is where data will eventually go.
	 */
	if (pte->pg_fod == 0) {
		fileno = -1;
		bnswap = bncache = bn = vtod(p, v, &u.u_dmap, &u.u_smap);
		vp = swapdev_vp;
	} else {
		fileno = ((struct fpte *)pte)->pg_fileno;
		bn = ((struct fpte *)pte)->pg_blkno;
		bnswap = vtod(p, v, &u.u_dmap, &u.u_smap);
		if (fileno > PG_FMAX)
			panic("pagein pg_fileno");
		if (fileno == PG_FTEXT) {
			if (p->p_textp == 0)
				panic("pagein PG_FTEXT");
			VOP_BMAP(p->p_textp->x_vptr, (daddr_t)0,
			    &vp, (daddr_t *)0);
			bncache = bn;
		} else if (fileno == PG_FZERO) {
			vp = swapdev_vp;
			bncache = bnswap;
		} else {
			printf("pagein: invalid fileno %d in pte %x\n",
				fileno, *(struct fpte *)pte);
			panic("pagein: invalid fileno");
		}
	}
	klsize = 1;
	opte = *pte;

	/*
	 * Check for text detached but in free list.
	 * This can happen only if the page is filling
	 * from a inode or from the swap device, (e.g. not when reading
	 * in 407/410 execs to a zero fill page.)
	 * Honor lock bit to avoid races with pageouts.
	 */
	if (type == CTEXT && fileno != PG_FZERO && !nohash) {
		si = splimp();
		while ((c = mfind(vp, bncache)) != 0) {
			pf = cmtopg(c - cmap);
			if (c->c_lock == 0)
				break;
			mwait(pf);
		}
		if (c) {
			if (c->c_type != CTEXT || c->c_gone == 0 ||
			    c->c_free == 0)
				panic("pagein mfind");
			p->p_textp->x_rssize += CLSIZE;
			/*
			 * Following code mimics memall().
			 */
			pf = cmtopg(c - cmap);
			munlink(pf);
			for_CLSIZE(j) {
				*(int *)&pte[j] = pf++;
				pte[j].pg_prot = opte.pg_prot;
			}
			c->c_free = 0;
			c->c_gone = 0;
			if (c->c_intrans || c->c_want)
				panic("pagein intrans|want");
			c->c_lock = 1;
			if (c->c_page != vtotp(p, v))
				panic("pagein c_page chgd");
			c->c_ndx = p->p_textp - &text[0];
			if (vp == swapdev_vp) {
				cnt.v_xsfrec++;
				pgtrace(TR_XSFREC);
			} else {
				cnt.v_xifrec++;
				pgtrace(TR_XIFREC);
			}
			cnt.v_pgrec++;
			u.u_ru.ru_minflt++;
			if (vp != swapdev_vp) {
				c = mfind(swapdev_vp, bnswap);
				if (c)
					munhash(swapdev_vp, bnswap);
				pte->pg_m = 1;
			}
			(void) splx(si);
			goto skipswap;
		}
		(void) splx(si);
	}

	/*
	 * Wasn't reclaimable or reattachable.
	 * Have to prepare to bring the page in.
	 * We allocate the page before locking so we will
	 * be swappable if there is no free memory.
	 * If we block we have to start over, since anything
	 * could have happened.
	 */
	sk = splimp();		/* lock memalls from here into kluster */
	if (freemem < CLSIZE * KLMAX) {
		pgtrace(TR_WAITMEM);
		while (freemem < CLSIZE * KLMAX)
			(void) sleep((caddr_t)&freemem, PSWP+2);
		pgtrace(TR_EWAITMEM);
		(void) splx(sk);
		pte = vtopte(p, v);
		if (pte->pg_v)
			goto valid;
		goto restart;
	}

	/*
	 * Now can get memory and committed to bringing in the page.
	 * Lock this process, get a page,
	 * construct the new pte, and increment
	 * the (process or text) resident set size.
	 */
	p->p_flag |= SPAGE;
	if (memall(pte, CLSIZE, p, type) == 0)
		panic("pagein memall");
	pte->pg_prot = opte.pg_prot;
	pf = pte->pg_pfnum;
	cmap[pgtocm(pf)].c_intrans = 1;
	distcl(pte);
	if (type == CTEXT) {
		p->p_textp->x_rssize += CLSIZE;
		distpte(p->p_textp, vtotp(p, v), pte);
	} else
		p->p_rssize += CLSIZE;

	/*
	 * Two cases: either fill on demand (zero, or from file or text)
	 * or from swap space.
	 */
	if (opte.pg_fod) {
		pte->pg_m = 1;
		if (fileno == PG_FZERO || fileno == PG_FTEXT) {
			/*
			 * Flush any previous text page use of this
			 * swap device block.
			 */
			si = splimp();
			if (type == CTEXT) {
				c = mfind(swapdev_vp, bnswap);
				if (c)
					munhash(swapdev_vp, bnswap);
			}
			(void) splx(si);
			/*
			 * If zero fill, short-circuit hard work
			 * by just clearing pages.
			 */
			if (fileno == PG_FZERO) {
				pgtrace(TR_ZFOD);
				for_CLSIZE(i) {       
					extern char CADDR1[];
					
					mapin(CMAP1, btop(CADDR1), pf+i, 
						 1, PG_V | PG_KW); 
					bzero(CADDR1, NBPG);
	/* mapping is invalid after bzero, page flush. */
					vac_pageflush((caddr_t)CADDR1); 
				}
				if (type != CTEXT)
					cnt.v_zfod += CLSIZE;
				(void) splx(sk);
				goto skipswap;
			}
			pgtrace(TR_EXFOD);
			cnt.v_exfod += CLSIZE;
		} else
			panic("pagein vread");
		/*
		 * Fill from inode.  Try to find adjacent
		 * pages to bring in also.
		 */
		v = fodkluster(p, v, pte, &klsize, vp, &bn);
		bncache = bn;
		(void) splx(sk);
		/*
		 * Blocks of an executable may still be in the buffer
		 * cache, so we explicitly flush them out to disk
		 * so that the proper data will be paged in.
		 */
		blkflush(vp, bn, (long)(klsize * CLBYTES));
#ifdef TRACE
		if (type != CTEXT)
			trace(TR_XFODMISS, vp, bn);
#endif
	} else {
		if (opte.pg_pfnum)
			panic("pagein pfnum");
		pgtrace(TR_SWAPIN);
		/*
		 * Fill from swap area.  Try to find adjacent
		 * pages to bring in also.
		 */
		v = kluster(p, v, pte, B_READ, &klsize,
		    (type == CTEXT) ? kltxt :
		    ((p->p_flag & SSEQL) ? klseql : klin), bn);
		(void) splx(sk);
		/* THIS COULD BE COMPUTED INCREMENTALLY... */
		bncache = bn = vtod(p, v, &u.u_dmap, &u.u_smap);
	}

	distcl(pte);
	swerror = swap(p, bn, ptob(v), klsize * ctob(CLSIZE),
	    B_READ, B_PGIN, vp, 0); 
#ifdef TRACE
	trace(TR_PGINDONE, vsave, u.u_procp->p_pid);
#endif

	/*
	 * Instrumentation.
	 */
	u.u_ru.ru_majflt++;
	cnt.v_pgin++;
	cnt.v_pgpgin += klsize * CLSIZE;
#ifdef PGINPROF
	a = vmtime(otime, olbolt, oicr) / 100;
	pgintime += a;
	if (a >= 0)
		vmfltmon(pmon, a, pmonmin, pres, NPMON);
#endif

skipswap:
	/*
	 * Fix page table entries.
	 *
	 * Only page requested in is validated, and rest of pages
	 * can be ``reclaimed''.  This allows system to reclaim prepaged pages
	 * quickly if they are not used and memory is tight.
	 */
	pte = vtopte(p, vsave);
	pte->pg_v = 1;
	distcl(pte);
	if (type == CTEXT) {
		if (swerror) {
			xinval(p->p_textp->x_vptr);
		} else {
			distpte(p->p_textp, vtotp(p, vsave), pte);
			if (opte.pg_fod)
				p->p_textp->x_flag |= XWRIT;
		}
		wakeup((caddr_t)p->p_textp);
	}

	/*
	 * Memall returned page(s) locked.  Unlock all
	 * pages in cluster.  If locking pages for raw i/o
	 * leave the page which was required to be paged in locked,
	 * but still unlock others.
	 * If text pages, hash into the cmap situation table.
	 */
	pte = vtopte(p, v);
	for (i = 0; i < klsize; i++) {
		c = &cmap[pgtocm(pte->pg_pfnum)];
		c->c_intrans = 0;
		if (type == CTEXT && c->c_blkno == 0 && bncache && !nohash
		    && !swerror) {
			mhash(c, vp, bncache);
			bncache += CLBYTES / DEV_BSIZE;
		}
		if (v != vsave || !dlyu)
			munlock(pte->pg_pfnum);
		if (v != vsave && type != CTEXT && preptofree &&
		    opte.pg_fod == 0) {
			/*
			 * Throw pre-paged data/stack pages at the
			 * bottom of the free list.
			 */
			p->p_rssize -= CLSIZE;
			memfree(pte, CLSIZE, 0);
		}
		newptes(pte, v, CLSIZE);
		v += CLSIZE;
		pte += CLSIZE;
	}

	/*
	 * All done.
	 */
	p->p_flag &= ~SPAGE;

#ifdef DPAGEOUT
	/*
	 * If process is declared fifo, memory is tight,
	 * and this was a data page-in, free memory
	 * klsdist pagein clusters away from the current fault.
	 */
	if ((p->p_flag&SSEQL) && freemem < lotsfree && type == CDATA) {
		int k = (vtodp(p, vsave) / CLSIZE) / klseql;
#ifdef notdef
		if (vsave > u.u_vsave)
			k -= klsdist;
		else
			k += klsdist;
		dpageout(p, k * klseql * CLSIZE, klout*CLSIZE);
		u.u_vsave = vsave;
#else
#ifdef sun
		ctxunload(p);		/* update software ref and mod bits */
#endif
		dpageout(p, (k - klsdist) * klseql * CLSIZE, klout*CLSIZE);
		dpageout(p, (k + klsdist) * klseql * CLSIZE, klout*CLSIZE);
#endif
	}
#else
#ifdef lint
	klsdist = klsdist;
#endif lint
#endif DPAGEOUT
}

#ifdef DPAGEOUT
/*
 * Take away n pages of data space
 * starting at data page dp.
 * Used to take pages away from sequential processes.
 * Mimics pieces of code in pageout() below.
 */
dpageout(p, dp, n)
	struct proc *p;
	int dp, n;
{
	register struct cmap *c;
	int i, klsize;
	register struct pte *pte;
	unsigned v;
	daddr_t daddr;

	if (dp < 0) {
		n += dp;
		dp = 0;
	}
	if (dp + n > p->p_dsize)
		n = p->p_dsize - dp;
	for (i = 0; i < n; i += CLSIZE, dp += CLSIZE) {
		pte = dptopte(p, dp);
		if (pte->pg_fod || pte->pg_pfnum == 0)
			continue;
		c = &cmap[pgtocm(pte->pg_pfnum)];
		if (c->c_lock || c->c_free)
			continue;
		if (pte->pg_v) {
#ifdef sun
#ifdef VAC
			if (vac && p->p_ctx != 0) {
				int proc2_ctx = getcontext();

				setcontext(
				    (int)(p->p_ctx->ctx_context));
				vac_pageflush(ptob(v));
				setcontext(proc2_ctx);
			}
#endif VAC
#endif sun
			pte->pg_v = 0;
			if (anycl(pte, pg_m))
				pte->pg_m = 1;
			distcl(pte);
		}
		if (dirtycl(pte)) {
			if (bswlist.av_forw == NULL)
				continue;
			mlock(pte->pg_pfnum);
			pte->pg_m = 0;
			distcl(pte);
			p->p_poip++;
			v = kluster(p, dptov(p, dp), pte, B_WRITE,
				&klsize, klout, (daddr_t)0);
			/* THIS ASSUMES THAT p == u.u_procp */
			daddr = vtod(p, v, &u.u_dmap, &u.u_smap);
#ifdef sun
#ifdef VAC
			if (vac && p->p_ctx != 0) {
				int proc2_ctx = getcontext();

				setcontext(
				    (int)(p->p_ctx->ctx_context));
				vac_flush(ptob(v), klsize * ctob(CLSIZE));
				setcontext(proc2_ctx);
			}
#endif VAC
#endif sun
			(void) swap(p, daddr, ptob(v), klsize * ctob(CLSIZE),
			    B_WRITE, B_DIRTY, swapdev_vp, pte->pg_pfnum);
		} else {
			if (c->c_gone == 0)
				p->p_rssize -= CLSIZE;
			memfree(pte, CLSIZE, 0);
			cnt.v_seqfree += CLSIZE;
		}
	}
}
#endif DPAGEOUT

int	fifo = 0;
int	potime = 1;

/*
 * The page out daemon, which runs as process 2.
 *
 * As long as there are at least lotsfree pages,
 * this process is not run.  When the number of free
 * pages stays in the range desfree to lotsfree,
 * this daemon runs through the pages in the loop
 * at a rate determined in vmsched(), simulating the missing
 * hardware reference bit, and cleaning pages and transferring
 * them to the free list.
 */
pageout()
{
	register struct proc *rp;
	register struct text *xp;
	register struct cmap *c;
	register struct pte *pte;
	int count, pushes;
	swblk_t daddr;
	unsigned v;
	int maxhand = pgtocm(maxfree);
	int klsize;
	int favored;

loop:
	/*
	 * Before sleeping, look to see if there are any swap I/O headers
	 * in the ``cleaned'' list that correspond to dirty
	 * pages that have been pushed asynchronously. If so,
	 * empty the list by calling cleanup().
	 *
	 * N.B.: We guarantee never to block while the cleaned list is nonempty.
	 */
	(void) spl6();
	if (bclnlist != NULL) {
		(void) spl0();
		cleanup();
		goto loop;
	}
	(void) sleep((caddr_t)&proc[2], PSWP+1);
	(void) spl0();
	count = 0;
	pushes = 0;
#ifdef sun
	potime++;
#endif
	favored = 0;
	while (nscan < desscan && freemem < lotsfree) {
top:
		/*
		 * An iteration of the clock pointer (hand) around the loop.
		 * Look at the page at hand.  If it is a
		 * locked (for physical i/o e.g.), system (u., page table)
		 * or free, then leave it alone.
		 * Otherwise, find a process and text pointer for the
		 * page, and a virtual page number in either the
		 * process or the text image.
		 */
		c = &cmap[hand];
		if (c->c_lock || c->c_free)
			goto skip;
		switch (c->c_type) {

		case CSYS:
			goto skip;

		case CTEXT: {
#ifndef vax
			int ref = 0, mod = 0;
#endif

			xp = &text[c->c_ndx];
#ifndef vax
			for (rp = xp->x_caddr; rp; rp = rp->p_xlink) {
				pte = tptopte(rp, c->c_page);
				ptesync(rp, tptov(rp, c->c_page));
				if (pte->pg_r)
					ref = 1;
				if (pte->pg_m)
					mod = 1;
				if (rp->p_flag & SFAVORD)
					favored = 1;
			}
#endif
			rp = xp->x_caddr;
			v = tptov(rp, c->c_page);
			pte = tptopte(rp, c->c_page);
#ifndef vax
			pte->pg_r = ref;
			pte->pg_m = mod;
#endif
			break;
			}

		case CDATA:
		case CSTACK:
			rp = &proc[c->c_ndx];
			while (rp->p_flag & SNOVM)
				rp = rp->p_xlink;
			favored = rp->p_flag & SFAVORD;
			xp = rp->p_textp;
			if (c->c_type == CDATA) {
				v = dptov(rp, c->c_page);
				pte = dptopte(rp, c->c_page);
			} else {
				v = sptov(rp, c->c_page);
				pte = sptopte(rp, c->c_page);
			}
#ifndef vax
			ptesync(rp, v);
#endif
			break;
		}
		if (favored && (avefree >= minfree || freemem >= minfree))
			goto skip;

		if (pte->pg_pfnum != cmtopg(hand))
			panic("bad c_page");

		/*
		 * If page is valid; make invalid but reclaimable.
		 * If this pte is not valid, then it must be reclaimable
		 * and we can add it to the free list.
		 */
#ifdef vax
		if (pte->pg_v) {
			pte->pg_v = 0;
#else
		if (pte->pg_r) {
			pte->pg_r = 0;
#endif
			if (anycl(pte, pg_m))
				pte->pg_m = 1;
			distcl(pte);
			if (c->c_type == CTEXT)
				distpte(xp, vtotp(rp, v), pte);
#ifdef sun
			rp->p_flag |= SPTECHG;
#endif
			if ((rp->p_flag & (SSEQL|SUANOM)) || fifo ||
			    rp->p_rssize > rp->p_maxrss)
				goto take;
		} else {
take:
#ifndef vax
			if (pte->pg_v) {
				pte->pg_v = 0;
				if (anycl(pte, pg_m))
					pte->pg_m = 1;
				distcl(pte);
				if (c->c_type == CTEXT)
					distpte(xp, vtotp(rp, v), pte);
				/*
				 * This page becomes invalid, so is its
				 * virtual to physical mapping.
				 * It is a user page, the context register
				 * must be set before a page flush.
				 * If this proc does not have a context, we
				 * have flushed this page in ctxfree().
				 * Dirty page will be flushed before swap()
				 * is called below.
				 */
#ifdef sun
#ifdef VAC
				if (vac && rp->p_ctx != 0) {
					int proc2_ctx = getcontext();

					setcontext(
					    (int)(rp->p_ctx->ctx_context));
					vac_pageflush(ptob(v));
					setcontext(proc2_ctx);
				} 
#endif VAC
				rp->p_flag |= SPTECHG;
#endif sun
			}
#endif
			if (c->c_type != CTEXT) {
				/*
				 * Guarantee a minimal investment in data
				 * space for jobs in balance set.
				 */
				if (rp->p_rssize < saferss - rp->p_slptime)
					goto skip;
			}

			/*
			 * If the page is currently dirty, we
			 * have to arrange to have it cleaned before it
			 * can be freed.  We mark it clean immediately.
			 * If it is reclaimed while being pushed, then modified
			 * again, we are assured of the correct order of 
			 * writes because we lock the page during the write.  
			 * This guarantees that a swap() of this process (and
			 * thus this page), initiated in parallel, will,
			 * in fact, push the page after us.
			 *
			 * The most general worst case here would be for
			 * a reclaim, a modify and a swapout to occur
			 * all before the single page transfer completes.
			 */
			if (dirtycl(pte)) {
				/*
				 * Limit pushes to avoid saturating
				 * pageout device.
				 *
				 * MAGIC 4 BECAUSE WE RUN EVERY 1/4 SEC (clock)
				 */
				if (pushes > maxpgio / 4)
					goto skip;
				pushes++;
				/*
				 * If the process is being swapped out
				 * or about to exit, do not bother with its
				 * dirty pages
				 */
				if (rp->p_flag & (SLOCK|SWEXIT))
					goto skip;

				/*
				 * Now carefully make sure that there will
				 * be a header available for the push so that
				 * we will not block waiting for a header in
				 * swap().  The reason this is important is
				 * that we (proc[2]) are the one who cleans
				 * dirty swap headers and we could otherwise
				 * deadlock waiting for ourselves to clean
				 * swap headers.  The sleep here on &proc[2]
				 * is actually (effectively) a sleep on both
				 * ourselves and &bswlist, and this is known
				 * to swdone and swap in vm_swp.c.  That is,
				 * &proc[2] will be awakened both when dirty
				 * headers show up and also to get the pageout
				 * daemon moving.
				 */
		loop2:
				(void) spl6();
				if (bclnlist != NULL) {
					(void) spl0();
					cleanup();
					goto loop2;
				}
				if (bswlist.av_forw == NULL) {
					bswlist.b_flags |= B_WANTED;
					(void) sleep((caddr_t)&proc[2], PSWP+2);
					(void) spl0();
					/*
					 * Page disposition may have changed
					 * since process may have exec'ed,
					 * forked, exited or just about
					 * anything else... try this page
					 * frame again, from the top.
					 */
#ifdef sun
					potime++;
#endif
					goto top;
				}
				(void) spl0();

				mlock((unsigned)cmtopg(hand));
#ifdef sun
				/*
				 * Make sure there is enough kernelmap to map
				 * the pages in physstrat to avoid sleeping
				 * there and deadlocking.
				 */
				{
					long a;
					
					a = rmalloc(kernelmap,
					    (long)(KLMAX*CLSIZE));
					if (a == NULL) {
						munlock((unsigned)cmtopg(hand));
						goto loop;
					}
					rmfree(kernelmap,
					    (long)(KLMAX*CLSIZE), a);
				}
#endif
				uaccess(rp, Pushmap, &pushutl);
				/*
				 * Now committed to pushing the page...
				 */
				pte->pg_m = 0;
				distcl(pte);
				if (c->c_type == CTEXT)  {
					xp->x_poip++;
					distpte(xp, vtotp(rp, v), pte);
				} else
					rp->p_poip++;
				v = kluster(rp, v, pte, B_WRITE, &klsize, klout, (daddr_t)0);
				if (klsize == 0)
					panic("pageout klsize");
				daddr = vtod(rp, v, &pushutl.u_dmap, &pushutl.u_smap);
				/*
				 * Mapping from pushutl to rp's physical u
				 * is not valid anymore.
				 */
				vac_flush((caddr_t)&pushutl + KERNSTACK,
				    sizeof (struct user) - KERNSTACK);
#ifdef sun
#ifdef VAC
				/*
				 * Flush dirty pages before swap().
				 * We flushed the outgoing page when it
				 * turns invalid above, paranoid.
				 */
				if (vac && rp->p_ctx != 0) {
					int proc2_ctx = getcontext();

					setcontext(
					    (int)(rp->p_ctx->ctx_context));
					vac_flush(ptob(v),
					    klsize * ctob(CLSIZE));
					setcontext(proc2_ctx);
				}
#endif VAC
#endif sun
				(void) swap(rp, daddr, ptob(v),
				    klsize * ctob(CLSIZE), B_WRITE,
				    B_DIRTY, swapdev_vp, pte->pg_pfnum);
				/*
				 * The cleaning of this page will be
				 * completed later, in cleanup() called
				 * (synchronously) by us (proc[2]).  In
				 * the meantime, the page frame is locked
				 * so no havoc can result.
				 */
				goto skip;

			}
			/*
			 * Decrement the resident set size of the current
			 * text object/process, and put the page in the
			 * free list. Note that we don't give memfree the
			 * pte as its argument, since we don't want to destroy
			 * the pte.  If it hasn't already been discarded
			 * it may yet have a chance to be reclaimed from
			 * the free list.
			 */
			if (c->c_gone == 0)
				if (c->c_type == CTEXT)
					xp->x_rssize -= CLSIZE;
				else
					rp->p_rssize -= CLSIZE;
			memfree(pte, CLSIZE, 0);
			cnt.v_dfree += CLSIZE;

			/*
			 * We managed to add a page to the free list,
			 * so we give ourselves another couple of trips
			 * around the loop.
			 */
			count = 0;
		}
skip:
		cnt.v_scan++;
		nscan++;
		if (++hand >= maxhand) {
			hand = 0;
			cnt.v_rev++;
			if (count > 2) {
				/*
				 * Extremely unlikely, but we went around
				 * the loop twice and didn't get anywhere.
				 * Don't cycle, stop till the next clock tick.
				 */
				goto loop;
			}
			count++;
		}
	}
	goto loop;
}

/*
 * Process the ``cleaned'' list.
 *
 * Scan through the linked list of swap I/O headers
 * and free the corresponding pages that have been
 * cleaned by being written back to the paging area.
 * If the page has been reclaimed during this time,
 * we do not free the page.  As they are processed,
 * the swap I/O headers are removed from the cleaned
 * list and inserted into the free list.
 */
cleanup()
{
	register struct buf *bp;
	register struct proc *rp;
	register struct text *xp;
	register struct cmap *c;
	register struct pte *pte;
	struct pte *upte;
	unsigned pf;
	register int i;
	int s, center;

	for (;;) {
		s = spl6();
		if ((bp = bclnlist) == 0)
			break;
		bclnlist = bp->av_forw;
		(void) splx(s);
#ifdef sun
		if (bp->b_kmx) {
			bp->b_un.b_addr = bp->b_saddr;
			mapout(&Usrptmap[bp->b_kmx], bp->b_npte);
			rmfree(kernelmap, (long)bp->b_npte, (long)bp->b_kmx);
			bp->b_kmx = 0;
		}
#endif
		pte = vtopte(&proc[2], btop(bp->b_un.b_addr));
		center = 0;
		for (i = 0; i < bp->b_bcount; i += CLSIZE * NBPG) {
			pf = pte->pg_pfnum;
			munlock(pf);
			c = &cmap[pgtocm(pf)];
			if (pf != bp->b_pfcent) {
				if (c->c_gone) {
					memfree(pte, CLSIZE, 0);
					cnt.v_dfree += CLSIZE;
				}
				goto skip;
			}
			center++;
			switch (c->c_type) {

			case CSYS:
				panic("cleanup CSYS");

			case CTEXT:
				xp = &text[c->c_ndx];
				xp->x_poip--;
				if (xp->x_poip == 0)
					wakeup((caddr_t)&xp->x_poip);
				break;

			case CDATA:
			case CSTACK:
				rp = &proc[c->c_ndx];
				while (rp->p_flag & SNOVM)
					rp = rp->p_xlink;
				rp->p_poip--;
				if (rp->p_poip == 0)
					wakeup((caddr_t)&rp->p_poip);
				break;
			}
			if (c->c_gone == 0) {
				switch (c->c_type) {

				case CTEXT:
					upte = tptopte(xp->x_caddr, c->c_page);
					break;

				case CDATA:
					upte = dptopte(rp, c->c_page);
					break;

				case CSTACK:
					upte = sptopte(rp, c->c_page);
					break;
				}
				if (upte->pg_v) 
					goto skip;
				if (c->c_type == CTEXT)
					xp->x_rssize -= CLSIZE;
				else
					rp->p_rssize -= CLSIZE;
			}
			memfree(pte, CLSIZE, 0);
			cnt.v_dfree += CLSIZE;
skip:
			pte += CLSIZE;
		}
		if (center != 1)
			panic("cleanup center");
		bp->b_flags = 0;
		bp->av_forw = bswlist.av_forw;
		bswlist.av_forw = bp;
		if (bswlist.b_flags & B_WANTED) {
			bswlist.b_flags &= ~B_WANTED;
			wakeup((caddr_t)&bswlist);
		}
	}
	(void) splx(s);
}

/*
 * Kluster locates pages adjacent to the argument pages
 * that are immediately available to include in the pagein/pageout,
 * and given the availability of memory includes them.
 * It knows that the process image is contiguous in chunks;
 * an assumption here is that CLSIZE * KLMAX is a divisor of dmmin,
 * so that by looking at KLMAX chunks of pages, all such will
 * necessarily be mapped swap contiguous.
 */
int	noklust = 0;
int	klicnt[KLMAX];
int	klocnt[KLMAX];

kluster(p, v, pte0, rw, pkl, klsize, bn0)
	register struct proc *p;
	unsigned v;
	struct pte *pte0;
	int rw, *pkl, klsize;
	daddr_t bn0;
{
	int type, cl, clmax;
	int kloff, k, klmax;
	register struct pte *pte;
	int klback, klforw;
	register int i;
	unsigned v0;
	daddr_t bn;

	if (rw == B_READ)
		klicnt[0]++;
	else
		klocnt[0]++;
	*pkl = 1;
	if (noklust || klsize <= 1 || klsize > KLMAX || (klsize & (klsize - 1)))
		return (v);
	if (rw == B_READ && freemem < CLSIZE * KLMAX)
		return (v);
	if (isassv(p, v)) {
		type = CSTACK;
		cl = vtosp(p, v) / CLSIZE;
		clmax = p->p_ssize / CLSIZE;
	} else if (isadsv(p, v)) {
		type = CDATA;
		cl = vtodp(p, v) / CLSIZE;
		clmax = p->p_dsize / CLSIZE;
	} else {
		type = CTEXT;
		cl = vtotp(p, v) / CLSIZE;
		clmax = p->p_textp->x_size / CLSIZE;
	}
	kloff = cl & (klsize - 1);
	pte = pte0;
	bn = bn0;
	for (k = kloff; --k >= 0;) {
		if (type == CSTACK)
			pte += CLSIZE;
		else
			pte -= CLSIZE;
		if (type == CTEXT && rw == B_READ && bn) {
			bn -= CLBYTES / DEV_BSIZE;
			if (mfind(swapdev_vp, bn))
				break;
		}
		if (!klok(pte, rw))
			break;
	}
	klback = (kloff - k) - 1;
	pte = pte0;
	if ((cl - kloff) + klsize > clmax)
		klmax = clmax - (cl - kloff);
	else
		klmax = klsize;
	bn = bn0;
	for (k = kloff; ++k < klmax;) {
		if (type == CSTACK)
			pte -= CLSIZE;
		else
			pte += CLSIZE;
		if (type == CTEXT && rw == B_READ && bn) {
			bn += (CLBYTES / DEV_BSIZE);
			if (mfind(swapdev_vp, bn))
				break;
		}
		if (!klok(pte, rw))
			break;
	}
	klforw = (k - kloff) - 1;
	if (klforw + klback == 0)
		return (v);
	pte = pte0;
	if (type == CSTACK) {
		pte -= klforw * CLSIZE;
		v -= klforw * CLSIZE;
	} else {
		pte -= klback * CLSIZE;
		v -= klback * CLSIZE;
	}
	*pkl = klforw + klback + 1;
	if (rw == B_READ)
		klicnt[0]--, klicnt[*pkl - 1]++;
	else
		klocnt[0]--, klocnt[*pkl - 1]++;
	v0 = v;
	for (i = 0; i < *pkl; i++) {
		if (pte == pte0)
			goto cont;
		if (rw == B_WRITE) {
			mlock(pte->pg_pfnum);
			pte->pg_m = 0;
			distcl(pte);
			if (type == CTEXT)
				distpte(p->p_textp, vtotp(p, v), pte);
		} else {
			struct pte opte;
			int pf;

			opte = *pte;
			if (memall(pte, CLSIZE, p, type) == 0)
				panic("kluster");
			pte->pg_prot = opte.pg_prot;
			pf = pte->pg_pfnum;
			cmap[pgtocm(pf)].c_intrans = 1;
			distcl(pte);
			if (type == CTEXT) {
				p->p_textp->x_rssize += CLSIZE;
				distpte(p->p_textp, vtotp(p, v), pte);
			} else
				p->p_rssize += CLSIZE;
			distcl(pte);
		}
cont:
		pte += CLSIZE;
		v += CLSIZE;
	}
	return (v0);
}

klok(pte, rw)
	register struct pte *pte;
	int rw;
{
	register struct cmap *c;

	if (rw == B_WRITE) {
		if (pte->pg_fod)
			return (0);
		if (pte->pg_pfnum == 0)
			return (0);
		c = &cmap[pgtocm(pte->pg_pfnum)];
		if (c->c_lock || c->c_intrans)
			return (0);
		if (!dirtycl(pte))
			return (0);
		return (1);
	} else {
		if (pte->pg_fod)
			return (0);
		if (pte->pg_pfnum)
			return (0);
		return (1);
	}
}

/*
 * Fodkluster locates pages adjacent to the argument pages
 * that are immediately available to include in the pagein,
 * and given the availability of memory includes them.
 * It wants to page in a file system block if it can.
 */
int nofodklust = 0;
int fodklcnt[MAXBSIZE/DEV_BSIZE];

fodkluster(p, v0, pte0, pkl, vp, pbn)
	register struct proc *p;
	unsigned v0;
	struct pte *pte0;
	int *pkl;
	struct vnode *vp;
	daddr_t *pbn;
{
	int type, kl;
	int kloff, k;
	register struct pte *pte;
	int klback, klforw;
	register int i;
	daddr_t v;
	struct cmap *c;
	daddr_t bn, bnswap;
	int klsize;
	long vmin, vmax;

	klsize = p->p_textp->x_vptr->v_vfsp->vfs_bsize / CLBYTES;
	fodklcnt[0]++;
	*pkl = 1;
	if (nofodklust || klsize <= 1 || klsize > MAXBSIZE ||
	    (klsize & (klsize - 1))) {
		return (v0);
	}
	if (freemem < klsize) {
		return (v0);
	}
	if (isadsv(p, v0)) {
		type = CDATA;
		vmin = dptov(p, 0);
		vmax = dptov(p, clrnd(btoc(u.u_exdata.ux_dsize)) - CLSIZE);
	} else {
		type = CTEXT;
		vmin = tptov(p, 0);
		vmax = tptov(p, clrnd(btoc(u.u_exdata.ux_tsize)) - CLSIZE);
	}
	kl = (*pbn * DEV_BSIZE) / CLBYTES;
	kloff = kl & (klsize - 1);
	pte = pte0;
	bn = *pbn;
	v = v0;
	for (k = kloff; --k >= 0;) {
		v -= CLSIZE;
		if (v < vmin) {
			break;
		}
		pte -= CLSIZE;
		if (pte->pg_fod == 0) {
			break;
		}
		bn -= CLBYTES / DEV_BSIZE;
		if (((struct fpte *) pte)->pg_blkno != bn) {
			break;
		}
		if (type == CTEXT) {
			if (mfind(vp, bn)) {
				break;
			}
			/*
			 * Flush any previous text page use of this
			 * swap device block.
			 */
			bnswap = vtod(p, (unsigned)v, &u.u_dmap, &u.u_smap);
			c = mfind(swapdev_vp, bnswap);
			if (c)
				munhash(swapdev_vp, bnswap);
		}
	}
	klback = (kloff - k) - 1;
	pte = pte0;
	bn = *pbn;
	v = v0;
	for (k = kloff; ++k < klsize;) {
		v += CLSIZE;
		if (v > vmax) {
			break;
		}
		pte += CLSIZE;
		if (pte->pg_fod == 0) {
			break;
		}
		bn += CLBYTES / DEV_BSIZE;
		if (((struct fpte *) pte)->pg_blkno != bn) {
			break;
		}
		if (type == CTEXT) {
			if (mfind(vp, bn)) {
				break;
			}
			/*
			 * Flush any previous text page use of this
			 * swap device block.
			 */
			bnswap = vtod(p, (unsigned)v, &u.u_dmap, &u.u_smap);
			c = mfind(swapdev_vp, bnswap);
			if (c)
				munhash(swapdev_vp, bnswap);
		}
	}
	klforw = (k - kloff) - 1;
	if (klforw + klback == 0) {
		return (v0);
	}
	pte = pte0;
	pte -= klback * CLSIZE;
	v0 -= klback * CLSIZE;
	*pbn -= klback * (CLBYTES / DEV_BSIZE);
	*pkl = klforw + klback + 1;
	fodklcnt[0]--;
	fodklcnt[*pkl - 1]++;
	v = v0;
	for (i = 0; i < *pkl; i++) {
		if (pte != pte0) {
			struct pte opte;
			int pf;

			opte = *pte;
			if (memall(pte, CLSIZE, p, type) == 0)
				panic("fodkluster");
			pte->pg_prot = opte.pg_prot;
			pf = pte->pg_pfnum;
			pte->pg_m = 1;
			cmap[pgtocm(pf)].c_intrans = 1;
			distcl(pte);
			if (type == CTEXT) {
				p->p_textp->x_rssize += CLSIZE;
				distpte(p->p_textp, vtotp(p, v), pte);
			} else
				p->p_rssize += CLSIZE;
			distcl(pte);
		}
		pte += CLSIZE;
		v += CLSIZE;
	}
	return (v0);
}

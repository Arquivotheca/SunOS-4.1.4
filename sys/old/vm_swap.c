/*	@(#)vm_swap.c 1.1 94/10/31 SMI; from UCB 4.14 83/07/01	*/

#include <machine/pte.h>

#include <sys/param.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/text.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/cmap.h>
#include <sys/vm.h>
#include <sys/systm.h>

/*
 * Swap a process in.
 */
swapin(p)
	register struct proc *p;
{
	register struct text *xp;
	register int i, s;

	if (xp = p->p_textp) 
		xlock(xp);
	p->p_szpt = clrnd(ctopt(p->p_ssize+p->p_dsize+p->p_tsize+UPAGES));
	if (vgetpt(p, memall) == 0)
		goto nomem;
	if (vgetu(p, memall, Swapmap, &swaputl, (struct user *)0) == 0) {
		vrelpt(p);
		goto nomem;
	}

	swdspt(p, &swaputl, B_READ);
	/*
	 * Make sure swdspt didn't smash u. pte's
	 */
	for (i = 0; i < UPAGES; i++) {
		if (Swapmap[i].pg_pfnum != p->p_addr[i].pg_pfnum)
			panic("swapin");
	}
	vrelswu(p, &swaputl);
	if (xp) {
		xlink(p);
		xunlock(xp);
	}

	p->p_rssize = 0;
	s = spl6();
	if (p->p_stat == SRUN)
		setrq(p);
	p->p_flag |= SLOAD;
	if (p->p_flag & SSWAP) {
		swaputl.u_pcb.pcb_sswap = (int *)&u.u_ssave;
		p->p_flag &= ~SSWAP;
	}
	(void) splx(s);
	p->p_time = 0;
	multprog++;
	cnt.v_swpin++;
	/* Mapping from swaputl to u of proc p is not valid anymore */
	vac_flush((caddr_t)&swaputl, sizeof (struct user));
	return (1);

nomem:
	if (xp)
		xunlock(xp);
	return (0);
}

int	xswapwant, xswaplock;
/*
 * Swap out process p.
 * ds and ss are the old data size and the stack size
 * of the process, and are supplied during page table
 * expansion swaps.
 */
swapout(p, ds, ss)
	register struct proc *p;
	size_t ds, ss;
{
	register struct pte *map;
	register struct user *utl;
	register int a;
	int s;
	int rc = 1;

	s = 1;
	map = Xswapmap;
	utl = &xswaputl;
	if (xswaplock & s)
		if ((xswaplock & 2) == 0) {
			s = 2;
			map = Xswap2map;
			utl = &xswap2utl;
		}
	a = spl6();
	while (xswaplock & s) {
		xswapwant |= s;
		(void) sleep((caddr_t)map, PSWP);
	}
	xswaplock |= s;
	(void) splx(a);
	uaccess(p, map, utl);
	if (vgetswu(p, utl) == 0) {
		swkill(p, "swapout: no swap space for U area");
		rc = 0;
		goto out;
	}
	utl->u_ru.ru_nswap++;
	utl->u_odsize = ds;
	utl->u_ossize = ss;
	p->p_flag |= SLOCK;
#ifdef sun
	ctxfree(p);
#endif
	if (p->p_textp) {
		if (p->p_textp->x_ccount == 1)
			p->p_textp->x_swrss = p->p_textp->x_rssize;
		xccdec(p->p_textp, p);
	}
	p->p_swrss = p->p_rssize;
	vsswap(p, dptopte(p, 0), CDATA, 0, ds, &utl->u_dmap);
	vsswap(p, sptopte(p, CLSIZE-1), CSTACK, 0, ss, &utl->u_smap);
	if (p->p_rssize != 0)
		panic("swapout rssize");

	swdspt(p, utl, B_WRITE);
	/* mapping from *utl to u of proc p is not valid anymore */
	vac_flush((caddr_t)utl + KERNSTACK,
	    sizeof (struct user) - KERNSTACK);
	(void) spl6();		/* hack memory interlock XXX */
	vrelu(p, 1);
	if ((p->p_flag & SLOAD) && (p->p_stat != SRUN || p != u.u_procp))
		panic("swapout");
	p->p_flag &= ~SLOAD;
	vrelpt(p);
	p->p_flag &= ~SLOCK;
	p->p_time = 0;

	multprog--;
	cnt.v_swpout++;

	if (runout) {
		runout = 0;
		wakeup((caddr_t)&runout);
	}
out:
	xswaplock &= ~s;
	if (xswapwant & s) {
		xswapwant &= ~s;
		wakeup((caddr_t)map);
	}
	if (rc == 0) {
		a = spl6();
		p->p_flag |= SLOAD;
		if (p != u.u_procp && p->p_stat == SRUN)
			setrq(p);
		(void) splx(a);
	}
	return (rc);
}

/*
 * Swap the data and stack page tables in or out.
 * Only hard thing is swapping out when new pt size is different than old.
 * If we are growing new pt pages, then we must spread pages with 2 swaps.
 * If we are shrinking pt pages, then we must merge stack pte's into last
 * data page so as not to lose them (and also do two swaps).
 */
swdspt(p, utl, rdwri)
	register struct proc *p;
	register struct user *utl;
{
	register int szpt, tsz, ssz;
	int tdlast, slast, tdsz;
	register struct pte *pte;
	register int i;

	szpt = clrnd(ctopt(p->p_tsize+p->p_dsize+p->p_ssize+UPAGES));
	tsz = p->p_tsize / NPTEPG;
	if (szpt == p->p_szpt) {
#ifdef sun
		struct pte upte[UPAGES];

		/*
		 * Save u page pte's in upte so they aren't
		 * clobbered by reading in other pte's, then
		 * restore them after the read is done.  We
		 * know it's safe to clobber them during the
		 * I/O cause we won't run this proc until we're
		 * done here.
		 */
		if (rdwri == B_READ)
			for (i = 0; i < UPAGES; i++)
				upte[i] = p->p_addr[i];
		swpt(rdwri, p, 0, tsz, (p->p_szpt - tsz) * NBPG);
		if (rdwri == B_READ)
			for (i = 0; i < UPAGES; i++)
				p->p_addr[i] = upte[i];
#else
		swpt(rdwri, p, 0, tsz,
		    (p->p_szpt - tsz) * NBPG - UPAGES * sizeof (struct pte));
#endif
		swptstat.pteasy++;
		goto check;
	}
	if (szpt < p->p_szpt)
		swptstat.ptshrink++;
	else
		swptstat.ptexpand++;
	ssz = clrnd(ctopt(utl->u_ossize+UPAGES));
	if (szpt < p->p_szpt && utl->u_odsize && (utl->u_ossize+UPAGES)) {
		/*
		 * Page tables shrinking... see if last text+data and
		 * last stack page must be merged... if so, copy
		 * stack pte's from last stack page to end of last
		 * data page, and decrease size of stack pt to be swapped.
		 */
		tdlast = (p->p_tsize + utl->u_odsize) % (NPTEPG * CLSIZE);
		slast = (utl->u_ossize + UPAGES) % (NPTEPG * CLSIZE);
		if (tdlast && slast && tdlast + slast <= (NPTEPG * CLSIZE)) {
			swptstat.ptpack++;
			tdsz = clrnd(ctopt(p->p_tsize + utl->u_odsize));
			bcopy((caddr_t)sptopte(p, utl->u_ossize - 1),
			    (caddr_t)&p->p_p0br[tdsz * NPTEPG - slast],
			    (unsigned)(slast * sizeof (struct pte)));
			ssz -= CLSIZE;
		}
	}
	if (ssz)
		swpt(rdwri, p, szpt - ssz - tsz, p->p_szpt - ssz, ssz * NBPG);
	if (utl->u_odsize)
		swpt(rdwri, p, 0, tsz,
		    (clrnd(ctopt(p->p_tsize + utl->u_odsize)) - tsz) * NBPG);
check:
	for (i = 0; i < utl->u_odsize; i++) {
		pte = dptopte(p, i);
#ifdef sun
		if (pte->pg_fod == 0 && (pte->pg_pfnum||pte->pg_m))
#else
		if (pte->pg_v || pte->pg_fod == 0 && (pte->pg_pfnum||pte->pg_m))
#endif
			panic("swdspt");
	}
	for (i = 0; i < utl->u_ossize; i++) {
		pte = sptopte(p, i);
#ifdef sun
		if (pte->pg_fod == 0 && (pte->pg_pfnum||pte->pg_m))
#else
		if (pte->pg_v || pte->pg_fod == 0 && (pte->pg_pfnum||pte->pg_m))
#endif
			panic("swdspt");
	}
}

swpt(rdwri, p, doff, a, n)
	int rdwri;
	struct proc *p;
	int doff, a, n;
{

	if (n <= 0)
		return;
	(void) swap(p, (swblk_t)(p->p_swaddr + ctod(UPAGES) + ctod(doff)),
	    (caddr_t)&p->p_p0br[a * NPTEPG], n, rdwri, B_PAGET, swapdev_vp, 0);
}

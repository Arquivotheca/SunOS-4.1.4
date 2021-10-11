#ident  "@(#)module_ross625.c 1.1 94/10/31 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <machine/module.h>

extern u_int	swapl();
extern void	mmu_flushpagectx();
extern void	vac_pagectxflush();

#ifndef	MULTIPROCESSOR
#define	xc_attention()
#define	xc_dismissed()
#else	MULTIPROCESSOR
extern	xc_attention();
extern	xc_dismissed();
#endif	MULTIPROCESSOR

int	ross625_test_mcr = 0;
int	ross620_ibuf = 1;	/* XXX diagnostic flag for disabling ibuf */
int	ross625_flag = 0;	/* used by bcopy to test for 625 */
int	ross625_cw = 0;		/* turn off cache-wrap enable */
int	ross625_wbe = 1;	/* write-buffer enable flag */

extern int use_bcopy;	/* XXX Tempoarily disabling bcopy on 625 */

/*
 * Support for modules based on the Ross RT625
 * memory management unit.
 */
#ifndef lint
extern void	ross625_mmu_getasyncflt();
extern u_int	ross625_pte_rmw();
#else	lint
void	ross625_mmu_getasyncflt(){}
u_int	ross625_pte_rmw(p,a,o) u_int *p,a,o;
{ return swapl((*p&~a)|o,p);}
#endif	lint
extern u_int	ross625_pte_offon();
extern void	ross625_module_wkaround();

#ifdef	MULTIPROCESSOR
extern u_int  (*mp_pte_offon)();
#endif	MULTIPROCESSOR

/*
 * MCR defines
 */
#define	MCR_CM		0x00000400	/* cache mode, 1 = copyback */
#define	MCR_CWE		0x00200000	/* cache wrap enable */
#define	MCR_WBE		0x00080000	/* write buffer enable */
#define	MCR_CE		0x00000100	/* cache enable */

void
ross625_vac_init()
{
	static int calls = 0;
	static u_int setbits = 0;
	static u_int clrbits = 0;
	extern int vac_copyback;
	extern char *vac_mode;
	extern void ross625_vac_init_asm();
	extern u_int mmu_getcr();

	/*
	 * If not first call, then use bits as set on first call.
	 * All modules must have same cache modes.
	 */
	if (!calls++) {
		/*
		 * Make sure we return with the cache *off*.
		 */
		clrbits |= MCR_CE;
		if (vac_copyback)
			setbits |= MCR_CM;
		else
			clrbits |= MCR_CM;
		if (ross625_cw)
			setbits |= MCR_CWE;
		else
			clrbits |= MCR_CWE;
		if (ross625_wbe)
			setbits |= MCR_WBE;
		else
			clrbits |= MCR_WBE;
	
	}
	ross625_vac_init_asm(clrbits, setbits);
	if (mmu_getcr() & MCR_CM)
		vac_mode = "COPYBACK";
	else
		vac_mode = "WRITETHRU";
}
#ifndef lint
extern void	ross625_vac_flushall();
extern void	ross625_vac_usrflush();
extern void	ross625_vac_ctxflush();
extern void	ross625_vac_rgnflush();
extern void	ross625_vac_segflush();
extern void	ross625_vac_pageflush();
extern void	ross625_vac_pagectxflush();
extern void	ross625_vac_flush();
extern void	ross625_cache_on();
extern void	ross625_mmu_setctx();
#else	lint
void	ross625_vac_init_asm(c,s) u_int c, s; { vac_copyback = c ^ s; }
void	ross625_vac_flushall(){}
void	ross625_vac_usrflush(){}
void	ross625_vac_ctxflush(){}
void	ross625_vac_rgnflush(){}
void	ross625_vac_segflush(){}
void	ross625_vac_pageflush(){}
void	ross625_vac_pagectxflush(){}
void	ross625_vac_flush(){}
void	ross625_cache_on(){}
void	ross625_mmu_setctx(){}
#endif	lint

/*
 *	PSR IMPL  PSR REV  MCR IMPL  MCR REV	 CPU TYPE
 *	--------  -------  --------  -------     --------
 *	   0x1	    0xF	     0x1	0x7       RT620,RT625
 */

/*
 * ross625_module_identify() must be called before ross_module_identify()
 * since the latter will return 1 even for a hyperSPARC and I want to
 * avoid changing module_ross.c.
 */
int
ross625_module_identify(mcr)
	int	mcr;
{
        if (((mcr >> 24) & 0xff) == 0x17)
                return (1);
 
        return (0);
}

void	/*ARGSUSED*/
ross625_module_setup(mcr)
	register int	mcr;
{
	register int vers;
	extern int	cpuid;

	if (ross625_test_mcr) {
		prom_printf("ross625_module_setup: pretending mcr is %x, not %x\n",
			ross625_test_mcr, mcr);
		mcr = ross625_test_mcr;
	}

	ross625_flag = 1;
	/*
	 * XXX For now, don't use bcopy on 625 modules...
	 */
	use_bcopy = 0;

	vers = (mcr >> 24) & 0xF;
#ifdef lint
	vers = vers;
#endif

	v_mmu_getasyncflt = ross625_mmu_getasyncflt;

	v_vac_init = ross625_vac_init;
	v_vac_flushall = ross625_vac_flushall;
	v_vac_usrflush = ross625_vac_usrflush;
	v_vac_ctxflush = ross625_vac_ctxflush;
	v_vac_rgnflush = ross625_vac_rgnflush;
	v_vac_segflush = ross625_vac_segflush;
	v_vac_pageflush = ross625_vac_pageflush;
	v_vac_pagectxflush = ross625_vac_pagectxflush;
	v_vac_flush = ross625_vac_flush;
	v_cache_on = ross625_cache_on;

	v_mmu_setctx = ross625_mmu_setctx;
	v_pte_offon = ross625_pte_offon;
	v_module_wkaround = ross625_module_wkaround;
#ifdef	MULTIPROCESSOR
	mp_pte_offon = ross625_pte_offon;
#endif	MULTIPROCESSOR
}

#include <machine/vm_hat.h>
#include <vm/as.h>

/*
 * ross625_pte_offon: change some bits in a specified pte,
 * keeping the VAC and TLB consistant.
 *
 * This is the bottom level of manipulation for PTEs,
 * and it knows all the right things to do to flush
 * all the right caches. Further, it is safe in the face
 * of an MMU doing a read-modify-write cycle to set
 * the REF or MOD bits.
 */
unsigned
ross625_pte_offon(ptpe, aval, oval)
	union ptpe	   *ptpe;
	unsigned	    aval;
	unsigned	    oval;
{
	unsigned	    rpte;
	unsigned	    wpte;
	unsigned	   *ppte;
	struct pte	   *pte;
	addr_t		    vaddr;
	struct ptbl	   *ptbl;
	struct as	   *as;
	struct ctx	   *ctx;
	u_int		    cno;

	/*
	 * If the ptpe address is NULL, we have nothing to do.
	 */
	if (!ptpe)
		return 0;

	ppte = &ptpe->ptpe_int;
	rpte = *ppte;
	wpte = (rpte & ~aval) | oval;

	/*
	 * Early Exit Options: If no change, just return.
	 */
	if (wpte == rpte)
		return rpte;

	/*
	 * Fast Update Options: If the old PTE was not valid,
	 * then no flush is needed: update and return.
	 */
	if ((rpte & PTE_ETYPEMASK) != MMU_ET_PTE)
		return ross625_pte_rmw(ppte, aval, oval);

	pte = &ptpe->pte;
	ptbl = ptetoptbl(pte);
	vaddr = ptetovaddr(pte);

	/*
	 * Decide which context this mapping
	 * is active inside. If the vaddr is a
	 * kernel address, force context zero;
	 * otherwise, peel the address space and
	 * context used by the ptbl.
	 *
	 * Fast Update Option:
	 *	If there is no address space or
	 *	there is no context, then this
	 *	mapping must not be active, so
	 *	we just update and return.
	 */
	if ((unsigned)vaddr >= (unsigned)KERNELBASE)
		cno = 0;
	else if (((as = ptbl->ptbl_as) == NULL) ||
		 ((ctx = as->a_hat.hat_ctx) == NULL))
		return ross625_pte_rmw(ppte, aval, oval);
	else
		cno = ctx->c_num;

	/*
	 * The Cy7C605 has this silly bug: if it takes a TLB hit on a
	 * translation that is clean while doing a write, and during
	 * the M-bit update cycle discovers that the PTE is invalid or
	 * unwritable, it keeps some "hidden state" that it needs to
	 * do an update cycle; on the next TLB walk, it writes
	 * whatever PTE it finds back after reading it, then does one
	 * junk request cycle to the MBus and sends trash back to the
	 * CPU. So, we can't allow any CPU to take a TLB hit on a
	 * valid writable clean TLB entry if the memory image of that
	 * PTE has been changed to be invalid or nonwritable.
	 *
	 * So, if the PTE starts out clean/valid/writable and we are
	 * invalidating or decreasing access, we must prevent the TLB
	 * hit by capturing the remote CPUs.
	 *
	 * XXX - for some reason, if we do not always capture, the
	 * system watchdogs on occasion. methinks there are more
	 * problems here than we thought.
	 */
	xc_attention();
#if 0
	rpte = ross625_pte_rmw(ppte, aval, oval);
	mmu_flushpagectx(vaddr, cno);
#ifdef	VAC
#define	REQUIRE_VAC_FLUSH	(PTE_PFN_MASK|PTE_CE_MASK|PTE_ETYPEMASK)
	if ((vac) &&				/* if using a VAC and */
	    (rpte & PTE_CE_MASK) &&		/*    page was cacheable and */
	    ((rpte & REQUIRE_VAC_FLUSH) !=	/*    we changed physical page, */
	     (wpte & REQUIRE_VAC_FLUSH)))	/*    decached, or demapped, */
		vac_pagectxflush (vaddr, cno);	/*   flush data from VAC */
#endif
#else
	vac_pagectxflush (vaddr, cno);		/*   flush data from VAC */
	rpte = ross625_pte_rmw(ppte, aval, oval);
	mmu_flushpagectx(vaddr, cno);		/* flush tlb */
#endif

	xc_dismissed();
	return rpte;
}

/*
 * At this time no module specific workaround for ROSS
 * Heh, Heh!!!
 */
void
ross625_module_wkaround() {}

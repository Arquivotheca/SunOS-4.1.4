#ident "@(#)memerr.c 1.1 94/10/31 SMI"

/*LINTLIBRARY*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <machine/memerr.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <vm/page.h>
#include <sys/vnode.h>
#include <machine/buserr.h>
#include <machine/mmu.h>

extern void	enable_dvma();
extern void	disable_dvma();
extern void	hat_pagesync();
extern void	hat_pageunload();

int		report_ce_console = 0;	/* don't print messages on console */
int		report_ce = 1;
u_int		efervalue = 0x101;

/*
 * Memory error handling for sun4m
 */

sun4m_memerr_init()
{
#ifndef SAS
	if (report_ce)		/* Enables ECC checking, reports on CEs */
		efervalue = efervalue | EFER_EE | EFER_EI;
	else			/* Else just turn on checking */
		efervalue = efervalue | EFER_EE;
#ifdef DEBUG
	printf("efer set to 0x%x\n", efervalue);
#endif
	*(u_int *) EFER_VADDR = *(u_int *)EFER_VADDR | efervalue;

#endif
}

/* ARGSUSED */
void
sun4m_ebe_handler(type, reg, vaddr, fault, rp)
        u_int type, reg;
        addr_t vaddr;
        unsigned fault;         /* only meaningful if type == MERR_SYNC */
        struct regs *rp;        /* ditto */
{
}

int		correctable = 1;
u_int		testloc[2];

int
testecc()
{
	/*
	 * Try generating an ECC error by turning on the diagnostic register
	 */
	/* Assume uniprocessor and dvma has finished */
	/* Also assume cache is off */

	disable_dvma();
	if (correctable)
		*(u_int *) EFDR_VADDR = EFDR_GENERATE_MODE | 0x85;
	else
		*(u_int *) EFDR_VADDR = EFDR_GENERATE_MODE | 0x61;

	testloc[0] = 0x00000000;
	testloc[1] = 0x00000000;
#if 0
	vac_flush(&testloc[0], 4);
	vac_flush(&testloc[1], 4);
#endif
	*(u_int *) EFDR_VADDR = 0;
	enable_dvma();
	if ((testloc[0] != 0x80000000) && (testloc[1] != 0))
		u.u_r.u_rv.R_val1 = -1;
	else
		u.u_r.u_rv.R_val1 = 0;
}

int
fix_nc_ecc(efsr, efar0, efar1)
	u_int		efsr, efar0, efar1;
{
	/* Fix a non-correctable ECC error by "retiring" the page */

	u_int		pagenum;
	struct page    *pp;

	pagenum = (((efar0 & EFAR0_PA) << EFAR0_MID_SHFT) |
			((efar1 & EFAR1_PGNUM) >> 4));

	if (efsr & EFSR_ME) {
		printf("Multiple ECC Errors\n");
		return (-1);
	}
	if (efar0 & EFAR0_S) {
		printf("ECC error recovery: Supervisor mode\n");
		return (-1);
	}
	pp = (struct page *) page_numtopp(pagenum);
	if (pp = (struct page *) 0) {
		printf("Ecc error recovery: no page structure\n");
		return (-1);
	}
	if (pp->p_vnode == 0) {
		printf("ECC error recovery: no vnode\n");
		return (-1);
	}
	if (pp->p_intrans) {
		printf("Ecc error recovery: i/o in progress\n");
		return (-1);
	}
	hat_pagesync(pp);

	if (pp->p_mod && hat_kill_procs(pp, pagenum))
		return (-1);

	page_giveup(pagenum, pp);
	return (0);
}

page_giveup(pagenum, pp)
	u_int		pagenum;
	struct page    *pp;
{

	hat_pageunload(pp);
	page_abort(pp);
	if (pp->p_free)
		page_unfree(pp);
	pp->p_lock = 1;
	printf("page %x marked out of service.\n",
	    ptob(pagenum));
}

#if defined (SUN4M_35)

extern void     enable_dvma();
extern void     disable_dvma();
extern void     hat_pagesync();
extern void     hat_pageunload();
extern void	msparc1_memerr_init_asm();
extern void	msparc1_memerr_disable_asm();
extern void	msparc_memerr_init_asm();
extern void	msparc_memerr_disable_asm();
extern int	tsunami;

/*
 * msparc memory error (parity) handling
 */

void
msparc_memerr_init()
{
	extern int use_pe;

	if (use_pe) {
		if (tsunami)
			msparc1_memerr_init_asm();
		else
			msparc_memerr_init_asm();
	} else {
		printf("parity checking disabled\n");
	}
}

void
msparc_memerr_disable()
{
	if (tsunami)
        	msparc1_memerr_disable_asm(); 
	else
        	msparc_memerr_disable_asm(); 
} 


#define USER    0x10000         /* user-mode flag added to type in trap.c */

/* ARGSUSED */
int
msparc_ebe_handler(type, reg, addr, fault, rp)
	u_int type;
	u_int reg;
	addr_t addr;
	unsigned fault;		/* only meaningful if type == MERR_SYNC */
	struct regs *rp;	/* ditto */
{
	u_int ctx;
	struct pte pte;
	struct pte *ppte = &pte;
	int transient;
	u_int paddr;

	paddr = va2pa(addr);

	switch (type) {
	      case MERR_SYNC:
		if (reg & MMU_SFSR_PERR) {
			printf("Synchronous parity error\n");

			if (X_FAULT_TYPE(reg) == FT_TRANS_ERROR) {
				/* Parity errors during table walks are unrecoverable */
				panic("parity error during table walk");
			} else {
				printf("Attempting recovery..\n");
				transient = (msparc_parerr_reset(paddr) != -1);
				ctx = mmu_getctx();
				*(u_int *) ppte = mmu_probe(addr);
				printf(" ctx=0x%x vaddr=0x%x paddr=0x%x pte=0x%x transient=%d\n",
				       ctx, addr, paddr, *(u_int *)ppte, transient);
				
				if (msparc_parerr_recover(addr, ppte, transient) == -1)
					panic("unrecoverable parity error");
			}
			printf("System operation can continue.\n");
			return (0);
		} else if (reg & (MMU_SFSR_TO | MMU_SFSR_BE)) {
                        /*
                         * If it's the result of a user process getting a
                         * timeout or bus error on the sbus, just return and
                         * let the process be sent a bus error signal.
                         */
			if (fault & USER)
				return (-1);
		}
		ctx = mmu_getctx();
		*(u_int *) ppte = mmu_probe(addr);
		printf("Non-parity Synchronous memory error\n");
		printf(" ctx=0x%x vaddr=0x%x pte=0x%x fsr=0x%x\n",
		       ctx, addr, *(u_int *)ppte, reg);
		panic("sync memory error");
		/*NOTREACHED*/
		break;

	      case MERR_ASYNC:
		printf("Asynchronous memory error\n");
		printf("addr=0x%x reg=0x%x\n", (int)addr, (int)reg);
		panic("async memory error");
		/*NOTREACHED*/
		break;
		
	      default:
		panic("msparc_ebe_handler: invalid type");
		break;
	}
}

/*
 * Patterns to use to determine if a location has a hard or soft parity
 * error.
 * The zero is also an end-of-list marker, as well as a pattern.
 */
static u_int parerr_patterns[] = {
	0xAAAAAAAA,	/* Alternating ones and zeroes */
	0x55555555,	/* Alternate the other way */
	0x01010101,	/* Walking ones ... */
	0x02020202,	/* ... four bytes at once ... */
	0x04040404,	/* ... from right to left */
	0x08080808,
	0x10101010,
	0x20202020,
	0x40404040,
	0x80808080,
	0x7f7f7f7f,	/* And now walking zeros, from left to right */
	0xbfbfbfbf,
	0xdfdfdfdf,
	0xefefefef,
	0xf7f7f7f7,
	0xfbfbfbfb,
	0xfdfdfdfd,
	0xfefefefe,
	0xffffffff,	/* All ones */
	0x00000000,	/* All zeroes -- must be last! */
};

/*
 * Reset a parity error so that we can continue operation.
 * Also, see if we get another parity error at the same location.
 * Return 0 if error reset, -1 if not.
 * %%% We need to test all the words in a cache line, if cache is on.
 */

extern void stphys();
extern u_int ldphys();

int
msparc_parerr_reset(addr)
	addr_t	addr;
{
	int	retval = 0;
	u_int	*p, i;

        /*
         * Test the word with successive patterns, to see if the parity
         * error is a permanent problem.
         */
	memerr_disable();
        for (p = parerr_patterns; *p; p++)
        {
                stphys((u_int)addr, *p);      /* store the pattern */
                i = ldphys((u_int)addr);

                if (i != *p)
                {
                        printf("Parity error at 0x%x with pattern 0x%x\n", addr, *p);
                        retval++;
                }
        }
	memerr_init();

        /*
         * Convert return value to what caller expects.
         */
        if (retval)
                retval = -1;

        printf("Parity error at %x is %s.\n", addr, retval ? "permanent" : "transient");
        return (retval);
}

/*
 * Recover from a parity error.  Returns 0 if successful, -1 if not.
 */

int
msparc_parerr_recover(vaddr, pte, pageok)
	addr_t	vaddr;
	struct	pte	*pte;
	int	pageok;
{
	struct	page	*pp;

	/*
	 * Pages with no page structures or vnode are kernel pages.
	 * We cannot regenerate them, since there is no copy of
	 * the data on disk.
	 * And if the page was being used for i/o, we can't recover?
	 */
	pp = page_numtopp(pte->PhysicalPageNumber);
	if (pp == (struct page *) 0) {
		printf("parity recovery: no page structure\n");
		return (-1);
	}
	if (pp->p_vnode == 0) {
		printf("parity recovery: no vnode\n");
		return (-1);
	}
	if (pp->p_intrans) {
		printf("parity recovery: i/o in progress\n");
		return (-1);
	}

	/*
	 * Sync all the page structures for this page, so that we get
	 * the bits for all the page structures, not just this one.
	 */
	hat_pagesync(pp);

	/*
	 * If the page was modified, then the disk version is out
	 * of date.  This means we have to kill all the processes
	 * that use the page.  We may find a kernel use of the page,
	 * which means we have to return an error indication so that
	 * the caller can panic.
	 */
	if (pp->p_mod)
		if (hat_kill_procs(pp, (u_int)vaddr) != 0)
			return (-1);

	/*
	 * Give up the page.  When the fetch is retried, the page
	 * will be automatically faulted in.  Note that we have to
	 * kill the processes before we give up the page, or else
	 * the page will no longer exist in the processes' address
	 * space.
	 */
	msparc_page_giveup(pte, pp, pageok);
	return (0);
}

/*
 * Give up this page, which contains the address that caused the
 * parity error.  If the parity error is permanent, then lock the
 * page so that it will never be used again (until we reboot).
 */
msparc_page_giveup(ppte, pp, pageok)
        struct pte      *ppte;
        struct page     *pp;
	int		 pageok;
{
        hat_pageunload(pp);
        page_abort(pp);

        /*
         * If retry failed, lock the page to keep it from being used again.
         * This will let the machine stay up for now, without the parity
	 * error affecting future processes.
	 *
         * XXX do we need to splvm() between the abort and the lock?
         */
        if (!pageok) {
                if (pp->p_free)
                        page_unfree(pp);
                pp->p_lock = 1;
        }
        printf("page %x %s service.\n",
                ptob(ppte->PhysicalPageNumber),
                pageok ? "back in" : "marked out of");
}
#endif SUN4M_35 

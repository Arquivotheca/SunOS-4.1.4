#ident  "@(#)module_swift.c 1.1 94/10/31 SMI"

/*
 * Copyright (c) 1990-1993 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/debug.h>
#include <machine/module.h>
#include <machine/cpu.h>
#include <machine/mmu.h>


/*
 * Support for modules based on the
 * FUJITSU SWIFT (microSPARC 2) memory management unit
 *
 * To enable this module, declare this near the top of module_conf.c:
 *
 *	extern int	swift_module_identify();
 *	extern void	swift_module_setup();
 *
 * and add this line to the module_info table in module_conf.c:
 *
 *	{ swift_module_identify, swift_module_setup },
 */

void	swift_mmu_print_sfsr();
u_int	swift_pte_offon();
void	swift_mmu_writeptp();

#ifndef lint
extern void     swift_mmu_getsyncflt();
extern void     swift_mmu_getasyncflt();
extern u_int	swift_pte_rmw();
extern void	swift_mmu_chk_wdreset();
extern void     swift_vac_flushall();
extern void     swift_vac_usrflush();
extern void     swift_vac_ctxflush();
extern void     swift_vac_rgnflush();
extern void     swift_vac_segflush();
extern void     swift_vac_pageflush();
extern void     swift_vac_pagectxflush();
extern void     swift_vac_flush();
extern void	swift_cache_init();
extern void	swift_cache_on();
#else   lint
void	swift_mmu_getsyncflt(){}
void	swift_mmu_getasyncflt(){}
u_int	swift_mmu_pte_rmw(){}
void	swift_mmu_chk_wdreset(){}
void    swift_vac_flushall(){}
void    swift_vac_usrflush(){}
void    swift_vac_ctxflush(){}
void    swift_vac_rgnflush(){}
void    swift_vac_segflush(){}
void    swift_vac_pageflush(){}
void    swift_vac_pagectxflush(){}
void    swift_vac_flush(){}
void	swift_cache_init(){}
void	swift_cache_on(){}
#endif  lint


#define SWIFT_KDNC		/* workaround for 1156639 */
#define SWIFT_KDNX		/* workaround for 1156640 */


#ifdef SWIFT_KDNC
int swift_kdnc = -1;	/* don't cache kernel data space */
#endif

#ifdef SWIFT_KDNX
int swift_kdnx = -1;	/* don't mark control space executable */
#endif

#ifndef lint
extern void	srmmu_mmu_flushall();
#else
void	srmmu_mmu_flushall();
#endif

extern int small_4m, no_vme, no_mix;
extern int hat_cachesync_bug;
static u_int swift_version = 0;
int swift = 0;

/*ARGSUSED*/
int
swift_module_identify(mcr)
	int	mcr;
{
	int 	psr;

	psr = getpsr();

        if (((psr >> 24) & 0xff) == 0x04) {
                return (1);
        }
        return (0);
}

/*ARGSUSED*/
void
swift_module_setup(mcr)
	int	mcr;
{
	extern u_int swift_getversion();
	int kdnc = 0;
	int kdnx = 0;

	swift_version = swift_getversion() >> 24;

	swift = 1;
	small_4m = 1;
	no_vme = 1;
	no_mix = 1;

	/* XXX - ??? */
	hat_cachesync_bug = 1;

	v_mmu_writeptp = swift_mmu_writeptp;
	v_mmu_print_sfsr = swift_mmu_print_sfsr;
	v_pte_offon = swift_pte_offon;
	
	/*
	 * MMU Fault operations
	 */
	v_mmu_chk_wdreset = swift_mmu_chk_wdreset;
	v_mmu_getsyncflt = swift_mmu_getsyncflt;
	v_mmu_getasyncflt = swift_mmu_getasyncflt;

	/*
	 * Cache operations
	 */
	v_vac_init = swift_cache_init;
	v_cache_on = swift_cache_on;
        v_cache_sync = swift_vac_flushall;
        v_vac_flushall = swift_vac_flushall;
       	v_vac_usrflush = swift_vac_usrflush;
       	v_vac_ctxflush = swift_vac_ctxflush;
       	v_vac_rgnflush = swift_vac_rgnflush;
       	v_vac_segflush = swift_vac_segflush;
       	v_vac_pageflush = swift_vac_pageflush;
       	v_vac_pagectxflush = swift_vac_pagectxflush;
       	v_vac_flush = swift_vac_flush;

	switch (swift_version) {

	case SWIFT_REV_1DOT0:
		/* pg1.0 Swifts. See bug id # 1139510 */
		v_mmu_flushseg = srmmu_mmu_flushall;
		v_mmu_flushrgn = srmmu_mmu_flushall;
		v_mmu_flushctx = srmmu_mmu_flushall;
		kdnc = 1;
		kdnx = 1;
		break;

	case SWIFT_REV_1DOT1:
		kdnc = 1;
		kdnx = 1;
		/* pg1.1 Swift. */
		break;

	case 0x20:
		/* pg2.0 Swift. */
		kdnc = 1;
		kdnx = 1;
		break;

	case 0x23:
		/* pg2.3 Swift. */
		kdnc = 1;
		kdnx = 1;
		break;

	case 0x25:
		/* pg2.5 Swift. */
		kdnx = 1;
		break;

	case 0x30:
		/* pg3.0 Swift. */
		kdnc = 1;
		kdnx = 1;
		break;

	case 0x31:
		/* pg3.1 Swift. */
		kdnx = 1;
		break;

	case 0x32:
		/* pg3.2 Swift. */
		break;


	default:
		break;
	}

#ifdef SWIFT_KDNC
	if (swift_kdnc == -1)
		swift_kdnc = kdnc;
#endif

#ifdef SWIFT_KDNX
	if (swift_kdnx == -1)
		swift_kdnx = kdnx;
#endif
}

#include <machine/vm_hat.h>
#include <vm/as.h>

/*
 * swift_pte_offon: change some bits in a specified pte,
 * keeping the VAC and TLB consistent.
 *
 * This version is correct for uniprocessors that implement
 * a write-through virtual cache. Cache flushing is an invalidation
 * function not dependent upon the ordering of vac & TLB flush.
 */
u_int
swift_pte_offon(ptpe, aval, oval)
	union ptpe *ptpe;	/* va of pte */
	u_int      aval;	/* don't preserve these bits */
	u_int      oval;	/* new pte bits */
{
	u_int     	*ppte;
	u_int      	rpte;	/* original pte value */
	u_int      	wpte;	/* new pte value */
	struct pte   	*pte;
	addr_t	    	vaddr;
	struct ptbl	*ptbl;
	struct as	*as;
	struct ctx	*ctx;
	u_int 		cno;
	extern char *etext;
#ifdef SWIFT_KDNX
	static int nonxcs = 0;
	struct pte *ptep;
	extern struct as kas;
#endif

	ASSERT((int)ptpe != 0);

#ifdef SWIFT_KDNX
	if (swift_kdnx == 1 && nonxcs == 0) {
		nonxcs = 1;
		ptep = (struct pte *) hat_ptefind(&kas, (addr_t)V_IOMMU_ADDR);
		ptep->AccessPermissions = MMU_STD_SRWUR;

		ptep = (struct pte *) hat_ptefind(&kas, (addr_t)V_SBUSCTL_ADDR);
		ptep->AccessPermissions = MMU_STD_SRWUR;

		/*
		 * make sure there are no mappings to non-main memory with
		 * execute permissions in the monitor's virtual address space
		 * or in the percpu area's
		 */
		for (vaddr = (addr_t)VA_PERCPUME;
		     vaddr < (addr_t)MONEND; vaddr += PAGESIZE) {

			/*
			 * get the pte for this va
			 */
			pte = (struct pte *)hat_ptefind(&kas, vaddr);
			rpte = ldphys((unsigned)pte - KERNELBASE);
			ptep = (struct pte *)&rpte;

			if (ptep->EntryType != MMU_ET_PTE)
				continue;

			/*
			 * if mapped to non-main memory then
			 * insure there are no execute permissions
			 */
			if (rpte & 0x0f000000) {
				u_char  perm = ptep->AccessPermissions;

				switch (ptep->AccessPermissions) {
                                /*
                                 * ACC 2,4,6 -> 0
                                 */
                                case MMU_STD_SRXURX:
                                case MMU_STD_SXUX:
                                case MMU_STD_SRX:
                                        perm = MMU_STD_SRUR;
                                        break;
                                /*
                                 * ACC 3 -> 1
                                 */
                                case MMU_STD_SRWXURWX:
                                        perm = MMU_STD_SRWURW;
                                        break;
                                /*
                                 * ACC 7 -> 5
                                 */
                                case MMU_STD_SRWX:
                                        perm = MMU_STD_SRWUR;
                                        break;
				default:
					break;
				}

				if (perm != ptep->AccessPermissions) {
					ptep->AccessPermissions = perm;
					stphys((unsigned)pte - KERNELBASE,
						rpte);
				}
			}
		}

		mmu_flushall();
	}
#endif

	ppte = &(ptpe->ptpe_int);

#ifdef SWIFT_KDNC
	/* XXX - check if page is to be marked writable */
	if (swift_kdnc == 1) {
	        pte = &ptpe->pte;
	        vaddr = ptetovaddr(pte);
		if (vaddr > (addr_t)&etext) {
			aval | = 0x80;
			oval &= ~0x80;
		}
	}
#endif SWIFT_KDNC


	rpte = *ppte;					/* read original value */
	wpte = (rpte & ~aval) | oval;			/* calculate new value */

	if (rpte == wpte)				/* if no change, */
		return rpte;				/*   all done. */

		
#ifdef SWIFT_KDNX
	if (swift_kdnx == 1) {
                /*
                 * Valid page entry and non-memory physical address
                 * then convert ACC=0x7 to ACC=0x5
                 * also convert ACC=0x6 to ACC=0x5
                 */
                if ((wpte & PTE_ETYPEMASK) == MMU_ET_PTE && 
			(wpte & 0x0f000000)) {
                        if (((wpte & 0x1c) == 0x1c)  ||
                            ((wpte & 0x1c) == 0x18)) {
                                oval = ((oval & ~0x1c) | 0x14);
				aval |= 0x1c;
			}
		}
	}
#endif
	/*
	 * Fast Update Options: If the old PTE was not valid,
	 * then no flush is needed: update and return.
	 */
	if ((rpte & PTE_ETYPEMASK) != MMU_ET_PTE)
		return swift_pte_rmw(ppte, aval, oval);

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
         *      If there is no address space or
         *      there is no context, then this
         *      mapping must not be active, so
         *      we just update and return.
         */
        if ((unsigned)vaddr >= (unsigned)KERNELBASE)
                cno = 0;
        else if (((as = ptbl->ptbl_as) == NULL) ||
                 ((ctx = as->a_hat.hat_ctx) == NULL))
                return swift_pte_rmw(ppte, aval, oval);
        else
                cno = ctx->c_num;

        rpte = swift_pte_rmw(ppte, aval, oval);
	mmu_flushpagectx(vaddr, cno);		/* flush mapping from TLB */
#ifdef	VAC
	if ((vac) && (rpte != wpte))	
		vac_pagectxflush(vaddr, cno);
#endif
	return rpte;
}

void
swift_mmu_print_sfsr(sfsr)
	u_int	sfsr;
{
	printf("MMU sfsr=%x:", sfsr);
	switch (sfsr & MMU_SFSR_FT_MASK) {
	case MMU_SFSR_FT_NO:
		printf(" No Error");
		break;
	case MMU_SFSR_FT_INV:
		printf(" Invalid Address");
		break;
	case MMU_SFSR_FT_PROT:
		printf(" Protection Error");
		break;
	case MMU_SFSR_FT_PRIV:
		printf(" Privilege Violation");
		break;
	case MMU_SFSR_FT_TRAN:
		printf(" Translation Error");
		break;
	case MMU_SFSR_FT_BUS:
		printf(" Bus Access Error");
		break;
	case MMU_SFSR_FT_INT:
		printf(" Internal Error");
		break;
	case MMU_SFSR_FT_RESV:
		printf(" Reserved Error");
		break;
	default:
		printf(" Unknown Error");
		break;
	}
	if (sfsr) {
		printf(" on %s %s %s at level %d",
			sfsr & MMU_SFSR_AT_SUPV ? "supv" : "user",
			sfsr & MMU_SFSR_AT_INSTR ? "instr" : "data",
			sfsr & MMU_SFSR_AT_STORE ? "store" : "fetch",
			(sfsr & MMU_SFSR_LEVEL) >> MMU_SFSR_LEVEL_SHIFT);
		
		if (sfsr & MMU_SFSR_BE)
			printf("\n\tBus Error");
		if (sfsr & MMU_SFSR_TO)
			printf("\n\tTimeout Error");
		if (sfsr & MMU_SFSR_PERR)
			printf("\n\tParity Error");
	}
	printf("\n");
}

/*
 * Swift writeptp function.
 *  This is required since microSPARC caches PTPs in the TLB.
 *  Storing a new level 0, 1 or 2 PTP must be followed by an
 *  appropriate level TLB flush. Level 0, 1 or 2 flush remove
 *  all PTEs within the implied address range and in addition
 *  remove *all* PTPs from the TLB (see Swift spec for details).
 */
void
swift_mmu_writeptp(ptp, value, addr, level, cxn)
	struct ptp *ptp;
	u_int	value;
	caddr_t	addr;
	int	level;
	int	cxn;
{
	u_int *iptp, old;

	iptp = (u_int *) ptp;
	old = *iptp;

	/* 
	 * Install new ptp before TLB flush, else may rereference
	 * leaving stale PTP in TLB.
	 */
	(void) swapl(value, (int *)iptp);

	if (PTE_ETYPE(old) == MMU_ET_PTP && cxn != -1) {

		switch (level) {
#if 0
		case 3:
			mmu_flushpagectx(addr, cxn);
			break;
#endif
		case 2:
			mmu_flushseg(addr, cxn);
			break;
		case 1:
			mmu_flushrgn(addr, cxn);
			break;
		case 0:
			mmu_flushctx(cxn);
			vac_ctxflush(cxn);
		}
	}
}


extern int	swift_mmu_ltic_ramcam();


/*
 * Lock Translation in TLB 
 *
 * Retrieve the translation(s) for the range of memory specified, and
 * lock them in the TLB; return number of translations locked down.
 *
 * FIXME -- this routine currently assumes that we are not using short
 * translations; if we want to lock down segment or region
 * translations, we need to replace the call to mmu_probe with
 * something that tells us the short translation bits, pass those bits
 * on to the tlb, and step our virtual address to the end of the
 * locked area.
 *
 * XXX - Too bad the PTE and RAM bits don't quite line up.
 */

int swift_tlb_lckcnt = 0;
int swift_tlb_maxlckcnt = 16;

int
swift_mmu_ltic(svaddr, evaddr)
	char  *svaddr;
	char  *evaddr;
{
	u_int		ram;
	u_int		utag;
	u_int		ltag;
	u_int		pte;
	int		rv = 0;
	u_int		perm;
	u_int		permbits;

	svaddr = (char *)((u_int)svaddr & MMU_PAGEMASK); 

	while (svaddr < evaddr) {

		pte = mmu_probe(svaddr, NULL);

		/*
		 * Only lock down valid supervisor mappings (ACC = 0x6, 0x7)
		 */
		if ((pte & (0x18|PTE_ETYPEMASK)) == (0x18|MMU_ET_PTE)) {

			/* 
			 * Upper tag - Swift Spec Rev 1.3 p122
			 *
			 * utag consists of virtual tag, context number,
			 * valid bit and level(?).
			 */

			utag = (u_int)svaddr | (0 << 4) | (1 << 3);


			/* 
			 * Lower tag - Swift Spec Rev 1.3 p123
			 *
			 * ltag consists of perm, supv, type & modified
			 */

			perm = (pte & 0x1c) >> 2;

			switch (perm) {

			case 0x6:
				permbits = 0x030;
				break;

			case 0x7:
				permbits = 0x070;
				break;

			default:
				/* only handle supv */
				break;
			}	

			ltag = permbits | (1 << 3) | (0 << 1) | 1;

			/* 
			 * Ram - Swift Spec Rev 1.3 p74
			 *
			 * ram consists of lvl, c, m, 1, acc and et 
			 * XXX - assume level3 pte
			 */

			ram = (0xe0000000) | pte | (1 << 6);

			/*
			 * call support routine to lock down mapping
			 * returns number of locked down entries or
			 * 0 if no entries left.
			 */
			if ((rv = swift_mmu_ltic_ramcam(ram, utag, ltag)) == 0)
				break;
		}
		svaddr += PAGESIZE;
	}
	return (rv);
}

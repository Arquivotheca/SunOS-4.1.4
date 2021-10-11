#ident  "@(#)module_tsu.c 1.1 94/10/31 SMI"

/*
 * Copyright (c) 1990,1991 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <machine/module.h>
#include <machine/cpu.h>
#include <machine/mmu.h>


/*
 * Support for modules based on the
 * TI TSUNAMI (microSPARC 1) memory management unit
 *
 * To enable this module, declare this near the top of module_conf.c:
 *
 *	extern int	tsu_module_identify();
 *	extern void	tsu_module_setup();
 *
 * and add this line to the module_info table in module_conf.c:
 *
 *	{ tsu_module_identify, tsu_module_setup },
 */

extern void     srmmu_mmu_flushall();
extern void     srmmu_mmu_flushpage();
#ifndef	lint
extern void	tsu_mmu_chk_wdreset();
extern void     tsu_mmu_getsyncflt();
extern void     tsu_mmu_getasyncflt();
extern void	tsu_cache_init();
extern void	tsu_cache_on();
extern void	tsu_cache_off();
extern void	tsu_cache_sync();
extern void	tsu_mmu_setctx();
#else
void	tsu_mmu_chk_wdreset(){}
void    tsu_mmu_getsyncflt(){}
void    tsu_mmu_getasyncflt(){}
void	tsu_cache_on(){}
void	tsu_cache_off(){}
void	tsu_cache_sync(){}
void	tsu_mmu_setctx(){}
#endif
void	tsu_mmu_writeptp();
void	tsu_mmu_print_sfsr();

extern int small_4m, no_vme, no_mix;

int tsu_cachebits = CPU_TSU_ID;	/* bits in MCR to enable */
int tsunami = 0;

void
tsu_cache_init()
{
	extern int use_ic;
	extern int use_dc;
	extern int use_ec;	/* enable ITBR */
	extern int use_cache;
	extern int cache;
	extern void tsu_cache_init_asm();

	if (use_cache) {
		if (use_ec) {
			tsu_cachebits &= ~CPU_TSU_ID; 
		} else {
			tsu_cachebits |= CPU_TSU_ID;
			printf("	ITBR DISABLED\n");
		}

		if (use_ic) {
			tsu_cachebits |= CPU_TSU_IE;
		} else {
			printf("	Instruction Cache DISABLED\n");
		}

		if (use_dc) {
			tsu_cachebits |= CPU_TSU_DE;
		} else {
			printf("	Data Cache DISABLED\n");
		}

	} else {
		printf("PAC DISABLED\n");
		cache = CACHE_NONE;
	}
	tsu_cache_init_asm();
}

/*
 *	PSR IMPL  PSR REV  MCR IMPL  MCR REV	 CPU TYPE
 *	--------  -------  --------  -------     --------
 *	   0x4	    0x1	     0x4	0x1	  Tsunami/Tsupernami
 */

/*ARGSUSED*/
int
tsu_module_identify(mcr)
	int	mcr;
{
	int	psr;

	psr = getpsr();

        if (((psr >> 24) & 0xff) == 0x41 &&
	    (((mcr >> 24) & 0xff) != 0x00)) /* covers viking 2.x case */
                return (1);

        return (0);
}


/*ARGSUSED*/
void
tsu_module_setup(mcr)
	int	mcr;
{
	small_4m = 1;
	no_vme = 1;
	no_mix = 1;
	tsunami = 1;

	/*
	 * MMU Fault operations
	 */

	v_mmu_chk_wdreset = tsu_mmu_chk_wdreset;
	v_mmu_getsyncflt = tsu_mmu_getsyncflt;
	v_mmu_getasyncflt = tsu_mmu_getasyncflt;
	v_mmu_writeptp = tsu_mmu_writeptp;

        /* The tsunami chip supports only two flush types, page and entire.
         * All other flush types (e.g. context, region, segment, ...) are
         * mapped to flush entire in the chip.  To avoid the borrowing
         * of contexts we map the corresponding flush routines to srmmu
         * flush all.
         */
        v_mmu_flushctx = srmmu_mmu_flushall;
        v_mmu_flushrgn = srmmu_mmu_flushall;
        v_mmu_flushseg = srmmu_mmu_flushall;
        v_mmu_flushpagectx = srmmu_mmu_flushall;

	v_mmu_setctx = tsu_mmu_setctx;
	v_mmu_print_sfsr = tsu_mmu_print_sfsr;

	/*
	 * Cache operations
	 */

	v_vac_init = tsu_cache_init;
	v_cache_on = tsu_cache_on;
	/* v_cache_off = tsu_cache_off */
	v_cache_sync = tsu_cache_sync;
}

void
tsu_mmu_print_sfsr(sfsr)
	u_int	sfsr;
{
	printf("MMU sfsr=%x:", sfsr);
	switch (sfsr & MMU_SFSR_FT_MASK) {
	      case MMU_SFSR_FT_NO: printf (" No Error"); break;
	      case MMU_SFSR_FT_INV: printf (" Invalid Address"); break;
	      case MMU_SFSR_FT_PROT: printf (" Protection Error"); break;
	      case MMU_SFSR_FT_PRIV: printf (" Privilege Violation"); break;
	      case MMU_SFSR_FT_TRAN: printf (" Translation Error"); break;
	      case MMU_SFSR_FT_BUS: printf (" Bus Access Error"); break;
	      case MMU_SFSR_FT_INT: printf (" Internal Error"); break;
	      case MMU_SFSR_FT_RESV: printf (" Reserved Error"); break;
	      default: printf (" Unknown Error"); break;
	}
	if (sfsr) {
		printf (" on %s %s %s at level %d",
			sfsr & MMU_SFSR_AT_SUPV ? "supv" : "user",
			sfsr & MMU_SFSR_AT_INSTR ? "instr" : "data",
			sfsr & MMU_SFSR_AT_STORE ? "store" : "fetch",
			(sfsr & MMU_SFSR_LEVEL) >> MMU_SFSR_LEVEL_SHIFT);
		
		if (sfsr & MMU_SFSR_BE)
			printf ("\n\tBus Error");
		if (sfsr & MMU_SFSR_TO)
			printf ("\n\tTimeout Error");
		if (sfsr & MMU_SFSR_PERR)
			printf ("\n\tParity Error");
	}
	printf("\n");
}

#include <machine/vm_hat.h>
#include <vm/as.h>

/*
 * The Tsunami writeptp function.
 */
void
tsu_mmu_writeptp(ptp, value, addr, level, cxn)
	struct ptp *ptp;
	u_int	value;
	caddr_t	addr;
	int	level;
	int	cxn;
{
	u_int *iptp, old;

	iptp = (u_int *) ptp;
	old = *iptp;

	(void) swapl(value, (int *)iptp);

	if (PTE_ETYPE(old) == MMU_ET_PTP && cxn != -1) {

		switch (level) {
		case 3:
			mmu_flushpagectx(addr, cxn);
			break;
		case 2:
			mmu_flushseg(addr, cxn);
			break;
		case 1:
			mmu_flushrgn(addr, cxn);
			break;
		case 0:
			mmu_flushctx(cxn);
			break;
		}
	}
}

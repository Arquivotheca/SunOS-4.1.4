/*	@(#)module.h 1.1 94/10/31 SMI	*/

#ifdef	NOPROM
#define	ROSS604		0x0010 		/* Ross 604 rev c, d, or e */
#define	ROSS604f	0x0011 		/* Ross 604 rev f */
#define	ROSS605b	0x001E 		/* Ross 605 rev b */
#define	ROSS605		0x001F 		/* Ross 605 rev a, a.1, or a.2 */
#else
#define	CPU_ROSS605	0x11
#define	CPU_ROSS625	0x1f
#define	CPU_VIKING	0x40
#define	CPU_TSUNAMI	0x41
#define	CPU_VIKING_MXCC	0x41
#define	CPU_SWIFT	0x04
#endif

/* mc_type defines */
#define	MC_MMC		0x01		/* galaxy MMC */
#define	MC_EMC		0x02		/* EMC */
#define	MC_LMC		0x03		/* EMC/LEGO */
#define	MC_SMC		0x04		/* EMC/SPAM */

/*
 * Viking revision defines
 */


#define VIK_REV_1DOT2   1		/* 1.2 vikings */
#define VIK_REV_2DOTX   2		/* 2.X (2.1, 2.2, 2.3, 2.4) Vikings */
#define VIK_REV_3DOT0   3		/* 3.0 Vikings */
#define VIK_REV_3DOT5   4		/* 3.5 Vikings */
#define VOY_REV_1DOT0   8

/*
 * Swift revision defines
 */

#define SWIFT_REV_1DOT0	0x00		/* 1.0 Swifts */
#define SWIFT_REV_1DOT1	0x11		/* 1.1 Swifts */
#define SWIFT_REV_2DOT1	0x20		/* 2.0, 2.1 Swifts */
#define SWIFT_REV_3DOT0	0x30		/* 3.0 Swifts */

#ifndef LOCORE
/*
 * We can dynamicly add or remove support for
 * modules of various sorts by adding them
 * to, or removing them from, this table.
 *
 * The semantics are: VERY VERY early in the
 * execution of the kernel, we loop through
 * calling the identify_func for each configured
 * module. The identify_func is responsible to
 * determine if running on that particular
 * module type. It should return 0 if not
 * that module type and a 1 if the correct type.
 * The identify_func must be VERY careful to 
 * correctly identify module types/revisions.
 * The Sun-4M Sys Arch Spec describes how
 * modules are identified using the IMPL/VERS
 * fields from the PSR or MCR. Some modules
 * tend to bump the MCR.REV field for each new silicon
 * revision so the primary identifier is usually
 * via the PSR.
 */
struct module_linkage {
	int	(*identify_func)(/* int mcr */);
	void	(*setup_func)(/* int mcr */);
};

/*
 * So we can see the "module_info" table
 * where we need it. The table itsself
 * is allocated and filled in the file
 *	machine/module_conf.c
 * which is available in binary configurations
 * so "module drivers" may be added.
 */
extern struct module_linkage	module_info[];

/*
 * The following pointers to functions are staticly
 * initialized to an innocuous "safe" value.
 * In general, MMU related things are set up to
 * do the right thing for the SPARC Reference MMU,
 * and VAC related things are pointed at an
 * empty stub somewhere, but this may change
 * without this header file being updated so
 * go check it out.
 *
 * It is the primary job of the "setup_func"
 * for a module to change these vectors wherever
 * necessary to reference the proper service
 * function for the detected module type.
 */
extern int    (*v_mmu_getcr)();
extern int    (*v_mmu_getctp)();
extern int    (*v_mmu_getctx)();
extern int    (*v_mmu_probe)();
extern void   (*v_mmu_setcr)();
extern void   (*v_mmu_setctp)();
extern void   (*v_mmu_setctx)();
extern void   (*v_mmu_flushall)();
extern void   (*v_mmu_flushctx)();
extern void   (*v_mmu_flushrgn)();
extern void   (*v_mmu_flushseg)();
extern void   (*v_mmu_flushpage)();
extern void   (*v_mmu_flushpagectx)();
extern void   (*v_mmu_getsyncflt)();
extern void   (*v_mmu_getasyncflt)();
extern void   (*v_mmu_chk_wdreset)();
extern void   (*v_mmu_sys_ovf)();
extern void   (*v_mmu_sys_unf)();
extern void   (*v_mmu_wo)();
extern void   (*v_mmu_wu)();
extern void   (*v_mmu_log_module_err)();
extern void   (*v_mmu_print_sfsr)();

extern u_int  (*v_pte_offon)();
extern void   (*v_mmu_writeptp)();
extern void   (*v_module_wkaround)();

#ifdef	VAC
extern void   (*v_vac_init)();
extern void   (*v_vac_flushall)();
extern void   (*v_vac_usrflush)();
extern void   (*v_vac_ctxflush)();
extern void   (*v_vac_rgnflush)();
extern void   (*v_vac_segflush)();
extern void   (*v_vac_pageflush)();
extern void   (*v_vac_pagectxflush)();
extern void   (*v_vac_flush)();
extern void   (*v_cache_on)();
extern void   (*v_cache_sync)();
extern int    (*v_vac_parity_chk_dis)();
#endif	VAC
#endif !LOCORE

/*      @(#)module_tsu_asm.s 1.1 94/10/31 SMI     
 *
 *	Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * assembly code support for modules based on the
 * TI TSUNAMI chip set.
 */

#include <sys/param.h>
#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/devaddr.h>
#include <percpu_def.h>
#include "assym.s"

#define	PA_SUNERGY_SYSCTL  0x71F00000 /* asi=20: system control/status */
 
        .seg    "text"
        .align  4
    	ALTENTRY(tsu_mmu_setctx)      	! void  srmmu_mmu_setctx(int v);
        set     RMMU_CTX_REG, %o5     	! set srmmu context number
        sta     %o0, [%o5]ASI_MOD
	or      %g0, FT_ALL<<8, %o0	! flush entire TLB 
        sta     %g0, [%o0]ASI_FLPR            
	retl
	nop
 
        !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        !!
        !! For sync faults, we need to check the fault type to determine
        !! where to look for the faulting address.  If it's a data fault,
        !! we get it from the sfar.  If it's an instruction fault, we get
        !! from the trap pc.
        !!
        !! NOTE: We are assuming:
        !!
        !!              l1 has the trap pc
        !!              l4 contains the fault type
        !!
        !!        These should be set up in the generic system trap handler.
        !!
        !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 
        ALTENTRY(tsu_mmu_getsyncflt)              
	mov     %l1, %g7                        ! for text faults use trap pc from l1
        and     %l4, 0xf, %g6                   ! l4 contains fault type
        cmp     %g6, T_TEXT_FAULT
        be      1f
        nop
        set     RMMU_FAV_REG, %g7               ! for data faults use sfar
        lda     [%g7]ASI_MOD, %g7

1:      set     RMMU_FSR_REG, %g6
        retl
        lda     [%g6]ASI_MOD, %g6
 
	ALTENTRY(tsu_mmu_chk_wdreset)
	set	PA_SUNERGY_SYSCTL, %o1		! system control/status
	lda	[%o1]ASI_MEM, %o0
	retl
	and	%o0, SYSCTLREG_WD, %o0		! mask WD reset bit


	ALTENTRY(tsu_mmu_getasyncflt)		! not supported
	sub	%g0, 1, %o1			! (mtos error)
	retl
	st	%o1, [%o0]

	/* 
	 * Tsunami has split I-, D- physical caches, flash clr only
 	 */

	/*
	 * void
	 * xxx_cache_init_asm()
	 */
	ALTENTRY(tsu_cache_init_asm)
	sta	%g0, [%g0]ASI_ICFCLR	! flash clear icache
	sta	%g0, [%g0]ASI_DCFCLR	! flash clear dcache
	set 	RMMU_CTL_REG, %o5
	lda 	[%o5]ASI_MOD, %o4	! fetch MCR 
	set	CPU_TSU_DE|CPU_TSU_IE|CPU_TSU_ID, %o3
	andn    %o4, %o3, %o4      
	sethi 	%hi(_tsu_cachebits), %o3
	ld	[%o3+%lo(_tsu_cachebits)], %o3
	or	%o4, %o3, %o4
	sta	%o4, [%o5]ASI_MOD	! update MCR 
	retl
	nop				! delay (don't optimize for now)

	/*
	 * void
	 * xxx_cache_on()
	 */
	ALTENTRY(tsu_cache_on)
	sta	%g0, [%g0]ASI_ICFCLR	! flash clear icache
	sta	%g0, [%g0]ASI_DCFCLR	! flash clear dcache
	set 	RMMU_CTL_REG, %o5
	lda 	[%o5]ASI_MOD, %o4	! fetch MCR 
	set	CPU_TSU_DE|CPU_TSU_IE|CPU_TSU_ID, %o3
	andn    %o4, %o3, %o4      
	sethi	%hi(_tsu_cachebits), %o3
	ld	[%o3+%lo(_tsu_cachebits)], %o3
	or	%o4, %o3, %o4
	retl
	sta	%o4, [%o0]ASI_MOD	! update MCR

	/*
	 * void
	 * xxx_cache_off()
	 */
	ALTENTRY(tsu_cache_off)
        set     RMMU_CTL_REG, %o5	! fetch MCR
	lda	[%o5]ASI_MOD, %o4
	andn 	%o4, CPU_TSU_IE|CPU_TSU_DE, %o4 ! disable I-$, D-$
	set	CPU_TSU_ID, %o3
	or	%o4, %o3, %o4		! disable ITBR
	retl				! leaf return
	sta 	%o4, [%o5]ASI_MOD	! update MCR

	/*
	 * void
	 * xxx_cache_sync()
	 */

        ALTENTRY(tsu_cache_sync)
        sta     %g0, [%g0]ASI_ICFCLR    ! flash clear icache
        retl
        sta     %g0, [%g0]ASI_DCFCLR    ! flash clear dcache

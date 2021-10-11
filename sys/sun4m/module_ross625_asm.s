/*
 *	@(#)module_ross625_asm.s 1.1 94/10/31 SMI
 *	Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * assembly code support for modules based on the
 * Ross RT625 Cache Controller and
 * Memory Management Units.
 */

#include <sys/param.h>
#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/devaddr.h>
#include <machine/async.h>
#include <percpu_def.h>
#include "assym.s"

#define ASI_IFLUSH      0x31            /* flash clear of ibuf */
#define ASI_CTAGS 	0xe             /* cache tags */

#define	MCR_256K	0x1000		/* cache size bit */

#define GET(val,p,d) \
        sethi %hi(val), p; \
        ld [p + %lo(val)], d ;

	!
	! This approach is based upon the latest errata 
	! (received 5/8/89) for "Sun-4M SYSTEM ARCHITECTURE"
	! which specifies that for Ross modules, in order to
	! fix a race condition, the AFSR must be read first,
	! and then the AFAR must only be read if the AFV bit 
	! of the AFSR is set.  Furthermore, the AFAR is
	! guaranteed not to go away until *it* is read.
	! Note that generic SRMMU behaves differently, in
	! that the AFAR must be read first, since reading
	! the AFSR unlocks these registers.
	ALTENTRY(ross625_mmu_getasyncflt)
	set	RMMU_AFS_REG, %o1
	lda	[%o1]ASI_MOD, %o1
	st	%o1, [%o0]
	btst	AFSREG_AFV, %o1
	bz	1f
	set	RMMU_AFA_REG, %o1
	lda	[%o1]ASI_MOD, %o1
	st	%o1, [%o0+4]
1:
	set	-1, %o1
	retl
	st	%o1, [%o0+8]

/*
 * ross625_pte_rmw: update a pte.
 * does no flushing.
 */
	ALTENTRY(ross625_pte_rmw)		! (ppte, aval, oval)
	mov	%psr, %g2
	andn	%g2, PSR_ET, %g1
	mov	%g1, %psr		! disable traps
	nop; nop; nop			! psr delay
	ld	[%o0], %o5		! get old value
	andn	%o5, %o1, %o5		! turn some bits off
	or	%o5, %o2, %o5		! turn some bits on
	swap	[%o0], %o5		! update the PTE
	mov	%g2, %psr		! restore traps
	nop				! psr delay
	retl
	mov	%o5, %o0

#ifdef	VAC
/*
 * Virtual Address Cache routines.
 *
 *	Standard register allocation:
 *
 * %g1	scratchpad / jump vector / alt ctx reg ptr
 * %o0	virtual address			ctxflush puts context here
 * %o1	incoming size / loop counter
 * %o2	context number to borrow
 * %o3	saved context number
 * %o4	srmmu ctx reg ptr
 * %o5	saved psr
 * %o6	reserved (stack pointer)
 * %o7	reserved (return address)
 */

/*
 * ross625_vac_init_asm
 *
 * void ross625_vac_init_asm(clr, set);
 *
 * Initialize cache tags.
 */
	ENTRY(ross625_vac_init_asm)	! (clrbits, setbits)
	!
	! Some OBPs are leaving the cache on in copyback mode when
	! we get here.  We may already have some data in the cache
	! that hasn't been written to memory yet.  Therefore,
	! we need to flush the cache before turning it off.
	! 
	GET(_vac_size, %g1, %o2)
	GET(_vac_linesize, %g1, %o3)
1:	subcc	%o2, %o3, %o2
	bnz	1b
	sta	%g0, [%o2]ASI_FCC	! really and entire (no match) flush

	set	RMMU_CTL_REG, %o5
	lda	[%o5]ASI_MOD, %o4	! read control reg
	andn	%o4, %o0, %o4		! turn some bits off
	or	%o4, %o1, %o4		! turn some bits on
	sta	%o4, [%o5]ASI_MOD	! update control reg

!	GET(_vac, %o5, %o2)		! check if cache is
!	tst	%o2			!	turned on
!	bz      9f                      ! cache off, return
!	nop
	mov	%g0, %o3		! address of cache tags in 
					! CTAGS space
	GET(_vac_size, %g1, %o1)	! cache size
	GET(_vac_linesize, %g1, %o2)	! line size

#ifdef SIMUL
	/*
	 * Don't clear entire cache in the hardware simulator,
	 * it takes too long and the simulator has already cleared it
	 * for us.
	 */
	set	256, %o1		! initialize only 256 bytes worth
#endif SIMUL

1:	subcc	%o1, %o2, %o1		! done yet?
	bg	1b
	sta	%g0, [%o3+%o1]ASI_CTAGS	! clear tag

	mov	2, %g1			! disable ibuf, disable flush trap
	.word	0xbf806000		! wrasr31 %g1

!	mov	%psr, %o1
!	andn	%o1, PSR_ET, %o2
!	mov	%o2, %psr
!	nop ; nop ; nop
	sta	%g0, [%g0]ASI_IFLUSH
!	mov	%o1, %psr
!	nop ; nop ; nop
9:
	retl
	nop
/*
 * ross625_vac_flushall
 *
 * void ross625_vac_flushall(void);
 *
 * Use context ASI flush to flush any entry at a given index in the cache.
 */
	ENTRY(ross625_vac_flushall)
	ALTENTRY(ross625_vac_rgnflush)
	ALTENTRY(ross625_vac_segflush)
	GET(_vac, %o5, %o2)		! check if cache is
	tst	%o2			!	turned on
        bz      9f                      ! cache off, return
	GET(_vac_size, %g1, %o1)	! cache size
	GET(_vac_linesize, %g1, %o2)	! line size

! %o1 = vac_size, %o2 = line size

1:	subcc   %o1, %o2, %o1
	bnz	1b
	sta	%g0, [%g0+%o1]ASI_FCC

! Flash clear of Ibuf
	sta	%g0, [%g0]ASI_IFLUSH
	nop

9:      retl
	nop

! ross625_vac_usrflush: flush all user data from the cache

	ALTENTRY(ross625_vac_usrflush)

	GET(_vac, %o5, %o2)		! check if cache is
	tst	%o2			!	turned on
        bz      9f                      ! cache off, return
	GET(_vac_size, %g1, %o1)	! cache size
	GET(_vac_linesize, %g1, %o2)	! line size

! %o1 = vac_size, %o2 = line size

1:	subcc   %o1, %o2, %o1
	bnz	1b
	sta	%g0, [%g0+%o1]ASI_FCU

! Flash clear of Ibuf
	sta	%g0, [%g0]ASI_IFLUSH
	nop

9:      retl
	nop


/*
 * BORROW_CONTEXT: temporarily set the context number
 * to that given in %o2. See above for register assignments.
 * NOTE: all interrupts below level 15 are disabled until
 * the next RESTORE_CONTEXT. Do we want to disable traps
 * entirely, to prevent L15s from being serviced in a borrowed
 * context number? (trying this out now)
 */


#define	BORROW_CONTEXT			\
	mov	%psr, %g7;		\
	andn	%g7, PSR_ET, %g6;	\
	mov	%g6, %psr;		\
	nop ;	nop;			\
\
	set	RMMU_CTP_REG, %g6;	\
	lda	[%g6]ASI_MOD, %g6;	\
	sll	%o2, 2, %g5;		\
	sll	%g6, 4, %g6;		\
	add	%g6, %g5, %g6;		\
	lda	[%g6]ASI_MEM, %g6;	\
	and	%g6, 3, %g6;		\
	subcc	%g6, MMU_ET_PTP, %g0;	\
\
	set	RMMU_CTX_REG, %g6;	\
	lda	[%g6]ASI_MOD, %g5;	\
	bne	1f;			\
	nop;				\
	sta	%o2, [%g6]ASI_MOD;	\
1:
/*
 * RESTORE_CONTEXT: back out from whatever BORROW_CONTEXT did.
 * NOTE: asssumes two cycles of PSR DELAY follow the macro;
 * currently, all uses are followed by "retl ; nop".
 */
#define	RESTORE_CONTEXT			\
	sta	%g5, [%g6]ASI_MOD;	\
	mov	%g7, %psr;		\
	nop

	ALTENTRY(ross625_vac_ctxflush)
	retl
	nop

! void ross625_vac_pageflush(va)   flush data in this page from the cache.
#ifdef	MULTIPROCESSOR
!					context number in %o2
#endif

	ALTENTRY(ross625_vac_pageflush)

#ifdef	MULTIPROCESSOR
	BORROW_CONTEXT
#endif	MULTIPROCESSOR
	! If we're running on a 256K cache system, flash clear the IBUF
	lda	[%g0]ASI_MOD, %o1
	set	MCR_256K, %o2
	btst	%o2, %o1
	bnz,a	1f
	sta	%g0, [%g0]ASI_IFLUSH
1:	set	PAGESIZE, %o1

	! Probe vaddr for valid mapping
	andn	%o0, 0xfff, %o0		! XXX Mask to page boundary
	or	%o0, 0x400, %o2

	lda	[%o2]ASI_FLPR,%o2	! probe addr
	tst	%o2
	set	RMMU_FSR_REG, %g1
	bz	9f			! Not a valid mapping
	nop

	GET(_vac_linesize, %o5, %o2)	! line size
        add     %o2, %o2, %o3           ! LS * 2
        add     %o2, %o3, %o4           ! LS * 3
        add     %o2, %o4, %o5           ! LS * 4

 	add	%o0, %o2, %o2
 	add	%o0, %o3, %o3
 	add	%o0, %o4, %o4


3:      subcc   %o1, %o5, %o1
        sta     %g0, [%o0 + %o1]ASI_FCP
        sta     %g0, [%o2 + %o1]ASI_FCP
        sta     %g0, [%o3 + %o1]ASI_FCP
        bg      3b
        sta     %g0, [%o4 + %o1]ASI_FCP


9:
        lda     [%g1]ASI_MOD, %g0       ! clear sfsr
#ifdef	MULTIPROCESSOR
	RESTORE_CONTEXT
#endif	MULTIPROCESSOR

	retl
	nop

! void ross625_vac_pagectxflush(va, ctx)	
! flush data in this page and ctx from the cache.

	ALTENTRY(ross625_vac_pagectxflush)

	BORROW_CONTEXT
	! If we're running on a 256K cache system, flash clear the IBUF
	lda	[%g0]ASI_MOD, %o1
	set	MCR_256K, %o2
	btst	%o2, %o1
	bnz,a	1f
	sta	%g0, [%g0]ASI_IFLUSH
1:	set	PAGESIZE, %o1

	! Probe vaddr for valid mapping
	andn	%o0, 0xfff, %o0		! XXX Mask to page boundary
	or	%o0, 0x400, %o2
	lda	[%o2]ASI_FLPR,%o2	! probe addr
	tst	%o2
	set	RMMU_FSR_REG, %g1
	bz	9f			! Not a valid mapping
	nop

	GET(_vac_linesize, %o5, %o2)	! line size
        add     %o2, %o2, %o3           ! LS * 2
        add     %o2, %o3, %o4           ! LS * 3
        add     %o2, %o4, %o5           ! LS * 4

 	add	%o0, %o2, %o2
 	add	%o0, %o3, %o3
 	add	%o0, %o4, %o4


3:      subcc   %o1, %o5, %o1
        sta     %g0, [%o0 + %o1]ASI_FCP
        sta     %g0, [%o2 + %o1]ASI_FCP
        sta     %g0, [%o3 + %o1]ASI_FCP
        bg      3b
        sta     %g0, [%o4 + %o1]ASI_FCP


9:
        lda     [%g1]ASI_MOD, %g0       ! clear sfsr
	RESTORE_CONTEXT

	retl
	nop

! void ross625_vac_flush(va, sz)	flush data in this range from the cache
#ifdef	MULTIPROCESSOR
!					context number in %o2
#endif	MULTIPROCESSOR

	ALTENTRY(ross625_vac_flush)
	! If we're running on a 256K cache system, flash clear the IBUF
	lda	[%g0]ASI_MOD, %o2
	set	MCR_256K, %o3
	btst	%o3, %o2
	bnz,a	1f
	sta	%g0, [%g0]ASI_IFLUSH
1:
	GET(_vac_size, %o5, %o3)	! cache size
	GET(_vac_linesize, %o5, %o2)	! line size
        add     %o1, %o2, %o1           ! round up nbytes
	sub	%o2, 1, %o5
	and	%o0, %o5, %o4		! offset into line 
	add	%o1, %o4, %o1		! add offset to length for the align
	andn	%o1, %o5, %o1
        cmp     %o1, %o3                ! if (nbytes > vac_size)
        bgu,a   1f                      ! ...
        mov     %o3, %o1                !       nbytes = vac_size
1:	andn	%o0, %o5, %o0           ! align address
 
2:      subcc   %o1, %o2, %o1
        bg      2b
        sta     %g0, [%o0 + %o1]0x11 	! XXX To avoid probing addr XXX
 
9:      retl
        nop 

	ALTENTRY(ross625_cache_on)
	set	0x100, %o2			! cache enable
	set	RMMU_CTL_REG, %o0
	lda	[%o0]ASI_MOD, %o1
	or	%o1, %o2, %o1
	sta	%o1, [%o0]ASI_MOD
	sta	%g0, [%g0]ASI_IFLUSH	! flash clear ibuf
	set	_ross620_ibuf, %g1	! check ibuf flag
	ld	[%g1], %g1
	orcc	%g1, 0, %g0
	bz	9f
	! enable ibuf, disable IFLUSH traps
	 mov	3, %g1			! enable ibuf, disable flush trap
	.word	0xbf806000		! wrasr31 %g1
9:
	retl
	nop

	ALTENTRY(check625_cache)
	set	RMMU_CTL_REG, %o0
	lda	[%o0]ASI_MOD, %o0
	retl
	nop

#endif	VAC

	ALTENTRY(ross625_mmu_setctx)
	set	RMMU_CTX_REG, %o5		! set srmmu context number
	sta	%g0,[%g0]ASI_IFLUSH		! Flush ibuf
	retl
	sta	%o0, [%o5]ASI_MOD

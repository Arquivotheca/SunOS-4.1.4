/*      @(#)module_swift_asm.s 1.1 94/10/31 SMI     
 *
 *	Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * assembly code support for modules based on the
 * Fujitsu SWIFT chip set.
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

#if !defined(lint)

/*
 * Swift Virtual Address Cache routines.
 */

#define Sctx	%g1
! preserve for RESTORE_CONTEXT
#define Spsr	%g2
! preserve for RESTORE_CONTEXT
#define Fctx	%o2
! flush context

#define Faddr	%o0
! XXX - check register assignments ok
#define Addr	%o1
#define Tmp1	%o2
#define Tmp2	%o3
#define Tmp3 	%o4
#define Tmp4 	%o5
#define Tmp5 	%g3
#define Tmp6	%g4
#define Tmp7	%g5

/*
 * FLUSH_CONTEXT: temporarily set the context number
 * to that given in Fctx. See above for register assignments.
 * Runs with traps disabled - XXX - interrupt latency issue?
 */

#define	FLUSH_CONTEXT			\
	mov	%psr, Spsr;		\
	andn	Spsr, PSR_ET, Tmp7;	\
	mov	Tmp7, %psr;		\
	nop ;	nop;			\
\
	set	RMMU_CTP_REG, Tmp7;	\
	lda	[Tmp7]ASI_MOD, Tmp7;	\
	sll	Fctx, 2, Sctx;		\
	sll	Tmp7, 4, Tmp7;		\
	add	Tmp7, Sctx, Tmp7;	\
	lda	[Tmp7]ASI_MEM, Tmp7;	\
	and	Tmp7, 3, Tmp7;		\
	subcc	Tmp7, MMU_ET_PTP, %g0;	\
\
	set	RMMU_CTX_REG, Tmp7;	\
	bne	1f;			\
	lda	[Tmp7]ASI_MOD, Sctx;	\
	sta	Fctx, [Tmp7]ASI_MOD;	\
1:


/*
 * RESTORE_CONTEXT: back out from whatever FLUSH_CONTEXT did.
 * NOTE: asssumes two cycles of PSR DELAY follow the macro;
 * currently, all uses are followed by "retl ; nop".
 */
#define	RESTORE_CONTEXT			\
	set	RMMU_CTX_REG, Tmp7;	\
	sta	Sctx, [Tmp7]ASI_MOD;	\
	mov	Spsr, %psr;		\
	nop;				\
	nop

#endif /* !defined(lint) */


/* I$ is 16KB with 32B linesize */
#define ICACHE_LINES    512
#define ICACHE_LINESHFT 5
#define ICACHE_LINESZ   (1<<ICACHE_LINESHFT)
#define ICACHE_LINEMASK (ICACHE_LINESZ-1)
#define ICACHE_BYTES    (ICACHE_LINES<<ICACHE_LINESHFT)

/* D$ is 8KB with 16B linesize */
#define DCACHE_LINES    512
#define DCACHE_LINESHFT 4
#define DCACHE_LINESZ   (1<<DCACHE_LINESHFT)
#define DCACHE_LINEMASK (DCACHE_LINESZ-1)
#define DCACHE_BYTES    (DCACHE_LINES<<DCACHE_LINESHFT)

/* Derived from D$/I$ values */
#define CACHE_LINES	1024
#define CACHE_LINESHIFT 4
#define CACHE_LINESZ   	(1<<CACHE_LINESHIFT)
#define CACHE_LINEMASK 	(CACHE_LINESZ-1)
#define CACHE_BYTES    	(CACHE_LINES<<CACHE_LINESHIFT)

#if defined(lint)

/* ARGSUSED */
void
swift_cache_init(void)
{}

#else	/* lint */

        ALTENTRY(swift_cache_init)

        set     CACHE_BYTES, Addr	! cache bytes to init
1:
        deccc   CACHE_LINESZ, Addr 
        sta     %g0, [Addr]ASI_ICT	! clear I$ tag
        bne     1b
        sta     %g0, [Addr]ASI_DCT      ! clear D$ tag
        retl
        nop

#endif	/* lint */

#if defined(lint)

void
swift_cache_on(void)
{}

#else	/* lint */

.global _use_dc 
.global _use_ic 
 
        ALTENTRY(swift_cache_on)

	sethi	%hi(_use_ic), %o3
	ld 	[%o3 + %lo(_use_ic)], %o3	! load use_ic
	tst	%o3
	be	1f
	clr	%o2

	or	%o2, 0x200, %o2			! enable I$

1:	sethi	%hi(_use_dc), %o3
	ld 	[%o3 + %lo(_use_dc)], %o3	! load use_dc
	tst	%o3
	be	2f
	nop

	or 	%o2, 0x100, %o2			! enable D$

2: 	set	RMMU_CTL_REG, %o0
	lda	[%o0]ASI_MOD, %o1
	andn	%o1, 0x300, %o1
	or	%o1, %o2, %o1
	retl
	sta	%o1, [%o0]ASI_MOD

#endif	/* lint */

#if defined(lint)

/*
 * swift_vac_flushall: flush entire cache 
 */

void
swift_cache_flushall(void)
{}

#else	/* lint */

	! XXX - only works for write-thru cache, for wb code see module_ross_asm.s 
	ALTENTRY(swift_vac_flushall)
        set     CACHE_BYTES, Addr		! cache bytes to invalidate
1:
        deccc   CACHE_LINESZ, Addr 
        sta     %g0, [Addr]ASI_ICT		! clear I$ tag
        bne     1b
        sta     %g0, [Addr]ASI_DCT      	! clear D$ tag

        retl
        nop

#endif	/* lint */

/*
 * swift_vac_flush: flush data for range 'va' to 'va + sz - 1' 
 */

#if defined(lint)

/* ARGSUSED */
void
swift_vac_flush(caddr_t va, int sz)
{}

#else	/* lint */

	ALTENTRY(swift_vac_flush)

	! XXX - optimize this - ddi_dma_sync() uses this?

     	and     Faddr, CACHE_LINEMASK, Tmp7    ! figure align error on start
        sub     Faddr, Tmp7, Faddr		! push start back that much
        add     Addr, Tmp7, Addr		! add it to size
 
1:      deccc   CACHE_LINESZ, Addr 
        sta     %g0, [Faddr]ASI_FCP
        bcc     1b
        inc     CACHE_LINESZ, Faddr 
 
        retl
        nop

#endif	/* lint */

/*
 * swift_vac_usrflush: flush all user data from the cache
 */

#if defined(lint)

void
swift_vac_usrflush(void)
{}

#else	/* lint */

	ALTENTRY(swift_vac_usrflush)
   	set     CACHE_BYTES/8, Addr 
	clr	Faddr
        add    	Faddr, Addr, Tmp1 
        add    	Tmp1, Addr, Tmp2 
        add    	Tmp2, Addr, Tmp3 
 
        add    	Tmp3, Addr, Tmp4 
        add     Tmp4, DCACHE_LINESZ, Tmp4 	! switch to odd D$ lines
        add    	Tmp4, Addr, Tmp5 
        add    	Tmp5, Addr, Tmp6 
        add    	Tmp6, Addr, Tmp7 

1:      deccc   ICACHE_LINESZ, Addr 
        sta     %g0, [Faddr]ASI_FCU
        sta     %g0, [Faddr + Tmp1]ASI_FCU
        sta     %g0, [Faddr + Tmp2]ASI_FCU
        sta     %g0, [Faddr + Tmp3]ASI_FCU
        sta     %g0, [Faddr + Tmp4]ASI_FCU
        sta     %g0, [Faddr + Tmp5]ASI_FCU
        sta     %g0, [Faddr + Tmp6]ASI_FCU
        sta     %g0, [Faddr + Tmp7]ASI_FCU
        bne     1b
	inc	ICACHE_LINESZ, Faddr
 
        retl
        nop

#endif	/* lint */

#if defined(lint)

/*
 * swift_vac_ctxflush: flush all data for ctx 'Fctx' from the cache
 */

/* ARGSUSED */
void
swift_vac_ctxflush(void * a, void * b, int Fctx)
{}

#else	/* lint */

	ALTENTRY(swift_vac_ctxflush)

	FLUSH_CONTEXT

   	set     CACHE_BYTES/8, Addr 
	clr	Faddr
        add    	Faddr, Addr, Tmp1 
        add    	Tmp1, Addr, Tmp2 
        add    	Tmp2, Addr, Tmp3 
 
        add    	Tmp3, Addr, Tmp4 
        add     Tmp4, DCACHE_LINESZ, Tmp4 	! switch to odd D$ lines
        add    	Tmp4, Addr, Tmp5 
        add    	Tmp5, Addr, Tmp6 
        add    	Tmp6, Addr, Tmp7 

1:      deccc   ICACHE_LINESZ, Addr 
        sta     %g0, [Faddr]ASI_FCC
        sta     %g0, [Faddr + Tmp1]ASI_FCC
        sta     %g0, [Faddr + Tmp2]ASI_FCC
        sta     %g0, [Faddr + Tmp3]ASI_FCC
        sta     %g0, [Faddr + Tmp4]ASI_FCC
        sta     %g0, [Faddr + Tmp5]ASI_FCC
        sta     %g0, [Faddr + Tmp6]ASI_FCC
        sta     %g0, [Faddr + Tmp7]ASI_FCC
        bne     1b
	inc	ICACHE_LINESZ, Faddr
 
	mov     KCONTEXT, Sctx 			! restore to kernel context

        RESTORE_CONTEXT

        retl
        nop

#endif	/* lint */


/*
 * swift_vac_rgnflush: flush data for region 'va' and ctx 'Fctx' from the cache
 */

#if defined(lint)

/* ARGSUSED */
void
swift_vac_rgnflush(caddr_t va, void * b, int Fctx)
{}

#else	/* lint */

	ALTENTRY(swift_vac_rgnflush)

	! XXX - should va be rounded to rgn boundary?

	FLUSH_CONTEXT

   	set     CACHE_BYTES/8, Addr 
	mov	Addr, Tmp1
        add    	Tmp1, Addr, Tmp2 
        add    	Tmp2, Addr, Tmp3 
 
        add    	Tmp3, Addr, Tmp4 
        add     Tmp4, DCACHE_LINESZ, Tmp4 	! switch to odd D$ lines
        add    	Tmp4, Addr, Tmp5 
        add    	Tmp5, Addr, Tmp6 
        add    	Tmp6, Addr, Tmp7 

1:      deccc   ICACHE_LINESZ, Addr 
        sta     %g0, [Faddr]ASI_FCR
        sta     %g0, [Faddr + Tmp1]ASI_FCR
        sta     %g0, [Faddr + Tmp2]ASI_FCR
        sta     %g0, [Faddr + Tmp3]ASI_FCR
        sta     %g0, [Faddr + Tmp4]ASI_FCR
        sta     %g0, [Faddr + Tmp5]ASI_FCR
        sta     %g0, [Faddr + Tmp6]ASI_FCR
        sta     %g0, [Faddr + Tmp7]ASI_FCR
        bne     1b
	inc	ICACHE_LINESZ, Faddr
 
        RESTORE_CONTEXT

        retl
        nop

#endif	/* lint */

/*
 * swift_vac_segflush: flush data for segment 'va' and ctx 'Fctx' from the cache
 */

#if defined(lint)

/* ARGSUSED */
void
swift_vac_segflush(caddr_t va, void * b, int Fctx)
{}

#else	/* lint */

	ALTENTRY(swift_vac_segflush)

	! XXX - should va be rounded to seg boundary?

	FLUSH_CONTEXT

   	set     CACHE_BYTES/8, Addr 
	mov	Addr, Tmp1
        add    	Tmp1, Addr, Tmp2 
        add    	Tmp2, Addr, Tmp3 
 
        add    	Tmp3, Addr, Tmp4 
        add     Tmp4, DCACHE_LINESZ, Tmp4 	! switch to odd D$ lines
        add    	Tmp4, Addr, Tmp5 
        add    	Tmp5, Addr, Tmp6 
        add    	Tmp6, Addr, Tmp7 

1:      deccc   ICACHE_LINESZ, Addr 
        sta     %g0, [Faddr]ASI_FCS
        sta     %g0, [Faddr + Tmp1]ASI_FCS
        sta     %g0, [Faddr + Tmp2]ASI_FCS
        sta     %g0, [Faddr + Tmp3]ASI_FCS
        sta     %g0, [Faddr + Tmp4]ASI_FCS
        sta     %g0, [Faddr + Tmp5]ASI_FCS
        sta     %g0, [Faddr + Tmp6]ASI_FCS
        sta     %g0, [Faddr + Tmp7]ASI_FCS
        bne     1b
	inc	ICACHE_LINESZ, Faddr
 
        RESTORE_CONTEXT

        retl
        nop

#endif	/* lint */

/*
 * swift_vac_pageflush: flush data for page 'va' from the cache
 */

#if defined(lint)

/* ARGSUSED */
void
swift_vac_pageflush(caddr_t va)
{}

#else	/* lint */

	ALTENTRY(swift_vac_pageflush)

 	set     PAGESIZE/8, Addr 
	mov	Addr, Tmp1
        add     Tmp1, Addr, Tmp2
        add     Tmp2, Addr, Tmp3
        add     Tmp3, Addr, Tmp4
        add     Tmp4, Addr, Tmp5
        add     Tmp5, Addr, Tmp6
        add     Tmp6, Addr, Tmp7
 
1:      deccc   CACHE_LINESZ, Addr 
        sta     %g0, [Faddr	  ]ASI_FCP
        sta     %g0, [Faddr + Tmp1]ASI_FCP
        sta     %g0, [Faddr + Tmp2]ASI_FCP
        sta     %g0, [Faddr + Tmp3]ASI_FCP
        sta     %g0, [Faddr + Tmp4]ASI_FCP
        sta     %g0, [Faddr + Tmp5]ASI_FCP
        sta     %g0, [Faddr + Tmp6]ASI_FCP
        sta     %g0, [Faddr + Tmp7]ASI_FCP
        bne     1b
	inc	CACHE_LINESZ, Faddr
 
        retl
        nop

#endif	/* lint */

/*
 * swift_vac_pagectxflush: flush data for page 'va' and ctx 'Fctx' from the cache
 */

#if defined(lint)

/* ARGSUSED */
void
swift_vac_pagectxflush(caddr_t va, void * b, int Fctx)
{}

#else	/* lint */

	ALTENTRY(swift_vac_pagectxflush)

	FLUSH_CONTEXT

 	set     PAGESIZE/8, Addr 
	mov	Addr, Tmp1
        add     Tmp1, Addr, Tmp2
        add     Tmp2, Addr, Tmp3
        add     Tmp3, Addr, Tmp4
        add     Tmp4, Addr, Tmp5
        add     Tmp5, Addr, Tmp6
        add     Tmp6, Addr, Tmp7
 
1:      deccc   CACHE_LINESZ, Addr 
        sta     %g0, [Faddr]ASI_FCP
        sta     %g0, [Faddr + Tmp1]ASI_FCP
        sta     %g0, [Faddr + Tmp2]ASI_FCP
        sta     %g0, [Faddr + Tmp3]ASI_FCP
        sta     %g0, [Faddr + Tmp4]ASI_FCP
        sta     %g0, [Faddr + Tmp5]ASI_FCP
        sta     %g0, [Faddr + Tmp6]ASI_FCP
        sta     %g0, [Faddr + Tmp7]ASI_FCP
        bne     1b
	inc	CACHE_LINESZ, Faddr

	RESTORE_CONTEXT
 
        retl
        nop

#endif	/* lint */

#if defined (lint)

/* ARGSUSED */
void
swift_getsyncflt(void)
{}

#else	/* lint */

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
 
        ALTENTRY(swift_mmu_getsyncflt)              
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
#endif	/* lint */
 
#define	PA_AURORA_SYSCTL  0x71F00000 /* asi=20: system control/status */

	ALTENTRY(swift_mmu_chk_wdreset)
	set	PA_AURORA_SYSCTL, %o1		! system control/status
	lda	[%o1]ASI_MEM, %o0
	retl
	and	%o0, SYSCTLREG_WD, %o0		! mask WD reset bit


	ALTENTRY(swift_mmu_getasyncflt)		! not supported
	sub	%g0, 1, %o1			! (mtos error)
	retl
	st	%o1, [%o0]

#ifdef lint

/* ARGSUSED */
u_int
swift_pte_rmw()
{}

#else

/*
 * swift_pte_rmw: update a pte.
 * does no flushing.
 */
	ALTENTRY(swift_pte_rmw)         ! (ppte, aval, oval)
	mov     %psr, %g2
	andn    %g2, PSR_ET, %g1
	mov     %g1, %psr               ! disable traps
	nop; nop; nop                   ! psr delay
	ld      [%o0], %o5              ! get old value
	andn    %o5, %o1, %o5           ! turn some bits off
	or      %o5, %o2, %o5           ! turn some bits on
	swap    [%o0], %o5              ! update the PTE
	mov     %g2, %psr               ! restore traps
	nop                             ! psr delay
	nop; nop                        ! psr paranoia
	retl
	mov     %o5, %o0

#endif


#if defined(lint)

/*ARGSUSED*/
int
swift_mmu_ltic_ramcam(/* u_int ram, u_int utag, u_int ltag */)
{ return (0); }

#else /* lint */
	.global	_swift_tlb_lckcnt
	.global _swift_tlb_maxlckcnt

	/*
	 * Lock a mapping into the TLB (XXX - will flush remove it?)
	 * Only allow 'swift_tlb_maxlckcnt' entries max to be locked
	 */
#undef Addr
#undef Data

#define Ram	%o0
#define Utag	%o1
#define Ltag	%o2
#define Addr	%o3
#define Data	%o4
#define Tmp	%o5

	ENTRY(swift_mmu_ltic_ramcam)
	set	_swift_tlb_maxlckcnt, Addr
	ld	[Addr], Data
	set 	_swift_tlb_lckcnt, Addr
	ld	[Addr], Tmp
	cmp	Tmp, Data		! any more locked slots?
	bge,a	1f
	mov	%g0, Ram

	! locking entry TLBSIZE-1-_swift_tlb_lckcnt
	! set TRCR.WP to above -1
	set	0x40-1-1, Data
	sub	Data, Tmp, Data 	! Data == new WP index 
	sethi	%hi(0x1000), Addr
	lda	[Addr]0x4, Tmp

	set	0x1f80, Addr
	andn	Tmp, Addr, Tmp
	sll	Data, 7, Addr 
	or	Tmp, Addr, Tmp

	sethi	%hi(0x1000), Addr
	sta	Tmp, [Addr]0x4		! set new WP

	inc	Data			! Data == new lock index
	sll	Data, 2, Data		! Data == TLB diag addr offset

	sta	Ram, [Data]0x6		! store Ram	
	
	inc	0x100, Data		! Data == Ltag addr
	sta	Ltag, [Data]0x6		! store Ltag

	inc	0x200, Data		! Data == Utag addr
	sta	Utag, [Data]0x6		! store Utag

        set     _swift_tlb_lckcnt, Addr 
        ld      [Addr], Tmp 
	inc	Tmp
	st	Tmp, [Addr]
	mov	Tmp, Ram

1:
	retl
	nop


#endif /* lint */



#if defined(lint)

/* ARGSUSED */
u_int
swift_getversion(/* void */)
{ return (0); }

#else	/* lint */

/*
 * swift_getversion will get the version of the Swift chip.  The
 * version is located in the VA mask register (PA = 0x1000.3018).
 */
	ENTRY(swift_getversion)
	set	0x10003018, %o1		! Address of VA mask register 
	retl
	lda	[%o1]0x20, %o0		! return VA mask register
#endif	/* lint */

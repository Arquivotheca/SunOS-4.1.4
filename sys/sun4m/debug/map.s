	.ident  "@(#)map.s 1.1 94/10/31 SMI"

!
!	Copyright (c) 1986 by Sun Microsystems, Inc.
!

/*
 * Additional memory mapping routines for use by standalone debugger,
 * setpgmap(), getpgmap() are taken from the boot code.
 */

#include "assym.s"
#include <sys/param.h>
#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/enable.h>
#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/eeprom.h>
#include <machine/asm_linkage.h>
#include <machine/reg.h>
#include <debug/debug.h>

	.seg	"data"
ross625_line_size:
	.word	64

	.seg	"text"
	.align	4


#define CACHE_LINESHFT  5
#define CACHE_LINESZ	(1<<CACHE_LINESHFT)

/*
 * Flush a page from the cache.
 *
 * vac_pageflush(vaddr)
 * caddr_t vaddr;
 */
	ENTRY(vac_pageflush)
	sethi	%hi(_v_vac_pageflush), %g1
	ld	[%g1+%lo(_v_vac_pageflush)], %g1
	jmp	%g1
	nop

	ENTRY(ross_vac_pageflush)
	set	PAGESIZE/8, %g1
	add	%o0, %g1, %o1
	add	%o1, %g1, %o2
	add	%o2, %g1, %o3
	add	%o3, %g1, %o4
	add	%o4, %g1, %o5
	add	%o5, %g1, %g2
	add	%g2, %g1, %g3

1:	deccc	CACHE_LINESZ, %g1
	sta	%g0, [%o0 + %g1]ASI_FCP
	sta	%g0, [%o1 + %g1]ASI_FCP
	sta	%g0, [%o2 + %g1]ASI_FCP
	sta	%g0, [%o3 + %g1]ASI_FCP
	sta	%g0, [%o4 + %g1]ASI_FCP
	sta	%g0, [%o5 + %g1]ASI_FCP
	sta	%g0, [%g2 + %g1]ASI_FCP
	bne	1b
	sta	%g0, [%g3 + %g1]ASI_FCP

	retl
	nop

/* ************** start ROSS  620/625 support ************** */

#define	RT625_CACHE_LINES	4096
#define RT625_SMALL_LINESIZE	32
#define RT625_BIG_LINESIZE	64

#define RT625_CTL_CE	0x00000100	/* cache enable */
#define RT625_CTL_CS	0x00001000	/* cache size */

#define RT625_ASI_CTAGS	0x0e		/* cache tags */
#define RT620_ASI_IC	0x31		/* icache control */

#define GET(val, r) \
	sethi	%hi(val), r; \
	ld	[r+%lo(val)], r

#define PUT(val, r, t) \
	sethi	%hi(val), t; \
	st	r, [t+%lo(val)]

	/*
	 * ross625_cache_init - initialize ross 625 cache
	 *	1) figure out the line size from the MMU ctl reg
	 *	2) clear the cache
	 *	3) turn off the cache
	 *	4) clear the cache tags
	 *	5) clear the icache
	 */
	ENTRY(ross625_cache_init)
	set	RMMU_CTL_REG, %o0
	lda	[%o0]ASI_MOD, %o1	! get the MMU ctl reg

	set	RT625_CTL_CS, %o2
	andcc	%o2, %o1, %o2
	bz	1f
	nop
	mov	RT625_BIG_LINESIZE,%o3	! CS on = 64 byte line size
	b	2f
	nop

1:	mov	RT625_SMALL_LINESIZE,%o3 ! CS off  = 32 byte line size
2:	PUT(ross625_line_size, %o3, %o2)

	set	RT625_CACHE_LINES,%o2		! clear cache
	GET(ross625_line_size,%o3)
	mov	0, %o4
1:	sta	%g0, [%o4]ASI_FCC
	subcc	%o2, 1, %o2
	bnz	1b
	add	%o4, %o3, %o4

	set	RMMU_CTL_REG, %o0	! turn cache off
	lda	[%o0]ASI_MOD, %o1
	andn	%o1, RT625_CTL_CE, %o1
	sta	%o1, [%o0]ASI_MOD

	set	RT625_CACHE_LINES, %o2	! clear cache tags
	GET(ross625_line_size,%o3)
	mov	0, %o4
1:	sta	%g0, [%o4]RT625_ASI_CTAGS
	subcc	%o2, 1, %o2
	bnz	1b
	add	%o4, %o3, %o4

	retl
	sta	%g0, [%g0]RT620_ASI_IC	! clear the icache

	/*
	 * ross625_cache_on - turn the cache on
	 */
	ENTRY(ross625_cache_on)
	set	RT625_CTL_CE, %o2	! cache enable
	set	RMMU_CTL_REG, %o0	! mmu control reg
	lda	[%o0]ASI_MOD, %o1
	or	%o1, %o2, %o1
	retl
	sta	%o1, [%o0]ASI_MOD

	/*
	 * ross625_vac_pageflush - flush the cache for the page in %o0
	 */
	ENTRY(ross625_vac_pageflush)
	set	PAGESIZE, %o1		! the page size
	GET(ross625_line_size,%o2)	! the line size
1:	subcc	%o1, %o2, %o1		! for all the lines in a page
	sta	%g0, [%o0]ASI_FCP	! flush cache line (PAGE)
	bne	1b
	add	%o0, %o2, %o0		! on to the next line
	retl
	sta	%g0, [%g0]RT620_ASI_IC	! clear the icache

	/*
	 * ross625_vac_flushall - flush the entire cache and TLB
	 */
	ENTRY(ross625_vac_flushall)
	set	RT625_CACHE_LINES, %o0	! for all the lines
	GET(ross625_line_size, %o1)	! a line at a time
	mov	0, %o2
1:	sta	%g0, [%o2]ASI_FCC	! clear the cache line
	subcc	%o0, 1, %o0		! for all the lines
	bnz	1b
	add	%o2, %o1, %o2		! on to next line

	mov	FT_ALL<<8, %g1
	retl
	sta	%g0, [%g1]ASI_FLPR	! flush the tags

/* **************   end ROSS  620/625 support ************** */

#define SWIFT_CACHE_LINESHIFT 4
#define SWIFT_CACHE_LINESZ    (1<<SWIFT_CACHE_LINESHIFT)

#define Faddr   %o0
#define Addr    %o1
#define Tmp1    %o2
#define Tmp2    %o3
#define Tmp3    %o4
#define Tmp4    %o5
#define Tmp5    %g3
#define Tmp6    %g4
#define Tmp7    %g5

        ENTRY(swift_vac_pageflush)
        set     PAGESIZE/8, Addr
        mov     Addr, Tmp1
        add     Tmp1, Addr, Tmp2
        add     Tmp2, Addr, Tmp3
        add     Tmp3, Addr, Tmp4
        add     Tmp4, Addr, Tmp5
        add     Tmp5, Addr, Tmp6
        add     Tmp6, Addr, Tmp7

1:      deccc   SWIFT_CACHE_LINESZ, Addr
        sta     %g0, [Faddr       ]ASI_FCP
        sta     %g0, [Faddr + Tmp1]ASI_FCP
        sta     %g0, [Faddr + Tmp2]ASI_FCP
        sta     %g0, [Faddr + Tmp3]ASI_FCP
        sta     %g0, [Faddr + Tmp4]ASI_FCP
        sta     %g0, [Faddr + Tmp5]ASI_FCP
        sta     %g0, [Faddr + Tmp6]ASI_FCP
        sta     %g0, [Faddr + Tmp7]ASI_FCP
        bne     1b
        inc     SWIFT_CACHE_LINESZ, Faddr
 
        retl
        nop

/*
 * int  ldphys(int paddr)
 * read word of memory at physical address
 * also called "ldphys" by some codes
 */
        ENTRY(ldphys)
	sethi	%hi(_cache), %o1
	ld	[%o1 + %lo(_cache)], %o1
	cmp	%o1, CACHE_PAC_E	! Viking/E$
	bnz,a	0f			! No,
	lda	[%o0]ASI_MEM, %o0	!    dont need AC bit set

	mov	%psr, %o1		! Save %psr in %o1
	andn    %o1, PSR_ET, %g1        ! disable traps
        mov     %g1, %psr
        nop; nop; nop                   ! PSR delay
        lda     [%g0]ASI_MOD, %o2       ! get MMU CSR, %o2 keeps the saved CSR
        set     CPU_VIK_AC, %o3         ! AC bit
        or      %o2, %o3, %o3           ! or in AC bit
        sta     %o3, [%g0]ASI_MOD       ! store new CSR
 
        lda     [%o0]ASI_MEM, %o0

	sta     %o2, [%g0]ASI_MOD       ! restore CSR;
        mov     %o1, %psr               ! restore psr; enable traps again
        nop				! PSR delay
0:
        retl
	nop
/*
 * void stphys(int paddr, int data)
 * write word of memory at physical address
 */
        ENTRY(stphys)
	sethi	%hi(_cache), %o4
	ld	[%o4 + %lo(_cache)], %o4
	cmp	%o4, CACHE_PAC_E	! Viking/E$?
	bnz,a	0f			! No, dont need AC bit set
        sta     %o1, [%o0]ASI_MEM

	mov	%psr, %o4		! Save %psr in %o4
	andn    %o4, PSR_ET, %g1        ! disable traps
        mov     %g1, %psr
        nop; nop; nop                   ! PSR delay
        lda     [%g0]ASI_MOD, %o2       ! get MMU CSR, %o2 keeps the saved CSR
        set     CPU_VIK_AC, %o3         ! AC bit
        or      %o2, %o3, %o3           ! or in AC bit
        sta     %o3, [%g0]ASI_MOD       ! store new CSR
 
        sta     %o1, [%o0]ASI_MEM

	sta     %o2, [%g0]ASI_MOD       ! restore CSR;
        mov     %o4, %psr               ! restore psr; enable traps again
        nop				! PSR delay
0:
        retl
	nop

/*
 * void mmu_flushall()
 * flush all entries from TLB
 * XXX - for the moment this will only work with GENERIC SRMMU
 * modules, also MP is not supported.
 */

	ENTRY(mmu_flushall)
	sethi	%hi(_v_mmu_flushall), %g1
	ld	[%g1+%lo(_v_mmu_flushall)], %g1
	jmp	%g1
	nop

	ENTRY(srmmu_flushall)
	or	%g0, FT_ALL << 8, %o0
	sta	%g0, [%o0]ASI_FLPR
	retl
	nop		! let mmu settle ??

/*
 * int mmu_getctp(void)
 * return current context table pointer
 */
	ENTRY(mmu_getctp)			! int	mmu_getctp(void);
	set	RMMU_CTP_REG, %o1		! get srmmu context table ptr
	retl
	lda	[%o1]ASI_MOD, %o0

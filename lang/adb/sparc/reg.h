/*	@(#)reg.h 1.10 86/07/31 SMI	*/
/*      @(#)reg.h 1.1 94/10/31 SMI for adb      */
/*      Modified from @(#)reg.h 1.7 86/04/09 SMI        */
 
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* EDITED VERSION FOR ADB CROSS-COMPILING AND CROSS-DEBUGGING */
/*	For now, we no longer need to have a special version
 *	of reg.h, because its structures no longer have alignment
 *	problems for the cross-debugger (xadb).
 */


/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef _REG_
#define _REG_

/*
 * Location of the users' stored
 * registers relative to R0.
 * Usage is u.u_ar0[XX].
 */
#define PSR	(0)
#define PC	(1)
#define NPC	(2)
#define Y	(3)
#define G1	(4)
#define G2	(5)
#define G3	(6)
#define G4	(7)
#define G5	(8)
#define G6	(9)
#define G7	(10)
#define O0	(11)
#define O1	(12)
#define O2	(13)
#define O3	(14)
#define O4	(15)
#define O5	(16)
#define O6	(17)
#define O7	(18)

/* the following defines are for portability */
#define PS	PSR
#define SP	O6
#define R0	O0
#define R1	O1		

/*
 * And now for something completely the same...
 */
#ifndef LOCORE
struct regs {	
	int	r_psr;		/* processor status register */
	int	r_pc;		/* program counter */
	int	r_npc;		/* next program counter */
	int	r_y;		/* the y register */
	int	r_g1;		/* user global regs */
	int	r_g2;
	int	r_g3;
	int	r_g4;
	int	r_g5;
	int	r_g6;
	int	r_g7;
	int	r_o0;
	int	r_o1;
	int	r_o2;
	int	r_o3;
	int	r_o4;
	int	r_o5;
	int	r_o6;
	int	r_o7;
};

#define r_ps	r_psr		/* for portablility */
#define r_r0	r_o0
#define r_sp	r_o6

#endif !LOCORE

/*
 * Floating point definitions.
 */

#define FPU			/* we have an external float unit */

#ifndef LOCORE

#define FQ_DEPTH	2		/* maximum instuctions in FQ */

struct	fq {
	unsigned long fq_instr;		/* FPU instruction */
	long *fq_addr;			/* addr of FPU instruction */
};

#define FPU_REGS_TYPE unsigned
#define FPU_FSR_TYPE unsigned

struct	fp_status {
	FPU_REGS_TYPE fpu_regs[32];		/* FPU floating point regs */
	FPU_FSR_TYPE fpu_fsr;		/* FPU status register */
	struct fq fpu_q[FQ_DEPTH];	/* FPU instruction address queue */
	unsigned char fpu_qcnt;		/* count of valid entries in fps_q */
	unsigned char fpu_en;		/* flag signifying fpu is in use */
	/* XXX - dgw - temporary fix until fsr can hold the following bits */
	unsigned char fpu_aexc;		/* accrued exception bits */
	unsigned char fpu_tem;		/* trap enable mask bits */
};
#endif !LOCORE

/*
 * Definition of bits in the Sun-4 FSR (Floating-point Status Register)
 *   ___________________________________________________________________
 *  | RD | RP | NXM |  res | TRAP | AEXC | PR | FTT | QNE | FCC | CEXC |
 *  |----|----|-----|------|------|------|----|-----|-----|-----|------|
 *    31   29   27   26  22 21  17 16  12  11  10  8   7   6   5 4    0
 */
#define FSR_CEXC	0x00000014	/* Current Exception */
#define FSR_FCC		0x00000060	/* Floating-point Condition Codes */
#define FSR_QNE		0x00000080	/* Queue not empty */
#define FSR_FTT		0x00000700	/* Floating-point Trap Type */
#define FSR_PR		0x00000800	/* Partial Remainder */
#define FSR_AEXC	0x0001f000	/* ieee accrued exceptions */
#define FSR_TEM		0x003e0000	/* ieee Trap Enable Mask */
#define FSR_RESV	0x07c00000	/* reserved */
#define FSR_NXM		0x08000000	/* iNexact eXception Mask */
#define FSR_RP		0x30000000	/* Rounding Precision */
#define FSR_RD		0xc0000000	/* Rounding Direction */

/*
 * Defintion of FIT (Floating-point Trap Type) field within the FSR
 */
#define FSR_FTT_IEEE	0x00000100	/* IEEE exception */
#define FSR_FTT_UNFINI	0x00000200	/* unfinished fpop */
#define FSR_FTT_UNIMPL	0x00000300	/* unimplemented fpop */
#define FSR_FTT_SEQ	0x00000400	/* sequence error */

/*
 * Definition of CEXC (Current EXCeption) bit field of fsr
 */
#define	FSR_CEXC_NX	0x00000001	/* inexact */
#define FSR_CEXC_DZ	0x00000002	/* divide-by-zero */
#define FSR_CEXC_UF	0x00000004	/* underflow */.
#define FSR_CEXC_OF	0x00000008	/* overflow */
#define FSR_CEXC_NV	0x00000010	/* invalid */

/*
 * Definition of AEXC (Accrued EXCeption) bit field of fsr
 */
#define	FSR_AEXC_NX	0x00000001	/* inexact */
#define FSR_AEXC_DZ	0x00000002	/* divide-by-zero */
#define FSR_AEXC_UF	0x00000004	/* underflow */.
#define FSR_AEXC_OF	0x00000008	/* overflow */
#define FSR_AEXC_NV	0x00000010	/* invalid */

/*
 * Definition of TEM (Trap Enable Mask) bit field of fsr
 */
#define	FSR_TEM_NX	0x00000001	/* inexact */
#define FSR_TEM_DZ	0x00000002	/* divide-by-zero */
#define FSR_TEM_UF	0x00000004	/* underflow */.
#define FSR_TEM_OF	0x00000008	/* overflow */
#define FSR_TEM_NV	0x00000010	/* invalid */

/*
 * Definition of RP (Rounding Precision) field of fsr
 */
#define RP_DBLEXT	0		/* double-extended */
#define RP_SINGLE	1		/* single */
#define RP_DOUBLE	2		/* double */
#define RP_RESERVED	3		/* unused and reserved */

/*
 * Defintion of RD (Rounding Direction) field of fsr
 */
#define RD_NEAR		0		/* nearest or even if tie */
#define RD_ZER0		1		/* to zero */
#define RD_POSINF	2		/* positive infinity */
#define RD_NEGINF	3		/* negative infinity */

/*
 * Register window definitions
 */
#define NWINDOW		8			/* # of implemented windows */
#define WINDOWSIZE	(16 * 4)		/* size fo each window */
#define WINDOWMASK	((1 << NWINDOW) - 1)

#ifndef LOCORE
struct window {
	int	w_local[8];		/* locals */
	int	w_in[8];		/* ins */
};

#define w_fp	w_in[6]			/* frame pointer */
#define w_rtn	w_in[7]			/* return address */

#endif !LOCORE


#ifdef LOCORE

#define SAVE_GLOBALS(RP) \
	st	%g1, [RP + G1*4]; \
	st	%g2, [RP + G2*4]; \
	st	%g3, [RP + G3*4]; \
	st	%g4, [RP + G4*4]; \
	st	%g5, [RP + G5*4]; \
	st	%g6, [RP + G6*4]; \
	st	%g7, [RP + G7*4]; \
	mov	%y, %g1; \
	st	%g1, [RP + Y*4]

#define RESTORE_GLOBALS(RP) \
	ld	[RP + Y*4], %g1; \
	mov	%g1, %y; \
	ld	[RP + G1*4], %g1; \
	ld	[RP + G2*4], %g2; \
	ld	[RP + G3*4], %g3; \
	ld	[RP + G4*4], %g4; \
	ld	[RP + G5*4], %g5; \
	ld	[RP + G6*4], %g6; \
	ld	[RP + G7*4], %g7

#define SAVE_OUTS(RP) \
	st	%i0, [RP + O0*4]; \
	st	%i1, [RP + O1*4]; \
	st	%i2, [RP + O2*4]; \
	st	%i3, [RP + O3*4]; \
	st	%i4, [RP + O4*4]; \
	st	%i5, [RP + O5*4]; \
	st	%i6, [RP + O6*4]; \
	st	%i7, [RP + O7*4]

#define RESTORE_OUTS(RP) \
	ld	[RP + O0*4], %i0; \
	ld	[RP + O1*4], %i1; \
	ld	[RP + O2*4], %i2; \
	ld	[RP + O3*4], %i3; \
	ld	[RP + O4*4], %i4; \
	ld	[RP + O5*4], %i5; \
	ld	[RP + O6*4], %i6; \
	ld	[RP + O7*4], %i7

#define SAVE_WINDOW(SBP) \
	st	%l0, [SBP + (0*4)]; \
	st	%l1, [SBP + (1*4)]; \
	st	%l2, [SBP + (2*4)]; \
	st	%l3, [SBP + (3*4)]; \
	st	%l4, [SBP + (4*4)]; \
	st	%l5, [SBP + (5*4)]; \
	st	%l6, [SBP + (6*4)]; \
	st	%l7, [SBP + (7*4)]; \
	st	%i0, [SBP + (8*4)]; \
	st	%i1, [SBP + (9*4)]; \
	st	%i2, [SBP + (10*4)]; \
	st	%i3, [SBP + (11*4)]; \
	st	%i4, [SBP + (12*4)]; \
	st	%i5, [SBP + (13*4)]; \
	st	%i6, [SBP + (14*4)]; \
	st	%i7, [SBP + (15*4)]

#define RESTORE_WINDOW(SBP) \
	ld	[SBP + (0*4)], %l0; \
	ld	[SBP + (1*4)], %l1; \
	ld	[SBP + (2*4)], %l2; \
	ld	[SBP + (3*4)], %l3; \
	ld	[SBP + (4*4)], %l4; \
	ld	[SBP + (5*4)], %l5; \
	ld	[SBP + (6*4)], %l6; \
	ld	[SBP + (7*4)], %l7; \
	ld	[SBP + (8*4)], %i0; \
	ld	[SBP + (9*4)], %i1; \
	ld	[SBP + (10*4)], %i2; \
	ld	[SBP + (11*4)], %i3; \
	ld	[SBP + (12*4)], %i4; \
	ld	[SBP + (13*4)], %i5; \
	ld	[SBP + (14*4)], %i6; \
	ld	[SBP + (15*4)], %i7

#endif LOCORE

#endif _REG_

/*	@(#)sparc.h 1.1 94/10/31 SMI	*/


/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */


#include <sys/vmparam.h>
#include <machine/mmu.h>
#include <sys/types.h>

/*
 * adb has used "addr_t" as == "unsigned" in a typedef, forever.
 * Now in 4.0 there is suddenly a new "addr_t" typedef'd as "char *".
 *
 * About a million changes would have to be made in adb if this #define
 * weren't able to unto that damage.
 */
#define addr_t unsigned

/*
 * setreg/readreg/writereg use the adb_raddr structure to communicate
 * with one another, and with the floating point conversion routines.
 * "normal" includes the window registers that are in the current window.
 */
struct adb_raddr {
    enum reg_type { r_normal, r_gzero, r_window, r_floating,  r_invalid }
	     ra_type;
    int	    *ra_raddr;
} ;

/*
 * adb keeps its own idea of the current value of most of the
 * processor registers, in an "allregs" structure.  This is used
 * in different ways for kadb, adb -k, and normal adb.  The struct
 * is defined in allregs.h, and the variable (adb_regs) is decleared
 * in accesssr.c.
 */

/*
 * adb's internal register codes for the sparc
 */

/* Integer Unit (IU)'s "r registers" */
#define REG_RN(n)	((n) - FIRST_STK_REG)
#define REG_G0		 0
#define REG_G1		 1
#define REG_G2		 2
#define REG_G3		 3
#define REG_G4		 4
#define REG_G5		 5
#define REG_G6		 6
#define REG_G7		 7

#define REG_O0		 8
#define REG_O1		 9
#define REG_O2		10
#define REG_O3		11
#define REG_O4		12
#define REG_O5		13
#define REG_O6		14
#define REG_O7		15

#define REG_L0		16
#define REG_L1		17
#define REG_L2		18
#define REG_L3		19
#define REG_L4		20
#define REG_L5		21
#define REG_L6		22
#define REG_L7		23

#define REG_I0		24
#define REG_I1		25
#define REG_I2		26
#define REG_I3		27
#define REG_I4		28
#define REG_I5		29
#define REG_I6		30
#define REG_I7		31	/* (Return address minus eight) */

#define REG_SP		14	/* Stack pointer == REG_O6 */
#define	REG_FP		30	/* Frame pointer == REG_I6 */

/* Miscellaneous registers */
#define	REG_Y		32
#define	REG_PSR		33
#define	REG_WIM		34
#define	REG_TBR		35
#define	REG_PC		36
#define	REG_NPC		37
#define LAST_NREG	37	/* last normal (non-Floating) register */

#define REG_FSR		38	/* FPU status */
#define REG_FQ		39	/* FPU queue */

/* Floating Point Unit (FPU)'s "f registers" */
#define REG_F0		40
#define REG_F16		56
#define REG_F31		71

/*
 * NREGISTERS does not include registers from non-current windows.
 * See below.
 */
#define	NREGISTERS	(REG_F31+1)
#define	FIRST_STK_REG	REG_O0
#define	LAST_STK_REG	REG_I5

/*
 * Here, we define codes for the window (I/L/O) registers
 * of the other windows.  Note:  all such codes are greater
 * than NREGISTERS, including the codes that duplicate the
 * registers within the current window.
 *
 * Usage:  REG_WINDOW( window_number, register code )
 *	where window_number is always relative to the current window;
 *		window_number == 0 is the current window.
 *	register_code is between REG_O0 and REG_I7.
 */
#define REG_WINDOW( win, reg ) \
	( NREGISTERS - REG_O0 + (win)*(16) + (reg))
#define WINDOW_OF_REG( reg ) \
	(((reg) - NREGISTERS + REG_O0 )/16)
#define WREG_OF_REG( reg ) \
	(((reg) - NREGISTERS + REG_O0 )%16)

#define MIN_WREGISTER REG_WINDOW( 0,         REG_O0 )
#define MAX_WREGISTER REG_WINDOW( NWINDOW-1, REG_I7 )

#define	REGADDR(r)	(4 * (r))


char	*regnames[]
#ifdef REGNAMESINIT
    = {
	/* IU general regs */
	"g0", "g1", "g2", "g3", "g4", "g5", "g6", "g7",
	"o0", "o1", "o2", "o3", "o4", "o5", "sp", "o7",
	"l0", "l1", "l2", "l3", "l4", "l5", "l6", "l7",
	"i0", "i1", "i2", "i3", "i4", "i5", "fp", "i7",

	/* Miscellaneous */
	"y", "psr", "wim", "tbr", "pc", "npc", "fsr", "fq",

	/* FPU regs */
	"f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",
	"f8",  "f9",  "f10", "f11", "f12", "f13", "f14", "f15",
	"f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
	"f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",
   }
#endif
    ;

#define	U_PAGE	UADDR

#define	TXTRNDSIZ	SEGSIZ

#define	MAXINT		0x7fffffff
#define	MAXFILE		0xffffffff

/*
 * All 32 bits are valid on sparc:  VADDR_MASK is a no-op.
 * It's only here for the sake of some shared code in kadb.
 */
#define VADDR_MASK	0xffffffff

#define	INSTACK(x)	( (x) >= STKmin && (x) <= STKmax )
int STKmax, STKmin;

#define	KVTOPH(x)	(((x) >= KERNELBASE)? (x) - KERNELBASE: (x))

/*
 * A "stackpos" contains everything we need to know in
 * order to do a stack trace.
 */
struct stackpos {
	 u_int	k_pc;		/* where we are in this proc */
	 u_int	k_fp;		/* this proc's frame pointer */
	 u_int	k_nargs;	/* This we can't figure out on sparc */
	 u_int	k_entry;	/* this proc's entry point */
	 u_int	k_caller;	/* PC of the call that called us */
	 u_int	k_flags;	/* sigtramp & leaf info */
	 u_int	k_regloc[ LAST_STK_REG - FIRST_STK_REG +1 ];
};
	/* Flags for k_flags:  */
#define K_LEAF		1	/* this is a leaf procedure */
#define K_CALLTRAMP	2	/* caller is _sigtramp */
#define K_SIGTRAMP	4	/* this is _sigtramp */
#define K_TRAMPED	8	/* this was interrupted by _sigtramp */

/*
 * sparc stack frame is just a struct window (saved local & in regs),
 * followed by some optional stuff:
 *	"6 words for callee to dump register arguments",
 *	"Outgoing parameters past the sixth",
 *  and "Local stack space".
 *
 * Since it is only used via "get", we don't really use it except
 * as documentation.
 *
 * struct sr_frame {
 *	struct window	fr_w;	// 8 each w_local, w_in //
 *	int		fr_regargs[ 6 ];
 *	int		fr_extraparms[ 1 ];
 * };
 */

/*
 * Number of registers usable as procedure arguments
 */
#define NARG_REGS 6

/*
 * Offsets in stack frame of interesting locations:
 */
#define FR_L0		(0*4)	/* (sr_frame).fr_w.w_local[ 0 ] */
#define FR_LREG(reg)	(FR_L0 + (((reg)-REG_L0) * 4))

#define FR_I0		(8*4)	/* (sr_frame).fr_w.w_in[ 0 ] */
#define FR_IREG(reg)	(FR_I0 + (((reg)-REG_I0) * 4))

#define FR_SAVFP	(14*4)	/* (sr_frame).fr_w.w_in[ 6 ] */
#define FR_SAVPC	(15*4)	/* (sr_frame).fr_w.w_in[ 7 ] */

#define FR_ARG0         (17*4)
#define FR_ARG1         (18*4)
#define FR_ARG2         (19*4)
#define FR_XTRARGS	(23*4)  /* >6 Outgoing parameters */

#ifndef KADB
/*
 * On a sparc, to get any but the G and status registers, you need to
 * have a copy of the pcb structure (from the u structure -- u.u_pcb).
 * See /usr/include/sys/user.h and /usr/include/machine/{pcb.h, reg.h}.
 */
struct pcb u_pcb ;
#endif !KADB


/***********************************************************************/
/*
 *	Breakpoint instructions
 */

/*
 * A breakpoint instruction lives in the extern "bpt".
 * Let's be explicit about it this time.  A sparc breakpoint
 * is a trap-always, trap number 1.
 *	"ta 1"
 */
#define SZBPT 4
#define PCFUDGE 0
#ifdef BPT_INIT
    extern	bpt;
	asm("		.global _bpt ");
	asm("		.align 4     ");
#	ifdef KADB
	    asm("_bpt:	ta   126   ");
#	else !KADB
	    asm("_bpt:	ta   1    ");
#	endif !KADB
/* int bpt = 0x91D02001 ; */
#else
int bpt;
#endif


/***********************************************************************/
/*
 *	These #defines are for those few places outside the
 *	disassembler that need to decode a few instructions.
 */

/* Values for the main "op" field, bits <31..30> */
#define SR_FMT2_OP		0
#define SR_CALL_OP		1
#define SR_FMT3a_OP		2

/* Values for the tertiary "op3" field, bits <24..19> */
#define SR_JUMP_OP		0x38
#define SR_TICC_OP		0x3A

/* A value to compare with the cond field, bits <28..25> */
#define SR_ALWAYS		8


/***********************************************************************/
/*
 *	These defines reduce the number of #ifdefs.
 */
#define t_srcinstr(item)  (item)	/* always 32 bits on sparc */
#define ins_type u_long
#define first_byte( int32 ) ((int32) >> 24)
#define INSTR_ALIGN_MASK 3
#define INSTR_ALIGN_ERROR "address must be aligned on a 4-byte boundary"
#define BPT_ALIGN_ERROR "breakpoint must be aligned on a 4-byte boundary"

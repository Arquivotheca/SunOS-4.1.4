#ifndef lint
static	char sccsid_sr[] = "@(#)accesssr.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "adb.h"
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/vmmac.h>
#include <signal.h>
#include "fpascii.h"
#include <sys/ptrace.h>
#include "symtab.h"
#include "allregs.h"

#include "sr_instruction.h"

/*
 * adb's idea of the current value of most of the
 * processor registers lives in "adb_regs".
 */

struct allregs adb_regs;


#ifdef KADB
/*
 * nwindows is now a variable whose value is known to the kernel -
 * it's not clear how the debugger is suppose to learn it - till then:
 */
#define NWINDOW 8 /* # of implemented windows */
#   define EIO		 5
#   define CWP  (((adb_regs.r_psr & 15) +1) % NWINDOW)

#define adb_oreg (adb_regs.r_window[ (-1 == ((CWP -1) % NWINDOW)) ? \
	NWINDOW - 1 : ((CWP -1) % NWINDOW)].rw_in)
#define adb_ireg (adb_regs.r_window[ CWP ].rw_in)
#define adb_lreg (adb_regs.r_window[ CWP ].rw_local)

#else !KADB

/*
 * Libkvm is used (by adb only) to dig things out of the kernel
 */
#include <kvm.h>
#include <sys/user.h>

extern kvm_t *kvmd;				/* see main.c */
extern struct asym *trampsym, *funcsym;		/* see setupsr.c */

#define adb_oreg (adb_regs.r_outs)
#define adb_ireg (adb_regs.r_ins)
#define adb_lreg (adb_regs.r_locals)

#endif !KADB

#define adb_sp	 (adb_oreg[6])
#define adb_o7	 (adb_oreg[7])


#ifndef KADB
/*
 * Read a word from kernel virtual address space (-k only)
 * Return 0 if success, else -1.
 */
kread(addr, p)
	unsigned addr;
	int *p;
{
	if (kvm_read(kvmd, (long)addr, p, sizeof *p) != sizeof *p)
		return (-1);
	return (0);
}

/*
 * Write a word to kernel virtual address space (-k only)
 * Return 0 if success, else -1.
 */
kwrite(addr, p)
	unsigned addr;
	int *p;
{
	if (kvm_write(kvmd, (long)addr, p, sizeof *p) != sizeof *p)
		return (-1);
	return (0);
}
#endif !KADB

extern	struct stackpos exppos;

tbia()
{
	exppos.k_fp = 0;
}


/*
 * Construct an informative error message
 */
 static void
regerr ( reg, wind ) {
  static char rw_invalid[ 60 ];
  char *wp;

	wp = wind ? "window-" : "";
	if ( reg < 0  ||  reg > NREGISTERS ) {
	    sprintf( rw_invalid, "Invalid %sregister %d", wp, reg );
	} else {
	    sprintf( rw_invalid, "Invalid %sregister %s (%d)", wp,
		regnames[ reg ], reg );
	}
	errflg = rw_invalid;
}


/*
 * reg_address is given an adb register code;
 * it fills in the (global)adb_raddr structure.
 * "Fills in" means that it figures out the register type
 * and the address of where adb keeps its idea of that register's
 * value (i.e., in adb's own (global)adb_regs structure).
 *
 * reg_address is called by setreg and readreg;
 * it returns nothing.
 */
  void
reg_address( reg )
int reg;
{
  register struct allregs *arp = &adb_regs;
  register struct adb_raddr *ra = &adb_raddr;

	ra->ra_type = r_normal;
	switch ( reg ) {
	 case REG_PSR:	ra->ra_raddr = & arp->r_psr;	break;
	 case REG_PC:	ra->ra_raddr = & arp->r_pc;	break;
	 case REG_NPC:	ra->ra_raddr = & arp->r_npc;	break;

#ifdef KADB
	 case REG_TBR:	ra->ra_raddr = & arp->r_tbr;	break;
	 case REG_WIM:	ra->ra_raddr = & arp->r_wim;	break;
#else
	 case REG_WIM:
	 case REG_TBR:
		regerr( reg, 0 );
		ra->ra_raddr = 0;
		ra->ra_type = r_invalid;
		break;
#endif !KADB

	 case REG_Y:	ra->ra_raddr = & arp->r_y;	break;

	 /* Globals */
	 case REG_G0:
		ra->ra_raddr = 0;
		ra->ra_type = r_gzero;
		break;

	 case REG_G1:
	 case REG_G2:
	 case REG_G3:
	 case REG_G4:
	 case REG_G5:
	 case REG_G6:
	 case REG_G7:
		ra->ra_raddr = & arp->r_globals[ reg - REG_G1 ];
		break;

	 /* Other registers (O, L, I) in the current window */
	 case REG_O0:
	 case REG_O1:
	 case REG_O2:
	 case REG_O3:
	 case REG_O4:
	 case REG_O5:
	 case REG_SP:   /* REG_O6 is == REG_SP */
	 case REG_O7:
		ra->ra_raddr = &( adb_oreg[ reg - REG_O0 ] );
		break;

	 case REG_L0:
	 case REG_L1:
	 case REG_L2:
	 case REG_L3:
	 case REG_L4:
	 case REG_L5:
	 case REG_L6:
	 case REG_L7:
		ra->ra_raddr = &( adb_lreg[ reg - REG_L0 ]);
		break;

	 case REG_I0:
	 case REG_I1:
	 case REG_I2:
	 case REG_I3:
	 case REG_I4:
	 case REG_I5:
	 case REG_I6:
	 case REG_I7:
		ra->ra_raddr = &( adb_ireg[ reg - REG_I0 ]);
		break;

	 case REG_FQ:	/* Can't get the FQ */
		regerr( reg, 0 );
		ra->ra_raddr = 0;
		ra->ra_type = r_invalid;
		break;

	 case REG_FSR:
		ra->ra_raddr = (int *) &core.c_fpu.fpu_fsr;
		ra->ra_type = r_normal;
		break;

	 default:
		if ( reg >= REG_F0  &&  reg <= REG_F31 ) {
    			ra->ra_type = r_floating;
			ra->ra_raddr = (int *)
			    &core.c_fpu.fpu_regs[ reg - REG_F0 ];
		}
#ifdef KADB
		else if ( reg <= MAX_WREGISTER ) {
		  int w, r;
			/*
			 * Figure out where adb likes to keep the
			 * registers from other register-windows.
			 */
			w = WINDOW_OF_REG( reg );
			r = WREG_OF_REG( reg );

			ra->ra_raddr = &( arp->r_window[ w ].rw_local[ r ] );
    			ra->ra_type = r_window;
		}
#endif KADB
		else {
			regerr( reg, 0 );
			ra->ra_raddr = 0;
    			ra->ra_type = r_invalid;
		}
		break;

	}

#ifdef DEBUG
	{ char *rtp;

		db_printf( "reg_address of reg %d <%s> is %X; ", reg,
		    (reg < NREGISTERS ? regnames[reg] : "a window reg"),
		    adb_raddr.ra_raddr );

		switch ( adb_raddr.ra_type ) {
		case r_normal:
			rtp= "r_normal";
			break;
		case r_gzero:
			rtp= "r_gzero";
			break;
#ifdef KADB
		case r_window:
			rtp= "r_window";
			break;
#endif KADB
		case r_floating:
			rtp= "r_floating";
			break;
		case r_invalid:
			rtp= "r_invalid";
			break;
		default:
			rtp= "default?";
			break;
		}

		db_printf( "type %d (%s)\n", adb_raddr.ra_type, rtp );
		if ( adb_raddr.ra_raddr ) {
		    db_printf( "contents %X\n", *( adb_raddr.ra_raddr ) );
		}
	}
#endif DEBUG
}


#ifdef KADB
/*
 * reg_windows -- called from setreg and readreg, this routine
 * stores or retrieves the value of a register from one of the
 * non-current register windows.  "reg" should be between
 * MIN_WREGISTER and MAX_WREGISTER.
 *
 * We have already called reg_address, so adb_raddr tells us
 * where in adb_regs to get or put the register value.
 */
/*
 * w = WINDOW_OF_REG( reg );
 * r = WREG_OF_REG( reg );
 */

reg_windows( reg, val, wrt_flag ) {
	struct pcb *pcb_ptr;
	addr_t usp;
	int *raddr;

	if ( reg < MIN_WREGISTER  ||  reg > MAX_WREGISTER ) {
		regerr( reg, 1 );
		return 0;
	}

	/* In any case, do adb_raddr.ra_raddr right. */
	if ( wrt_flag ) {
		*( adb_raddr.ra_raddr ) = val;
	} else {
		val = *( adb_raddr.ra_raddr );
	}
	return val;

} /* end reg_windows */
#endif KADB


setreg(reg, val)
int reg;
int val;
{
	reg_address( reg );
	switch ( adb_raddr.ra_type ) {

	 case r_gzero:  /* Always zero -- setreg does nothing */
			break;

	 case r_floating: /* Treat floating regs like normals */
	 case r_normal: /* Normal one -- we have a good address */
			*( adb_raddr.ra_raddr ) = val;
			break;

#ifdef KADB
	 case r_window: /* Go figure it out */
			(void) reg_windows( reg, val, 1 );
			break;
#endif KADB
	}
}


/*
 * readreg -- retrieve value of register reg from adb_regs.
 */
readreg(reg)
int reg;
{
    int val=0;

	reg_address( reg );
	db_printf( "\nreadreg:  Reg_Address of reg %d is %X\n",
	    reg, adb_raddr.ra_raddr );
	switch ( adb_raddr.ra_type ) {

	 case r_gzero:  /* Always zero -- val is already zero */
			break;

	 case r_floating: /* Treat floating regs like normals */
	 case r_normal: /* Normal one -- we have a good address */
			val = *( adb_raddr.ra_raddr );
			break;

#ifdef KADB
	 case r_window: /* Go figure it out */
			val = reg_windows( reg, val, 0 );
			break;
#endif KADB
	}
	return val;
}

#ifdef KADB
/*
 * For ptrace(SETREGS or GETREGS) to work, the registers must be in
 * the form that they take in the core file (instead of the form used
 * by the access routines in this file, i.e., the full machine state).
 * These routines copy the relevant registers.
 */
regs_to_core ( ) {
  struct regs	  *r = &core.c_regs;
  struct allregs *a = &adb_regs;

	db_printf( "Copy regs to core from adb_regs\n" );
	r->r_psr = a->r_psr;
	r->r_pc = a->r_pc;
	r->r_npc = a->r_npc;
	r->r_y = a->r_y;
	r->r_g1 = a->r_globals[0];
	r->r_g2 = a->r_globals[1];
	r->r_g3 = a->r_globals[2];
	r->r_g4 = a->r_globals[3];
	r->r_g5 = a->r_globals[4];
	r->r_g6 = a->r_globals[5];
	r->r_g7 = a->r_globals[6];
	r->r_o0 = a->r_window[0].rw_in[0];
	r->r_o1 = a->r_window[0].rw_in[1];
	r->r_o2 = a->r_window[0].rw_in[2];
	r->r_o3 = a->r_window[0].rw_in[3];
	r->r_o4 = a->r_window[0].rw_in[4];
	r->r_o5 = a->r_window[0].rw_in[5];
	r->r_o6 = a->r_window[0].rw_in[6];
	r->r_o7 = a->r_window[0].rw_in[7];
	db_printf( "Done copying regs to core from adb_regs\n" );
}

#else !KADB

core_to_regs ()
{
	register struct regs *r = &core.c_regs;
	register struct allregs *a = &adb_regs;
	register int reg;

	db_printf( "Copy regs from core to adb_regs\n");
	a->r_psr = r->r_psr;
	a->r_pc  = r->r_pc;
	a->r_npc = r->r_npc;
	a->r_y   = r->r_y;
	a->r_globals[0] = r->r_g1;
	a->r_globals[1] = r->r_g2;
	a->r_globals[2] = r->r_g3;
	a->r_globals[3] = r->r_g4;
	a->r_globals[4] = r->r_g5;
	a->r_globals[5] = r->r_g6;
	a->r_globals[6] = r->r_g7;
	a->r_outs[0] = r->r_o0;
	a->r_outs[1] = r->r_o1;
	a->r_outs[2] = r->r_o2;
	a->r_outs[3] = r->r_o3;
	a->r_outs[4] = r->r_o4;
	a->r_outs[5] = r->r_o5;
	a->r_outs[6] = r->r_o6;
	a->r_outs[7] = r->r_o7;
	for (reg = REG_L0; reg <= REG_L7; reg++){
		a->r_locals[ reg - REG_L0] = get(r->r_o6 + FR_LREG(reg), SSP);
	}
	for (reg = REG_I0; reg <= REG_I7; reg++){
		a->r_ins[ reg - REG_I0] = get(r->r_o6 + FR_IREG(reg), SSP);
	}
	db_printf( "Done copying regs from core to adb_regs\n");
}

/*
 * For ptrace(SETREGS or GETREGS) to work, the registers must be in
 * the form that they take in the core file (instead of the form used
 * by the access routines in this file, i.e., the full machine state).
 * These routines copy the relevant registers.
 */
regs_to_core ()
{
	register struct regs *r = &core.c_regs;
	register struct allregs *a = &adb_regs;
	register int reg;

	db_printf( "Copy regs to core from adb_regs\n" );
	r->r_psr = a->r_psr;
	r->r_pc = a->r_pc;
	r->r_npc = a->r_npc;
	r->r_y = a->r_y;
	r->r_g1 = a->r_globals[0];
	r->r_g2 = a->r_globals[1];
	r->r_g3 = a->r_globals[2];
	r->r_g4 = a->r_globals[3];
	r->r_g5 = a->r_globals[4];
	r->r_g6 = a->r_globals[5];
	r->r_g7 = a->r_globals[6];
	r->r_o0 = a->r_outs[0];
	r->r_o1 = a->r_outs[1];
	r->r_o2 = a->r_outs[2];
	r->r_o3 = a->r_outs[3];
	r->r_o4 = a->r_outs[4];
	r->r_o5 = a->r_outs[5];
	r->r_o6 = a->r_outs[6];
	r->r_o7 = a->r_outs[7];
	for (reg = REG_L0; reg <= REG_L7; reg++){
		put(r->r_o6 + FR_LREG(reg), SSP, a->r_locals[ reg - REG_L0]);
	}
	for (reg = REG_I0; reg <= REG_I7; reg++){
		put(r->r_o6 + FR_IREG(reg), SSP, a->r_ins[ reg - REG_I0]);
	}
	printf( "Done copying regs to core from adb_regs\n" );
}
#endif !KADB


writereg(i, val)
	register int i;
{
	extern int errno;

	if (i >= NREGISTERS) {
		errno = EIO;
		return (0);
	}
	setreg(i, val);

#ifdef KADB

	/* kadb:  normal regs are in adb_regs */
	ptrace(PTRACE_SETREGS, pid, &adb_regs, 0, 0);
	ptrace(PTRACE_GETREGS, pid, &adb_regs, 0, 0);

#else !KADB

	/* normal adb:  regs in adb_regs must be copied */
	regs_to_core( );

	db_printf( "writereg:  e.g., PC is %X\n", core.c_regs.r_pc );

	ptrace(PTRACE_SETREGS, pid, &core.c_regs, 0, 0);
	ptrace(PTRACE_GETREGS, pid, &core.c_regs, 0, 0);

	db_printf( "writereg after rw_pt:  e.g., PC is %X\n",
		core.c_regs.r_pc );

	ptrace(PTRACE_SETFPREGS, pid, core.c_fpu.fpu_regs, 0, 0);
	ptrace(PTRACE_GETFPREGS, pid, core.c_fpu.fpu_regs, 0, 0);

#endif !KADB

#ifdef DEBUG
	{ int x = readreg(i);
	    if (x != val) {
		printf(" writereg %d miscompare: wanted %X got %X ",
			    i, val, x);
	    }
	}
#endif
	return (sizeof (int));
}



/*
 * stacktop collects information about the topmost (most recently
 * called) stack frame into its (struct stackpos) argument.  It's
 * easy in most cases to figure out this info, because the kernel
 * is nice enough to have saved the previous register windows into
 * the proper places on the stack, where possible.  But, if we're
 * a leaf routine (one that avoids a "save" instruction, and uses
 * its caller's frame), we must use the r_i* registers in the
 * (struct regs).  *All* system calls are leaf routines of this sort.
 *
 * On a sparc, it is impossible to determine how many of the
 * parameter-registers are meaningful.
 */

stacktop(spos)
	register struct stackpos *spos;
{
	register int i;
	int leaf;
	char *saveflg;

	for (i = FIRST_STK_REG; i <= LAST_STK_REG; i++)
		spos->k_regloc[ REG_RN(i) ] = REGADDR(i);

	spos->k_fp = readreg(REG_SP);
	spos->k_pc = readreg(REG_PC);
	spos->k_flags = 0;

	saveflg = errflg;
	/*
	 * If we've stopped in a routine but before it has done its
	 * "save" instruction, is_leaf_proc will tell us a little white
	 * lie about it, calling it a leaf proc.
	 */
	leaf = is_leaf_proc( spos, readreg(REG_O7));
	if (errflg) {
		/*
		 * oops -- we touched something we ought not to have.
		 * cannot trace caller of "start"
		 */
		spos->k_entry = MAXINT;
		spos->k_nargs = 0;
		errflg = saveflg; /* you didn't see this */
		return;
	}
	errflg = saveflg;

	if ( leaf ) {

#ifdef DEBUG
{ extern int adb_debug;

    if ( adb_debug  &&  spos->k_fp != core.c_regs.r_sp ) {
	printf( "stacktop -- fp %X != sp %X.\n", spos->k_fp, core.c_regs.r_sp );
    }
}
#endif DEBUG
		if ( (spos->k_flags & K_SIGTRAMP) == 0 )
		    spos->k_nargs = 0;

	} else {

		findentry(spos, 1);

	}
}



/*
 * findentry -- assuming k_fp (and k_pc?) is already set, we set the
 * k_nargs, k_entry and k_caller fields in the stackpos structure.  This
 * routine is called from stacktop() and from nextframe().  It assumes it
 * is not dealing with a "leaf" procedure.  The k_entry is easy to find
 * for any frame except a "leaf" routine.  On a sparc, since we cannot
 * deduce the nargs, we'll call it "6".  (This can be overridden with the
 * "$cXX" command, where XX is a one- or two-digit hex number which will
 * tell adb how many parameters to display.)
 *
 * Note -- findentry is also expected to call findsym, thus setting
 * cursym to the symbol at the entry point for the current proc.
 * If this call was an indirect one, we rely on the symbol thus
 * found; otherwise we could not find our entry point.
 */
#define CALL_INDIRECT -1 /* flag from calltarget for an indirect call */

findentry (spos, fromTop )
	register struct stackpos *spos;
	int fromTop;
{
	char *saveflg = errflg;
	long offset;

	errflg = 0;
	spos->k_caller = get( spos->k_fp + FR_SAVPC, DSP );
	if ( errflg  &&  fromTop ) {
		errflg = 0;
		spos->k_fp = readreg(REG_FP);
		spos->k_caller = get( spos->k_fp + FR_SAVPC, DSP );
	}
	if ( errflg == 0 ) {
		spos->k_entry = calltarget( spos->k_caller );
	}

#if DEBUG
	dump_spos( "Findentry called with :->", spos );
#endif DEBUG

	if ( errflg == 0 ) {
	    db_printf( "findentry:  caller 0x%X, entry 0x%X\n",
		spos->k_caller, spos->k_entry );
	} else {
	    db_printf( "findentry:  caller 0x%X, errflg %s\n",
		spos->k_caller, errflg );
	}

	if ( errflg  ||  spos->k_entry == 0 ) {
		/*
		 * oops -- we touched something we ought not to have.
		 * cannot trace caller of "start"
		 */
		spos->k_entry = MAXINT;
		spos->k_nargs = 0;
		spos->k_fp = 0;   /* stopper for printstack */
		errflg = saveflg; /* you didn't see this */
		return;
	}
	errflg = saveflg;

	/* first 6 args are in regs -- may be overridden by trampcheck */
	spos->k_nargs = NARG_REGS;
	spos->k_flags &= ~K_LEAF;

	if ( spos->k_entry == CALL_INDIRECT ) {
		offset = findsym( spos->k_pc );
		if ( offset != MAXINT ) {
			spos->k_entry = cursym->s_value;
		} else {
			spos->k_entry = MAXINT;
		}
#ifndef KADB
		trampcheck( spos );
#endif !KADB
	}

#if DEBUG
	dump_spos( "Findentry returns :->", spos );
#endif DEBUG

}



/*
 * is_leaf_proc -- figure out whether the routine we are in SAVEd its
 * registers.  If it did NOT, is_leaf_proc returns true and sets the k_entry
 * and k_caller fields of spos.   Here's why we have to know it:
 *
 *	Normal (non-Leaf) routine	Leaf routine
 * sp->		"my" frame		   caller's frame
 * i7->		caller's PC		   caller's caller's PC
 * o7->		invalid			   caller's PC
 *
 * I.e., we don't know who our caller is until we know if we're a
 * leaf proc.   (Note that for adb's purposes, we are considered to be
 * in a leaf proc even if we're stopped in a routine that will, but has
 * not yet, SAVEd its registers.)
 *
 * The way to find out if we're a leaf proc is to find our own entry point
 * and then check the following few instructions for a "SAVE" instruction.
 * If there is none that are < PC, then we are a leaf proc.
 *
 * We find our own entry point by looking for a the largest symbol whose
 * address is <= the PC.  If the executable has been stripped, we will have
 * to do a little more guesswork; if it's been stripped AND we are in a leaf
 * proc, AND the call was indirect through a register, we may be out of luck.
 */

static
is_leaf_proc ( spos, cur_o7 )
	register struct stackpos *spos;
	addr_t cur_o7;		/* caller's PC if leaf proc (rtn addr) */
				/* invalid otherwise */
{
	addr_t	usp,		/* user's stack pointer */
		upc,		/* user's program counter */
		sv_i7,		/* if leaf, caller's PC ... */
				/* else, caller's caller's PC */
		cto7,		/* call target of call at cur_o7 */
				/* (if leaf, our entry point) */
		cti7,		/* call target of call at sv_i7 */
				/* (if leaf, caller's entry point) */
		near_sym;	/* nearest symbol below PC; we hope it ... */
				/* ... is the address of the proc we're IN */
	int offset, leaf;
	char *saveflg = errflg;

	errflg = 0;

		/* Collect the info we'll need */
	usp = spos->k_fp;
	upc = spos->k_pc;

#ifdef DEBUG
{ extern int adb_debug;

	/*
	 * Used to set usp, upc from core.c_regs.  Make sure that
	 * it's now ok not to.
	 */
	if ( adb_debug && (usp != core.c_regs.r_sp || upc != core.c_regs.r_pc )) {
		printf( "is_leaf_proc -- usp or upc confusion:  usp %X upc %X\n",
			usp, upc );
		printf( "\tcore.c_regs.r_( sp %X, pc %X ).\n",
		core.c_regs.r_sp, core.c_regs.r_pc );
	}
}
#endif DEBUG

	offset = findsym( upc, ISYM );
	if ( offset == MAXINT ) {
		near_sym = -1;
	} else {
		near_sym = cursym->s_value;

		/*
		 * has_save will look at the first four instructions
		 * at near_sym, unless upc is within there.
		 */
		if ( has_save( near_sym, upc ) ) {
			/* Not a leaf proc.  This is the most common case. */
			return 0;
		}
	}


	/*
	 * OK, we either had no save instr or we have no symbols.
	 * See if the saved o7 could possibly be valid.  (We could
	 * get fooled on this one, I think, if we're really in a non-leaf,
	 * have no symbols, and o7 still (can it?) has the address of
	 * an indirect call (a call to ptrcall or a jmp [%reg], [%o7]).)
	 *
	 * Also, if we ARE a leaf, and have no symbols, and o7 was an
	 * indirect call, we *cannot* find our own entry point.
	 */

	/*
	 * Is there a call at o7?  (or jmp w/ r[rd] == %o7)
	 */
	cto7 = calltarget( cur_o7 );
	if ( cto7 == 0 ) return 0;		/* nope */

	/*
	 * Is that call near (but less than) where the pc is?
	 * If it's indirect, skip these two checks.
	 */
	db_printf( "Is_leaf_proc cur_o7 %X, cto7 %X\n", cur_o7, cto7 );

	if ( cto7 == CALL_INDIRECT ) {
		if ( near_sym != -1 ) {
			cto7 = near_sym;	/* best guess */
		} else {
			errflg = "Cannot trace stack";
			return 0;
		}
	} else {
		(void) get( cto7, ISP );	/* is the address ok? */
		if ( errflg  ||  cto7 > upc ) {
			errflg = saveflg;
			return 0;	/* nope */
		}

		/*
		 * Is the caller's call near that call?
		 */
		sv_i7 = get( usp + FR_SAVPC, SSP );
		cti7 = calltarget( sv_i7 );

		db_printf( "Is_leaf_proc caller's call sv_i7 %X, cti7 %X\n",
		    sv_i7, cti7 );

		if ( cti7 != CALL_INDIRECT ) {
			if ( cti7 == 0 ) return 0;
			(void) get( cti7, ISP );	/* is the address ok? */
			if ( errflg  ||  cti7 > cur_o7 ) {
				errflg = saveflg;
				return 0;	/* nope */
			}
		}

		/*
		 * check for a SAVE instruction
		 */
		if ( has_save( cto7 ) ) {
			/* not a leaf. */
			return 0;
		}
	}

	/*
	 * Set the rest of the appropriate spos fields.
	 */
	spos->k_caller = cur_o7;
	spos->k_entry = cto7;
	spos->k_flags |= K_LEAF;

	/*
	 * Yes, it is possible (pathological, but possible) for a
	 * leaf routine to be called by _sigtramp.  Check for this.
	 */
#ifndef KADB
	trampcheck( spos );
#endif !KADB

	return 1;

} /* end is_leaf_proc */



/*
 * The "save" is typically the third instruction in a routine.
 * SAVE_INS is set to the farthest (in bytes!, EXclusive) we should reasonably
 * look for it.  "xlimit" (if it's between procaddr and procaddr+SAVE_INS)
 * is an overriding upper limit (again, exclusive) -- i.e., it is the
 * address after the last instruction we should check.
 */
#define SAVE_INS (5*4)

has_save ( procaddr, xlimit )
	addr_t procaddr, xlimit;
{
	char *asc_instr, *disassemble();

	if ( procaddr > xlimit  ||  xlimit > procaddr + SAVE_INS ) {
		xlimit = procaddr + SAVE_INS;
	}

	/*
	 * Find the first three instructions of the current proc:
	 * If none is a SAVE, then we are in a leaf proc and will
	 * have trouble finding caller's PC.
	 */
	while ( procaddr < xlimit ) {
		asc_instr = disassemble( get( procaddr, ISP), procaddr );

		if ( firstword( asc_instr, "save" ) ) {
			return 1;	/* yep, there's a save */
		}

		procaddr += 4;	/* next instruction */
	}

	return 0;	/* nope, there's no save */
} /* end has_save */


static
firstword ( ofthis, isthis )
	char *ofthis, *isthis;
{
	char *ws;

	while ( *ofthis == ' '  ||  *ofthis == '\t' )
		++ofthis;
	ws = ofthis;
	while ( *ofthis  &&  *ofthis != ' '  &&  *ofthis != '\t' )
		++ofthis;

	*ofthis = 0;
	return strcmp( ws, isthis ) == 0;
}



/*
 * calltarget returns 0 if there is no call there or if there is
 * no "there" there.  A sparc call is a 0 bit, a 1 bit and then
 * the word offset of the target (I.e., one fourth of the number
 * to add to the pc).  If there is a call but we can't tell its
 * target, we return "CALL_INDIRECT".
 *
 * Two complications:
 * 1-	it might be an indirect jump, in which case we can't know where
 *	its target was (the register was very probably modified since
 *	the call occurred).
 * 2-	it might be a "jmp [somewhere], %o7"  (r[rd] is %o7).
 *	if somewhere is immediate, we can check it, but if it's
 *	a register, we're again out of luck.
 */
static
calltarget ( calladdr )
	addr_t calladdr;
{
	char *saveflg = errflg;
	long instr, offset, symoffs, ct, jt;

	errflg = 0;
	instr = get( calladdr, ISP );
	if ( errflg ) {
		errflg = saveflg;
		return 0;	/* no "there" there */
	}

	if ( X_OP(instr) == SR_CALL_OP ) {
		/* Normal CALL instruction */
		offset = SR_WA2BA( instr );
		ct = offset + calladdr;

		/*
		 * If the target of that call (ct) is an indirect jump
		 * through a register, then say so.
		 */
		instr = get( ct, ISP );
		if ( errflg ) {
			errflg = saveflg;
			return 0;
		}

		if ( ijmpreg( instr ) )	return CALL_INDIRECT;
		else			return ct;
	}

	/*
	 * Our caller wasn't a call.  Was it a jmp?
	 */
	return  jmpcall( instr );
}


static struct {
	int op, rd, op3, rs1, imm, rs2, simm13;
} jmp_fields;

static
ijmpreg ( instr ) long instr; {
	if ( splitjmp( instr ) ) {
		return jmp_fields.imm == 0  ||  jmp_fields.rs1 != 0;
	} else {
		return 0;
	}
}

static
jmpcall ( instr ) long instr; {
  int dest;

	if ( splitjmp( instr ) == 0 ||	    /* Give up if it ain't a jump, */
		jmp_fields.rd != REG_O7 ) { /* or it doesn't save pc into %o7 */
		return 0;
	}

	/*
	 * It is a jump that saves pc into %o7.  Find its target, if we can.
	 */
	if ( jmp_fields.imm == 0  ||  jmp_fields.rs1 != 0 )
		return CALL_INDIRECT;	/* can't find target */

	/*
	 * The target is simm13, sign extended, not even pc-relative.
	 * So sign-extend and return it.
	 */
	return SR_SEX13(instr);
}

static
splitjmp ( instr ) long instr; {

	jmp_fields.op = X_OP( instr );
	jmp_fields.rd = X_RD( instr );
	jmp_fields.op3 = X_OP3( instr );
	jmp_fields.rs1 = X_RS1( instr );
	jmp_fields.imm = X_IMM( instr );
	jmp_fields.rs2 = X_RS2( instr );
	jmp_fields.simm13 = X_SIMM13( instr );

	if ( jmp_fields.op == SR_FMT3a_OP )
		return  jmp_fields.op3 == SR_JUMP_OP;
	else
		return 0;
}


/*
 * look for a PIC PLT entry : a SETHI/OR/JMP trio. If the pattern
 * matches, extract the jmp dest addr from the sethi and or instructions
 */
ispltentry(addr)
	int *addr;
{
	unsigned int instr[3], dest;
	int i;
	char *saveflg;

	for (i = 0; i < 3; i++, addr++) {
		errflg = 0;
		instr[i] = get( addr, ISP );
		if ( errflg ) {
			errflg = saveflg;
			return 0;
		}
	}
	if (	X_OP(instr[0]) == SR_FMT2_OP && X_OP2(instr[0]) == SR_SETHI_OP &&
		X_OP(instr[1]) == SR_FMT3a_OP  && X_IMM(instr[1]) &&
			X_OP3(instr[1]) == SR_OR_OP &&
		X_OP(instr[2]) == SR_FMT3a_OP && X_OP3(instr[2]) == SR_JUMP_OP ) {
			dest = SR_HI( SR_SEX22( X_DISP22(instr[0]) ) ) |
						  SR_SEX13( X_SIMM13(instr[1]) );
	} else {
		dest = 0;
	}
	return dest;
}



/*
 * nextframe replaces the info in spos with the info appropriate
 * for the next frame "up" the stack (the current routine's caller).
 *
 * Called from printstack (printsr.c) and qualified (expr.c).
 */
nextframe (spos)
	register struct stackpos *spos;
{
	int val, regp, i;
	register instruc;

#ifndef KADB
	if (spos->k_flags & K_CALLTRAMP) {
		trampnext( spos );
		errflg = 0;
	} else
#endif !KADB
	{
		if (spos->k_entry == MAXINT) {
			/*
			 * we don't know what registers are
			 * involved here--invalidate all
			 */
			for (i = FIRST_STK_REG; i <= LAST_STK_REG; i++) {
				spos->k_regloc[ REG_RN(i) ] = -1;
			}
		} else {
			for (i = FIRST_STK_REG; i <= LAST_STK_REG; i++) {
				spos->k_regloc[ REG_RN(i) ] =
					spos->k_fp + 4*REG_RN(i);
			}
		}

		/* find caller's pc and fp */
		spos->k_pc = spos->k_caller;

		if ( (spos->k_flags & (K_LEAF|K_SIGTRAMP)) == 0 ) {
			spos->k_fp = get(spos->k_fp+FR_SAVFP, DSP);
		}
		/* else (if it is a leaf or SIGTRAMP), don't change fp. */

		/*
		 * now we have assumed the identity of our caller.
		 */
		if ( spos->k_flags & K_SIGTRAMP ) {
		    /* Preserve K_LEAF */
		    spos->k_flags = (spos->k_flags | K_TRAMPED) & ~K_SIGTRAMP;
		} else {
		    spos->k_flags = 0;
		}
		findentry(spos, 0);
	}
	db_printf( "nextframe returning %X\n",  spos->k_fp );
	return (spos->k_fp);

} /* end nextframe */


#ifndef KADB

/*
 * signal handling routines, sort of:
 * The following set of routines (tramp*) handle situations where
 * the stack trace includes a caught signal.
 *
 * If the current PC is in _sigtramp, then the debuggee has
 * caught a signal.  This causes an anomaly in the stack trace.
 *
 * trampcheck is called from findentry, and just sets the K_CALLTRAMP
 * flag (warning nextframe that the next frame will be _sigtramp).  Its
 * only effect on this line of the stack trace is to make
 * sure that only three arguments are printed out.
 *
 * [[  trampnext (see below) is called from nextframe just before the	]]
 * [[  "_sigtramp" frame is printed out; it sets up the stackpos so	]]
 * [[  as to be able to find the interrupted routine.			]]
 *
 * When the return address is in sigtramp, the current routine
 * is your signal-catcher routine.  It has been called with three
 * parameters:  (the signal number, a signal-code, and an address
 * of a sigcontext structure).
 */
trampcheck (spos) register struct stackpos *spos; {

	if ( trampsym == 0 ) return;

#if DEBUG
	db_printf( "In trampcheck, cursym " );
	if ( cursym == 0 ) db_printf( "is NULL\n" );
	else db_printf( "%X <%s>\n", cursym->s_value, cursym->s_name );
#endif DEBUG

	findsym(spos->k_caller, ISYM);

#if DEBUG
	db_printf( "In trampcheck after findsym, cursym " );
	if ( cursym == 0 ) db_printf( "is NULL\n" );
	else db_printf( "%X <%s>\n", cursym->s_value, cursym->s_name );
#endif DEBUG

	if ( cursym != trampsym ) return;

	spos->k_flags |= K_CALLTRAMP;
	spos->k_nargs = 3;

#if DEBUG
	db_printf( "trampcheck:  trampsym 0x%X->%X caller %X pc %X\n",
		trampsym, trampsym->s_value, spos->k_caller, spos->k_pc );
#endif
}

/*
 * trampnext sets the stackpos structure up so that "_sigtramp"
 * (the C library routine that catches signals and calls your C
 * signal handler) will show up as the next routine in the stack
 * trace, and so that the next routine found after that will be
 * the one that was interrupted.  One complication is that the
 * interrupted routine may have been a leaf routine.
 *
 * Let's give the stack frame where we found the PC in sigtramp a
 * name:  "STSF".
 *
 * if the interrupted routine was not a leaf routine:
 *    STSF:fp points to a garbage register-save window.  Ignore it.
 *    The sigcontext contains an sp and a pc -- the pc is an address
 *    in the routine that was interrupted, and the sp points to a valid
 *    stack frame.  Just go on from there.
 *
 * if the interrupted routine was a leaf, it's much less straightforward:
 *    STSF:fp points to a register-save window that includes
 *    the return address of the leaf's caller, and the arguments
 *    (o-regs) that were passed to the leaf, but not a valid sp.
 *    The next stack frame is found through the old-sp in the sigcontext.
 *
 * This strategy will be complicated further if we decide to support
 * the "$cXX" command that looks for more than six parameters.
 */

/*
 * This sneaky and icky routine is called only by trampnext.
 * Older _sigtramps (pre-beta?) didn't put the "hidden struct arg" into
 * the stack frame that _sigtramp creates.  So we check at the right place
 * as well as the preceding word.
 */
static struct sigcontext *
find_scp ( kfp, was_leaf ) addr_t kfp; {
  addr_t wf, scp;

	wf = get( kfp + FR_IREG(REG_FP), DSP );
	scp = get( wf + FR_ARG2, DSP );
	if ( scp < wf ) {
	    scp = get( wf + FR_ARG1, DSP );
	}
	if ( was_leaf == 0 ) {
	    return find_scp( wf, 1 ); 	/* lie a little */
	}
#if DEBUG
	db_printf( "find_scp kfp %X, find_scp returns %X, ", kfp, scp );
#endif
	return (struct sigcontext *)scp;
}

trampnext (spos) struct stackpos *spos; {
  int was_leaf;
  struct stackpos tsp;
  addr_t maybe_o7, save_fp;
  struct sigcontext *scp; /* address in the subprocess.  Don't dereference */

#if DEBUG
	dump_spos( "trampnext :-", spos );

	db_printf( "In trampnext, cursym " );
	if ( cursym == 0 ) db_printf( "is NULL\n" );
	else db_printf( "%X <%s>\n", cursym->s_value, cursym->s_name );
#endif DEBUG

	/*
	 * The easy part -- set up the spos for _sigtramp itself.
	 */
	spos->k_pc = spos->k_caller;
	spos->k_entry = trampsym->s_value;
	was_leaf = ( spos->k_flags & K_LEAF );
	spos->k_flags = K_SIGTRAMP;
	spos->k_nargs = 0;

	/*
	 * The hard part -- set up the spos to enable it to find
	 * _sigtramp's caller.  Need to know whether it was a leaf.
	 */
	scp = find_scp( spos->k_fp, was_leaf );
	tsp = *spos;

	maybe_o7 = get( spos->k_fp + FR_SAVPC, DSP );
	tsp.k_pc = get( &scp->sc_pc, DSP );

	/* set K_LEAF for use in printstack */
	if ( is_leaf_proc( &tsp, maybe_o7 ) ) {
	    db_printf( "trampnext thinks it's a leaf proc.\n" );
	    spos->k_flags |= K_LEAF;
	} else {
	    db_printf( "trampnext thinks it's not a leaf proc.\n" );
	}

	spos->k_fp = get( &scp->sc_sp, DSP );
	spos->k_caller = get( &scp->sc_pc, DSP );

#if DEBUG
	db_printf( "In trampnext, cursym " );
	if ( cursym == 0 ) db_printf( "is NULL\n" );
	else db_printf( "%X <%s>\n", cursym->s_value, cursym->s_name );
	dump_spos( "trampnext thinks it's done.  Its own tsp ==", &tsp );
	dump_spos( "trampnext has spos ==", spos );
#endif

} /* end trampnext */

#endif /* (not KADB) */

#if DEBUG
dump_spos ( msg, ppos ) char *msg; struct stackpos *ppos; {
  int orb;

	if ( adb_debug == 0 ) return;

	printf( "%s\n", msg );
	printf( "\tPC %8X\tEntry  %8X\t\tnargs %d\n",
	    ppos->k_pc, ppos->k_entry, ppos->k_nargs );

	printf( "\tFP %8X\tCaller %8X\t\t", ppos->k_fp, ppos->k_caller );

	orb = 0;
	if ( ppos->k_flags & K_CALLTRAMP ) {
		orb=1; printf( "K_CALLTRAMP" );
	}
	if ( ppos->k_flags & K_SIGTRAMP ) {
		if ( orb ) printf( " | " );
		printf( "K_SIGTRAMP" );
		orb=1;
	}
	if ( ppos->k_flags & K_LEAF ) {
		if ( orb ) printf( " | " );
		printf( "K_LEAF" );
		orb=1;
	}
	if ( ppos->k_flags & K_TRAMPED ) {
		if ( orb ) printf( " | " );
		printf( "K_TRAMPED" );
		orb=1;
	}

	printf( "\n" );
}

#endif

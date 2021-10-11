#ifndef lint
static  char sccsid[] = "@(#)adb_ptrace.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <sys/ptrace.h>
#include "adb.h"
#include "sr_instruction.h"

/*
 * This will replace all of adb's ptrace calls that might be asking
 * for a single-step operation, which must be simulated on a sparc.
 * These all happen to be located in runpcs.c.
 *
 * This file has its own equivalents for setbp and delbp,
 * named ss_setbp and ss_delbp.  These do their thing one
 * breakpoint at a time, and have different error messages.
 *
 * In case you're new to this architecture, here's how the PC
 * and NPC get set correctly by the kernel as it returns control
 * to the Process Being Debugged (PBD):
 *	adb might call ptrace( PTRACE_SETREGS, ... ); in order to set
 *		the PC and NPC into the kernel's (struct regs).  For
 *		clarity, let's call these "wPC" and "wNPC" for "wanted".
 *	adb calls ptrace( PTRACE_CONT, ... );
 *	the kernel picks two of its private ("I" or "L") registers
 *	to use, let's call them %x and %y; it sets %x := wPC and
 *	%y := wNPC, and then executes the following instruction pair:
 *			JMP	[%x]
 *			RETT	[%y]
 *	or, in a little more detail (each non-CTI finishes by copying
 *	PC := NPC; then NPC += 4;):
 *					PC == j-4, NPC := j.
 *		j-4:	non-CTI INSTRUCTION
 *					PC == j,   NPC := j+4.
 *		j:	JMP	[%x]
 *					PC == j+4, NPC := %x == wPC.
 *		j+4:	RETT	[%y]
 *					PC == wPC  , NPC := %y == wNPC.
 *		... and goes on into PBD's code ...
 */

typedef enum {
	br_error,
	not_branch,
	bicc,
	bicc_annul,
	ba,
	ba_annul,
	ticc,
	ta
} br_type;

extern debugging;

	
/*
 * The chg_pc argument allows us to tell the kernel whether the user
 * explicitly asked to change the pc.  If so (chg_pc != 0), the kernel
 * will set npc to (new)pc+4.  Otherwise, we just give the kernel a "1"
 * for the PC, indicating that it should just go on from "here", and not
 * change npc.
 */
adb_ptrace ( mode, pid, upc, xsig, chg_pc ) enum ptracereq mode; {
  int rtn;
  struct bkpt bk_npc, bk_pc8, bk_trg ;
  addr_t	 npc,    pc8,    trg ;
  addr_t	ptpc;			/* pc to give ptrace */
  br_type br, figure_branch( );

	ptpc =  chg_pc ?  upc  :  1 ;

	if( mode != PTRACE_SINGLESTEP )
		return ptrace( mode, pid, ptpc, xsig );

	/*
	 * We must set at least two breakpoints in order to be sure that
	 * we're really single-stepping:
	 *	put one at NPC.
	 *	IF instr(PC) is a Ticc (trap),
	 *		put one at NPC (which will (usually?) be == PC+4)
	 *	ELSE IF instr(PC) is conditional branch with ANNUL bit,
	 *		THEN put one at PC+8!
	 *	ELSE IF instr(PC) is unconditional branch, with ANNUL,
	 *		THEN put one at the branch target.
	 * The FIRST single-step would be a special case because we do not
	 * know NPC, but adb does not allow a single-step without first
	 * "run"ning and hitting a breakpoint.
	 */
	npc = (addr_t) core.c_regs.r_npc ;
	pc8 = (addr_t) (core.c_regs.r_pc +8);

	/* figure_branch sets trg to target, if it found a branch */
	br = figure_branch( core.c_regs.r_pc, &trg );

	/*
	 * Normally, this is the one breakpoint that we'll need.
	 * We always put one here.
	 */
	bk_npc.flag = 1;
	bk_npc.loc  = npc;
	ss_setbp( &bk_npc );

	if( br == bicc_annul  &&  pc8 != npc ) {
		bk_pc8.flag = 1;
		bk_pc8.loc  = pc8 ;
		ss_setbp( &bk_pc8 );
	} else {
		bk_pc8.flag = 0;
	}

	if( br == ba_annul  &&  trg != npc  &&  trg != pc8 ) {
		bk_trg.flag = 1;
		bk_trg.loc  = trg ;
		ss_setbp( &bk_trg );
	} else {
		bk_trg.flag = 0;
	}


	/*
	 * FINALLY, pretend to do the single-step.
	 */
#ifdef KADB
	(void) ptrace( PTRACE_SINGLESTEP, pid, upc, xsig );
	/* Did kadb really want this one? */
	rtn = ptrace( PTRACE_CONT, pid, upc, xsig );
#else
	rtn = ptrace( PTRACE_CONT, pid, ptpc, xsig );
#endif

	bpwait( PTRACE_CONT );
	chkerr();

	ss_delbp( &bk_npc );
	ss_delbp( &bk_pc8 );
	ss_delbp( &bk_trg );

	return rtn;
}


static
ss_delbp ( bp ) register struct bkpt *bp; {

	if (bp->flag && (bp->flag & BKPT_ERR) == 0)
		(void) writeproc(bp->loc, (char *)&bp->ins, SZBPT);
}

static
ss_setbp ( bp ) struct bkpt *bp; {

	if (bp->flag) {
		if( ((int) bp->loc) & INSTR_ALIGN_MASK
		|| readproc(bp->loc, (char *)&bp->ins, SZBPT) != SZBPT
		|| writeproc(bp->loc, (char *)&bpt, SZBPT) != SZBPT) {
			prints("single-stepper cannot set breakpoint: ");
			psymoff(bp->loc, ISYM, "\n");
			bp->flag |= BKPT_ERR;	/* turn on err flag */
		} else {
			bp->flag &= ~BKPT_ERR;	/* turn off err flag */
#ifdef DEBUG
			if (debugging >= 2)
				prints("set breakpoint\n");
#endif
		}
	}
}


/*
 * figure_branch -- return the "branch-type" of the instruction at iaddr.
 * If it is a branch, set *targetp to be the branch's target address.
 */
 static br_type
figure_branch ( iaddr, targetp )  addr_t iaddr, *targetp; {
  long instr, annul, cond, op2, op3;
  long br_offset;	/* Must be *signed* for the sign-extend to work */
  br_type brt;

	/* Don't forget that it might be a register-indirect branch!!!!! */
	instr = get( iaddr, ISP );
	if( errflg ) {
	    return br_error;
	}

	op2 = X_OP2(instr);
	op3 = X_OP3(instr);
	annul = X_ANNUL(instr);
	cond = X_COND(instr);

	brt = not_branch;
	switch( X_OP(instr) ) {	/* switch on main opcode */
	 case SR_CALL_OP:
		break;

	 case SR_FMT2_OP:
		switch( op2 ) {
		 case SR_FBCC_OP:
		 case SR_BICC_OP:
			if( cond == SR_ALWAYS ) { brt = ba; }
			else			{ brt = bicc; }
			brt = (br_type)( (long)brt + annul );

			/* Hey!  C Optimizer!  Look at this macro usage! */
			br_offset = SR_WA2BA( SR_SEX22( X_DISP22(instr) ) );

			*targetp = iaddr + br_offset;
			break;
		}
		break;

	 case SR_FMT3a_OP:
		if( op3 == SR_TICC_OP ) {
			if( cond == SR_ALWAYS ) { brt = ta; }
			else			{ brt = ticc; }
		}
		break;

	 default:		/* format three */
		break;

	} /* end switch on main opcode */

	return brt;

} /* end figure_branch */

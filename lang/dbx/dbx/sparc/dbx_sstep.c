#ifndef lint
static	char sccsid[] = "@(#)dbx_sstep.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Simulate single-stepping on sparc
 */

#include "defs.h"
#include <sys/ptrace.h>
#include <sys/types.h>

#define Word sas_Word
#include "sas_includes.h"
#undef  Word
#undef  reg

#include "machine.h"
#include "runtime.h"
#include "process.h"
#include "mappings.h"

/*
 * This replaces all of dbx's ptrace calls that might be asking
 * for a single-step operation, which must be simulated on a sparc.
 * These all happen to be located in runpcs.c.
 *
 * In case you're new to this architecture, here's how the PC
 * and NPC get set correctly by the kernel as it returns control
 * to the Process Being Debugged (PBD):
 *	dbx might call ptrace( PTRACE_SETREGS, ... ); in order to set
 *		the PC and NPC into the kernel's (struct regs).  For
 *		clarity, let's call these "wPC" and "wNPC" for "wanted".
 *	dbx calls ptrace( PTRACE_CONT, ... );
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
 * dbx_ptrace -- pretend that ptrace has a single-step capability.
 *
 * In most cases, this merely means setting a breakpoint at nPC and
 * continuing -- the instruction at PC will be run, then the bkpt at
 * nPC will give me back control.
 *
 * One exception is if nPC is a branch which has the annul bit set.
 * Executing the branch is no problem, but the trap at nPC may be
 * annulled, so we need to put another bkpt at PC+8 (in case the branch
 * is not taken), and another at the target of the branch (in case it
 * is taken).
 *
 * The other exception is if we're working on a CTI couple.
 * (This routine is used only in pstep, in process.c.)
 */

public int dbx_ptrace ( mode, pid, upc, xsig )
enum ptracereq mode;
{
  int rtn;
  Address npc, pc8, trg ;
  int     f_npc, f_pc8, f_trg;		/* flags */
  br_type br, figure_branch( );
  int status;

	if( mode != PTRACE_SINGLESTEP ) {
	    panic( "dbx_ptrace called with mode != PTRACE_SINGLESTEP" );
	}

	npc = reg( REG_NPC );
	pc8 = reg( REG_PC ) +8;

	/* figure_branch sets trg to target, if it found a branch */
	br = figure_branch( reg( REG_PC ), &trg );

	/*
	 * Normally, this is the one breakpoint that we'll need.
	 * We always put one here.
	 */
	f_npc = 1;
	setbp( npc );

	f_pc8 = 0;
	f_trg = 0;

	if( br == bicc_annul ) {
	    if( pc8 != npc ) {
		f_pc8 = 1;
		setbp( pc8 );
	    }
	} else {
	    if( br == ba_annul  &&  trg != npc  ) {
		if( trg ) {
		    f_trg = 1;
		    setbp( trg );
		}
	    }
	}

	{ extern int errno;

	    /*
	     * PTRACE_CONT will continue "from here" if given a pc of 1.
	     * If given the actual pc from which to continue, the kernel
	     * will set npc to pc+4, which we don't want.
	     */
	    if (ptrace(PTRACE_CONT, pid, 1, 0, 0) < 0) {
		panic("error %d trying to continue process", errno);
	    }

	    pwait( pid, &status );
	}

	if( f_npc ) unsetbp( npc );
	if( f_pc8 ) unsetbp( pc8 );
	if( f_trg ) unsetbp( trg );
	return status;
}


/*
 * figure_branch -- return the "branch-type" of the instruction at iaddr.
 * If it is a branch, set *targetp to be the branch's target address.
 */
private br_type figure_branch ( iaddr, targetp )
    Address iaddr;
    Address *targetp;
{
  long instr, annul, cond, op2, op3;
  long br_offset;	/* Must be *signed* for the sign-extend to work */
  br_type brt;

	dread( &instr, iaddr, sizeof instr );

	if( read_err_flag ) {
	    return br_error;
	}

	op2 = X_OP2(instr);
	op3 = X_OP3(instr);
	annul = X_ANNUL(instr);
	cond = X_COND(instr);

	brt = not_branch;
	*targetp = 0;

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

			br_offset = SR_WA2BA( SR_SEX223( X_DISP223(instr) ) );

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

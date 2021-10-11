static char sccs_id[] = "@(#)check_nodes.c	1.1\t10/31/94";
/*
 * This file contains code associated with:
 *	(a) checking SPARC code for contextual errors
 */
#ifdef CHIPFABS
/*	(b) workarounds for the various fabrication rounds of the
 *		Sunrise Integer Unit and Floating-Point Unit chips.
 */
#endif /*CHIPFABS*/

#include <stdio.h>
#include <sun4/asm_linkage.h>
#include "structs.h"
#include "errors.h"
#include "globals.h"
#include "node.h"
#include "opcode_ptrs.h"
#include "opt_branches.h"
#include "opt_utilities.h"
#include "registers.h"

#ifdef CHIPFABS
# include "sym.h"		/* for "opc_lookup()"	*/
# include "optimize.h"		/* for "OPT_NONE"	*/
# include "instructions.h"
#endif /* CHIPFABS*/

#define NOT_STACK_ALIGNED(X)  (X != SA(X))  /* is true if X is not a multiple
					       of STACK_ALIGN */

static Bool
node_is_in_executable_delay_slot(np)
	register Node *np;
{
	register Node *prev_np;

	/* Am in an executable delay slot if the previous instruction has a
	 * delay slot, that instruction is not an Unconditional Branch with
	 * Annul, and the one before it does NOT have a delay slot.
	 * (If the previous instruction has a delay slot but is already in a
	 * delay slot ITSELF, then it part of a CTI-pair and the following
	 * node (the one we're looking at) is NOT in a delay slot.
	 * Yes, it sounds complicated; read the Processor Architecture manual).
	 *
	 * Note that this could be considerably simplified if we assumed that
	 * we would never see CTI-pairs (adjacent CTI's), but since this part
	 * of the code is used to check hand-coded ass'y language (which may
	 * contain CTI-pairs) as well as compiler-generated code (which never
	 * contains CTI-pairs), we MUST check for them here.
	 */

	 /* Find the previous real "instruction" node (i.e. one that generates
	  * code).
	  * This means skipping back over all nodes which do not generate
	  * code EXCEPT for ".empty", which is significant here.
	  */
	prev_np = np;
	do 
	{
		prev_np = prev_np->n_prev;
	} while ( (!NODE_GENERATES_CODE(prev_np))  &&
		  (!NODE_IS_EMPTY_PSEUDO(prev_np)) &&
		  (!NODE_IS_MARKER_NODE(prev_np))
		);

	return ( (NODE_HAS_DELAY_SLOT(prev_np)) &&
		  !( /*(the instruction will NEVER be executed anyway)*/
		     NODE_IS_BRANCH(prev_np) &&
		     DELAY_SLOT_WILL_NEVER_BE_EXECUTED(prev_np)
		   ) &&
		 (!node_is_in_executable_delay_slot(prev_np))
	       );
}


static void
insert_a_NOP_before_node(np)
	register Node *np;
{
	register Node *nop_np;
#  ifdef DEBUG
	if(debug)fprintf(stderr,"[NOP auto-inserted before line# %d]\n",np->n_lineno);
#  endif

	/* create a node with a "nop" instruction */
	nop_np = new_node(np->n_lineno, opcode_ptr[OP_NOP]);
	nop_np->n_internal = TRUE;  /* this is an internally-generated node */

	/* ...and link the two nodes together. */
	insert_before_node(np, nop_np);
}


static void
insert_a_NOP_after_node(np)
	register Node *np;
{
	register Node *nop_np;
#  ifdef DEBUG
	if(debug)fprintf(stderr,"[NOP auto-inserted after line# %d]\n",np->n_lineno);
#  endif

	/* create a node with a "nop" instruction */
	nop_np = new_node(np->n_lineno, opcode_ptr[OP_NOP]);
	nop_np->n_internal = TRUE;  /* this is an internally-generated node */

	/* ...and link the two nodes together. */
	insert_after_node(np, nop_np);
}


#ifdef CHIPFABS
static Node *
create_FMOVS_node(lineno, regno)
	register LINENO lineno;
	register Regno regno;
{
	register Node *fmovs_np;

	/* create a node with a "fmovs" instruction from an FPreg to itself */
	fmovs_np = new_node(lineno, opcode_ptr[OP_FMOVS]);
	fmovs_np->n_operands.op_regs[O_RS2] = regno;
	fmovs_np->n_operands.op_regs[O_RD]  = regno;
	fmovs_np->n_internal = TRUE;  /* this is an internally-generated node */

	return fmovs_np;
}
static void
insert_an_FMOVS_after_node(np, regno)
	register Node *np;
	register Regno regno;
{
	register Node *fmovs_np;
#  ifdef DEBUG
	if(debug) fprintf(stderr, "[FMOVS auto-inserted after line# %d]\n",
				np->n_lineno);
#  endif

	/* create a node with a "fmovs" instruction from an FPreg to itself */
	fmovs_np = create_FMOVS_node(np->n_lineno, regno);

	/* ...and link the two nodes together. */
	insert_after_node(np, fmovs_np);
}
static void
insert_an_FMOVS_before_node(np, regno)
	register Node *np;
	register Regno regno;
{
	register Node *fmovs_np;
#  ifdef DEBUG
	if(debug) fprintf(stderr, "[FMOVS auto-inserted before line# %d]\n",
				np->n_lineno);
#  endif

	/* create a node with a "fmovs" instruction from an FPreg to itself */
	fmovs_np = create_FMOVS_node(np->n_lineno, regno);

	/* ...and link the two nodes together. */
	insert_before_node(np, fmovs_np);
}
#endif /* CHIPFABS*/


void
check_list_for_errors(marker_np)
	register Node *marker_np;
{
	register Node *np;
	register struct opcode *opcp;
	register Regno rd;
	register Node *next_np;

	/* "next_np" is used because after delete_node(p), "p" has been
	 * deallocated, and nobody really guarantees us that the contents of
	 * "p" (specifically, p->n_next) are still good.  (Yes, I know,
	 * practical experience says otherwise, but better safe now than sorry
	 * later!)
	 */

	/* do nothing if list is non-existant */
	if (marker_np == NULL)	return;

	for ( np = marker_np->n_next;
	      next_np = np->n_next, np != marker_np;
	      np = next_np
	    )
	{
		opcp = np->n_opcp;

		switch ( opcp->o_func.f_group )
		{
		case FG_LD:
			/* Only for LDFSR */
			if (!IS_FSR_REG(np->n_operands.op_regs[O_RD])) break;
			else
		        {
				register Node * next_instr;
				register int i, j;
				next_instr = next_code_generating_node(np);
#define LDFSR_DELAY  3
			
				for (i = 0; i < LDFSR_DELAY; i++)
			        {
					if ( (OP_FUNC(next_instr).f_group == FG_BR) &&
					     (OP_FUNC(next_instr).f_br_cc == CC_F) )
					{
#ifndef INSTRUCTION_SCHEDULER_GOT_SMART_ABOUT_FBCC
					/* Only warn about this if the peephole
					 * optimizer is turned off.  This is
					 * because the instruction scheduler
					 * was written before this became an
					 * architectural requirement :-(, and
					 * doesn't know that 3 instructions are
					 * required between LDFSR and FBCC's.
					 */
						if ( optimization_level == 0 )
#endif /*INSTRUCTION_SCHEDULER_GOT_SMART_ABOUT_FBCC*/
						{
							warning(WARN_LDFSR_BFCC,
								input_filename,
								np->n_lineno);
						}
						/* Insert the appropriate number of
						 * nops.
						 */
						for (j = i; j < LDFSR_DELAY; j++)
							insert_a_NOP_after_node(np);

						break;
					}
					/*  In this case we will just put one nop
					 *  just to play "safe".
					 *  We will have :
					 *  LDFSR
					 *  nop
					 *  branch
					 *  delay_slot  (can't be a Fbcc) 
					 *  we're safe whatever follows.
					 */					 
					else if ( (i == 0) &&
						  (OP_FUNC(next_instr).f_group == FG_BR) )
					{
						insert_a_NOP_after_node(np);
						break;
					}
					next_instr = next_code_generating_node(next_instr);
				}
			}
			break;

		case FG_BR:
			/* Following only applies to branches on FCC's. */
			if (opcp->o_func.f_br_cc != CC_F)	break;
			
			if ( node_is_in_executable_delay_slot(np) )
			{
				warning(WARN_DELAY_SLOT, input_filename,
					np->n_lineno, "FBfcc");
			}
			else
			{
				/* Check that the instruction preceeding the
				 * FBCC does not access the FPU, which would
				 * be architecturally invalid.
				 */
				register Node *prev_np;
				prev_np =previous_instruction_or_label_node(np);
				if (NODE_IS_LABEL(prev_np))
				{
					/* Save ourselves a lot of work and just
					 * insert a NOP.  Otherwise, we have to
					 * look at the previous instruction, as
					 * well as all of the delay slots of
					 * CTI's which target this label to see
					 * if any of THEM touch the FPU.
					 */
					insert_a_NOP_before_node(np);
				}
				else if (prev_np->n_opcp->o_func.f_accesses_fpu)
				{
					/* It's an FPU instruction -- a no-no
					 * right before a branch on Floating-
					 * Point condition codes!
					 */
#ifndef INSTRUCTION_SCHEDULER_GOT_SMART_ABOUT_FBCC
					/* Only warn about this if the peephole
					 * optimizer is turned off.  This is
					 * because the instruction scheduler
					 * was written before this became an
					 * architectural requirement :-(, and
					 * doesn't know to only put non-FP
					 * instructions before FBCC's.
					 */
					if ( optimization_level == 0 )
#endif /*INSTRUCTION_SCHEDULER_GOT_SMART_ABOUT_FBCC*/
					{
						warning(ERR_FPI_BFCC,
							input_filename,
							np->n_lineno);
					}
					insert_a_NOP_before_node(np);
				}
			}
			break;

		case FG_LABEL:
			if ( node_is_in_executable_delay_slot(np) )
			{
				warning(WARN_DELAY_SLOT, input_filename,
					np->n_lineno, "label");
			}
			break;

		case FG_SET:
			if ( node_is_in_executable_delay_slot(np) )
			{
				warning(WARN_DELAY_SLOT, input_filename,
				   np->n_lineno,
				   "2-word \"set\" pseudo-instruction");
			}
			break;

		case FG_PS:
			switch ( opcp->o_func.f_subgroup )
			{
			case FSG_BOF:
				input_filename = np->n_flp->fl_fnamep;
				break;
			}
			break;

		case FG_RETT:
			if ((np->n_prev->n_opcp->o_func.f_group != FG_JMP) &&
			    (np->n_prev->n_opcp->o_func.f_group != FG_JMPL))
			{
				error(ERR_RETT_NO_JMP, input_filename,
					np->n_lineno);
			}
			break;
		
		case FG_ARITH: 
			switch ( opcp->o_func.f_subgroup )
		        {
			case FSG_SAVE:
			        if (np->n_operands.op_imm_used &&
				    ((np->n_operands.op_symp == NULL &&
				     NOT_STACK_ALIGNED(np->n_operands.op_addend)) ||
				    (np->n_operands.op_symp != NULL && 
				     np->n_operands.op_symp->s_segment == ASSEG_ABS &&
				     NOT_STACK_ALIGNED(np->n_operands.op_addend + np->n_operands.op_symp->s_value))))
				{
				      warning(WARN_STACK_ALIG, input_filename,
					      np->n_lineno,STACK_ALIGN);
			        }
				break;
			}
			break;
		}
	}


	if ( node_is_in_executable_delay_slot(np) )
	{
		/* The user has terminated a segment in the assembly with a CTI
		 * which may execute its delay slot.
		 * This leaves the delay slot to be filled in by whatever
		 * instruction "happens" to be linked in after this module.
		 *
		 * This is dangerous and the user should be warned.
		 */

		warning(WARN_CTI_EOSEG, input_filename, np->n_prev->n_lineno);
	}
}


#ifdef CHIPFABS
void
setup_fab_opcodes()
{
	/* change opcode table back to old opcodes, for early chip fabs */

	if (iu_fabno == 1)
	{
		/*-------------------------------------------------*/
		/* Change opcodes from Fab-2 back to Fab-1 opcodes */
		/*-------------------------------------------------*/

		register struct opcode *opcp;

		opcode_ptr[OP_SETHI]->o_instr |= (0x02 << 22);
		opcode_ptr[OP_NOP]->o_instr   |= (0x02 << 22); /*also a SETHI*/
		opcode_ptr[OP_UNIMP]->o_instr = IF3cd1X(3,0x3f);

#define FIX_FBfcc(opcp)	((opcp)->o_instr &= ~(0x02 << 22)) /*fix op2*/
		FIX_FBfcc(opcode_ptr[OP_FBN]);
		FIX_FBfcc(opcode_ptr[OP_FBU]);
		FIX_FBfcc(opcode_ptr[OP_FBG]);
		FIX_FBfcc(opcode_ptr[OP_FBUG]);
		FIX_FBfcc(opcode_ptr[OP_FBL]);
		FIX_FBfcc(opcode_ptr[OP_FBUL]);
		FIX_FBfcc(opcode_ptr[OP_FBLG]);
		FIX_FBfcc(opcode_ptr[OP_FBNE]);
		FIX_FBfcc(opcode_ptr[OP_FBNZ]);
		FIX_FBfcc(opcode_ptr[OP_FBE]);
		FIX_FBfcc(opcode_ptr[OP_FBZ]);
		FIX_FBfcc(opcode_ptr[OP_FBUE]);
		FIX_FBfcc(opcode_ptr[OP_FBGE]);
		FIX_FBfcc(opcode_ptr[OP_FBUGE]);
		FIX_FBfcc(opcode_ptr[OP_FBLE]);
		FIX_FBfcc(opcode_ptr[OP_FBULE]);
		FIX_FBfcc(opcode_ptr[OP_FBO]);
		FIX_FBfcc(opcode_ptr[OP_FBA]);

		FIX_FBfcc(opc_lookup("fbn,a"));
		FIX_FBfcc(opc_lookup("fbu,a"));
		FIX_FBfcc(opc_lookup("fbg,a"));
		FIX_FBfcc(opc_lookup("fbug,a"));
		FIX_FBfcc(opc_lookup("fbl,a"));
		FIX_FBfcc(opc_lookup("fbul,a"));
		FIX_FBfcc(opc_lookup("fblg,a"));
		FIX_FBfcc(opc_lookup("fbne,a"));
		FIX_FBfcc(opc_lookup("fbnz,a"));
		FIX_FBfcc(opc_lookup("fbe,a"));
		FIX_FBfcc(opc_lookup("fbz,a"));
		FIX_FBfcc(opc_lookup("fbue,a"));
		FIX_FBfcc(opc_lookup("fbge,a"));
		FIX_FBfcc(opc_lookup("fbuge,a"));
		FIX_FBfcc(opc_lookup("fble,a"));
		FIX_FBfcc(opc_lookup("fbule,a"));
		FIX_FBfcc(opc_lookup("fbo,a"));
		FIX_FBfcc(opc_lookup("fba,a"));

#define FIX_FPop(name)						\
			( opcp = opc_lookup(name),		\
	  		  /* fix op */				\
	  		  opcp->o_instr &= ~(MASK(2)<<30),	\
	  		  opcp->o_instr |= (0x03 << 30),	\
	  		  /* fix op3 */				\
	  		  opcp->o_instr &= ~(MASK(6)<<19),	\
	  		  opcp->o_instr |= (0x28 << 19)		\
			)
		FIX_FPop("fitos");
		FIX_FPop("fitod");
		FIX_FPop("fitox");
		FIX_FPop("fstoi");
		FIX_FPop("fdtoi");
		FIX_FPop("fxtoi");
		FIX_FPop("fstod");
		FIX_FPop("fstox");
		FIX_FPop("fdtos");
		FIX_FPop("fdtox");
		FIX_FPop("fxtod");
		FIX_FPop("fxtos");

		FIX_FPop("fmovs");
		FIX_FPop("fnegs");
		FIX_FPop("fabss");
		FIX_FPop("fsqrts");
		FIX_FPop("fsqrtd");
		FIX_FPop("fsqrtx");

		FIX_FPop("fadds");
		FIX_FPop("faddd");
		FIX_FPop("faddx");
		FIX_FPop("fsubs");
		FIX_FPop("fsubd");
		FIX_FPop("fsubx");
		FIX_FPop("fmuls");
		FIX_FPop("fmuld");
		FIX_FPop("fmulx");
		FIX_FPop("fdivs");
		FIX_FPop("fdivd");
		FIX_FPop("fdivx");

#define FIX_FPCMP(name)	( opcp = opc_lookup(name),		\
			  opcp->o_instr &= ~(MASK(6)<<19),	\
			  opcp->o_instr |= (0x29 << 19),	\
			  opcp->o_instr |= (MASK(2)<<30)	\
			)
		FIX_FPCMP("fcmps");
		FIX_FPCMP("fcmpd");
		FIX_FPCMP("fcmpx");
		FIX_FPCMP("fcmpes");
		FIX_FPCMP("fcmped");
		FIX_FPCMP("fcmpex");
	}


	if (fpu_fabno == 1)
	{
		/* Re-map some opcodes, so that they can be emulated via
		 * traps, instead of executed.
		 */
#define REMAP_OPC(from,to)  opc_lookup(from)->o_instr = opc_lookup(to)->o_instr
		/** REMAP_OPC("fitos",  "fscales"); **/
		/** REMAP_OPC("fitod",  "fscaled"); **/
		/** REMAP_OPC("fstod",  "fexpos");  **/
		/** REMAP_OPC("fstoi",  "fclasss"); **/
	}
}
#endif /*CHIPFABS*/


#ifdef CHIPFABS
static Node *
perform_fpu_fab1_workarounds(np)
	register Node *np;	/* instruction node pointer */
{
	if ( node_is_in_executable_delay_slot(np) )
	{
		warning(WARN_DELAY_SLOT, input_filename, np->n_lineno,
			"FPU Instruction");
	}

	/* No actual fab-1 FPU workarounds to do here, at the moment. */
	return np->n_next;
}
#endif /*CHIPFABS*/


#ifdef CHIPFABS
static Node *
perform_fpu_fab2_workarounds(i1np)
	Node *i1np;		/* instruction 1 node pointer */
{
	register Node *fmuld_np;/* instruction 1 node ptr, known to be FMULD. */
	register Node *i2np;	/* instruction 2 node pointer */
	register Regno fmuld_rd;
	register Regno i2rs1;
	register Bool continue_looping;

			
	if ( ! ( i1np->n_opcp->o_func.f_fp_1164 &&
		 (i1np->n_opcp->o_func.f_rd_width == 8) )
	   )
	{
		/* No fix will be necessary after this instruction;
		 * it's not an FMULD.  Only FMULD's require the fix.
		 */
		return i1np->n_next;
	}
	else	fmuld_np = i1np;

	fmuld_rd = fmuld_np->n_operands.op_regs[O_RD];

	/* If the FMULD is in an excuatble delay slot, then we have to take
	 * more extraordinary measures to install a workaround.
	 *
	 * In the case of a call/jmp/ret's delay slot:
	 *
	 *	Change:				To:
	 *	-------------------------	--------------------
	 *					fmuld   --,--,REG
	 *	call/jmp/ret	--		call/jmp/ret    --
	 *	fmuld	--,--,REG		fmovs   REG,REG
	 *
	 * Here are the cases for being in the delay slot of a Branch:
	 *
	 *				put FMOVS	put FMOVS
	 *				after LABEL:?	after FMULD instr?
	 *				-------------	------------------
	 *	b<cc>	LABEL
	 *	fmuld	--,--,--	yes		yes
	 *
	 *	b<cc>,a	LABEL
	 *	fmuld	--,--,--	yes		no
	 *
	 *	ba	LABEL
	 *	fmuld	--,--,--	yes		no
	 *
	 *	ba,a	LABEL		(FMULD is not in an EXECUTABLE delay
	 *	fmuld	--,--,--	 slot; this will be handled below as
	 *				 a "normal" case)
	 */
	if ( node_is_in_executable_delay_slot(fmuld_np) )
	{
		register Node *cti_np;
		register Node *after_delay_slot_np;

		/* Get a pointer to the Control Transfer Instruction's node. */
		cti_np = previous_code_generating_node(fmuld_np);

		after_delay_slot_np = fmuld_np->n_next;

		/* Now create the proper workaround. */
		switch ( cti_np->n_opcp->o_func.f_group )
		{
		case FG_CALL:
		case FG_JMP:
		case FG_RET:
		case FG_RETT:
			/* move the FMULD from the delay slot to before the
			 * CTI.
			 */
			make_orphan_node(fmuld_np);
			insert_before_node(cti_np, fmuld_np);
			/* now put an appropriate FMOVS where the FMULD was. */
			insert_an_FMOVS_before_node(after_delay_slot_np,
							fmuld_rd);
			return after_delay_slot_np;	/* all done! */
		case FG_BR:
			/* Note: the "ba,a" case will never reach here, as the
			 * FMULD will not be considered as "in an EXECUTABLE
			 * delay slot" in that case.
			 */
			/* In all 3 other cases, put an FMOVS after the
			 * target label of the branch.
			 */
			insert_an_FMOVS_after_node(
						get_branch_target_np(cti_np),
						fmuld_rd);
			/* And, if it's a Conditional Branch WITHOUT Annul,
			 * add an FMOVS after the FMULD.
			 */
			if ( BRANCH_IS_CONDITIONAL(cti_np) &&
			     BRANCH_IS_NOT_ANNULLED(cti_np) )
			{
				insert_an_FMOVS_after_node(fmuld_np, fmuld_rd);
			}
			return after_delay_slot_np;	/* all done! */
		default:
			internal_error("perform_fpu_fab2_workarounds(): fg=%d?",
					cti_np->n_opcp->o_func.f_group);
		}
	}

	/* Starting with the current floating-point instruction (an FMULD,
	 * executes on the Weitek 1164), find the next F.P. instruction.
	 * If it:
	 *	a)  executes on the opposite Weitek chip (1165), and
	 *	b)  uses the result (RD) of the current instruction as its
	 *		FIRST operand (RS1),
	 * then we have a fab-2 FPU bug which needs to be worked around.
	 * We also need to play it safe and insert a workaround if we encounter
	 * any Control Transfer Instruction.
	 *
	 * The fix is to do one of the following, in order of preference:
	 *	a)  reverse operands 1 and 2 of the the second floating-point
	 *		instruction (which can only be used if that instruction
	 *		is commutative, and obviously can't be used at all in
	 *		the case of a CTI), or
	 *	b) insert an FMOVS instruction before the 2nd F.P. instruction,
	 *		which moves the result of that instruction to itself.
	 */
	for ( i2np = fmuld_np->n_next, continue_looping = TRUE;
	      continue_looping && !(NODE_IS_MARKER_NODE(i2np));
	      i2np = i2np->n_next)
	{
		switch ( i2np->n_opcp->o_func.f_group )
		{
		case FG_BR:
		case FG_CALL:
		case FG_JMP:
		case FG_RET:
		case FG_RETT:
		case FG_TRAP:
			/* If we hit a control transfer, then we always have
			 * to play it safe and insert the "fix" instruction.
			 */
			insert_an_FMOVS_before_node(i2np, fmuld_rd);
			continue_looping = FALSE;
			break;

		case FG_FLOAT2:
		case FG_FLOAT3:
			/* If we hit another Floating-point instruction, we can
			 * see if there is really a problem or not.  If not,
			 * then don't insert the "fix" instruction.  And if so,
			 * try to apply an alternate fix before inserting an
			 * extra instruction.
			 */
			if ( i2np->n_opcp->o_func.f_fp_1165
				&&
		     	     ( fmuld_rd == (i2rs1=i2np->n_operands.op_regs[O_RS1]) )
				&&
			     ( i2rs1 != i2np->n_operands.op_regs[O_RS2] )
			   )
			{
				/* OK, we have the problem and must apply some
				 * type of workaround fix.
				 * If the second instruction is commutative,
				 * then swapping its operands will be a fix
				 * (we already ascertained that they are
				 *  different registers).
				 * If not, then insert the "fix" instruction.
				 */
				if ( i2np->n_opcp->o_func.f_ar_commutative )
				{
					/* swap the two source operands */
					i2np->n_operands.op_regs[O_RS1] =
						i2np->n_operands.op_regs[O_RS2];
					i2np->n_operands.op_regs[O_RS2] = i2rs1;
#ifdef DEBUG
					if(debug) fprintf(stderr,
					   "[RS1/RS2 auto-swapped @line# %d]\n",
					   i2np->n_lineno);
#endif
				}
				else	insert_an_FMOVS_before_node(i2np, fmuld_rd);

				/* We're done for now. */
				continue_looping = FALSE;
			}
			else if ( i2np->n_opcp->o_func.f_fp_1164 ||
				  i2np->n_opcp->o_func.f_fp_1165 )
			{
				/* The second instruction was a Weitek-touching
				 * F.P instruction (as opposed to an FMOV, FABS,
				 * or FNEG).
				 * There is no problem BETWEEN these two F.P.
				 * instructions.  There may be one between the
				 * second F.P. instruction and the one after
				 * it, but that will be checked the next time
				 * this routine is called.
				 * So, we're done for now.
				 */
				continue_looping = FALSE;
			}

			break;
		
		default:
			/* continue looking... */
			break;
		}
	}

	return fmuld_np->n_next;
}
#endif /*CHIPFABS*/

#ifdef CHIPFABS
static Node *
perform_fpu_fab1234_workarounds(i1np)
       register Node *i1np;
{
       register Node *ctinp;

	/* This is a workaround for an FPU.  The bug will be fixed in
	 * fab-5.  We need to insert an
	 * FMOVS after each F<divide> instruction.  There are 3 cases:
	 *	
	 *	current code				patched code
	 *    ------------------------------------------------------------
	 * case1:
	 *    non-cti instr (control transfer instr)	non-cti instr
	 *    fdiv					fdiv
	 *    						fmovs
	 *
	 * case2:
	 *    b<cc>,a  LABEL				b<cc>,a   LABEL
	 *    fdiv					fdiv
	 *						...
	 *					LABEL:
	 *						fmovs
	 *
	 *  Note: we do not need to worry about the unconditional branch because
	 *  the annul bit acts reverse for that case.
	 *
	 * case3:
	 *    cti (control transfer instr)		fdiv
	 *    fdiv					cti
	 *						fmovs
	 *						
	 */
       if (OP_FUNC(i1np).f_subgroup == FSG_FDIV){
	       ctinp = i1np->n_prev;
		/* CASE 1 */
		if ( !NODE_HAS_DELAY_SLOT(ctinp) )  /* Not really a cti node */
			insert_an_FMOVS_after_node(i1np,
						   i1np->n_operands.op_regs[O_RD]);
		/* CASE 2 */
		else if (NODE_IS_BRANCH(ctinp)  && 
	  		 BRANCH_IS_ANNULLED(ctinp)) {
			 if (BRANCH_IS_CONDITIONAL(ctinp) && 
			     (OP_FUNC(ctinp).f_subgroup != FSG_N))
				 insert_an_FMOVS_after_node(
						get_branch_target_np(ctinp),
						i1np->n_operands.op_regs[O_RD]);
		/* CASE 3 */
		} else {
			make_orphan_node(i1np);
			insert_before_node(ctinp, i1np);
			insert_an_FMOVS_after_node(ctinp, i1np->n_operands.op_regs[O_RD]);
		}
	}
        if ( (OP_FUNC(i1np).f_subgroup == FSG_FSQT) ||
	     ( (OP_FUNC(i1np).f_rs1_width == 16) ||
	       (OP_FUNC(i1np).f_rd_width == 16)  &&
	       !( (OP_FUNC(i1np).f_subgroup == FSG_FMOV) ||   /* Not really xtended */
                  (OP_FUNC(i1np).f_subgroup == FSG_FABS) ||    /* instr. Just pseudo */
                  (OP_FUNC(i1np).f_subgroup == FSG_FNEG) ) ) )
	{
		register Node *st_fsr_np;
		/* create a "st   %fsr,[%sp-4]" node... */
		st_fsr_np = new_node(i1np->n_lineno, opcode_ptr[OP_STFSR]);
		st_fsr_np->n_internal = TRUE;
		st_fsr_np->n_operands.op_regs[O_RD] = RN_FSR;
		st_fsr_np->n_operands.op_regs[O_RS1] = RN_SP;
		st_fsr_np->n_operands.op_reltype = REL_13;
		st_fsr_np->n_operands.op_addend = -4;
		st_fsr_np->n_operands.op_imm_used = TRUE;
		
		/* ...and link the two nodes together. */
		if (node_is_in_executable_delay_slot(i1np)) 
		        insert_before_node(i1np->n_prev, st_fsr_np);
		else    insert_before_node(i1np, st_fsr_np);
	}
			
        return i1np;
        

}
#endif /* CHIPFABS */


#ifdef CHIPFABS
void
perform_fab_workarounds(marker_np)
	register Node *marker_np;
{
	register Node *np;
	register struct opcode *opcp;
	register Node *next_np;
	register Regno rd;

	/* do nothing if list is non-existant */
	if (marker_np == NULL)	return;

	for (np = marker_np->n_next;   np != marker_np;   np = next_np)
	{
		opcp = np->n_opcp;
		next_np = np->n_next;

		switch ( opcp->o_func.f_group )
		{
		case FG_LD:
			/* this is only for the first-fab IU chips ... */
			if ( iu_fabno != 1 )	break;

			/* put a NOP after LD's when the next instruction
			 * depends on the result of the LD.
			 */
			rd = np->n_operands.op_regs[O_RD];

			if ( node_is_in_executable_delay_slot(np) )
			{
				/*** THIS SHOULD BE MADE "PROPER" (SMART)! ***/
				warning(WARN_DELAY_SLOT, input_filename,
					np->n_lineno, "LD");
			}
			else
			{
				/* If the next instruction depends on the result
			 	 * of the LD, then insert a NOP between them.
				 */
				next_np = next_instruction(np);

				if ( instr1_depends_on_instr2(next_np, np) )
				{
					/* It depends on the LD's result;
					 * add a NOP.
					 */
					insert_a_NOP_after_node(np);
				}
				else
				{
					/* The next instruction doesn't depend
					 * on the result of the LD, so don't
					 * add a NOP.
					 */
				}
			}
			break;

		case FG_FLOAT2:
			if ( opcp->o_func.f_subgroup == FSG_FCMP )
			{
				if (iu_fabno == 1)
				{
				/* for the 1st-fab IU (yes, IU, not FPU) chips:
				 * Check that there are 2 instructions which
				 * don't touch the FPU, after every FCMP.
				 * If not, insert NOP's to make sure there are.
				 *
				 * This should even work in the presence of a
				 * CTI, since it and its delay slot instruction
				 * will STILL be the next two instructions
				 * physically present.
				 */

					if ( node_is_in_executable_delay_slot(np) )
				        {
						warning(WARN_DELAY_SLOT, input_filename,
							np->n_lineno, "FCMP");
					}
					else
					{
						register Node *n2p, *n3p;
						n2p = next_instruction(np);
						n3p = next_instruction(n2p);
						if (NODE_ACCESSES_FPU(n2p))
					        {
							insert_a_NOP_after_node(np);
							insert_a_NOP_after_node(np);
						}
						else if (NODE_ACCESSES_FPU(n3p))
					        {
							insert_a_NOP_after_node(np);
						}
					}
				}
		       
				else if ( (iu_fabno == 2) && (fpu_fabno == 1) )
			        {
				/* For IU-2/FPU-1 combo, duplicate every FCMP.*/

					register Node *fcmp_np;

					/* create a node with an "FCMP" instruction */
					fcmp_np = new_node(np->n_lineno, np->n_opcp);
					copy_node(np, fcmp_np, OPT_NONE);
					fcmp_np->n_internal = TRUE;
					
					/* ...and link the two nodes together. */
					insert_before_node(np, fcmp_np);
				}
		       
				/*  For FPU-fab[1-4] we need 2 non-fpu instructions
				 * between and EXTENDED compare and a FBcc.
				 */
				if ( (fpu_fabno <= 4) &&
				     (opcp->o_func.f_rs1_width == 16) )
			        {
					if ( node_is_in_executable_delay_slot(np) )
				        {
						warning(WARN_DELAY_SLOT, input_filename,
							np->n_lineno, "FCMP");
					}
					else
				        {
						register Node *n2p, *n3p;
						n2p = next_instruction(np);
						n3p = next_instruction(n2p);
						if (NODE_IS_BRANCH(n2p) &&
						    (n2p->n_opcp->o_func.f_br_cc == CC_F))
					        { 
							insert_a_NOP_after_node(np);
							insert_a_NOP_after_node(np);
						}
						else if (NODE_IS_BRANCH(n3p) &&
							 (n3p->n_opcp->o_func.f_br_cc == CC_F))
							insert_a_NOP_after_node(np);
					}
				}
			}
			/* fall into FLOAT3 case. */

		case FG_FLOAT3:
			/* fallen into from FLOAT2 case. */
			switch (fpu_fabno)
			{
			case 1:
			        np = perform_fpu_fab1234_workarounds(np);
			        next_np = perform_fpu_fab1_workarounds(np);
				break;
			case 2:
			        np = perform_fpu_fab1234_workarounds(np);
				next_np = perform_fpu_fab2_workarounds(np);
				break;
 		        case 3:
			case 4:
			        np = perform_fpu_fab1234_workarounds(np);
				next_np = np->n_next;
			default:
				/* do nothing. */
				break;
			}
			break;

		case FG_PS:
			switch ( opcp->o_func.f_subgroup )
			{
			case FSG_BOF:
				input_filename = np->n_flp->fl_fnamep;
				break;
			}
			break;
		}
	}
}
#endif /*CHIPFABS*/

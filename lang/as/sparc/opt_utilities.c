static char sccs_id[] = "@(#)opt_utilities.c	1.1\t10/31/94";
/*
 * This file contains code which performs utility functions for SPARC
 * code optimizations.
 */

#include <stdio.h>
#include "structs.h"
#include "sym.h"	/* for opc_lookup() */
#include "bitmacros.h"
#include "node.h"
#include "opcode_ptrs.h"
#include "optimize.h"
#include "opt_utilities.h"
#include "registers.h"
#include "opt_regsets.h"


Bool
is_jmp_at_top_of_cswitch(np)
	register Node *np;
{
	/* (ASSUME structure of a C-Switch here).
	 *
	 * This node is a JMP.  If it is one at the head of a C switch table
	 * (after we have internally converted them from ".word" to "*.cswitch"
	 *  pseudos) then return TRUE, else return FALSE.
	 * A C switch table looks like:
	 *	jmp	<REG>		<-- np
	 *	<instr>			<-- np->n_next  (delay slot)
	 *  <Label>:			<-- n3p
	 *	*.cswitch <val>		<-- n4p
	 *	...
	 */
	register Node *n3p, *n4p;

	if (np->n_opcp->o_func.f_group != FG_JMP)
	{
		return FALSE;
	}
	else
	{
		n3p = np->n_next->n_next;
		n4p = n3p->n_next;

		return ( (n3p->n_opcp->o_func.f_group == FG_LABEL) &&
			 (n4p->n_opcp->o_func.f_group == FG_CSWITCH)
		       );
	}
	/*NOTREACHED*/
}


/* Find the previous node which generates code.
 * This skips back over labels and non-code-generating pseudo-ops.
 */
Node *
previous_code_generating_node(np)
	register Node *np;
{
	do
	{ 
		np = np->n_prev;
	} while ( (!NODE_GENERATES_CODE(np)) &&
		  (!NODE_IS_MARKER_NODE(np))
		);
	
	return np;
}


/* Find the previous node which either is a label or generates code.
 */
Node *
previous_code_generating_node_or_label(np)
	register Node *np;
{
	do
	{ 
		np = np->n_prev;
	} while ( (!NODE_IS_LABEL(np)) &&
		  (!NODE_GENERATES_CODE(np)) &&
		  (!NODE_IS_MARKER_NODE(np))
		);
	
	return np;
}


/* Find the next node which generates code.
 * This skips forward over labels and non-code-generating pseudo-ops.
 */
Node *
next_code_generating_node(np)
	register Node *np;
{
	do
	{ 
		np = np->n_next;
	} while ( (!NODE_GENERATES_CODE(np)) &&
		  (!NODE_IS_MARKER_NODE(np))
		);
	
	return np;
}


/* Find the next node which either is a label or generates code.
 */
Node *
next_code_generating_node_or_label(np)
	register Node *np;
{
	do
	{ 
		np = np->n_next;
	} while ( (!NODE_IS_LABEL(np)) &&
		  (!NODE_GENERATES_CODE(np)) &&
		  (!NODE_IS_MARKER_NODE(np))
		);
	
	return np;
}

/* next_instruction() returns a pointer to the next instruction node.
 * This is the same as next_code_generating_node(), EXCEPT that it skips over
 * CSWITCH nodes.
 */
Node *
next_instruction(np)
	register Node *np;
{
	do
	{
		np = np->n_next;
	} while ( ( (!NODE_GENERATES_CODE(np) || NODE_IS_CSWITCH(np)) ) &&
		  (!NODE_IS_MARKER_NODE(np))
	      );
	return np;
}


/* previous_instruction_or_label_node() returns a pointer to the previous
 * instruction or label node.
 */
Node *
previous_instruction_or_label_node(np)
	register Node *np;
{
	do
	{
		np = np->n_prev;
	} while ( (!NODE_IS_LABEL(np)) &&
		  ( (!NODE_GENERATES_CODE(np) || NODE_IS_CSWITCH(np)) ) &&
		  (!NODE_IS_MARKER_NODE(np))
	      );
	return np;
}


/* next_non_directive() returns a pointer to the next non-pseudo-op node
 * (e.g. instruction, code-generating pseudo-op/pseudo-instruction, or label)
 */
Node *
next_non_directive(np)
	register Node *np;
{
	do
	{
		np = np->n_next;
	} while ( NODE_IS_PSEUDO(np) && !NODE_GENERATES_CODE(np) );
	/* ("NODE_IS_PSEUDO(np)" also implies that np is not the Marker node) */

	return np;
}


Bool
opt_node_is_in_delay_slot(np)
	register Node *np;
{
	register Node *prev_np;

#ifdef DEBUG
	if (!EMPTY_nodes_were_deleted)
	{
		internal_error("opt_node_is_in_delay_slot(): \".empty\" nodes not yet deleted");
		/*NOTREACHED*/
	}
#endif /*DEBUG*/

	/* Am in a delay slot if the previous instruction has a delay slot.
	 * We can use a much simpler test here than in check_nodes.c because
	 * we assume we can ignore:
	 *	-- CTI-pairs (since we only optimize COMPILER-generated code,
	 *		which never contains them)
	 *	-- ".empty" pseudos (which are supposed to deleted before
	 *		optimization is attempted)
	 */

	 /* Find the previous real "instruction" node (i.e. one that generates
	  * code).
	  */
	prev_np = previous_code_generating_node(np);

	return NODE_HAS_DELAY_SLOT(prev_np);
}


Bool
is_an_unconditional_control_transfer(np)
	register Node *np;
{
	switch ( np->n_opcp->o_func.f_group )
	{
	case FG_RET:
	case FG_JMP:
		return TRUE;
	case FG_BR:
		return BRANCH_IS_UNCONDITIONAL(np);

	default:	/* includes CALL, JMPL (indirect calls), and TRAP */
		return FALSE;
	}
	/*NOTREACHED*/
}


/* can_fall_into() returns TRUE if its argument points to a node into which
 * execution can "fall through" from the previous instruction.
 * It will return FALSE if the previous instruction
 *	(a) is in the delay slot of a Return, Jump, or UnConditional Branch
 *	(b) is a code-emitting pseudo-op (e.g. .byte, .half, .word, .skip)
 * Otherwise, it returns TRUE (this also covers the case of the given node
 * itself being in a delay slot).
 */
Bool
can_fall_into(np)
	register Node *np;
{
	register Node *prev_np;

	/* Find the previous code-generating node, or label. */
	prev_np = previous_code_generating_node_or_label(np);

	if ( NODE_IS_LABEL(prev_np) )
	{
		/* Then we can fall into this node by a branch to the
		 * preceeding label.
		 */
		return TRUE;
	}
	else
	{
		/* The previous node of interest was a code-generating node,
		 * instead of a label.
		 */
		register Node *prev_prev_np;
		prev_prev_np = previous_code_generating_node(prev_np);

		/* Check it out. */
		if (prev_prev_np->n_opcp->o_func.f_has_delay_slot)
		{
			/* Here, the previous instruction was in a delay slot.
			 * If it was the delay slot of an Conditional control
			 * transfer, then we can fall thru, otherwise, not.
			 */
			if (is_an_unconditional_control_transfer(prev_prev_np))
			{
				return FALSE;
			}
			else	return TRUE;
		}
		else
		{
			/* Here, the previous instruction was NOT in a delay
			 * slot (and therefore does not follow a control
			 * transfer).
			 */
			if ( NODE_IS_PSEUDO(prev_np) &&
			     NODE_GENERATES_CODE(prev_np) )
			{
				/* cannot fall into from "prev_np" */
				return FALSE;
			}
			else if ( NODE_IS_MARKER_NODE(prev_np))	return FALSE;
			else if ( NODE_IS_CSWITCH(prev_np) )	return FALSE;
			else
			{
				/* (probably) can fall into from "prev_np". */
				return TRUE;
			}
		}
	}

	/*NOTREACHED*/
}


void
append_block_after_node(head_np, tail_np, np)
	register Node *head_np;
	register Node *tail_np;
	register Node *np;
{
	/* "head_np" and "tail_np" delimit an orphan block of doubly-linked
	 * nodes.  Insert the whole block just after "np" in another list.
	 */
	head_np->n_prev    = np;
	tail_np->n_next    = np->n_next;
	np->n_next->n_prev = tail_np;
	np->n_next         = head_np;
}


Node *
new_optimizer_NOP_node(lineno, optno)
	register LINENO lineno;
	register OptimizationNumber optno;
{
	register Node *nop_np; 

	nop_np = opt_new_node(lineno, opcode_ptr[OP_NOP], optno);
	nop_np->n_internal = TRUE;

	return nop_np;
}


struct opcode *
reverse_condition_opcode(opcp)
	register struct opcode *opcp;
{
	if (opcp->o_func.f_br_annul)
	{
		switch (opcp->o_func.f_subgroup)
		{
		case FSG_N:		return opc_lookup("ba,a");
		case FSG_A:		return opc_lookup("bn,a");

		case FSG_E:		return opc_lookup("bne,a");
		case FSG_LE:		return opc_lookup("bg,a");
		case FSG_L:		return opc_lookup("bge,a");
		case FSG_LEU:		return opc_lookup("bgu,a");
		case FSG_CS:		return opc_lookup("bcc,a");
		case FSG_NEG:		return opc_lookup("bpos,a");
		case FSG_VS:		return opc_lookup("bvc,a");
		case FSG_NE:		return opc_lookup("be,a");
		case FSG_G:		return opc_lookup("ble,a");
		case FSG_GE:		return opc_lookup("bl,a");
		case FSG_GU:		return opc_lookup("bleu,a");
		case FSG_CC:		return opc_lookup("bcs,a");
		case FSG_POS:		return opc_lookup("bneg,a");
		case FSG_VC:		return opc_lookup("bvs,a");

		case FSG_FU:		return opc_lookup("fbo,a");
		case FSG_FG:		return opc_lookup("fbule,a");
		case FSG_FUG:		return opc_lookup("fble,a");
		case FSG_FL:		return opc_lookup("fbuge,a");
		case FSG_FUL:		return opc_lookup("fbge,a");
		case FSG_FLG:		return opc_lookup("fbue,a");
		case FSG_FNE:		return opc_lookup("fbe,a");
		case FSG_FE:		return opc_lookup("fbne,a");
		case FSG_FUE:		return opc_lookup("fblg,a");
		case FSG_FGE:		return opc_lookup("fbul,a");
		case FSG_FUGE:		return opc_lookup("fbl,a");
		case FSG_FLE:		return opc_lookup("fbug,a");
		case FSG_FULE:		return opc_lookup("fbg,a");
		case FSG_FO:		return opc_lookup("fbu,a");

		case FSG_C3:		return opc_lookup("cb012,a");
		case FSG_C2:		return opc_lookup("cb013,a");
		case FSG_C23:		return opc_lookup("cb01,a");
		case FSG_C1:		return opc_lookup("cb023,a");
		case FSG_C13:		return opc_lookup("cb02,a");
		case FSG_C12:		return opc_lookup("cb03,a");
		case FSG_C123:		return opc_lookup("cb0,a");
		case FSG_C0:		return opc_lookup("cb123,a");
		case FSG_C03:		return opc_lookup("cb12,a");
		case FSG_C02:		return opc_lookup("cb13,a");
		case FSG_C023:		return opc_lookup("cb1,a");
		case FSG_C01:		return opc_lookup("cb23,a");
		case FSG_C013:		return opc_lookup("cb2,a");
		case FSG_C012:		return opc_lookup("cb3,a");

		default:		return (struct opcode *)NULL;
		}
	}
	else
	{
		switch (opcp->o_func.f_subgroup)
		{
		case FSG_N:		return opcode_ptr[OP_BA];
		case FSG_A:		return opcode_ptr[OP_BN];

		case FSG_E:		return opcode_ptr[OP_BNE];
		case FSG_LE:		return opcode_ptr[OP_BG];
		case FSG_L:		return opcode_ptr[OP_BGE];
		case FSG_LEU:		return opcode_ptr[OP_BGU];
		case FSG_CS:		return opcode_ptr[OP_BCC];
		case FSG_NEG:		return opcode_ptr[OP_BPOS];
		case FSG_VS:		return opcode_ptr[OP_BVC];
		case FSG_NE:		return opcode_ptr[OP_BE];
		case FSG_G:		return opcode_ptr[OP_BLE];
		case FSG_GE:		return opcode_ptr[OP_BL];
		case FSG_GU:		return opcode_ptr[OP_BLEU];
		case FSG_CC:		return opcode_ptr[OP_BCS];
		case FSG_POS:		return opcode_ptr[OP_BNEG];
		case FSG_VC:		return opcode_ptr[OP_BVS];

		case FSG_FU:		return opcode_ptr[OP_FBO];
		case FSG_FG:		return opcode_ptr[OP_FBULE];
		case FSG_FUG:		return opcode_ptr[OP_FBLE];
		case FSG_FL:		return opcode_ptr[OP_FBUGE];
		case FSG_FUL:		return opcode_ptr[OP_FBGE];
		case FSG_FLG:		return opcode_ptr[OP_FBUE];
		case FSG_FNE:		return opcode_ptr[OP_FBE];
		case FSG_FE:		return opcode_ptr[OP_FBNE];
		case FSG_FUE:		return opcode_ptr[OP_FBLG];
		case FSG_FGE:		return opcode_ptr[OP_FBUL];
		case FSG_FUGE:		return opcode_ptr[OP_FBL];
		case FSG_FLE:		return opcode_ptr[OP_FBUG];
		case FSG_FULE:		return opcode_ptr[OP_FBG];
		case FSG_FO:		return opcode_ptr[OP_FBU];

		case FSG_C3:		return opcode_ptr[OP_CB012];
		case FSG_C2:		return opcode_ptr[OP_CB013];
		case FSG_C23:		return opcode_ptr[OP_CB01];
		case FSG_C1:		return opcode_ptr[OP_CB023];
		case FSG_C13:		return opcode_ptr[OP_CB02];
		case FSG_C12:		return opcode_ptr[OP_CB03];
		case FSG_C123:		return opcode_ptr[OP_CB0];
		case FSG_C0:		return opcode_ptr[OP_CB123];
		case FSG_C03:		return opcode_ptr[OP_CB12];
		case FSG_C02:		return opcode_ptr[OP_CB13];
		case FSG_C023:		return opcode_ptr[OP_CB1];
		case FSG_C01:		return opcode_ptr[OP_CB23];
		case FSG_C013:		return opcode_ptr[OP_CB2];
		case FSG_C012:		return opcode_ptr[OP_CB3];

		default:		return (struct opcode *)NULL;
		}
	}
	/*NOTREACHED*/
}


void
change_branch_target(branch_np, symp)
	register Node *branch_np;
	register struct symbol *symp;
{
#ifdef DEBUG
	if ( ! NODE_IS_BRANCH(branch_np) )
	{
		internal_error("change_branch_target(): not a BR!");
	}
#endif /*DEBUG*/

	if (branch_np->n_operands.op_symp == symp)
	{
		/* The branch instruction is ALREADY the one we are asking
		 * for it to be changed to, so save ourselves some work and
		 * don't do anything here.
		 *
		 * Also, if the branch instruction was the only reference to
		 * the target label and we executed the code below, we would
		 * produce a minor disaster: delete_node_reference_to_label()
		 * would cause the target label and its defining node to be
		 * DELETED, then we would continue to try to reference both
		 * of the deleted (as_free()'d) structures (symbol and node).
		 */
	}
	else
	{
		/* Delete the branch instruction's reference to the
		 * OLD target label.
		 */
		delete_node_reference_to_label(branch_np,
					       branch_np->n_operands.op_symp);

		/* Retarget the instruction to the new label. */
		branch_np->n_operands.op_symp = symp;

		/* Put it on the list of nodes which reference that label. */
		node_references_label(branch_np, symp);
	}
}


struct opcode *
branch_with_toggled_annul_opcp(opcp)
	register struct opcode *opcp;
{
	/* This function returns a pointer to a branch opcode, which is the
	 * same as the given branch opcode except that it is one with the
	 * Annul bit toggled.
	 */
	
	char annul_opcode_name[16];

	strcpy(annul_opcode_name, opcp->o_name);
	if (opcp->o_func.f_br_annul)
	{
		/* zap the ",a" on the end */
		annul_opcode_name[strlen(annul_opcode_name)-2] = '\0';
	}
	else	strcat(annul_opcode_name, ",a");
	return opc_lookup(annul_opcode_name);
}


Bool
node_depends_on_register(np, regno)
	register Node *np;
	register Regno regno;
{
	/* Returns TRUE if node "np" depends on (uses as a source operand
	 * the contents of) register "regno".
	 */
	return ( (regno != RN_GP(0))
			&&
		 (np->n_opcp->o_func.f_node_union == NU_OPS)
			&&
	 	 /* The test for "generates code" has to be here so that
		  * pseudo-op nodes (e.g. ".noalias") which use the "operand"
		  * structure aren't construed as "dependent" on the contents
		  * of the register(s) they reference.
		  */
		 (np->n_opcp->o_func.f_generates_code)
			&&
		 ( (np->n_operands.op_regs[O_RS1] == regno) ||
		   (np->n_operands.op_regs[O_RS2] == regno) ||
		   ( (np->n_opcp->o_func.f_group == FG_ST) &&
		     (np->n_operands.op_regs[O_RD] == regno) )
		 )
	       );
}


Bool
instr1_depends_on_instr2(instr1_np, instr2_np)
	register Node *instr1_np;
	register Node *instr2_np;
{
	/* Returns TRUE if any registers or condition codes which are set
	 * (written) by the "instr2_np" node are referenced (read) by the
	 * "instr1_np" node, i.e. "instr1_np" depends on the action/results
	 * of "instr2_np".
	 */

	return ( !regset_empty( regset_intersect(REGS_READ(instr1_np),
						 REGS_WRITTEN(instr2_np)) ) );
}


Bool
instrs_are_mutually_independent(instr1_np, instr2_np)
	register Node *instr1_np;
	register Node *instr2_np;
{
	/* Returns TRUE if no registers or condition codes which are set
	 * (written) by the "instr2_np" node are referenced (read) by the
	 * "instr1_np" node, and vice-versa.
	 * I.e. neither instruction depends on the action/results of the other.
	 */

	return ( regset_empty( regset_intersect(REGS_READ(instr1_np),
						REGS_WRITTEN(instr2_np)) )
				&&
		 regset_empty( regset_intersect(REGS_READ(instr2_np),
						REGS_WRITTEN(instr1_np)) )
	       );
}


Bool
branch_is_last_reference_to_target_label(br_np)
	register Node *br_np;
{
	register struct symbol *symp;
#ifdef DEBUG
	if ( !NODE_IS_BRANCH(br_np) )	internal_error("branch_is_last_reference_to_label(): not BR?");
#endif /*DEBUG*/

	/* The given branch is only DEFINITELY known to be the last remaining
	 * reference to its target label iff there is only one reference left
	 * to the label AND the label is not globally known (i.e. can't be
	 * branched to or (more likely) called from outside this module).
	 */

	symp = br_np->n_operands.op_symp;

	return ( (symp->s_ref_count == 1) &&
		 BIT_IS_OFF(symp->s_attr, SA_GLOBAL) );
}


Bool
nodes_are_equivalent(n1p, n2p)
	register Node *n1p, *n2p;
{
	/* If they're not the same instruction, they're not equal. */
	if ( n1p->n_opcp != n2p->n_opcp )	return FALSE;

	/* OK, so they ARE the same instuction.  Are their OPERANDS equal? */
	switch (n1p->n_opcp->o_func.f_node_union)
	{
	case NU_NONE:
		/* Instruction has no operands, so they're equal. */
		return TRUE;
	case NU_OPS:
		return ( bcmp(&n1p->n_operands, &n2p->n_operands,
			      sizeof(struct operands)) == 0 );
	case NU_STRG:
	case NU_FLP:
	case NU_VLHP:
		/* We should never be comparing these operand types here. */
		internal_error("nodes_are_equivalent(): NU_(%d)?!",
				(int)(n1p->n_opcp->o_func.f_node_union) );
		/*NOTREACHED*/
	}

	/*NOTREACHED*/
}


void
copy_node(from_node, to_node, optno)
	register Node *from_node;
	register Node *to_node;
	register OptimizationNumber optno;
{
	register Node *saved_n_next;
	register Node *saved_n_prev;

	/*Copy the "from" node to the "to" node, except for the list pointers.*/
	saved_n_next = to_node->n_next;
	saved_n_prev = to_node->n_prev;
	*(to_node) = *(from_node);
	to_node->n_next = saved_n_next;
	to_node->n_prev = saved_n_prev;

	to_node->n_symref_next = NULL;
#ifdef DEBUG
	to_node->n_optno = optno;
#endif /*DEBUG*/

	/* If the node is one with operands, then mark another
	 * reference to the symbol (if any) it refers to.
	 */
	if (to_node->n_opcp->o_func.f_node_union == NU_OPS)
	{
		node_references_label(to_node, to_node->n_operands.op_symp);
	}
}


#ifdef DEBUG
Node *
opt_new_node(lineno, opcp, optno)
	register LINENO lineno;
	register struct opcode *opcp;
	register OptimizationNumber optno;
{
	register Node *np;

	np = new_node(lineno, opcp);
	np->n_optno = optno;
	return np;
}
#endif /*DEBUG*/

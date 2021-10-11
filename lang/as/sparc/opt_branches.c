static char sccs_id[] = "@(#)opt_branches.c	1.1\t10/31/94";
/*
 * This file contains code which performs SPARC code optimizations
 * related to branches/jumps/calls (control transfers).
 */

#include <stdio.h>
#include "structs.h"
#include "actions.h"
#include "globals.h"
#include "node.h"
#include "label.h"
#include "opcode_ptrs.h"
#include "optimize.h"
#include "opt_unique_label.h"
#include "opt_utilities.h"
#include "opt_statistics.h"
#include "opt_loop_invert.h"
#include "opt_simplify_annul.h"


/* get_branch_target_np() returns a pointer to the target node of the given
 * Branch instruction, and does some error checking.
 */
Node *
get_branch_target_np(np)
	register Node *np;	/* pointer to Branch node */
{
	register Node *target_label_np;	/* pointer to target of Branch node */

	target_label_np = np->n_operands.op_symp->s_def_node;
	if (target_label_np == NULL)
	{
		internal_error(
			"get_branch_target_np(): Branch to gbl label (\"%s\")?",
			np->n_operands.op_symp->s_symname);
		/*NOTREACHED*/
	}
	else	return target_label_np;
}


static Bool
retarget_branch_sooner(branch_np, target_label_np, new_next_npp)
	register Node *branch_np;	/* node of the branch instruction     */
	register Node *target_label_np; /* node of the target label of branch */
	register Node **new_next_npp;
{
	/* "branch_np" points to a Branch-Always instruction.
	 *
	 * Try to retarget the branch to before the given label ("sooner"),
	 * so as to delete instructions which appear before the branch which
	 * are identical to those which appear just before the target of the
	 * branch.
	 *
	 * This optimization is also known as "cross-jumping".
	 *
	 * There are three main patterns in this one:
	 *
	 * Pattern #1: --------------------------------------------------------
	 *		<last matching instruction>	<-- last_branch_np
	 *		<...matching instructions...>
	 *		b	Lxx			<-- np
	 *		nop				<-- np->n_next
	 *		:
	 *		:
	 *		<last matching instruction>	<-- last_target_np
	 *		<...matching instructions...>
	 *	Lxx: 					<-- target_label_np
	 *
	 * which we will try to turn into:
	 *
	 *		b	Lyy			<-- np
	 *		nop				<-- np->n_next
	 *		:
	 *		:
	 *	Lyy:
	 *		<last matching instruction>	<-- last_target_np
	 *		<...matching instructions...>
	 *	Lxx: ! (may get deleted)		<-- target_label_np
	 *
	 * Pattern #2: --------------------------------------------------------
	 *		<last matching instruction>	<-- last_branch_np
	 *		<...matching instructions...>
	 *		b,a	Lxx			<-- np
	 *		<branch delay slot>		<-- np->n_next
	 *		:
	 *		:
	 *		<last matching instruction>	<-- last_target_np
	 *		<...matching instructions...>
	 *	Lxx: 					<-- target_label_np
	 *
	 * which we will try to turn into:
	 *
	 *		b,a	Lyy			<-- np
	 *		<branch delay slot>		<-- np->n_next
	 *		:
	 *		:
	 *	Lyy:
	 *		<last matching instruction>	<-- last_target_np
	 *		<...matching instructions...>
	 *	Lxx: ! (may get deleted)		<-- target_label_np
	 *
	 * Pattern #3: --------------------------------------------------------
	 *		<last matching instruction>	<-- last_branch_np
	 *		<...matching instructions...>
	 *		b	Lxx			<-- np
	 *		<useful instruction>		<-- np->n_next
	 *		:
	 *		:
	 *		<last matching instruction>	<-- last_target_np
	 *		<...matching instructions...>
	 *		<useful instruction>
	 *	Lxx: 					<-- target_label_np
	 *
	 * which we will try to turn into:
	 *
	 *		b	Lyy			<-- np
	 *		nop				<-- np->n_next
	 *		:
	 *		:
	 *	Lyy:
	 *		<last matching instruction>	<-- last_target_np
	 *		<...matching instructions...>
	 *		<useful instruction>
	 *	Lxx: ! (may get deleted)		<-- target_label_np
	 *
	 * --------------------------------------------------------------------
	 *
	 * Note that in case #3, where the branch isn't annuled and the delay
	 * slot is not a NOP (contains a useful instruction), the delay slot
	 * instruction must be considered as one of the instructions to be
	 * matched with those before the target!
	 * Also in that case, we have to be very careful how we construct the
	 * final pattern.
	 */
	register Node *before_branch_np;/* points to a node before the branch */
	register Node *before_target_np;/* points to a node before the target */
	register Node *last_branch_np;	/* ptr to last node before a branch,
					 *   which matched one before target.
					 */
	register Node *last_target_np;	/* ptr to last node before a target,
					 *   which matched one before branch.
					 */
	register Node *delay_slot_np;	/* pts to node in branch's delay slot */
	register Bool  delay_slot_is_part_of_match;
	register Bool  delay_slot_matched;
	register int   nodes_matched;

	register Bool changed;
	
	/* The optimization must be enabled,
	 * and the branch must be UnConditional.
	 */
	if ( !do_opt[OPT_BRSOONER] || BRANCH_IS_CONDITIONAL(branch_np) )
	{
		return FALSE;
	}

	/* Here, we have an UnConditional Branch node.  Try to perform
	 * the branch-sooner (a.k.a. "cross-jumping") optimization.
	 */

	changed = FALSE;

	delay_slot_np = branch_np->n_next;
	delay_slot_is_part_of_match = ( (!BRANCH_IS_ANNULLED(delay_slot_np)) &&
					(!NODE_IS_NOP(delay_slot_np)) );

	if ( delay_slot_is_part_of_match )
	{
		/* The delay slot isn't annulled and doesn't contain a NOP,
		 * so it must be taken into consideration.
		 * (Oh, for the simpler days of life without delay slots...)
		 */
		last_target_np = previous_code_generating_node(target_label_np);
		if ( nodes_are_equivalent(delay_slot_np, last_target_np) )
		{
			delay_slot_matched = TRUE;
		}
		else	delay_slot_matched = FALSE;
	}
	else	last_target_np = target_label_np;

	if ( (!delay_slot_is_part_of_match) ||
	     (delay_slot_is_part_of_match && delay_slot_matched)
	   )
	{
		/* After checking the delay slot, all is still "go" for this
		 * optimization.
		 */

		for ( last_branch_np = branch_np, nodes_matched = 0;
		      /* no loop test here */;
		      /* no loop incr here */
		    )
		{
			before_branch_np
				= previous_code_generating_node_or_label(
								last_branch_np);
			before_target_np
				= previous_code_generating_node(last_target_np);

			/* STOP comparing nodes if any of these are true:
			 *	- we hit a label before the branch
			 *	- the block of code before the branch begins
			 *		to overlap with the block of code
			 *		before the target label
			 *	- we hit the Marker node before either of them
			 *	- we hit an UNIMP instruction (which may not be
			 *		moved because the compiler puts it
			 *		after CALL's when a structure value is
			 *		expected to be returned by a function)
			 *      - we hit a cswitch (or computed goto) table
			 *	- the two instructions are just plain DIFFERENT.
			 */
			if ( NODE_IS_LABEL(before_branch_np)       ||
			     (before_target_np == delay_slot_np)   ||
			     (before_branch_np == target_label_np) ||
			     NODE_IS_MARKER_NODE(before_branch_np) ||
			     NODE_IS_MARKER_NODE(before_target_np) ||
			     NODE_IS_UNIMP(before_branch_np)	   ||
			     NODE_IS_UNIMP(before_target_np)	   ||
			     NODE_IS_CSWITCH(before_branch_np)     ||
			     NODE_IS_CSWITCH(before_target_np)     ||
			     !nodes_are_equivalent(before_branch_np,
						   before_target_np)
			   )
			{
				break;	/* out of "for" loop */
			}
			else
			{
				last_branch_np = before_branch_np;
				last_target_np = before_target_np;
				nodes_matched++;
			}
		}

		if ( opt_node_is_in_delay_slot(last_branch_np) ||
		     opt_node_is_in_delay_slot(last_target_np)
		   )
		{
			/* At least one of the last-matched nodes is in a delay
			 * slot: BACK OFF a node, because if the CTI before it
			 * didn't match, then the instruction in the delay
			 * slot doesn't really match either.
			 */
			/* We could be a bit smarter here and NOT back off if
			 * the instruction is in the delay slot of an
			 * UnConditional Brach with Annul set, but that seems
			 * too risky at the moment.  Let's leave it for later
			 * (or never).
			 */
			last_target_np =
				next_code_generating_node(last_target_np);
			last_branch_np =
				next_code_generating_node(last_branch_np);
			--nodes_matched;
		}

		if (nodes_matched > 0)
		{
			/* Then we found at least one node which matched. */

			register Node *new_target_label_np;
			register Node *next_before_branch_np;

			/*Create our own label node, to have one to branch to.*/
			new_target_label_np = unique_optimizer_label_node(
						last_target_np->n_lineno,
						OPT_BRSOONER);

			/* Insert it just before the last matching node before
			 * the old target label.
			 */
			insert_before_node(last_target_np, new_target_label_np);


			/* Change the branch node's reference to the new target
			 * label.  (If this was the only reference to the old
			 * label, this will cause the old label node to be
			 * deleted, too).
			 */
			change_branch_target(branch_np,
				     new_target_label_np->n_operands.op_symp);

			/* If the delay slot instruction was one of the nodes
			 * which was part of the match, then it is no longer
			 * needed; delete it and replace it with a NOP.
			 * Since we delete a node AND add one, don't change
			 * "optimizer_stats.branches_sooner_nodes_deleted".
			 */
			if ( delay_slot_is_part_of_match && delay_slot_matched )
			{
				delete_node(delay_slot_np);

				insert_after_node(branch_np,
				    new_optimizer_NOP_node(branch_np->n_lineno,
							   OPT_BRSOONER));

				*new_next_npp = branch_np->n_next->n_next;
			}

			/* Now delete all of the nodes prior to the branch which
			 * had matched the nodes before the target label.
			 */
			for ( before_branch_np =
				previous_code_generating_node(branch_np);
			      /* test inside of loop */;
			      before_branch_np = next_before_branch_np
			    )
			{
				next_before_branch_np =
					previous_code_generating_node(before_branch_np);
				delete_node(before_branch_np);
				optimizer_stats.branches_sooner_nodes_deleted++;

				/* Stop when have deleted last matching node.
				 * (have to fake a "for" which does "do-while"
				 * instead of "while" loop here)
				 */
			        if (before_branch_np == last_branch_np)	break;
			}

			optimizer_stats.branches_sooner_successful++;
			changed = TRUE;
		}
	}

	return changed;
}


/* Simplify branches to branches by making this branch branch directly to
 * the target of the second branch, when possible.
 */
static Bool
branch_to_branch(np, target_instr_np)
	register Node *np;
	register Node *target_instr_np;	/* 1st instruction after target label */
{
	/* "When possible" means when the target instruction is also
	 * a branch (and doesn't have a useful instruction in its delay slot);
	 * and is either UnConditional, or is branching on the same condition
	 * as the first branch (and the target instruction(branch) doesn't
	 * depend on the instruction in the delay slot of the first branch!).
	 *
	 * "Doesn't have a useful instruction in its delay slot" means that
	 * either it has a NOP there, or the delay-slot instruction is never
	 * executed because the branch is UnConditional and Annulled.
	 *
	 * Later, maybe we could be smarter about the instruction in
	 * the delay slot of the 2nd branch (i.e. not flatly requiring
	 * it to be a NOP)...
	 */
	if ( do_opt[OPT_BR2BR]
			&&
	     NODE_IS_BRANCH(target_instr_np)
			&&
	     ( DELAY_SLOT_WILL_NEVER_BE_EXECUTED(target_instr_np) ||
	       NODE_IS_NOP(next_instruction(target_instr_np)) )
			&&
	     ( BRANCH_IS_UNCONDITIONAL(target_instr_np) ||
	       ( (target_instr_np->n_opcp->o_func.f_subgroup ==
		    np->n_opcp->o_func.f_subgroup) &&
		 ( ! instr1_depends_on_instr2(target_instr_np, np->n_next) )
	       )
	     )
	   )
	{
		if (target_instr_np->n_operands.op_symp ==
			np->n_operands.op_symp)
		{
			/* Here, both are branches to the same place,
			 * i.e. the second branch, which loops back to
			 * itself.  Leave this case alone.
			 */
			return FALSE;
		}
		else
		{
			/* Copy the 2nd branch's target to the 1st */
			change_branch_target(np,
					target_instr_np->n_operands.op_symp);

			optimizer_stats.branches_to_branches_simplified++;
			return TRUE;
		}
	}
	else	return FALSE;
}


/* retarget_branch() is given a pointer to a Branch node (possibly either
 * Conditional or UnConditional), and tries to do the following:
 *	-- if it's an unreachable branch to itself, delete the branch (and the
 *		corresponding label).
 *	-- if it's a branch to a label before which a series of instructions
 *		are identical to those before this branch, then delete those
 *		instructions before the branch and retarget the branch to
 *		those instructions before the label.
 *	-- if it's a branch to another branch, re-target it if possible to the
 *		target of the second branch.
 */
static Bool
retarget_branch(np, new_next_npp)
	register Node *np;
	register Node **new_next_npp;
{
	register Node *target_label_np;	/* node of the target label of branch */
	register Node *target_instr_np;	/* 1st instruction after target label */

#ifdef DEBUG
	if ( !NODE_IS_BRANCH(np) )
	{
		internal_error("retarget_branch(): %d != FG_BR",
				(int)(np->n_opcp->o_func.f_group));
		/*NOTREACHED*/
	}
#endif /*DEBUG*/

	target_label_np = get_branch_target_np(np);

	/* Get a pointer to the instruction following the target label.  */
	target_instr_np = next_instruction(target_label_np);

	/* Eliminate orphan (unreachable) branches to themselves */
	if (target_instr_np == np)
	{
		/* Here, it is a branch to itself (probably due to code
		 * between the label and this branch being deleted).
		 *
		 * If this branch is the ONLY reference to the label,
		 * AND code can't fall through into this code, then we
		 * can delete both nodes as UnReachable code.
		 */
		if ( do_opt[OPT_DEADLOOP] &&
		     branch_is_last_reference_to_target_label(np) &&
		     (cannot_fall_into(target_label_np))
		   )
		{
			/* delete the instruction in the delay slot */
			delete_node(next_instruction(np));

			/* Delete the branch instruction.
			 * (this delete_node() should delete the
			 *  preceeding label, too)
			 */
			delete_node(np);

			optimizer_stats.dead_loops_deleted++;
			return TRUE;
		}
		else	return FALSE;
	}

	/* Try to retarget the branch to before the given label,
	 * so as to delete instructions which appear before the branch which
	 * are identical to those which appear just before the target of the
	 * branch.
	 */
	if ( retarget_branch_sooner(np, target_label_np, new_next_npp) )
	{
		return TRUE;
	}

	/* Simplify branches to branches by making this branch branch directly
	 * to the target of the second branch, when possible.
	 */
	if ( branch_to_branch(np, target_instr_np) )
	{
		return TRUE;
	}

	return FALSE;
}


Bool
trivial_branch(branch_np, new_next_npp)
	register Node *branch_np;
	register Node **new_next_npp;
{
	register Node *delay_slot_np;
	register Node *instr_after_delay_slot_np;
	register Node *target_label_np;
	register Node *instr_after_target_label_np;
	register Bool changed;

	if ( !do_opt[OPT_TRIVBR] )	return FALSE;

#ifdef DEBUG
	if ( !NODE_IS_BRANCH(branch_np) )
	{
		internal_error("trivial_branch(): %d != FG_BR",
				(int)(branch_np->n_opcp->o_func.f_group));
		/*NOTREACHED*/
	}
#endif /*DEBUG*/

	/* If we match this pattern:
	 *		ba{,a}	Lxx 		<-- branch_np
	 *		<delay instr>		<-- delay_slot_np
	 *	Lxx:				<-- target_label_np
	 *		<instr>			<-- instr_after_target_label_np
	 *					 == instr_after_delay_slot_np
	 *
	 * ...or this pattern:
	 *		b<cond>	Lxx 		<-- branch_np
	 *		<delay instr>		<-- delay_slot_np
	 *	Lxx:				<-- target_label_np
	 *		<instr>			<-- instr_after_target_label_np
	 *					 == instr_after_delay_slot_np
	 *
	 * Then we turn it into:
	 *		<delay instr> !deleted?	<-- delay_slot_np
	 *	Lxx:		      !deleted?	<-- target_label_np
	 *		<instr>			<-- instr_after_target_label_np
	 *---------------------------------------------------------------------
	 * The case which we MUST leave ALONE is:
	 *		b<cond>,a	Lxx 	<-- branch_np
	 *		<delay instr>		<-- delay_slot_np
	 *	Lxx:				<-- target_label_np
	 *		<instr>			<-- instr_after_target_label_np
	 *					 == instr_after_delay_slot_np
	 *
	 */

	changed = FALSE;

	target_label_np = get_branch_target_np(branch_np);
	instr_after_target_label_np = next_instruction(target_label_np);

	delay_slot_np = next_instruction(branch_np);
	instr_after_delay_slot_np = next_instruction(delay_slot_np);

	if ( (instr_after_target_label_np == instr_after_delay_slot_np) &&
	     ( !( BRANCH_IS_CONDITIONAL(branch_np) &&
		  BRANCH_IS_ANNULLED(branch_np)
		)
	     )
	   )
	{
		/* Here, the target of the branch is the instruction following
		 * the delay slot of the branch, and this is NOT the case which
		 * we must leave as-is.
		 *
		 * Therefore, we can delete the branch as a trivial jump.
		 */

		delete_node(branch_np);
		optimizer_stats.trivial_branches_deleted++;
		changed = TRUE;

		/* If the instruction in the branch's delay slot will never be
		 * executed (i.e. is in the delay slot of an anulled UnCondi-
		 * tional branch) or is a NOP, we can delete it, too.
		 * If it is a NOP it would get deleted later anyway, but why
		 * not do it now "while we're here"?
		 */
		if ( DELAY_SLOT_WILL_NEVER_BE_EXECUTED(branch_np) ||
		     NODE_IS_NOP(delay_slot_np) )
		{
			delete_node(delay_slot_np);
			optimizer_stats.useless_NOPs_deleted++;
			*new_next_npp = instr_after_delay_slot_np;
		}
		else	*new_next_npp = delay_slot_np;
	}

	return changed;
}


static Bool
branch_around_branch(cond_branch_np, new_next_npp)
	register Node *cond_branch_np;
	register Node **new_next_npp;
{
	register Node *delay_slot1_np;
	register Node *uncond_branch_np;
	register Node *target_label_np;
	register Node *delay_slot2_np;
	register Bool changed;

	/* "cond_branch_np" refers to a Conditional branch node.
	 * If the node immediately(!) following its delay slot is an
	 * UnConditional branch and the Conditional branch simply branches
	 * around it, then change it if possible, removing the UnConditional
	 * branch.
	 *
	 * Pattern #1 is:
	 *
	 *		b<cond>		Lx	<-- cond_branch_np
	 *		<delay slot 1>		<-- delay_slot1_np
	 *		ba{,a}		Lx	<-- uncond_branch_np
	 *		nop			<-- delay_slot2_np
	 *	Lx:				<-- target_label_np
	 *
	 * Change it to:
	 *		<delay slot 1>		<-- delay_slot1_np
	 *	Lx: ! (may get deleted)		<-- target_label_np
	 *
	 * ----------------------------------------------------------------
	 * Pattern #2 is:
	 *
	 *		b<cond>		Lx	<-- cond_branch_np
	 *		<delay slot 1>		<-- delay_slot1_np
	 *		ba{,a}		Ly	<-- uncond_branch_np
	 *		nop			<-- delay_slot2_np
	 *	Lx:				<-- target_label_np
	 *
	 * Try to change it to (where cond' = opposite branch condition):
	 *
	 *		b<cond'>	Ly	<-- cond_branch_np
	 *		<delay slot 1>		<-- delay_slot1_np
	 *	Lx: ! (may get deleted)		<-- target_label_np
	 *
	 * ----------------------------------------------------------------
	 * And, being even more clever, we can do Pattern #3, too:
	 *
	 *		b<cond>		Lx	<-- cond_branch_np
	 *		nop			<-- delay_slot1_np
	 *		ba		Ly	<-- uncond_branch_np
	 *		<delay slot 2>		<-- delay_slot2_np
	 *	Lx:				<-- target_label_np
	 *
	 * ...by changing it to (where cond' = opposite branch condition)
	 * (must be VERY careful if Lx = Ly here!):
	 *
	 *		b<cond'>,a	Ly	<-- cond_branch_np
	 *		<delay slot 2>		<-- delay_slot2_np
	 *	Lx: ! (may get deleted)		<-- target_label_np
	 */	

	changed = FALSE;

	delay_slot1_np = next_instruction(cond_branch_np);

	/* Do NOT use next_instruction() here; there cannot be an intervening
	 * label between delay slot #1 and the following branch (if present).
	 */
	uncond_branch_np = delay_slot1_np->n_next;

	delay_slot2_np = uncond_branch_np->n_next;
	target_label_np = get_branch_target_np(cond_branch_np);

	/* See if we really DO have a conditional branch which is not annulled,
	 *
	 * and an unconditional branch right after the Conditional branch's
	 * delay slot,
	 *
	 * and that the target label of the Conditional branch appears right
	 * after the UnConditional branch's delay slot,
	 *
	 * and that the instruction in the delay slot of the UnConditional
	 * branch is a NOP or an instruction which never gets executed anyway.
	 *
	 */
	if ( (!BRANCH_IS_ANNULLED(cond_branch_np))
			&&
	     ( NODE_IS_BRANCH(uncond_branch_np) &&
	       BRANCH_IS_UNCONDITIONAL(uncond_branch_np)
	     )
			&&
	     (target_label_np == delay_slot2_np->n_next)
	   )
	{
		if ( do_opt[OPT_BRARBR1]
				&&
		     ( cond_branch_np->n_operands.op_symp ==
		       uncond_branch_np->n_operands.op_symp
		     )
				&&
		     ( NODE_IS_NOP(delay_slot2_np) ||
		       DELAY_SLOT_WILL_NEVER_BE_EXECUTED(uncond_branch_np)
		     )
		   )
		{
			/* Delete the Conditional branch, the UnConditional
			 * Branch, and the UnConditional Branch's delay slot.
			 */
			delete_node(cond_branch_np);
			delete_node(uncond_branch_np);
			delete_node(delay_slot2_np);
			optimizer_stats.branches_around_branches_deleted_1++;

			*new_next_npp = delay_slot1_np->n_next;
			changed = TRUE;
		}
		else if ( do_opt[OPT_BRARBR2]
				&&
		     ( NODE_IS_NOP(delay_slot2_np) ||
		       DELAY_SLOT_WILL_NEVER_BE_EXECUTED(uncond_branch_np)
		     )
		   )
		{
			/* Reverse the sense of the Condition tested by the
			 * Conditional branch.
			 */
			cond_branch_np->n_opcp =
			      reverse_condition_opcode(cond_branch_np->n_opcp);
#ifdef DEBUG
		if (cond_branch_np->n_opcp == NULL)     internal_error("branch_around_branch(): case 1: no rev cond?");
#endif /*DEBUG*/

			/* Retarget the Conditional branch from its old target
			 * label to the new one (which will also cause deletion
			 * of the old label node if this was the only reference
			 * to it).
			 */
			change_branch_target(cond_branch_np,
					  uncond_branch_np->n_operands.op_symp);
			
			/* Then delete the original UnConditional branch and
			 * the delay-slot-2 instruction.
			 */
			delete_node(uncond_branch_np);
			delete_node(delay_slot2_np);
			optimizer_stats.branches_around_branches_deleted_2++;

			*new_next_npp = delay_slot1_np->n_next;
			changed = TRUE;
		}
		else if ( do_opt[OPT_BRARBR3]
				&&
			  ( NODE_IS_NOP(delay_slot1_np) &&
			    (!BRANCH_IS_ANNULLED(cond_branch_np)) &&
			    (!BRANCH_IS_ANNULLED(uncond_branch_np))
			  )
			)
		{
			/* We have the second case. */

			/* Reverse the sense of the Condition tested by the
			 * Conditional branch.
			 */
			cond_branch_np->n_opcp =
			      reverse_condition_opcode(cond_branch_np->n_opcp);
#ifdef DEBUG
			if (cond_branch_np->n_opcp == NULL)     internal_error("branch_around_branch(): case 2: no rev cond?");
#endif /*DEBUG*/

			if ( NODE_IS_NOP(delay_slot2_np) )
			{
				/* Delay_slot2 is a NOP, and it is preferrable
				 * (for the Instruction Scheduler's sake) to
				 * leave the branch as-is (i.e. Un-annulled).
				 */
			}
			else
			{
				/* There is not a NOP in the second delay slot,
				 * so we must turn the branch into its
				 * Annulled version for the code to be correct.
				 */
				cond_branch_np->n_opcp =
					branch_with_toggled_annul_opcp(
							cond_branch_np->n_opcp);
			}

#ifdef DEBUG
			if (cond_branch_np->n_opcp == NULL)
			{
				internal_error("branch_around_branch(): no annul version (of \"%s\")?",
					cond_branch_np->n_opcp->o_name);
			}
#endif /*DEBUG*/

			/* Retarget the Conditional branch from its old target
			 * label to the new one (which will also cause deletion
			 * of the old label node if this was the only reference
			 * to it).
			 */
			change_branch_target(cond_branch_np,
					  uncond_branch_np->n_operands.op_symp);
			
			/* Then delete the original UnConditional branch and
			 * the delay-slot-1 (NOP) instruction.
			 */
			delete_node(delay_slot1_np);
			delete_node(uncond_branch_np);
			optimizer_stats.branches_around_branches_deleted_3++;

			*new_next_npp = delay_slot2_np->n_next;
			changed = TRUE;
		}
	}

	return changed;
}


static Bool
move_br_target(np, new_next_npp)
	register Node *np;
	register Node **new_next_npp;
{
	register Bool changed;
	register Node *target_label_np;

	/* "np" points to a Branch-Always instruction.
	 * If possible, replace that branch instruction with the code from the
	 * target of the branch.
	 *
	 * "If possible" means if that code (at the branch target) is not
	 * reached from ANYWHERE else but this branch instruction.
	 * Specifically:
	 *	(a) the target label of the branch is not referenced anywhere
	 *	    else,
	 *	(b) the target code cannot be fallen into from above.
	 *
	 * Graphically, the pattern looks like:
	 *		b{,a}	Lxx			<-- np
	 *		<branch delay slot>		<-- np->n_next
	 *		<instr's after branch>
	 *		.....
	 *		! (cannot fall through here!)
	 *	Lxx: ! (b is the only reference to Lxx)	<-- target_label_np
	 *		<instr after label>		<-- first_np
	 *		.....
	 *		<uncond'l Branch, Jmp, or Return>
	 *		<delay slot, or Cswitch code>	<-- last_np
	 *
	 * And we will try to turn it into:
	 *		<branch delay slot>!(deleted if the branch was annulled)
	 *		<instr after label>		<-- first_np
	 *		<uncond'l Branch, Jmp, or Return>
	 *		<delay slot, or Cswitch code>	<-- last_np
	 *		<instr's after branch>
	 *		.....
	 */

	if ( !do_opt[OPT_MOVEBRT] )	return FALSE;

#ifdef DEBUG
	if ( !BRANCH_IS_UNCONDITIONAL(np) )
	{
		internal_error("move_br_target(): \"%s\" not uncond BR!",
				np->n_opcp->o_name);
		/*NOTREACHED*/
	}
#endif /*DEBUG*/

	changed = FALSE;

	target_label_np = get_branch_target_np(np);

	if ( branch_is_last_reference_to_target_label(np) &&
	     cannot_fall_into(target_label_np)
	   )
	{
		/* Here, the Branch is the only node which references the target
		 * label, and there is no fall-through into the target label.
		 * So, try to replace the Branch with the code which starts
		 * after the target label and ends with an unconditional Branch,
		 * Jump, Return, or C-Switch.
		 */
		register Node *first_np;	/* first node to move	*/
		register Node *last_np;		/* last node to move	*/

		/* now find the bounds of the code block to move */
		for( last_np = first_np = target_label_np->n_next;
		     !NODE_IS_MARKER_NODE(last_np->n_next);
		     last_np = last_np->n_next
		   )
		{
			if ( is_an_unconditional_control_transfer(last_np) )
			{
				if (is_jmp_at_top_of_cswitch(last_np))
				{
					/* Pick up the whole C-Switch table.
					 * It has:  jmp, delay slot, label,
					 *	    then CSWITCH's.
					 * (ASSUME CSWITCH structure here).
					 */
					for (last_np = last_np->n_next->n_next;
					     (last_np->n_next->n_opcp->
						o_func.f_group == FG_CSWITCH);
					     last_np = last_np->n_next)
					{
						/* no loop body */
					}
				}
				else
				{
					/* This is the end of the block to move.
					 * Pick up the node in the delay slot
					 * and stop.
					 */
					last_np = last_np->n_next;
				}
				break;
			}
		}

		if (last_np == np->n_next)
		{
			/* Here, the instruction at the end of the block we
			 * were going to move is the delay slot of the original
			 * branch;  therefore, this turned out to be a loop
			 * with our branch at its bottom.  So, we can't move
			 * anything after all.
			 */
		}
		else
		{
			register Node *delay_slot_np = np->n_next;

			/* Make the code we want to move into an orphan block.*/
			make_orphan_block(first_np, last_np);

			/* Insert the block after the delay slot node. */
			append_block_after_node(first_np, last_np, np->n_next);

			/* If the branch is annulled (therefore the delay slot
			 * instruction will never get executed anyway), or the
			 * delay slot just contained a NOP, delete the delay
			 * slot instruction.
			 */
			if ( DELAY_SLOT_WILL_NEVER_BE_EXECUTED(np) ||
			     NODE_IS_NOP(delay_slot_np)
			   )
			{
				/* Delete the delay slot instruction. */
				delete_node(delay_slot_np);
				optimizer_stats.branch_targets_moved_nodes_deleted++;
			}

			/* Delete the original branch.  This should also cause
			 * deletion of the target (label) of the branch.
			 */
			delete_node(np);
			optimizer_stats.branch_targets_moved_nodes_deleted++;

			/* The next intruction to look at optimizing should be
			 * the 1st instruction we moved.
			 */
			*new_next_npp = first_np;

			optimizer_stats.branch_targets_moved++;
			changed = TRUE;
		}
	}

	return changed;
}


static Bool
optimize_branch(np, new_next_npp)
	register Node *np;
	register Node **new_next_npp;
{
	register Node *target_label_np;

	/* First, can we re-target the branch? */
	if ( retarget_branch(np, new_next_npp) )
	{
		return TRUE;
	}

	/* Then, delete it if it is a trivial jump
	 * (i.e. jumps to the instruction following the delay slot)
	 */
	if (trivial_branch(np, new_next_npp))
	{
		return TRUE;
	}

	if ( BRANCH_IS_UNCONDITIONAL(np) )
	{
		/* It is an unconditional branch (i.e. branch always). */

		/* Move code from the target of the branch to after it,
		 * if possible.
		 */
		if ( move_br_target(np, new_next_npp) )
		{
			return TRUE;
		}

		/* For an UnConditional branch, the "dead" code following its
		 * delay slot can be deleted, up to the next label.
		 */
		if ( delete_dead_code(np->n_next->n_next, new_next_npp) )
		{
			return TRUE;
		}
	}
	else
	{
		/* It is a conditional branch. */

		/* Simplify it, if it is a Conditional branch around an
		 * Unconditional branch.
		 */
		if ( branch_around_branch(np, new_next_npp) )
		{
			return TRUE;
		}

		/* Simplify it, if it is a Conditional branch with Annul,
		 * where the instruction in its delay slot matches the
		 * following instruction.
		 */
		if ( branch_simplify_annul(np, new_next_npp) )
		{
			return TRUE;
		}
	}

	return FALSE;
}


/* untangle_jumps():
 *	- removes trivial branches (branches to instruction after delay slot)
 *	- re-routes branches to branches
 *	- re-routes branches around branches
 *	- deletes dead code after unconditional branches and jumps
 * ...all the while, trying to watch out for delay-slot and annul-bit "gotchas".
 *
 * We make lots of ASSUMPTIONS here (CTI = Control-Transfer Instruction):
 *	-- that the compiler NEVER generates CTI-pairs (adjacent CTI's)
 *	-- that the compiler NEVER generates code with a LABEL in the delay
 *		slot of a branch/jump/return.
 *	-- that the instruction in a delay slot ALWAYS IMMEDIATELY follows
 *		the CTI.
 *	-- that the compiler NEVER generates code-emitting pseudos (e.g.
 *		.byte, .half, .word, .skip) for instructions (so we know that
 *		if we see one of those, control will never pass through it).
 *	-- that C switch-tables coming into the assembler always start with a
 *		label (i.e. look like "jmp <REG> ; <delay-slot-instr> ;
 *		LABEL: ; .word ...").  Furthermore, there is always a
 *		reference to that label (so it won't get deleted).
 *	-- that the optimizer never deletes the delay-slot instruction of a
 *		unconditional Branch-with-annul while it is iterating (although
 *		it is OK during a final post-pass).
 *
 * untangle_jumps() returns TRUE if any changes were made; FALSE otherwise.
 *
 * (This routine was known as "tangle()" in the 680x0 peephole optimizer)
 */
Bool
untangle_jumps(marker_np)
	register Node *marker_np;
{
	register Node *np;
	register unsigned long int iteration_count;
	Node *next_np;
	/* "next_np" is used because after delete_node(p), "p" has been
	 * deallocated, and nobody really guarantees us that the contents of
	 * "p" (specifically, p->n_next) are still good.  (Yes, I know,
	 * practical experience says otherwise, but better safe now than sorry
	 * later!)
	 */
	Bool changed_this_iteration;
	Bool changed;
#ifdef DEBUG
	register Node *saved1_next_np, *saved1_np;
	register Node *saved2_next_np, *saved2_np;
#endif

	iteration_count = 0;
	changed = FALSE;
	do
	{
		changed_this_iteration = FALSE;

		for ( np = marker_np->n_next;
		      next_np = np->n_next, np != marker_np;
		      np = next_np
		    )
		{
			switch (np->n_opcp->o_func.f_group)
			{
			case FG_JMP:
				/* Can't delete code in a cswitch table after
				 * a JMP, but can delete code after other JMP's.
				 * delete_dead_code() checks for the C-Switch
				 * itself, though.
				 */
				/* fall through */
			case FG_RET:
#ifdef DEBUG
				saved1_next_np = next_np;
				saved1_np      = np;
#endif
				/* This node is an indirect Jump or a Return;
				 * therefore we can delete any code beginning
				 * with the node AFTER its delay slot, up to
				 * just before the next label.
				 */
				if (delete_dead_code(np->n_next->n_next,
							&next_np))
				{
					changed_this_iteration = TRUE;
				}
				break;

			case FG_BR:	/* This node is a branch. */
#ifdef DEBUG
				saved1_next_np = next_np;
				saved1_np      = np;
#endif
				if( optimize_branch(np, &next_np) )
				{
					changed_this_iteration = TRUE;
				}
				break;
#ifdef DEBUG
			default:
				saved1_next_np = next_np;
				saved1_np      = np;
#endif
			}
#ifdef DEBUG
			saved2_next_np = next_np;
			saved2_np      = np;
#endif
		}

		if (++iteration_count > 1000)
		{
			internal_error("untangle_jumps(): inf loop?");
			/*NOTREACHED*/
		}

		changed = changed || changed_this_iteration;
	 } while (changed_this_iteration) ;

#ifdef DEBUG
			opt_dump_nodes(marker_np, "after untangle_jumps()");
#endif /*DEBUG*/
	return changed;
}

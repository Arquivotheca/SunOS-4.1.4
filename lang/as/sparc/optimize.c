static char sccs_id[] = "@(#)optimize.c	1.1\t10/31/94";
/*
 * This file contains code which performs SPARC code optimizations.
 */

#include <stdio.h>
#include "structs.h"
#include "globals.h"
#include "node.h"
#include "check_nodes.h"
#include "opcode_ptrs.h"
#include "sym.h"
#include "registers.h"
#include "optimize.h"
#include "opt_branches.h"
#include "opt_relabel.h"
#include "opt_dead_code.h"
#include "opt_utilities.h"
#include "opt_statistics.h"
#include "opt_loop_invert.h"
#include "opt_once_only.h"
#include "opt_leaf_routine.h"

Bool optimizer_debug = FALSE;	/* TRUE if optimizer debug output is on */
Bool optimizer_debug2 = FALSE;	/* TRUE if optimizer debug2 output is on */

Bool do_opt[OPT_N_OPTTYPES] = { FALSE };	/* optimizations which are on */

static Bool some_optimization_was_selected = FALSE;
static Bool opt_stats = FALSE;		/* optimizer stats output on?	  */

static void	pre_optimization_pass();    /* forward routine declaration */
static Bool	delete_useless_NOPs();      /* forward routine declaration */


static void
initialize_do_opt(val)
	register Bool val;
{
	register int i;

	for (i = 0;   i < OPT_N_OPTTYPES;   i++)	do_opt[i] = val;
}

static Bool
select_opt(n)
	register int n;
{
	if (!some_optimization_was_selected)
	{
		initialize_do_opt(FALSE);
		some_optimization_was_selected = TRUE;
	}

	if ( n < OPT_N_OPTTYPES )
	{
		do_opt[n] = TRUE;
		return TRUE;
	}
	else	return FALSE;
}

static Bool
deselect_opt(n)
	register int n;
{
	if (!some_optimization_was_selected)
	{
		initialize_do_opt(TRUE);
		some_optimization_was_selected = TRUE;
	}

	if ( n < OPT_N_OPTTYPES )
	{
		do_opt[n] = FALSE;
		return TRUE;
	}
	else	return FALSE;
}


Bool
optimization_options(turn_opt_on, optptr)
	Bool  turn_opt_on;
	register char *optptr;
{
	Bool first_capO_this_time;
	static Bool first_capO_ever = TRUE;

	/* Scan the option letters for Optimization (-O)
	 * Set internal optimization options and the global optimization
	 * level, as specified.
	 *
	 * TRUE is returned if the option string seemed valid,
	 * FALSE if there was any error in it.
	 */

	switch (*optptr)
	{
	case '-':	/* The initial option letter. */
		turn_opt_on = TRUE;
		optptr++;
		break;
	case '+':	/* The initial option letter. */
		turn_opt_on = FALSE;
		optptr++;
		break;
	default:
		/* do nothing; assume "turn_opt_on" has already been set. */
		break;
	}

	for (first_capO_this_time = TRUE;  *optptr != NULL;  optptr++)
	{
		switch(*optptr)
		{
		case '~':	/* deselect ("not") an optimization */
			optptr++;	/* pts at following letter now */
			if ( (*optptr >= 'A') && (*optptr <= 'Z') )
			{
				if (!deselect_opt(*optptr - 'A'))  return FALSE;
			}
			else	return FALSE;	/* bad option syntax */
			break;

		case 'O':
			if (first_capO_this_time)
			{
				/* The Optimize option letter itself. */

				if (first_capO_ever)
				{
					initialize_do_opt(TRUE);
					first_capO_ever = FALSE;
				}

				if ( *(optptr+1) == '\0' )
				{
					/* "-O" all by itself means one of two
					 * things...  If the optimization level
					 * =0 (which may mean that it has never
					 * been set), then "-O" means "-O1".
					 * Otherwise, a plain "-O" is a no-op.
					 * "+O" always means turn OFF opt'n.
					 */ 
					if (turn_opt_on)
					{
						/* "-O" */
						if (optimization_level == 0)
						{
							optimization_level = 1;
						}
					}
					else
					{
						/* "+O" */
						optimization_level = 0;
					}
				}
				else
				{
					/* It's not a plain "-O".  Process the
					 * suboptions as we go through the loop.
					 */
				}

				first_capO_this_time = FALSE;
			}
			else
			{
				/* Since it's the second (or later) 'O' which
				 * we've seen in this option, it must be an
				 * optimization selection letter, e.g. "-OO".
				 */
				if (!select_opt('O' - 'A'))  return FALSE;
			}
			break;

		case 'd':	/* activate optimizer debug output */
			optimizer_debug = turn_opt_on;
			opt_stats = turn_opt_on;
			break;

		case 'D':	/* activate secondary optimizer debug output */
			optimizer_debug2 = turn_opt_on;
			break;

		case 's':	/* activate optimizer statistics reporting */
			opt_stats = turn_opt_on;
			break;

		default:
			/* '1' - '9' select an optimization level;
			 * 'A' - 'Z' select (turn on) a particular optimization.
			 */
			if ( (*optptr >= '0') && (*optptr <= '9') )
			{
				optimization_level = (*optptr - '0');
			}
			else if ( (*optptr >= 'A') && (*optptr <= 'Z') )
			{
				if (!select_opt(*optptr - 'A'))  return FALSE;
			}
			else /* ignore others for now */ ;
			break;
		}
	}

	if (some_optimization_was_selected)
	{
		if ( do_opt[OPT_LOOPINV] )
		{
			/* Since deleting branches-to-branches and trivial
			 * branches is a prerequisite for loop-inversion to
			 * work, force them to be enabled whenever loop-
			 * inversion is enabled.
			 */
			do_opt[OPT_BR2BR]  = TRUE;
			do_opt[OPT_TRIVBR] = TRUE;
		}
	}
	else
	{
		/* No specific optimization was specified, so turn 'em all on */
		initialize_do_opt(TRUE);
		some_optimization_was_selected = TRUE;
	}

	return TRUE;
}


#ifdef DEBUG
void
opt_dump_nodes(marker_np, string)
	register Node *marker_np;
	register char *string;
{
	static int page_no = 1;

	if (optimizer_debug)
	{
		register int i;

		printf("!====== %s ====== (pg # %d)\n", string, page_no++);
		disassemble_list_of_nodes(marker_np);
		fputs("!==============", stdout);
		for (i = strlen(string);  i > 0;  --i)  putchar('=');
		fputs("\n\f", stdout);
	}
}
#endif /*DEBUG*/

static void
optimize_procedure(marker_np)
	register Node *marker_np;
{
	/* A single procedure is optimized by this routine.
	 * It is assumed that the list it is passed doesn't contain more
	 * than a single procedure!
	 */

	register Node *np;
	register struct opcode *opcp;
	register Bool changed_1;
	register Bool changed_2;
	register int inner_loop_count = 0;
	register int main_loop_count  = 0;

	Bool saved_do_opt[OPT_N_OPTTYPES];	/* saved copy of do_opt[].    */
	int  saved_optimization_level;		/* saved copy of opt'n_level  */

	/* save a copy of the permanent optimization options. */
	bcopy (do_opt, saved_do_opt, sizeof(do_opt));
	saved_optimization_level = optimization_level;

	if ( NODE_IS_PROC_PSEUDO(marker_np->n_next) &&
	     NODE_IS_OPTIM_PSEUDO(marker_np->n_next->n_next)
	   )
	{
		/* A ".optim" node is present.  Use its string argument as
		 * a new (but temporary) set of optimization options.
		 */
		if ( ! optimization_options(TRUE,
				marker_np->n_next->n_next->n_string) )
		{
			internal_error("invalid options in .optim: \"%s\"",
				marker_np->n_next->n_next->n_string);
			/*NOTREACHED*/
		}
	}

	if (optimization_level > 0)
	{

		/* Do a pre-optimization clean-up pass over the node list. */
		pre_optimization_pass(marker_np);

		/* Note: in these loops, we really want an "||=" operator
		 * (for brevity) but it's not in C, so we substitute "|=".
		 */

		do
		{
			optimizer_stats.optimizer_main_loop_count++;
			if(++main_loop_count >
				optimizer_stats.max_main_loops_per_proc)
			{
				optimizer_stats.max_main_loops_per_proc =
					main_loop_count;
			}

			changed_1 = FALSE;
			do
			{
				optimizer_stats.optimizer_inner_loop_count++;

				if(++inner_loop_count >
				     optimizer_stats.max_inner_loops_per_proc)
				{
					optimizer_stats.max_inner_loops_per_proc
						= inner_loop_count;
				}
				changed_2 = FALSE;   
				changed_2 |= relabel(marker_np );
				changed_2 |= untangle_jumps(marker_np);
				changed_2 |= invert_loops(marker_np);

				/***** more to add here? *****/

				changed_1 |= changed_2;
			} while (changed_2);

			regset_propagate( marker_np );

			changed_1 |=
			   propagate_constants_and_register_contents(marker_np);
			changed_1 |=
			   opt_dead_arithmetic(marker_np, marker_np->n_prev);

			/***** more to add here? *****/

		} while (changed_1);

		/* Try to change remaining SET's into purely SETHI's,
		 * when possible.
		 */
		set_to_sethi( marker_np );

		/* At this point there are NOPs not in delay slots that inhibit
		   recognition of the "epilogue" pattern so we delete them */
		delete_useless_NOPs(marker_np);

		/* Perform the optimizations to be done ONCE only.  */ 
		once_only_optimizations(marker_np);


		/* Turn it into a "leaf" routine, if possible. */
		(void) make_leaf_routine( marker_np );

		/* Do instruction scheduling, if it is enabled */
		if (do_opt[OPT_SCHEDULING])
		{
			scheduler( marker_np );
		}

		/* Delete all NOPs which are NOT in delay slots of CTIs
		 * (i.e. useless NOPs).
		 */
		delete_useless_NOPs(marker_np);
	}

	/* restore the permanent optimization options from our saved copy. */
	bcopy (saved_do_opt, do_opt, sizeof(do_opt));
	optimization_level = saved_optimization_level;
}


void
optimize_segment(marker_np)
	register Node *marker_np;
{
	/* Optimize the list for a whole segment, a procedure at a time.
	 * This is done by breaking isolating each procedure into its own
	 * circular list (disjoint from the main one) while it's being
	 * optimized.
	 *
	 * Both code BETWEEN ".proc" nodes, and code before the first ".proc"
	 * node will get optimized.
	 */

	/* We take this:
	 *		....
	 *		<any node (#1)>		<-- before_proc_np
	 *		.proc			<-- proc_first_np
	 *		....
	 *		<any node (#2)>		<-- proc_last_np
	 *	 	.proc !(or marker node)	<-- after_proc_np
	 *
	 * Extract this circular (doubly-linked) list, composed of one
	 * procedure:
	 *	+----->	! [marker node]		<-- &temp_marker_node
	 *	|	.proc			<-- proc_np
	 *	|	....
	 *	+------	<any node (#2)>		<-- last_np
	 *
	 * Run it through optimization, unlink the temporary marker node, and
	 * put it back on the original list.
	 *
	 * Keep repeating this down through the main list until we're done.
	 */

	register Node *np;
	register Node *before_proc_np;
	register Node *proc_first_np;
	register Node *proc_last_np;
	register Node *after_proc_np;
	Node temp_marker_node;

	/* do nothing if list is non-existant */
	if (marker_np == NULL)	return;

	/* Initialize the per-procedure marker node. */
	temp_marker_node = *marker_np;

	for (np = marker_np->n_next;  np != marker_np;  np = np->n_next)
	{
		/* We always start an optimization block with a ".proc" node,
		 * EXCEPT at the very beginning, when we must start with the
		 * first node (whether it is a ".proc", or not), to make sure
		 * that we cover all of the segment's code with the optimizer.
		 */
		if ( NODE_IS_PROC_PSEUDO(np) || (np == marker_np->n_next) )
		{
			proc_first_np  = np;
			before_proc_np = proc_first_np->n_prev;

			/* Find the end of the procedure. */
			for ( np = np->n_next;
			      !NODE_IS_PROC_PSEUDO(np) && (np != marker_np);
			      np = np->n_next
			    )
			{
				/* no loop body */
			}
			after_proc_np = np;
			proc_last_np  = after_proc_np->n_prev;

			/* Disconnect the procedure from the list, and put
			 * it in its own circular list, with its very own
			 * marker node.
			 */
			make_orphan_block(proc_first_np, proc_last_np);
			temp_marker_node.n_prev = &temp_marker_node;
			temp_marker_node.n_next = &temp_marker_node;
			append_block_after_node(proc_first_np, proc_last_np,
						&temp_marker_node);
					
			if (NODE_IS_PROC_PSEUDO(proc_first_np))
			{
				optimizer_stats.procedures_optimized++;
			}
			else	optimizer_stats.pre_procedures_optimized++;

			/* OK, now optimize it! */
			optimize_procedure(&temp_marker_node);

			/* Note that, after optimization, the nodes we pointed
			 * to inside the list to be optimized COULD have been
			 * deleted.  Therefore, re-initialize the beginning
			 * and ending pointers.
			 */
			proc_first_np = temp_marker_node.n_next;
			proc_last_np  = temp_marker_node.n_prev;

			/*After optimization, put it back on the original list*/
			make_orphan_block(proc_first_np, proc_last_np);
			append_block_after_node(proc_first_np, proc_last_np, 
						before_proc_np);
			
			/* Change the loop variable, to continue after the
			 * procedure we just finished.
			 */
			np = proc_last_np;
		}
	}

	if (opt_stats)	report_optimizer_stats(marker_np);
}


static Bool
is_jmp_at_top_of_pre_opt_cswitch(np)
	register Node *np;
{
	/* (ASSUME structure of CSWITCH code here).
	 * This node is a JMP.  If it is one at the head of a C switch table
	 * as it comes out of the compiler then return TRUE, else return FALSE.
	 * A C switch table looks like:
	 *	jmp	<REG>		<-- np
	 *	<instr>			<-- np->n_next  (delay slot)
	 *  <Label>:			<-- n3p
	 *	.word <val>		<-- n4p
	 *	...
	 */
	register Node *n3p, *n4p;

#ifdef DEBUG
	if (np->n_opcp->o_func.f_group != FG_JMP)
	{
		internal_error(
		  "is_jmp_at_top_of_pre_opt_cswitch(): f_group==%d (not JMP)?",
		  np->n_opcp->o_func.f_group);
	}
#endif
	n3p = np->n_next->n_next;
	n4p = n3p->n_next;

	return ( (n3p->n_opcp->o_func.f_group == FG_LABEL) &&

		 (n4p->n_opcp->o_func.f_group == FG_PS)   &&
		 (n4p->n_opcp->o_func.f_subgroup == FSG_WORD)
	       );
}

static Bool
are_consecutive_ld_st(np1,np2)
	register Node *np1, *np2;
{

#ifdef DEBUG
	if (np1->n_opcp->o_func.f_group != FG_LD &&
	    np1->n_opcp->o_func.f_group != FG_ST)
	{
		internal_error(
		  "are_consecutive_ld_st(): f_group==%d (not LD or ST)?",
		  np1->n_opcp->o_func.f_group);
	}
	if (np2->n_opcp->o_func.f_group != FG_LD &&
	    np2->n_opcp->o_func.f_group != FG_ST)
	{
		internal_error(
		  "are_consecutive_ld_st(): f_group==%d (not LD or ST)?",
		  np2->n_opcp->o_func.f_group);
	}
#endif
	if (np1->n_operands.op_regs[O_RS1] != np2->n_operands.op_regs[O_RS1])
		return FALSE;
	if (np1->n_operands.op_regs[O_RD] != (np2->n_operands.op_regs[O_RD]-1))
		return FALSE;
	if (np1->n_operands.op_imm_used)
	{
		if (!np2->n_operands.op_imm_used) 
			return FALSE;
		else if (np1->n_operands.op_addend != (np2->n_operands.op_addend-4))
			return FALSE;
		else 
			return TRUE;
	}
	else
        {
		if (!np2->n_operands.op_imm_used || np2->n_operands.op_addend != 4)
			return FALSE;
	        else 
			return TRUE;
	}
}




#ifdef DEBUG
Bool EMPTY_nodes_were_deleted = FALSE;
#endif /*DEBUG*/


static void
pre_optimization_pass(marker_np)
	register Node *marker_np;
{
	register Node *np;
	register Node *next_np;
#ifdef DEBUG
	static char label_after_CTI_msg[] =
		"pre_optimization_pass(): Label follows CTI, line #%d";

	EMPTY_nodes_were_deleted = TRUE;	/* done in this pass */
#endif /*DEBUG*/

	for ( np = marker_np->n_next;
	      next_np = np->n_next, np != marker_np;
	      np = next_np
	    )
	{
		regset_setnode(np);

		switch (np->n_opcp->o_func.f_group)
		{
		case FG_PS:
			switch ( np->n_opcp->o_func.f_subgroup )
			{
			case FSG_EMPTY:
				/* Delete all ".empty" nodes.
				 * (a) they are no longer needed, and
				 * (b) they SHOULD be delete-able during
				 *	dead-code elimination during optimiza-
				 *	tion, but wouldn't be deleted then
				 *	because ".empty" does not generate any
				 *	code.  Therefore, just delete all of
				 *	them now and avoid the problem later.
				 */
				delete_node(np);
				break;
			}
			break;

		case FG_JMP:
#ifdef DEBUG
			if ( NODE_IS_LABEL(np->n_next) )
			{
				internal_error( label_after_CTI_msg,
						np->n_next->n_lineno);
				/*NOTREACHED*/
			}
#endif /*DEBUG*/
			if ( is_jmp_at_top_of_pre_opt_cswitch(np) )
			{
				/* (ASSUME structure of CSWITCH code here)
				 * Have "jmp <>; <nop>; <lbl>: ; .word <val>;".
				 * Change each ".word" into a "*.cswitch".
				 */
				for (np = np->n_next->n_next->n_next;
				     next_np = np->n_next,
				      (np->n_opcp->o_func.f_group == FG_PS) &&
				      (np->n_opcp->o_func.f_subgroup==FSG_WORD);
				     np = next_np
				    )
				{
					/* Change "np" into a CSWITCH node */
					np->n_opcp =
						opcode_ptr[OP_PSEUDO_CSWITCH];

					/* Special case for cswitch nodes that
					 * have an expression (PIC) instead
					 * of a label.  We use the unused 
					 * n_prev field in the defining node
					 * to point back to the the node.
					 */
					if ((np->n_operands.op_symp != NULL) &&
					    OP_FUNC(np->n_operands.op_symp->s_def_node).f_group == FG_SYM_REF)
						np->n_operands.op_symp->s_def_node->n_prev = np;
					
				}
			}
			else if ( (np->n_operands.op_regs[O_RD] == RN_GP(0)) &&
				  ( (np->n_operands.op_regs[O_RS1]==RN_GP(15))||
				    (np->n_operands.op_regs[O_RS1]==RN_GP(31))
				  ) &&
				  (np->n_operands.op_addend == 8)
				)
			{
				/* We have "jmp %i7+8" which is really "ret",
				 * or "jmp  %o7+8", which is really "retl".
				 * (%i7 == reg 31;  %o7 == reg 15)
				 */
				if (np->n_operands.op_regs[O_RS1] == RN_GP(31))
                		{
                			np->n_opcp = opcode_ptr[OP_RET];
                		}
                		else	np->n_opcp = opcode_ptr[OP_RETL];

				/* Leave the operands alone; they are fine. */
			}
			break;

#ifdef DEBUG
		case FG_JMPL:
		case FG_CALL:
		case FG_BR:
		case FG_RET:
			if ( NODE_IS_LABEL(np->n_next) )
			{
				internal_error( label_after_CTI_msg,
						np->n_next->n_lineno);
				/*NOTREACHED*/
			}
			break;
#endif /*DEBUG*/

		case FG_COMMENT:
			/* Comment nodes can end up in delay slots if we are
			 * re-assembling a file which was DISassembled before
			 * with comment fields output at the end of each line.
			 * Since ANY node between a CTI and its delay-slot
			 * instruction will give the peephole optimizer a
			 * headache, delete it.  (The compiler normally never
			 * puts anything between CTI & delay slot).
			 */
			if ( opt_node_is_in_delay_slot(np) )
			{
				delete_node(np);
			}
			break;
		case FG_LD:
			if ( (np->n_opcp == opcode_ptr[OP_LDF]) &&
			     (np->n_next->n_opcp == opcode_ptr[OP_LDF]) )
			{
				if (are_consecutive_ld_st(np,np->n_next))
			        {
					delete_node(np->n_next);
					np->n_opcp = opc_lookup("*ld2");
					next_np = np->n_next;
					regset_setnode(np);
				}
			}
			break;
		case FG_ST:
			if ( (np->n_opcp == opcode_ptr[OP_STF]) &&
			     (np->n_next->n_opcp == opcode_ptr[OP_STF]) )
			{
				if (are_consecutive_ld_st(np,np->n_next))
				{
					delete_node(np->n_next);
					np->n_opcp = opc_lookup("*st2");
					next_np = np->n_next;
					regset_setnode(np);
				}
			}
			break;
		}
	}

#ifdef DEBUG
	opt_dump_nodes(marker_np, "after pre_optimization_pass()");
#endif /*DEBUG*/
}


static Bool
delete_useless_NOPs(marker_np)
	register Node *marker_np;
{
	register Node *np;
	register Node *next_np;
	register Node *next_instr_np;
	register Bool changed;

	if ( !do_opt[OPT_NOPS] )	return FALSE;

	changed = FALSE;

	for ( np = marker_np->n_next;
	      next_np = np->n_next, np != marker_np;
	      np = next_np
	    )
	{
		/* If the node is a NOP and isn't in the delay slot of a CTI
		 * or just before a Bfcc (branch on F.P. condition codes),
		 * then it is useless; delete it.
		 */
		if ( NODE_IS_NOP(np) &&
		     !opt_node_is_in_delay_slot(np) &&
		     !( (next_instr_np = next_instruction(np)), 
		        NODE_IS_FBRANCH(next_instr_np) )
		   )
		{
			delete_node(np);
			optimizer_stats.useless_NOPs_deleted++;
			changed = TRUE;
		}
	}

#ifdef DEBUG
	opt_dump_nodes(marker_np, "after delete_useless_NOPs()");
#endif /*DEBUG*/
	return changed;
}


/*----------------------------------------------------------------------------
 * some correponding routines from 68k c2 to this peephole optimizer:
 *	68k		SPARC
 *	-------------	-------------------------------
 *
 *	fallthrough	can_fall_into		(but arg is different)
 *	delete_dead	delete_dead_code
 *	tangle		untangle_jumps & subordinate functions
 *	unreference	delete_node_reference_to_label
 *	newreference	node_references_label
 *	invloop		invert_loop
 *----------------------------------------------------------------------------
 */

static char sccs_id[] = "@(#)opt_loop_invert.c	1.1\t10/31/94";
/*
 * This file contains code which performs the SPARC code optimization
 * of "loop inversion".
 */

#include <stdio.h>
#include "structs.h"
#include "globals.h"
#include "node.h"
#include "label.h"
#include "opcode_ptrs.h"
#include "optimize.h"
#include "opt_unique_label.h"
#include "opt_utilities.h"
#include "opt_statistics.h"


static Node *
append_unique_label_after_node(np, optno)
	register Node *np;
	register int   optno;
{
	register Node *label_np;

	label_np = unique_optimizer_label_node(np->n_lineno, optno);
	insert_after_node(np, label_np);
	label_np->n_seq_no = np->n_seq_no;

	return label_np;
}


/* invert_a_loop() checks to see if the given branch instruction node is the
 * conditional one at the top of a loop, and if so, "inverts" the loop.
 * This means that the conditional branch is moved to the bottom of the loop,
 * its condition reversed, and a branch-always placed at the top, targeted at
 * the one at the bottom.
 *
 * The whole point is to take advantage of the fact that "taken" branches
 * executing faster than "untaken" branches.  Since loop branches are taken
 * more often than not, we want the conditional branch at the bottom of the
 * loop.
 */
static Bool
invert_a_loop(head_branch_np, new_next_npp)
	register Node *head_branch_np;
	register Node **new_next_npp;
{
	register Node *head_label_np;
	register Node *tail_label_np;
	register Node *tail_branch_np;
	register Node *lnew1_np;	/* new label #1	*/
	register Node *np;
	NODESEQNO orig_head_label_seqno;
	NODESEQNO orig_tail_label_seqno;
	register Node *delay_slot_2_np;
	Bool delay_slot_2_was_never_executed;
	Bool changed;
	Node *before_loop_np;
	Node *after_loop_np;
	Node *tail_branch_delay_slot_np;
	int loop_test_size = 0;

#define max_loop_test_size 20

	/* The pattern we are looking for is:
	 *
	 *		<any node>		<-- before_loop_np
	 *	Lhead:				<-- head_label_np
	 *		<the loop-test code>
	 *		b<cond> Ltail		<-- head_branch_np
	 *		<delay slot 1>
	 *		<loop body>
	 *		b{,a}	Lhead		<-- tail_branch_np
	 *		<delay slot 2>
	 *	Ltail:				<-- tail_label_np
	 *		<any node>		<-- after_loop_np
	 *
	 * ...and if found, through several steps we will turn it into:
	 *
	 *		<any node>		<-- before_loop_np
	 *		<the loop-test code>
	 *		b<cond> Ltail
	 *		<delay slot 1>
	 *	Lnew1:				<-- lnew1_np
	 *		<loop body>
	 *	Lhead:
	 *              <the loop-test code>
	 *		b<rev-cond> Lnew1	<-- head_branch_np
	 *		<delay slot 1>
	 *	Ltail:				<-- tail_label_np
	 *		<any node>		<-- after_loop_np
	 */

	/* First, does it match the original pattern?
	 * And, while, we're checking, accumulate the various node pointers.
	 */
#ifdef DEBUG
	if (!NODE_IS_BRANCH(head_branch_np))	internal_error("invert_loop(): not a BR?");
#endif /*DEBUG*/
	/* If the "head" branch is UnConditional or is Annulled, then no match.
	 */
	if ( BRANCH_IS_UNCONDITIONAL(head_branch_np) ||
	     BRANCH_IS_ANNULLED(head_branch_np) )	return FALSE;

	tail_label_np = head_branch_np->n_operands.op_symp->s_def_node;
#ifdef DEBUG
	if (tail_label_np == NULL) internal_error("invert_loop(): BR to Gbl?");
	if (tail_label_np->n_opcp->o_func.f_group != FG_LABEL) 
	{
		internal_error("invert_loop(): head branch not to LABEL?");
	}
#endif /*DEBUG*/
	/* The tail branch should be just before the tail label, followed by
	 * its delay slot and possibly by comment node(s).
	 * However, if there is a LABEL node in between, then "no match".
	 */
	tail_branch_delay_slot_np =
		previous_code_generating_node_or_label(tail_label_np);
	if (NODE_IS_LABEL(tail_branch_delay_slot_np))
	{
		return FALSE;
	}

	/* If it not an UnConditional branch, then no match.	*/
	tail_branch_np = tail_branch_delay_slot_np->n_prev;
	if ( (!NODE_IS_BRANCH(tail_branch_np)) ||
	     BRANCH_IS_CONDITIONAL(tail_branch_np)
	   )
	{
		return FALSE;
	}

	/* The head label is referenced by the tail branch. */
	head_label_np = tail_branch_np->n_operands.op_symp->s_def_node;

	/* Now, make sure that all of these things are in the correct ORDER
	 * in the code: head label, head branch, tail branch, tail label.
	 * If not, then no match.
	 */
	if ( !( (head_label_np->n_seq_no  < head_branch_np->n_seq_no) &&
	        (head_branch_np->n_seq_no < tail_branch_np->n_seq_no) &&
	        (tail_branch_np->n_seq_no < tail_label_np->n_seq_no)
	      )
	   )
	{
		return FALSE;
	}

	/* OK, we match the pattern.  Now start transforming this loop. */
	/* Before we check to see if the code we copy is not too long. */

 	np = head_label_np->n_next;
	while (np != head_branch_np) {
		loop_test_size += 1;
		if (loop_test_size > max_loop_test_size) return FALSE;
		if (NODE_IS_BRANCH(np))
		{
			Node * dest_np;
			dest_np = np->n_operands.op_symp->s_def_node;
			if (dest_np->n_seq_no > tail_label_np->n_seq_no) 
			        return FALSE;
		}
		np = np->n_next;	
	}

	orig_head_label_seqno = head_label_np->n_seq_no;
	orig_tail_label_seqno = tail_label_np->n_seq_no;

	/* Save pointers to just before and after the loop. */
	before_loop_np = head_label_np->n_prev;
	after_loop_np  = tail_label_np->n_next;

	/* Step 1:
	 *	Insert the new label, <Lnew1> after the delay slot of the
	 *	first branch.
	 */
	lnew1_np = append_unique_label_after_node(head_branch_np->n_next,
						  OPT_LOOPINV);

	/* The loop should now look like:
	 *
	 *		<any node>		<-- before_loop_np
	 *	Lhead:				<-- head_label_np
	 *		<the loop-test code>
	 *		b<cond> Ltail		<-- head_branch_np
	 *		<delay slot 1>
	 *	Lnew1:				<-- lnew1_np
	 *		<loop body>
	 *		b{,a}  Lhead		<-- tail_branch_np
	 *		<delay slot 2>
	 *	Ltail:				<-- tail_label_np
	 *		<any node>		<-- after_loop_np
	 */

	/* Step 2a:
	 *	    Copy the head label, "loop-test" code and the following
	 *          conditional branch to before the tail_label_np.  If any 
	 *          node in that segment is a label we "move" it to the
	 *          new place rather than "copying" it so all references to
	 *          the former label are automatically changed to the new
	 *          label.
	 *          
	 *
	 */
	{
		register Node *np, *nnp,   /* temp node pointers */
		              *last_insert, *first_insert;

		np = head_label_np->n_next;
		/*  move the head_label_np   */
		make_orphan_node(head_label_np);
		first_insert = head_label_np;
		last_insert = first_insert;
		last_insert->n_next = NULL;
		/* copy (or move) the remainning nodes */
		while (np != lnew1_np)
		{
		        if (NODE_IS_LABEL(np) || NODE_IS_EQUATE(np)) {
			      np = np->n_prev;
			      nnp = np->n_next;
			      make_orphan_node(nnp);
		        }
			else {
			      nnp = new_node(np->n_lineno,np->n_opcp);
			      copy_node(np, nnp, OPT_LOOPINV);
			      if (np == head_branch_np) head_branch_np = nnp;
			}
			np = np->n_next;
			last_insert->n_next = nnp;
			last_insert = nnp;
			last_insert->n_next = NULL;
	        }
		/* put them in the right place now */
		insert_before_node(tail_label_np,first_insert);
        }

	/* Step 2b:
	 *	    Retarget the new conditional branch to <Lnew1>.
	 */
	change_branch_target(head_branch_np, lnew1_np->n_operands.op_symp); 


	/* Step 2c:
	 *	    Reverse the sense of the new conditional branch.
	 */
	head_branch_np->n_opcp =
		reverse_condition_opcode(head_branch_np->n_opcp);

	/* Step 2d:
	 *	    Adjust the new conditional branch (and its delay slot)'s
	 *	    seq#, to prevent infinite loops of loop-inversion.
	 */
	head_branch_np->n_seq_no         = orig_tail_label_seqno;
	head_branch_np->n_next->n_seq_no = orig_tail_label_seqno;

#ifdef DEBUG
	if (head_branch_np->n_opcp == NULL)	internal_error("invert_loop(): no rev cond?");
#endif /*DEBUG*/

	/* The loop should now look like:
	 *
         *		<any node>		<-- before_loop_np
	 *		<the loop-test code>
	 *		b<cond> Ltail
	 *		<delay slot 1>
	 *	Lnew1:				<-- lnew1_np
	 *		<loop body>
	 *		b{,a}  Lhead		<-- tail_branch_np
	 *		<delay slot 2>
	 *	Lhead:
	 *              <the loop-test code>
	 *		b<rev-cond> Lnew1	<-- head_branch_np
	 *		<delay slot 1>
	 *	Ltail:				<-- tail_label_np
	 *		<any node>		<-- after_loop_np
	 */
	

	/* Step 3:
	 *	    Make sure we get rid of the UnConditional branch and its
	 *	    delay slot if it wasn't used.
	 */
	delay_slot_2_np = tail_branch_np->n_next;
	delay_slot_2_was_never_executed = BRANCH_IS_ANNULLED(tail_branch_np);

	if (  NODE_IS_NOP(delay_slot_2_np) ||
	        delay_slot_2_was_never_executed )
	{
		delete_node(delay_slot_2_np); 
	}
	delete_node(tail_branch_np); 
	tail_branch_np = lnew1_np->n_prev->n_prev;
	tail_branch_np->n_seq_no = orig_head_label_seqno;

	/* "Ta-Da"!  The loop should now look like:
	 *
	 *		<any node>		<-- before_loop_np
	 *		<the loop-test code>    
	 *		b<cond> Ltail           <-- tail_branch_np
	 *		<delay slot 1>
	 *	Lnew1:				<-- lnew1_np
	 *		<loop body>
	 *	Lhead:
	 *              <the loop-test code>
	 *		b<rev-cond> Lnew1	<-- head_branch_np
	 *		<delay slot 1>
	 *	Ltail:				<-- tail_label_np
	 *		<any node>		<-- after_loop_np
	 */

	/* The next node to look at will be the one following the end of this
	 * loop.   Nested loops will be inverted on subsequent iterations;
	 * this is intentionally done yet so that branches-to-branches we may
	 * have created here may be unscrambled before doing another loop
	 * inversion.  Otherwise, we can get into an infinite loop in this
	 * function.
	 */
	*new_next_npp = after_loop_np;

	optimizer_stats.loops_inverted++;
	changed = TRUE;

	return changed;
}


static void
resequence_nodes(start_np, end_np, seq_no)
	register Node *start_np;	/* The starting node */
	register Node *end_np;
	register NODESEQNO seq_no;
{
	/* This routine gives linear sequence#s to the nodes in the list.
	 * It is used so that the loop-inversion optimization can ascertain
	 * the relative positions of two nodes on the node list.
	 */
	register Node *np;

	for (np = start_np;  /* test at bottom of loop */;  np = np->n_next)
	{
		np->n_seq_no = seq_no++;
		if (np == end_np)	break;
	}
}


Bool
invert_loops(marker_np)
	register Node *marker_np;
{
	register Node *np;
	register Bool changed;
	Node *next_np;

	changed = FALSE;

	if ( do_opt[OPT_LOOPINV] )
	{
		/* Assign linear sequence numbers to all nodes in the list. */
		resequence_nodes(marker_np, marker_np->n_prev, (NODESEQNO)0);

		/* Now, invert any potential code loops in the node list.
		 * (why we would want to do so is explained in the
		 *  invert_a_loop() routine).
		 */
		for ( np = marker_np->n_next;
		      next_np = np->n_next, np != marker_np;
		      np = next_np
		    )
		{
			if ( NODE_IS_BRANCH(np) )
			{
				if (invert_a_loop(np, &next_np))  changed=TRUE;
			}
		}
	}

#ifdef DEBUG
	opt_dump_nodes(marker_np, "after invert_loops()");
#endif /*DEBUG*/
	return changed;
}

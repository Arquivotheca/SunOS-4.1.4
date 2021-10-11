static char sccs_id[] = "@(#)opt_simplify_annul.c	1.1\t10/31/94";

#include <stdio.h>
#include "structs.h"
#include "globals.h"
#include "node.h"
#include "optimize.h"
#include "opt_utilities.h"
#include "opt_statistics.h"


Bool
branch_simplify_annul(cond_branch_np, new_next_npp)
	register Node *cond_branch_np;
	register Node **new_next_npp;
{
	register Node *delay_slot_np;
	register Node *after_delay_slot_np;
	Bool	changed;

	/* "cond_branch_np" refers to a Conditional branch node, perhaps with
	 * its Annul bit set.  If its Annul bit is set and the node immediately
	 * following its delay slot is a duplicate instruction of the one in
	 * the delay slot, then the extra instruction is superfluous if we drop
	 * the Annul bit.
	 * So, delete the duplicate instruction and drop the Annul bit.
	 *
	 * This situation typically occurs due to Instruction Scheduling.
	 *
	 * The pattern is:
	 *		b<cond>,a	Lx	<-- cond_branch_np
	 *		<instruction>		<-- delay_slot_np
	 *		<instruction>		<-- after_delay_slot_np
	 *
	 * Change it to:
	 *		b<cond>		Lx	<-- cond_branch_np
	 *		<instruction>		<-- delay_slot_np
	 *
	 */	

	/* See if we really DO have an conditional branch which is annulled,
	 * followed by two equivalent instructions.
	 */
	if ( do_opt[OPT_SIMP_ANNUL]
			&&
	     BRANCH_IS_ANNULLED(cond_branch_np)
			&&
	     ( delay_slot_np = next_instruction(cond_branch_np),
	       /* Do NOT use next_instruction() here; there cannot be an
		* intervening label between the delay slot instruction and the
		* following instruction.
		*/
	       after_delay_slot_np =
			      next_code_generating_node_or_label(delay_slot_np),
	       nodes_are_equivalent(delay_slot_np, after_delay_slot_np)
	     )
	   )
	{
		/* Bingo. Drop the Annul bit and delete the extra instruction.*/
		cond_branch_np->n_opcp =
			branch_with_toggled_annul_opcp(cond_branch_np->n_opcp);

		delete_node(after_delay_slot_np);

		optimizer_stats.conditional_annuls_simplified++;

		*new_next_npp = delay_slot_np->n_next;
		
		changed = TRUE;
	}
	else	changed = FALSE;

	return changed;
}

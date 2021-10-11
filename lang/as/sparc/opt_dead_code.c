static char sccs_id[] = "@(#)opt_dead_code.c	1.1\t10/31/94";
/*
 * This file contains code which performs SPARC code optimizations
 * related to dead-code elimination.
 */

#include <stdio.h>
#include "structs.h"
#include "node.h"
#include "optimize.h"
#include "opt_statistics.h"


/*
 * delete_dead_code() deletes code inclusively from the given node (which is
 * assumed to be the node just after the delay slot of a CTI) up through
 * the node just before the next label.  Non-code-generating pseudo-ops are
 * left untouched, though.
 *
 * If anything was deleted, it returns:
 *	[1] TRUE as the function value
 *	[2] a pointer to the next node after the deleted code (a label), in the
 *		node-pointer pointed to by its second argument
 * Otherwise, it returns:
 *	[1] FALSE as the function value
 *	[2] The node-ptr pointed to by its second argument is left untouched.
 */
Bool
delete_dead_code(np, new_next_npp)
	register Node *np;
	register Node **new_next_npp;
{
	register enum functional_group fg;
	register Bool changed;
	register Node *next_np;
	/* "next_np" is used because after delete_node(p), "p" has been
	 * deallocated, and nobody really guarantees us that the contents of
	 * "p" (specifically, p->n_next) are still good.  (Yes, I know,
	 * practical experience says otherwise, but better safe now than sorry
	 * later!)
	 */

	if ( !do_opt[OPT_DEADCODE] )	return FALSE;

	/* If this is a CSWITCH immediately following a CTI, then the C-Switch
	 * was "part of" the CTI and cannot be deleted.
	 *
	 * There really "should" always be a label there (which would stop the
	 * loop below) anyway, but just in case...
	 */
	if ( NODE_IS_CSWITCH(np) ||
	     (NODE_IS_LABEL(np) && NODE_IS_CSWITCH(np->n_next))
	   )	return FALSE;

	/* Stop deleting when we get to a label, or the end of the list. */
	for ( changed = FALSE;
	      next_np = np->n_next, fg = np->n_opcp->o_func.f_group,
		( (fg != FG_LABEL) && (fg != FG_MARKER) );
	      np = next_np
	    )
	{
		if ( NODE_GENERATES_CODE(np) && !NODE_IS_PSEUDO(np) )
		{
			/* Here, we have a node which:
			 *	a) is in a section of dead code,
			 *  and b) generates code (as opposed to a pseudo-op
			 *		which can't be deleted),
			 *  and c) is not part of C "switch" code which
			 *		shouldn't be deleted.
			 */
			delete_node(np);
			changed = TRUE;
			optimizer_stats.dead_code_deleted_after_uncond_CTIs++;
		}
	}

	if (changed)	*new_next_npp = next_np;

	return changed;
}

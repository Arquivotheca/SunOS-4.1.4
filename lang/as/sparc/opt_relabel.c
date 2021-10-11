static char sccs_id[] = "@(#)opt_relabel.c	1.1\t10/31/94";
/*
 * This file contains code which relabels SPARC code, which is done during
 * optimization.
 */

#include <stdio.h>
#include "structs.h"
#include "bitmacros.h"
#include "node.h"
#include "label.h"
#include "optimize.h"
#include "opt_statistics.h"


static Node *
find_last_label_in_group(np)
	register Node *np;
{
	register Node *last_np;
	register Node *last_label_np;
	register Node *global_label_np;

	/* Find the last label in this group of adjacent labels.
	 * While looking for the last one, note if any of the ones
	 * which were passed were definitions of GLOBAL labels.
	 */
	for (last_np = last_label_np = np, global_label_np = NULL;
	     /* comparison in body of loop, not here */ ;
	     last_np = last_np->n_next)
	{
		if ( NODE_IS_LABEL(last_np) )
		{
			last_label_np = last_np;

			if (BIT_IS_ON(last_label_np->n_operands.op_symp->s_attr,
					SA_GLOBAL))
			{
				global_label_np = last_label_np;
			}
		}

		/* Leave last_label_np pointing at the last LABEL node in
		 * the group; i.e. stop when we hit a code-generating node
		 * (or, of course, the end of the node list).
		 *
		 * This check is here instead of in the "for" loop
		 * header so that the above test for "global" will
		 * also be done on the last node in the group.
		 */
		if ( NODE_GENERATES_CODE(last_np->n_next) ||
		     NODE_IS_MARKER_NODE(last_np->n_next) )
		{
			break;
		}
	}

	/* If any one of the labels was indeed a GLOBAL one (and it
	 * wasn't the LAST one), move the GLOBAL label to make it the
	 * LAST one in the list.  This is because we can't eliminate a
	 * global label but we CAN delete a local one.
	 */
	if ( (global_label_np != NULL) && (global_label_np != last_label_np) )
	{
		/* if it was originally the first label of the group,
		 * we'd better re-adjust the pointer to the first node.
		 */
		if (global_label_np == np)	np = np->n_next;

		/* move the global label to after the previous "last"
		 * one in the group.
		 */
		make_orphan_node(global_label_np);
		insert_after_node(last_label_np, global_label_np);

		/* now the global label is the new last one */
		last_label_np = global_label_np;
	}

	return last_label_np;
}


/* relabel():
 *	- finds all unreferenced labels and deletes them
 *	- finds all instances of multiple labels in a row and collapses them
 *		into one label.
 * relabel() returns TRUE if any changes were made.
 */
Bool
relabel(marker_np)
	register Node *marker_np;
{
	register Node *label_ref_np;
	register Node *label_ref_next_np;
	register Node *last_label_np;
	register Node *np;
	register struct symbol *last_label_symp;
	register Bool changed;
	register Node *next_np;

	/* "next_np" is used because after delete_node(p), "p" has been
	 * deallocated, and nobody really guarantees us that the contents of
	 * "p" (specifically, p->n_next) are still good.  (Yes, I know,
	 * practical experience says otherwise, but better safe now than sorry
	 * later!)
	 */

	changed = FALSE;
	for ( np = marker_np->n_next;
	      next_np = np->n_next, np != marker_np;
	      np = next_np
	    )
	{
		if ( !NODE_IS_LABEL(np) )	continue;

		/* Find the last label in this group of adjacent labels,
		 * moving a global label to the end if there are any.
		 */
		last_label_np   = find_last_label_in_group(np);
		last_label_symp = last_label_np->n_operands.op_symp;

		/* For all labels but the last, retarget all references to
		 * that label to refer to the last label.
		 * Exception: if the label is a Global label, do NOT retarget
		 * any references to it.
		 */
		for ( /*no loop init*/;
		      next_np = np->n_next, np != last_label_np;
		      np = next_np
		    )
		{
			/* If this is not a label (i.e. is some other non-code-
			 * generating node in between labels), or (as stated
			 * above) if it's a Global label, do NOT retarget any
			 * references to it.
			 */
			if ( !NODE_IS_LABEL(np) ||
			     BIT_IS_ON(np->n_operands.op_symp->s_attr,
					SA_GLOBAL))
			{
				continue;
			}

			/* Retarget each reference to the current label, to
			 * make it instead refer to the last label in the group.
			 * This leaves the current label with no references to
			 * it, so we can then delete it.
			 */
			for ( label_ref_np = np->n_operands.op_symp->s_ref_list;
			      label_ref_np != NULL;
			      label_ref_np = label_ref_next_np
			    )
			{
				label_ref_next_np = label_ref_np->n_symref_next;

				/*
				 * This call could be made here, but since ALL
				 * references to the label are getting zapped,
				 * it is much more efficient to just set the
				 * label symbol's s_ref_list=NULL and its
				 * s_ref_count=0 after the end of the loop.
				 *
				 * delete_node_reference_to_label(label_ref_np,
				 *	label_ref_np->n_operands.op_symp);
				 *
				 */
				label_ref_np->n_operands.op_symp =
							last_label_symp;
				node_references_label(label_ref_np,
							last_label_symp);
			}

			/* (see comment inside above loop regarding the two
			 *  following statements)
			 */
			np->n_operands.op_symp->s_ref_count = 0;
			np->n_operands.op_symp->s_ref_list  = NULL;

			/* Now delete the extraneous label node (and the symbol
			 * it defined).
			 */
			delete_node(np);

			changed = TRUE;
			optimizer_stats.redundant_labels_deleted++;
		}

		/* Now we are looking at the last label in a group of them.
		 * Delete IT if it is unreferenced, and isn't a Global symbol.
		 */
		if ( (last_label_symp->s_ref_count == 0) &&
		     BIT_IS_OFF(last_label_np->n_operands.op_symp->s_attr,
					SA_GLOBAL)
		   )
		{
			delete_node(last_label_np);

			changed = TRUE;
			optimizer_stats.unreferenced_labels_deleted++;
		}
	}

#ifdef DEBUG
			opt_dump_nodes(marker_np, "after relabel()");
#endif /*DEBUG*/
	return changed;
}

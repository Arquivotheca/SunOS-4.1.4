static char sccs_id[] = "@(#)node.c	1.1\t10/31/94";
#include <stdio.h>
#include "structs.h"
#include "actions.h"
#include "globals.h"
#include "lex.h"
#include "alloc.h"
#include "node.h"
#include "disassemble.h"
#include "label.h"
#include "opcode_ptrs.h"

static Node *markp     = NULL;	/* ptr to marker node for current segment    */

void
set_node_info(curr_markp)
	Node *curr_markp;
{
	markp = curr_markp;
}

Node *
new_node(lineno, opcp)
	register LINENO lineno;
	register struct opcode *opcp;
{
	register Node *np;

	np = alloc_node_struct();
	bzero((char *)np, sizeof(Node));
	np->n_lineno = lineno;
	np->n_opcp   = opcp;

	return np;
}


void
free_node(np)
	register Node * np;
{
	/* If the node uses a value-list, then free it before we free the node*/
	if (np->n_opcp->o_func.f_node_union == NU_VLHP)
	{
		free_value_list(np->n_vlhp);
	}

	free_node_struct(np);
}


Node *
new_marker_node(segname)
	char *segname;	/* name of the segment to which this list belongs */
{
	register Node *np;

	np = new_node(0, opcode_ptr[OP_MARKER]);

	np->n_internal = TRUE;
	np->n_string   = segname;

	/* initialize the doubly-linked list to be circular */
	np->n_next = np;
	np->n_prev = np;

	return np;
}


void
nodes_init()
{
}


Node *
first_node()
{
	/* return ptr to first real node (not the marker node) */
	return  markp->n_next;
}


/* fill_back_links(head_np):
 *	Given a singly-linked list of nodes, pointed to by "head_np", this
 *	function will fill in the back-links to form a doubly-linked list,
 *	and return a pointer to the last element in the list.
 */
static Node *
fill_back_links(head_np)
	register Node *head_np;	/* head of given list of nodes */
{
	register Node *tail_np;	/* tail of given list of nodes */

	/* this loop does two things:
	 *	-- traverses the list to find the last element
	 *	-- sets up all of the internal back-links (n_prev) in the list
	 */
	for ( tail_np = head_np;
	      tail_np->n_next != NULL;
	      tail_np = tail_np->n_next
	    )
	{
		tail_np->n_next->n_prev = tail_np;
	}

	return  tail_np;
}


/* insert_after_node(p, head_np):
 *	Adds a (list of) nodes AFTER the given node "p".
 *	Assumes that if there is more than one node, the n_next fields connect
 *	them in a singly-linked list.  This function will fill in the back-links
 *	to form the doubly-linked list, and insert the list.
 */
void
insert_after_node(p, head_np)
	register Node *p;	/* node after which to insert  */
	register Node *head_np;	/* head of given list of nodes */
{
	register Node *tail_np;	/* tail of given list of nodes */

	tail_np = fill_back_links(head_np);

	head_np->n_prev = p;
	tail_np->n_next = p->n_next;

	head_np->n_prev->n_next = head_np;
	tail_np->n_next->n_prev = tail_np;
}


/* insert_before_node(p, head_np):
 *	Adds a (list of) nodes BEFORE the given node "p".
 *	Assumes that if there is more than one node, the n_next fields connect
 *	them in a singly-linked, null-terminated list.  This function will fill
 *	in the back-links to form the doubly-linked list, and insert the list.
 */
void
insert_before_node(p, head_np)
	register Node *p;	/* node before which to insert  */
	register Node *head_np;	/* head of given list of nodes */
{
	register Node *tail_np;	/* tail of given list of nodes */

	tail_np = fill_back_links(head_np);

	head_np->n_prev = p->n_prev;
	tail_np->n_next = p;

	head_np->n_prev->n_next = head_np;
	tail_np->n_next->n_prev = tail_np;
}


static void
add_optim_node(optim_np, last_proc_np)
	register Node *optim_np;
	register Node *last_proc_np;
{
	/* We always want to place an ".optim" node immediately after the
	 * preceeding ".proc" node.  If there was a previous ".optim" node
	 * there, zap it, since the new one overrides it.
	 */

	if (last_proc_np == NULL)
	{
		internal_error(".optim before first .proc");
		/*NOTREACHED*/
	}

	if ( (last_proc_np->n_next != NULL) &&
	     NODE_IS_OPTIM_PSEUDO(last_proc_np->n_next)
	   )
	{
		/* The .proc node already has a .optim node after it.
		 * Delete it; it will be replaced by the new one, below.
		 */
		delete_node(last_proc_np->n_next);
	}

	insert_after_node(last_proc_np, optim_np);
}


/* add_nodes(head_np):
 *	Adds a (list of) nodes onto the very END of the list.
 *	This assumes that if there is more than one node, the n_next fields
 *	connect them in a singly-linked, null-terminated list.
 *	insert_before_node() will fill in the back-links to form the
 *	doubly-linked list.
 */
void
add_nodes(head_np)
	register Node *head_np;	/* head of given list of nodes */
{
	if (head_np == NULL)
	{
		/* ignore any request with a NULL ptr (which we get sometimes
		 * when there has been an error in the input, because using
		 * NULL is easier to pass than a pointer to a fake node).
		 */
	}
	else
	{
		if ( NODE_IS_SEG_PSEUDO(head_np) )
		{
			/* The node(s) to be added begin with a ".seg"
			 * node; therefore, add the nodes ONE AT A TIME
			 * to the correct segment(s)'s list(s).
			 * (insert_before_node() adds them all to one segment).
			 *
			 * We are ASSUMING here that we'll never get a
			 * list of nodes which has an embedded ".seg" but
			 * doesn't START with a ".seg".
			 *
			 * (This whole mess is required because of the
			 *  ".reserve" pseudo-op)
			 */

			register Node *np, *next_np;

			for ( np = head_np;   np != NULL;   np = next_np )
			{
				next_np = np->n_next;

				if ( NODE_IS_SEG_PSEUDO(np) )
				{
					change_named_segment(np->n_string);
					switch (assembly_mode)
					{
					case ASSY_MODE_BUILD_LIST:
						/* Just change the segment;
						 * don't add the ".seg" node to
						 * the list -- the marker node
						 * will suffice for that.
						 */
						 break;
					case ASSY_MODE_QUICK:
						/* Maybe disassemble it.  */ 
						if (do_disassembly)
						{
							disassemble_node(np);
						}
						break;
					}
					/* Might as well delete this node. */
					delete_node(np);
				}
				else
				{
					switch (assembly_mode)
					{ 
					case ASSY_MODE_BUILD_LIST:
						/* Go ahead and add the node.
						 * "np->n_next = NULL" so ONLY
						 * this ONE node will get
						 * added/emitted by the call to
						 * insert_before_node().
						 */
						np->n_next = NULL;

						insert_before_node(markp, np);
						break;
					case ASSY_MODE_QUICK:
						/* Emit the node NOW. */
						emit_node(np);
						/* And delete it. */
						delete_node(np);
						break;
					}
				}
			}
		}
		else
		{
			static Node *last_proc_np = (Node *)NULL;
			register Node *np, *next_np;

			/* go ahead and add or emit the whole list */
			switch (assembly_mode)
			{    
			case ASSY_MODE_BUILD_LIST:

				/* go ahead and add the whole list */

				if ( NODE_IS_PROC_PSEUDO(head_np) )
				{
					/* remember the location of the last
					 * ".proc" node, for ".optim".
					 */
					last_proc_np = head_np;
				}

				if ( NODE_IS_OPTIM_PSEUDO(head_np) )
				{
					/* (we assume that there is only one
					 *  node on this head_np list!)
					 */
					add_optim_node(head_np, last_proc_np);
				}
				else
				{
					insert_before_node(markp, head_np);
				}
				break;

			case ASSY_MODE_QUICK:
				/* emit the node(s) NOW */

				for (np = head_np;  np != NULL;  np = next_np)
				{
					next_np = np->n_next;
					emit_node(np);
					delete_node(np); /* ...and delete it. */
				}
				break;
			}
		}
	}
}


/* make_orphan_node() undoes a node insertion,
 *	but does not zap the node itself.
 */
void
make_orphan_node(np)
	register Node *np;
{
	/* The tests for non-NULL are here in case we try to orphan a
	 * node which is already an orphan.  E.g., when deleting a node
	 * which was never added to a list.
	 */
	if (np->n_prev != NULL)	np->n_prev->n_next = np->n_next;
	if (np->n_next != NULL)	np->n_next->n_prev = np->n_prev;
	np->n_next = (Node *)NULL;
	np->n_prev = (Node *)NULL;
}


void
make_orphan_block(head_np, tail_np)
	register Node *head_np;
	register Node *tail_np;
{
	/* Make the block of nodes (in linked list) inclusive of "head_np"
	 * and "tail_np" into an "orphan" block; i.e. remove them from the
	 * list as a block, and link the list together where they were removed.
	 */
	head_np->n_prev->n_next = tail_np->n_next;
	tail_np->n_next->n_prev = head_np->n_prev;
	head_np->n_prev = (Node *)NULL;
	tail_np->n_next = (Node *)NULL;
}


/* delete_node()
 *	undoes a node insertion,
 *	deletes any node references to symbols,
 *	AND throws away the node, permanently.
 */
void
delete_node(np)
	register Node *np;
{
	/* First, make sure that it's an orphan node. */
	make_orphan_node(np);

	/* Then, deal with any symbol references or value lists */
	switch ( np->n_opcp->o_func.f_node_union )
	{
	case NU_OPS:
		delete_node_symbol_reference(np, np->n_operands.op_symp);
		break;
	}

	/* Then free the node itself, and the value-list associated with
	 * it, if any.
	 */
	free_node(np);
}

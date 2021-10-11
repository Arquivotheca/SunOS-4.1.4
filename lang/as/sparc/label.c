static char sccs_id[] = "@(#)label.c	1.1\t10/31/94";
/*
 * This file contains code associated with maintaining the lists of label
 * definitions and references.
 */

#include <stdio.h>
#include "structs.h"
#ifdef DEBUG
#  include "sym.h"
#endif /*DEBUG*/
#include "bitmacros.h"
#include "errors.h"
#include "globals.h"
#include "node.h"

/*
 * Labels are always referenced in nodes via the "n_operands.op_symp"
 * pointer.  They should only be used in nodes where:
 *  ( (n_opcp->o_func.f_node_union == NU_OPS) && (n_operands.op_symp != NULL) )
 */

#ifdef FIX_STAB_LABEL_REFERENCES    /* [see Bugtraq bugid #1011656] */
/*
 * The above is actually untrue, and hides a lurking bug.
 * (n_opcp->o_func.f_node_union == NU_VLHP) nodes can also reference labels.
 * Read on...
 *
 * ".stabn" and ".stabs" nodes also actually refer to labels, but through
 * the "v_symp" element of one of the "n_vlhp" list of values (i.e. pseudo-op
 * arguments) hanging off of their node structure -- not through
 * "n_operands.op_symp", which is assumed by the routines in this file.
 *
 * Currently, STAB references to labels aren't recorded when created via a
 * call to node_references_label(), although they should be.  (See the
 * #ifdef'd-out code in actions_instr.c which would do so).
 *
 * If STABN's or STABS's are present, disassembly ("-S") is turned on, and
 * label nodes to which the STAB's refer are deleted (due to peephole
 * optimization), the assembler coredumps in disassemble.c while trying to
 * print out a (long-gone) label name.
 *
 * Note that we have never encountered this problem before, mostly out of
 * dumb luck.  Tracking label references is only important during optimization,
 * so we know whether it's safe to delete a label node or not. STAB's are
 * normally only present if the source file was compiled with the "-g" flag.  
 * When the compiler and assembler are run under the Sun compiler driver,
 * the driver happens to force the above to be mutually-exclusive events; i.e.
 * "-g" and "-O" aren't allowed.  However, a relatively innocuous sequence of
 * events can bring about an assembler coredump.  Example:
 *	% cd /usr/src/lang/as/sparc
 *	% cc -S -g -DDEBUG -DCHIPFABS -DIU_FABNO=2 -DFPU_FABNO=3 \
 *		-DSUN3OBJ obj.c  -o /tmp/obj.s
 *	% as -S -O1 /tmp/obj.s  -o /dev/null >/dev/null
 *	Segmentation fault (core dumped)
 *	% 
 *
 * When the #ifdef is removed from around the routine call in actions_instr.c,
 * and these routines are converted, all should be OK.
 *
 * [An alternative is that if optimization_level > 0 and a STABS or STABN is
 *  seen, the assembler could print an abusive message and exit(1).  This is
 *  not the preferred approach, but is better than coredumping].
 */
#endif /*FIX_STAB_LABEL_REFERENCES*/


void
node_defines_label(np, symp)	/* mark that a node DEFINES a label */
	register Node *np;
	register struct symbol *symp;
{
	/* Make the symbol structure point to the defining Label node. */
	if ( TRACKING_LABEL_REFERENCES )
	{
		symp->s_def_node = np;
	}
}

void
node_references_label(np, symp)	/* mark that a node REFERENCES a label */
	register Node *np;
	register struct symbol *symp;
{
	if ( (symp != NULL) && TRACKING_LABEL_REFERENCES )
	{
		/* insert the node onto the beginning of the list of nodes which
		 * reference this label symbol.
		 */
#ifdef DEBUG
		if (np == symp->s_ref_list)	internal_error("node_references_label(): dup ref");
#endif
		np->n_symref_next = symp->s_ref_list;
		symp->s_ref_list = np;

		/* ...and increment the count of the # of nodes on the list */
		symp->s_ref_count++;
	}
}


static void
delete_node_definition_of_label(symp)
	register struct symbol *symp;
{
	if ( TRACKING_LABEL_REFERENCES )
	{
		/* This node defines a symbol, and the node is being deleted.
		 * Deal with the symbol definition appropriately.
		 */
		if (symp->s_ref_count != 0)
		{
			/* There are still references to the label we're
			 * trying to delete; there is a problem here!
			 */
			internal_error( "delete_node_definition_of_label(): >0 ref's to \"%s\"",
			    symp->s_symname);
		}
		else if ( BIT_IS_OFF(symp->s_attr, SA_GLOBAL) )
		{
			/* This label isn't Global and there are no internal
			 * references left to it, so nobody will mind if we
			 * really DO delete it.
			 */
			sym_delete(symp->s_symname);
		}
	}
	else
	{
		/* Since we never entered references, there's nothing to
		 * delete.
		 */
	}
}


void
delete_node_reference_to_label(np, symp)
	register Node *np;
	register struct symbol *symp;
{
	register Node **npp;
#ifdef DEBUG
	register Bool got_it = FALSE;
#endif DEBUG

	/* If we aren't tracking label references at all, or there is no
	 * symbol reference to delete, ignore this call.
	 */
	if ( (!TRACKING_LABEL_REFERENCES) || (symp == NULL) )	return;

	/* Find the node in the list of nodes which reference this label
	 * symbol.  When found, delete it from the list and adjust the
	 * count of nodes in the list.
	 */
	for ( npp = &(symp->s_ref_list);    *npp != NULL;
	      npp = &((*npp)->n_symref_next)
	    )
	{
		if ( *npp == np )
		{
			/* Found the node on the list. */

			/* Point the previous pointer to the next node. */
			*npp = np->n_symref_next;

			/* Decrement the count of the # of nodes on the list.
			 * If it's down to zero, delete the definition of the
			 * symbol, both its node (if any) and in the symbol
			 * table.
			 */
			if ( (--(symp->s_ref_count)) == 0 )
			{
				if (symp->s_def_node == NULL)
				{
					/* There was no definition node for the
					 * symbol; we must have just deleted the
					 * last reference to a Global symbol,
					 * or some other symbol for which there
					 * is no node present (e.g. an Equate).
					 * Just delete it from the symbol table.
					 */
					sym_delete(symp->s_symname);
				}
				else if (BIT_IS_OFF(symp->s_attr, SA_GLOBAL))
				{
					/* The label is a local one; delete
					 * the definition node of the symbol
					 * (i.e. its Label or Equate node),
					 * if any, which will also delete
					 * the symbol from the symbol table.
					 */
					delete_node(symp->s_def_node);
				}
			}
#ifdef DEBUG
			got_it = TRUE;
#endif /*DEBUG*/
			break;	/* out of "for"-loop */
		}
	}
#ifdef DEBUG
	if (!got_it)	internal_error("delete_node_reference_to_label(): missing ref in list for label \"%s\"", symp->s_symname);
#endif /*DEBUG*/
}


void
delete_node_symbol_reference(np, symp)
	register Node *np;
	register struct symbol *symp;
{
	/* This routine is used when we don't implicitly know whether the node
	 * in question defines the symbol referenced, or just refers to it.
	 * This routine figures out which is the case, from what type of node
	 * it is, and then calls the appropriate routine to delete the
	 * reference.
	 */
	if ( symp != NULL )
	{
		/* This node either references or defines a symbol, and the
		 * node is being deleted.  Deal with the symbol def/ref
		 * appropriately.
		 */

		switch (np->n_opcp->o_func.f_group)
		{
		case FG_LABEL:
		case FG_EQUATE:
			/* This node defines a label. */
			delete_node_definition_of_label(symp);
			break;
		
		default:
			/* This node references a label.
			 * Remove this node's reference to that label, and
			 * if this is the last reference, delete the label too.
			 */
			delete_node_reference_to_label(np, symp);
			break;
		}
	}
}


#ifdef DEBUG
#include "disassemble.h"
void
print_sym_list_node(np)
	register Node *np;
{
	if (np == NULL)	printf("\t%5s:\t", "");
	else		printf("\t%5u:\t", np->n_lineno);
	disassemble_node(np);
}

int
print_sym_list(symname)
	char *symname;
{
	register struct symbol *symp;
	register Node *np;

	if ( (symp = sym_lookup(symname)) == NULL )
	{
		printf("[\"%s\" not found]", symname);
	}
	else
	{
		printf("defined:\n");
		print_sym_list_node(symp->s_def_node);
		printf("referenced [count = %d]:\n", symp->s_ref_count);
		for (np=symp->s_ref_list;  np != NULL;  np=np->n_symref_next)
		{
			print_sym_list_node(np);
		}
	}

	return 0;
}
#endif /*DEBUG*/

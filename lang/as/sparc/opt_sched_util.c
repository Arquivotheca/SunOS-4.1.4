#ifndef lint
static	char sccsid[] = "@(#)opt_sched_util.c 1.1     94/10/31";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * utility routines for instruction scheduling.
 */

#include <stdio.h>
#include "structs.h"
#include "globals.h"
#include "alloc.h"
#include "registers.h"
#include "opt_utilities.h"
#include "opt_scheduling.h"

static LIST_NODE *free_list_node = (LIST_NODE *)NULL;
#define CHUNK 100

void
append_node( np ) register Node *np;
{ /*
   * append the np after last_node
   * and mark the np as the last_node.
   * THERE IS SIDE EFFECT on last_node.
   */
	/* first, take the np out the node list. */
	np->n_next->n_prev = np->n_prev;
	np->n_prev->n_next = np->n_next;

	/* now, put it after the last_node */
	np->n_next = last_node->n_next;
	np->n_prev = last_node;
	last_node->n_next->n_prev = np;
	last_node->n_next = np;
	last_node = np;
}

Node *
swap_node( cti_node, delay_node ) register Node *delay_node, *cti_node;
{
/*
 * put delay_node in front of cti_node
 */
	register Node *np;

	np = cti_node->n_prev;
	np->n_next = cti_node->n_prev = delay_node;
	delay_node->n_prev = np;
	np = delay_node->n_next;
	np->n_prev = delay_node->n_next = cti_node;
	cti_node->n_next = np;
	return( delay_node );
}

LIST_NODE *
new_lnode() {
/*
 * Maintain a free list of new_lnode and return a at each call
 */
 	register int i;
	register LIST_NODE *lnp;

	if( !free_list_node ) { /* There is no more free node */
		free_list_node = (LIST_NODE *)as_calloc( CHUNK, sizeof( LIST_NODE ) );
		for( i = 0, lnp = free_list_node; i < CHUNK-1; ++i, ++lnp ) {
			lnp->next = lnp+1; /* links the free nodes. */
		}
	}

	lnp = free_list_node;
	free_list_node = lnp->next;
	return lnp;
}

Bool
insert_lnode( np, nlpp ) Node *np; LIST_NODE **nlpp;
{ /*
   * This routine insert np on top
   * of *nlpp list and return TRUE if NOT in the *nlpp list,
   * otherwise do nothing and return FALSE.
   */

	register LIST_NODE *list_node;
	LIST_NODE *new_lnode();

	for( list_node = *nlpp ; list_node; list_node = list_node->next ) {
		/* make sure np is not in the *nlpp list */
		if( list_node->nodep == np ) {
			return FALSE;
		}
	}

	list_node = new_lnode();
	list_node->nodep = np;
	list_node->next = *nlpp;
	*nlpp = list_node;
	return TRUE;
}

void
delete_lnode( head, pre_node, node ) LIST_NODE **head, *pre_node, *node;
{ /*
   * deleting the node which follows
   * pre_node (if pre_node is NOT NULL)
   * from the *head list.
   */

	if( pre_node != (LIST_NODE *)NULL ) {  /* deleting middle one */
		pre_node->next = node->next;

		/* put node n_prev into the free_list_node list */
		node->next = free_list_node;
		free_list_node = node;
		node->nodep = (Node *)NULL;
		return;
	}
	/*  deleting the head */
	*head = node->next;
	
	/* put node n_prev into the free_list_node list */
	node->next = free_list_node;
	free_list_node = node;
	node->nodep = (Node *)NULL;
	return;
}

void
delete_list( lnpp ) LIST_NODE **lnpp;
{ /*
   * deleting a list of lnode
   */
	register LIST_NODE *lnp;

	if( *lnpp == (LIST_NODE *)NULL )
		return;
		
	for( lnp = *lnpp; lnp->next; lnp = lnp->next ) {
		lnp->nodep = (Node *)NULL;
	}
	lnp->nodep = (Node *)NULL;
	lnp->next = free_list_node;
	free_list_node = *lnpp;
	*lnpp = (LIST_NODE *)NULL;
}

void
delete_free( lnp, is_fpu ) LIST_NODE *lnp; Bool is_fpu;
{ /*
   * deleting lnp from the free_iu_nodes or free_fpu_nodes.
   */
	register LIST_NODE *pre_lnode, *lnode, **free_node_addr;
	void delete_lnode();
	
	pre_lnode = (LIST_NODE *)NULL;
	if( is_fpu ) {
		free_node_addr = &free_fpu_nodes;
	} else {
		free_node_addr = &free_iu_nodes;
	}
	
	for( lnode = *free_node_addr; lnode; lnode = lnode->next ) {
		if( lnode->nodep == lnp->nodep ) { /* found */
		/* but lnode NOT necessary == lnp */
		/* DO DELETE lnode instead of lnp */
			delete_lnode( free_node_addr, pre_lnode, lnode );
			return;
		}
		pre_lnode = lnode;
	}
	/* NOT found */
	/* ERROR */
}
	
void
dump_depend_graph()
{ /*
   * print the dependences information for debugging.
   */
	register LIST_NODE *lp;
	void print_depend_node();

	printf("FREE IU NODE :\n");
	
	for( lp = free_iu_nodes; lp; lp = lp->next ) {
		print_depend_node( lp->nodep, 0);
	}

	printf("FREE FPU NODE :\n");
	
	for( lp = free_fpu_nodes; lp; lp = lp->next ) {
		print_depend_node( lp->nodep, 0 );
	}
}

void
print_depend_node( np, level ) register Node *np; register int level;
{
	register LIST_NODE *lp;
	register int i;
	Bool disassemble_node();

	for( i = 0; i < level; i++) {
		printf("   ");
	}
	(void) disassemble_node( np );

	for( i = 0; i < level; i++) {
		printf("   ");
	}
	printf("DEP : %d, FPU_DEP : %d, IU_DEP %d\n",
		np->dependences, np->inst_fpu_dep, np->inst_iu_dep );

	for( lp = np->depend_list; lp; lp = lp->next ) {
		print_depend_node( lp->nodep, level+1 );
	}
}

Bool
exchangable( cti_np, np ) register Node *cti_np, *np;
{
/*
 * np has delay slot (CALL, BR, JMP, RET).
 * see if the instruction in the delay slot can be moved
 * to ahead of np.
 */

	if( NODE_HAS_DELAY_SLOT( np ) ) {
		return FALSE;
	}
#ifdef CHIPFABS
	if( iu_fabno == 1 && NODE_IS_LOAD( np ) ) {
		return FALSE;
	}
#endif
	if( NODE_IS_BRANCH(cti_np) ) {
		if( BRANCH_IS_ANNULLED(cti_np) ) {/* do NOT even try */
			return FALSE;
		}
		if( BRANCH_IS_UNCONDITIONAL(cti_np) ) {/* alway ok */
			return TRUE;
		} 
		/* conditional branch withOUT annuled */
		if( NODE_SETSCC(np) ){
			return FALSE;
		} else { /* instruction does not set cc will be ok to move */
			 /*  ahead of the CTI */
			return TRUE;
		}
	}
	if( NODE_IS_CALL( cti_np ) ) {
               if (OP_FUNC(cti_np).f_group == FG_CALL){
                        if( REG_IS_READ(np, RN_CALLPTR ) ||
                            REG_IS_WRITTEN(np, RN_CALLPTR ) ) {
                                return FALSE;
                        } else {
                                return TRUE;
                        }
                } else { /* this is really a jmpl */
                        if( REG_IS_READ(np, cti_np->n_operands.op_regs[O_RD]) ||
                            REG_IS_WRITTEN(np, cti_np->n_operands.op_regs[O_RD]) ||
                            REG_IS_READ(np, cti_np->n_operands.op_regs[O_RS1]) ||
                            REG_IS_WRITTEN(np, cti_np->n_operands.op_regs[O_RS1]) ||
                            REG_IS_READ(np, cti_np->n_operands.op_regs[O_RS2]) ||
                            REG_IS_WRITTEN(np, cti_np->n_operands.op_regs[O_RS2]) ) {
                                return FALSE;
                        } else {
                                return TRUE;
                        }
                }
        }
	if( NODE_IS_RET( cti_np ) ) {/**** forget it for now 10/9/86 **** */
		if ( REG_IS_READ(np, RN_RETPTR) || REG_IS_WRITTEN(np, RN_RETPTR)){
			return FALSE;
		} else {
			return TRUE;
		}
	}
	if( NODE_IS_JMP( cti_np ) ) { /* if indirect jmp, check register
				       * difination */
		return FALSE;
	}
	return FALSE;
}

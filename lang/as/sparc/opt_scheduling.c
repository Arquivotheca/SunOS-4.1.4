#ifndef lint
static	char sccsid[] = "@(#)opt_scheduling.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * This file contains the major routiens and the driver of doing
 * instruction scheduling.  First, it goes throught the basic
 * block and base on the resouce use and deifne information,
 * finds out the dependences between the instructions, then
 * fines the 'optimal' sequence of instructions.
 */

/* On SPARC, there are three things for the instruction scheduling to do:
 *	1. Maximizes the FPU and IU through put.
 *	2. Eliminates the interlock after the LOAD instructions.
 *	3. Fills the delay slot of the CTI instructions.
 */

#include <stdio.h>
#include "structs.h"
#include "opt_scheduling.h"
#include "opt_utilities.h"
#include "opcode_ptrs.h"
#include "registers.h"
#include "optimize.h"

RESOURCE resources[MAX_RESOURCES];
INDIRECT_POINTER indirect_pointer[MAX_GP];     /* use indirect_pointer[0] as "WILD" pointer */

Node *last_node, *branch_node;
LIST_NODE *free_iu_nodes, *free_fpu_nodes, *delay_slot_lnode;
LIST_NODE *branches_with_nop;
int limit_bb = 0;
int number_of_nodes, free_node_count;

void
scheduler(marker_np) register Node *marker_np;
{
	register Node *np, *nextp;
	register int bbno;
	register LIST_NODE *lp;
	Bool start_bb = TRUE;
	int bb_size;
	static Node *massage_delay_slot();
	static void begin_bb(), end_bb(), mark_memory_alias(), init_alias();
	void find_dependences(), resequences(), delete_node(),
		delete_list(), copy_target_to_delay_slot(), untangle_jumps();

	if( !do_opt[OPT_SCHEDULING] ) {
		return;
	}

	last_node = marker_np;
	init_alias();
	
	for( np = marker_np->n_next; np != marker_np; np = nextp ) {
#ifdef DEBUG
		if( limit_bb != 0 && bbno++ > limit_bb ) {
			break;
		}
#endif

		if( start_bb ) {
			begin_bb();
			start_bb = FALSE;
			bb_size = 0;
		}

		if( ( !NODE_GENERATES_CODE( np ) /* not interested */) ||
			( NODE_IS_PSEUDO( np ) ) ||
			( NODE_IS_CSWITCH(np) ) ) {/* There suppose NO code
					            *  between CSWITCHs and the
					            *  CSWITCH LABEL.
						    */
			if( NODE_IS_ALIAS( np ) ) {
				mark_memory_alias( np );
			}
			last_node = np;
		} else {
			if( NODE_IS_NOP( np ) ) {/* useless NOPs */
				nextp = np->n_prev;/* remembler the np->n_prev
						    *  before delete np */
				delete_node(np);
				np = nextp;/* np = original np->n_prev */
			} else {
				if( NODE_HAS_DELAY_SLOT(np) ) { /* CALL, JMP, BR, RET */
					np = massage_delay_slot( np );
					find_dependences( np );
					if( !NODE_IS_CALL(np) ) {
						branch_node = np;
						np = np->n_next;/*skip the delay slot*/
					} else {
						if( NODE_IS_UNIMP(np->n_next->n_next) ) {
					/* skip the delay slot AND the UNIMP instrcution */
							np = np->n_next->n_next;
						} else {
							np = np->n_next;/*skip the delay slot*/
						}
					}
				} else {
					find_dependences( np );
				}
			}
		}
		bb_size++;
		if( (branch_node != (Node *)NULL ) ||    /* original np is a BRANCH_NODE */ 
		    (NODE_IS_LABEL(np->n_next) )   ||    /* next np is a LABEL */
		    (NODE_IS_MARKER_NODE(np->n_next)) || /* end of procedure */
		    (bb_size == MAX_BB_SIZE) ) {          /* this is too long pholks */
			end_bb();
			/* MARK nextp before resequences() changes the order */
			nextp = np->n_next;
			resequences();
			start_bb = TRUE;
		} else {
			nextp = np->n_next;
		}
	}
	for( lp = branches_with_nop; lp; lp = lp->next ) {
	/* NOW try to fill up these delay slot */
		copy_target_to_delay_slot( lp->nodep, lp->nodep->n_next );
	}
	delete_list( &branches_with_nop );
	untangle_jumps(marker_np);
}

static void
begin_bb() {
	register i;
	register LIST_NODE *lnp;
	void delete_list();
	static void init_alias();
	
	free_iu_nodes = free_fpu_nodes = delay_slot_lnode = (LIST_NODE *)NULL;
	branch_node = (Node *)NULL;
	number_of_nodes = free_node_count = 0;
	
	for( i = 0; i < MAX_RESOURCES; i++ ) {
		delete_list( &resources[i].def_np );
		delete_list( &resources[i].use_np );
		resources[i].used = FALSE;
	}
	for( i = 0; i < MAX_GP; i++ ) {
		delete_list( &indirect_pointer[i].resources.def_np );
		delete_list( &indirect_pointer[i].resources.use_np );
		indirect_pointer[i].resources.used = FALSE;
	}
	init_alias();
}


static void
end_bb()
{ /*
   * We are at the end of a basic block.  We have to assume
   * that all the resources will be used outside this basic
   * block. Finally, we should clean up the free_iu_nodes, free_fpu_nodes.
   */
	register int i;
	register LIST_NODE *pre_node, *free_inst, *lnode, *next_node;
	Node *last_def, *np;
	void find_dependences(), delete_lnode(), compute_dependences();

	for( i = 0; i < MAX_RESOURCES; i++ ) {
	/* 
	 * has to assume that all the definitions 
	 * will be used outside this basic block.
	 * Do NOT need for MEMORY definition.
	 */
		if( resources[i].used == FALSE &&
				resources[i].def_np != (LIST_NODE *)NULL ) {
			last_def = resources[i].def_np->nodep;
			for( lnode = resources[i].def_np->next; lnode; lnode = lnode->next ) {
				add_depend( lnode->nodep, last_def );
			}
		}
	}
	/*
	 * take the nodes that have dependences
	 * and the branch_node out of free_iu_nodes
	 * and free_fpu_nodes.
	 */
	pre_node = (LIST_NODE *)NULL;
	for( free_inst = free_iu_nodes; free_inst; ) {
		if( free_inst->nodep->dependences ||
 		    free_inst->nodep == branch_node ) { /* not really FREE yet */
			next_node = free_inst->next;
			delete_lnode( &free_iu_nodes, pre_node, free_inst );
			free_inst = next_node;
		} else {
			pre_node = free_inst;
			free_inst = free_inst->next;
		}
	}

	pre_node = (LIST_NODE *)NULL;
	for( free_inst = free_fpu_nodes; free_inst; ) {
		if( free_inst->nodep->dependences ||
		   free_inst->nodep == branch_node ) { /* not really FREE yet */
			next_node = free_inst->next;
			delete_lnode( &free_fpu_nodes, pre_node, free_inst );
			free_inst = next_node;
		} else {
			pre_node = free_inst;
			free_inst = free_inst->next;
		}
	}
	/* Computes total (accumulated) dependences */
	for( free_inst = free_iu_nodes; free_inst; free_inst = free_inst->next) {
		compute_dependences( free_inst->nodep, ++free_node_count );
	}
	for( free_inst = free_fpu_nodes; free_inst; free_inst = free_inst->next) {
		compute_dependences( free_inst->nodep, ++free_node_count );
	}
}

static void
mark_memory_alias( np ) register Node *np;
{
	register int i, register_no1, register_no2;
	register LIST_NODE *lp;
	static void init_alias();
	
	if( np->n_opcp->o_func.f_subgroup == FSG_NOALIAS ) { /* .noalias %r1,%r2 */
		register_no1 = np->n_operands.op_regs[O_RS1];
		register_no2 = np->n_operands.op_regs[O_RS2];

		SET_NO_ALIAS( register_no1, register_no2 );
		SET_NO_ALIAS( register_no2, register_no1 );
	} else { /* .alias */
		init_alias();
		/* copy all the def and use to indirect_pointer[0] */
		for( i = 1; i < MAX_GP; i++ ) {
			if( i == RN_SP || i == RN_FPTR ) {
				continue;
			}
			for( lp = indirect_pointer[i].resources.def_np; lp; lp = lp->next ) {
				(void)insert_lnode( lp->nodep,
						&indirect_pointer[0].resources.def_np  );
			}
			delete_list( &indirect_pointer[i].resources.def_np );
			for( lp = indirect_pointer[i].resources.use_np; lp; lp = lp->next ) {
				(void)insert_lnode( lp->nodep,
						&indirect_pointer[0].resources.use_np  );
			}
			delete_list( &indirect_pointer[i].resources.use_np );
		}
	}
}

static void
init_alias(){
/*
 * Mark all the GP register are alias to each other.
 */
	register int i;

	for( i = 0; i < MAX_GP; i++ ) {
		indirect_pointer[i].no_alias = 0;
	}
	/* assume SP and FP ALWAYS points to different section of memory */
	SET_NO_ALIAS( RN_SP, RN_FPTR );
	SET_NO_ALIAS( RN_FPTR, RN_SP );
}

/*
 * massage_delay_slot:
 * This routine takes a Node pointer which points to a instrcution
 * with delay slot as parameter.  If there is a useful (anything
 * except nop) instrution in the delay slot, this routine will try
 * to move it before the CTI instruction and put a nop in the delay
 * slot so the scheduler sometimes can do a better job.  For example :
 *
 *		ld	....,reg1
 *		use	reg1
 *		branch
 *		use 	reg2
 *
 * can be translated to
 *		ld	....,reg1
 *		use	reg2
 *		branch
 *		use	reg1
 *
 * If this can NOT be done safely, it will "charge" all the usages and
 * difinations of the instruction in the delay slot to the CTI instead,
 * so the scheduler can treat CTI and delay slot as a singal instruction.
 */

static Node *
massage_delay_slot( np ) register Node *np;
{
	register Node *nop_np;
	void insert_after_ndoe();
	Bool exchangable();
	register_set regset_add();
	Node *swap_node(), *new_node();
	
	if( NODE_IS_NOP( np->n_next ) ) {
		return np;
	}
	if( ! exchangable( np, np->n_next ) ) { /* can NOT move */
		REGS_READ(np) = regset_add( REGS_READ(np), REGS_READ(np->n_next) );
		REGS_WRITTEN(np) = regset_add( REGS_WRITTEN(np), REGS_WRITTEN(np->n_next) );
		return( np );
	} else {
		np = swap_node( np, np->n_next ); /* np -> orginal delay slot inst. */
		find_dependences( np );
		nop_np = new_node(np->n_lineno, opcode_ptr[OP_NOP]);
		nop_np->n_internal = TRUE;  /* this is an internally-generated node */
		insert_after_node(np->n_next, nop_np);
		return( np->n_next ); /* original CTI instruction */
	}
}



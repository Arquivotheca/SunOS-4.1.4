#ifndef lint
static	char sccsid[] = "@(#)opt_depend.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include "structs.h"
#include "opt_scheduling.h"
#include "registers.h"
#include "globals.h"

#define KNOWN_OFFSET(np)	  ((((np)->n_operands.op_imm_used) && \
					((np)->n_operands.op_symp == (struct symbol *)NULL)) \
					|| ((!((np)->n_operands.op_imm_used)) && \
					((np)->n_operands.op_regs[O_RS2] == RN_G0 ) ) )

/*
 * this file contains all the routines that find
 * out the dependences of the instructions.
 */

/*
 * Algorithm:
 *	An instruction B has to follow instruction A if,
 *	in lexical order, B follows A and
 *	in general:
 *		1. B uses a resource which be defined by A OR
 *		2. B defines a resource which be used by A.
 *
 *	or for some special cases:
 *		3. B is a branch OR
 *		4. B defines a resource which will be used later
 *		   and A deinfed the same resource without be
 *		   used.  We can NOT delete A because resources
 *		   can be partially defined and/or A may defined
 *		   some other resources as well.
 */

void
find_dependences( np ) Node *np;
{
	register int i;
	register LIST_NODE *lp;
	static void use_resource(), def_resource(),
		    use_memory(), def_memory();
	
	np->inst_fpu_dep = -1;	/* initialized to no count */
	np->delay_candidate = 0;/* This field is valid only after np is FREE */
				/* Before that it'll be used internally for */
				/* compute_dependences */
	number_of_nodes += 1;
	
	for( i = 0; i < MAX_RESOURCES; i++ ) {
		if( REG_IS_READ( np, i ) ) {
			if( i == RN_MEMORY ) {
				use_memory( np, i );
			} else {
				use_resource( np, i );
			}
		}
		if( REG_IS_WRITTEN( np, i ) ) {
			if( i == RN_MEMORY ) {
				def_memory( np, i );
			} else {
				def_resource( np, i );
			}
		}
	}
}

static void
use_resource( np, i ) Node *np; int i;
{
	register Node *node;
	register LIST_NODE *def_node;
	Bool insert_lnode();
	void add_depend();
	void delete_list();

	if( resources[i].used == FALSE ) { /* this is the first use after last define */
		delete_list( &resources[i].use_np );
		resources[i].used = TRUE;
	}
	(void) insert_lnode( np, &resources[i].use_np );
	if( resources[i].def_np == (LIST_NODE *)NULL ) {
		/*
		 * no definition in this bb yet,
		 * so this is a POTENTIAL free node.
		 */
		if( NODE_ACCESSES_FPU(np) ) {
			(void) insert_lnode( np, &free_fpu_nodes );
		} else {
			(void) insert_lnode( np, &free_iu_nodes );
		}
		return;
	}

	/* use should follow the definition. */
	add_depend( resources[i].def_np->nodep, np ); 
	if( resources[i].use_np->next == (LIST_NODE *)NULL ) {
		/*
		 * This is the first usage of the last definition,
		 * last definition must follow previous definitions.
		 * See comment 4 at the begining of this file.
		 */
		node = resources[i].def_np->nodep;
		for(def_node = resources[i].def_np->next; def_node; def_node = def_node->next){
			add_depend( def_node->nodep, node );
		}
	}
}

static void
def_resource( np, i ) Node *np; int i;
{
	register Node *node;
	register LIST_NODE *use_node;
	void add_depend();
	void delete_list();
	Bool insert_lnode();

	if( resources[i].used == FALSE ) {
		if( resources[i].use_np ) {/* uses of previous definiation */
			for( use_node = resources[i].use_np; use_node;
						use_node = use_node->next ) {
				add_depend( use_node->nodep, np );
			}
		} else { /* potential free node */
			if( NODE_ACCESSES_FPU( np ) ) {
				(void) insert_lnode( np, &free_fpu_nodes );
			} else {
				(void) insert_lnode( np, &free_iu_nodes );
			}
		}
		(void) insert_lnode( np, &resources[i].def_np );
		return;
	}
	/* else, there are uses, so this np has to follow all the uses */
	for( use_node = resources[i].use_np; use_node; use_node = use_node->next ) {
		add_depend( use_node->nodep, np );
	}

	/* delete all the previous defininations and */
	/* reset the used flag */
	delete_list( &resources[i].def_np );
	resources[i].used = FALSE;
	
	if( REG_IS_READ(np, i) ) { /* if the current node (np) uses
				    * resources[i], then put the usage
				    * back into the resources[i].use_np
				    */
		(void) insert_lnode( np, &resources[i].use_np );
	}
	(void) insert_lnode( np, &resources[i].def_np );
	return;
}

void
add_depend( leader, follower ) Node *leader, *follower;
{
	register LIST_NODE *list_node;
	Bool insert_lnode();

	if( leader == follower ) {
		return;
	}
	
	if( leader == (Node *) NULL || follower == (Node *)NULL ) {
		return;
	}
	
	if( insert_lnode( follower, &(leader->depend_list) ) ) {/* it is new. */
		follower->dependences += 1;
	}
	return;
}

static void
use_memory( np ) Node *np;
{
	register int register_no;
	register int i;
	register LIST_NODE *lp, *def_lp;
	void add_depend();
	static int ref_ptr();
	static Bool overlap();
	Bool insert_lnode();
	
	register_no = ref_ptr( np );
	if( register_no == 0 ) { /* It's not a load or load thru WILD pointer */
		/* depends on ALL the definitions */
		for( i = 0; i < MAX_GP; i++ ) {
			for( lp = indirect_pointer[i].resources.def_np; lp; lp = lp->next ) {
				add_depend( lp->nodep, np );
			}
		}
		if( optimization_level < 3 ) {/* depend on the latest WILD use */
			if( indirect_pointer[0].resources.use_np ) {
				add_depend( indirect_pointer[0].resources.use_np->nodep, np );
				delete_list( &indirect_pointer[0].resources.use_np );
			}
		}
	} else { /* LOAD thur KNOWN pointer*/
		for( i = 0; i < MAX_GP; i++ ) {
			if( NO_ALIAS( register_no, i ) ) { /* ALWAYS alias with itself */
				continue;
			}
			for( lp = indirect_pointer[i].resources.def_np; lp; lp = lp->next ) {
				if( i != register_no || overlap( lp->nodep, np ) ) {
					add_depend( lp->nodep, np );
				}
			}
		}
	}
	(void) insert_lnode( np, &indirect_pointer[register_no].resources.use_np );
	if( np->dependences == 0 ) {
		if( NODE_ACCESSES_FPU(np) ) {
			(void) insert_lnode( np, &free_fpu_nodes );
		} else {
			(void) insert_lnode( np, &free_iu_nodes );
		}
	}
}

static void
def_memory( np ) Node *np;
{
	/* All the definitions have to depend on each 
	 * other if there are POSSIBLE overlapping.
	 * Define has to depend on all the previous
	 * uses if there are possible overlapping.
	 */
	register int register_no;
	register int i;
	register LIST_NODE *lp;
	void add_depend();
	static int ref_ptr();
	static Bool overlap();
	Bool insert_lnode();
	
	register_no = ref_ptr( np );
	if( register_no == 0 ) { /* It''s not a store or store thur WILD pointer */
		for( i = 0; i < MAX_GP; i++ ) {
			for( lp = indirect_pointer[i].resources.def_np; lp; lp = lp->next ) {
				add_depend( lp->nodep, np );
			}
			for( lp = indirect_pointer[i].resources.use_np; lp; lp = lp->next ) {
				add_depend( lp->nodep, np );
			}
		}
		delete_list( &indirect_pointer[0].resources.def_np );
		delete_list( &indirect_pointer[0].resources.use_np );
	} else { /* STORE thur KNOWN pointer */
		for( i = 0; i < MAX_GP; i++ ) {
			if( NO_ALIAS( register_no, i ) ) { /* ALWAYS alias with itself */
				continue;
			}
			for( lp = indirect_pointer[i].resources.def_np; lp; lp = lp->next ) {
				if( i != register_no || overlap( lp->nodep, np) ) {
					add_depend( lp->nodep, np );
				}
			}
			for( lp = indirect_pointer[i].resources.use_np; lp; lp = lp->next ) {
				if( i != register_no || overlap( lp->nodep, np) ) {
					add_depend( lp->nodep, np );
				}
			}
		}
	}
	(void) insert_lnode( np, &indirect_pointer[register_no].resources.def_np );
	if( np->dependences == 0 ) {
		if( NODE_ACCESSES_FPU(np) ) {
			(void) insert_lnode( np, &free_fpu_nodes );
		} else {
			(void) insert_lnode( np, &free_iu_nodes );
		}
	}
}

static int
ref_ptr( np ) register Node *np;
{
	register int base_reg;
	
	if( ( ! ( NODE_IS_LOAD( np ) ) ) && ( ! (NODE_IS_STORE( np ) ) ) ) {
		return 0;
	}
	/* LOAD */
	if( KNOWN_OFFSET( np ) ) {
		base_reg = np->n_operands.op_regs[O_RS1];
		if( indirect_pointer[base_reg].no_alias ) {
			return base_reg;
		}
	}
	return 0;
}

static Bool
overlap( np1, np2 ) register Node *np1, *np2;
{ /*
   * Both np1 and np2 must be either LOAD or STORE instruction with
   * SAME base register and a KNOWN displacement.
   * This routine will return TRUE if np1 and np2 are POSSIBLE to
   * points to same or overlaped memory.
   * We do NOT have to know the modification of the base register
   * after the loads or stores, because the instruction that modify
   * the base register will "depend" on all the previous loads and stores.
   */
	register int from1, from2, size1, size2;
	
	from1 = np1->n_operands.op_addend;
	from2 = np2->n_operands.op_addend;
	
	if( from1 == from2 ) {
		return TRUE;
	}
	size1 = np1->n_opcp->o_func.f_rd_width;
	size2 = np2->n_opcp->o_func.f_rd_width;
	
	if( from1 < from2 ) {
		if( from2 < from1 + size1 ) {
			return TRUE;
		} else {
			return FALSE;
		}
	} else { /* from2 < from1 */
		if( from1 < from2 + size2 ) {
			return TRUE;
		} else {
			return FALSE;
		}
	}
}

void
compute_dependences( np, node_count )
register Node *np;
register int node_count;
{
	register LIST_NODE *lp;
	static void accumulate_dependences();
	Bool insert_lnode();
	
/*
 * accumulate the dependences count recursively.
 */
 	accumulate_dependences( np, node_count );
	np->delay_candidate = 0;
	
	for( lp = np->depend_list; lp; lp = lp->next ) {
		if( NODE_IS_CALL( lp->nodep ) && exchangable(lp->nodep, np) &&
			NODE_IS_NOP(lp->nodep->n_next) && (! NODE_HAS_DELAY_SLOT(np))
#ifdef CHIPFABS
			&& ( !( iu_fabno == 1 && NODE_IS_LOAD( np ) ) )
#endif
		) { /* np is a candidate to put into lp->nodep delay slot */
			if( insert_lnode( lp->nodep, &(np->cti_node) ) ) {
				np->delay_candidate += 1;
			}
		}
	}
}

static void
accumulate_dependences( np, node_count )
register Node *np;
register int node_count;
{
	register LIST_NODE *lp;
	static void accumulate_dependences();
	
	np->inst_iu_dep = np->inst_fpu_dep = 0;
	
	for( lp = np->depend_list; lp; lp = lp->next ) {
		if( lp->nodep->delay_candidate == node_count ) { /* already computed */
			continue;
		}
		accumulate_dependences( lp->nodep, node_count );
		np->inst_fpu_dep += lp->nodep->inst_fpu_dep;
		np->inst_iu_dep += lp->nodep->inst_iu_dep;
		
		if( NODE_ACCESSES_FPU( lp->nodep ) ) {
			np->inst_fpu_dep += 1;
		} else {
			np->inst_iu_dep += 1;
		}
	}
	np->delay_candidate = node_count; /* mark as computed for this free node */
}

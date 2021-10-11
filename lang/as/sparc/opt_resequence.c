#ifndef lint
static	char sccsid[] = "@(#)opt_resequence.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include "structs.h"
#include "opt_scheduling.h"
#include "registers.h"
#include "optimize.h"
#include "opt_utilities.h"
#include "globals.h"

static int interlock_reg;

/* Initially no nodes in the fpu.  This will keep track of the nodes that are
 * in the fpu at a given point.  The "last in" node first and the other ones 
 * following in reverse order.
 */
static FPU_NODE fpu_status[MAX_FPU_NODES] = {
	{ (Node*)NULL, 0 } ,
	{ (Node*)NULL, 0 } ,
	{ (Node*)NULL, 0 } 
};

static int st_buf_status = 0;
static int fpu_nodes = 0;

void
resequences() {
	register Node *delay_np, *unimplement_node;
	register LIST_NODE *lnode, *depend_node;
	static LIST_NODE *best_node(), *best_delay_node();
	Bool insert_lnode(), exchangable();
	void append_node(), delete_free(), delete_node();
	static void free_depend();
	
	interlock_reg = 0;
	
	while( free_iu_nodes || free_fpu_nodes ) {
		lnode = best_node();
		if( NODE_IS_CALL( lnode->nodep ) ) {
			if( NODE_IS_UNIMP( lnode->nodep->n_next->n_next ) ) {
				unimplement_node = lnode->nodep->n_next->n_next;
			} else {
				unimplement_node = (Node *)NULL;
			}
			if( ! NODE_IS_NOP( lnode->nodep->n_next ) ) {
				/* delay_slot_lnode must be NULL */
				delay_np = lnode->nodep->n_next;
			} else {
				if( delay_slot_lnode == (LIST_NODE *) NULL ) {
					delay_slot_lnode = best_delay_node( lnode->nodep );
					if( delay_slot_lnode == (LIST_NODE *) NULL ) {
						delay_np = lnode->nodep->n_next;
					} else { /* found an useful inst for delay slot */
						delete_node( lnode->nodep->n_next );
						delay_np = delay_slot_lnode->nodep;
					}
				} else { /* has an useful inst for delay slot */
					delay_np = delay_slot_lnode->nodep;
					delete_node( lnode->nodep->n_next );
				}
			}
			append_node( lnode->nodep );
			append_node( delay_np );
			if( unimplement_node ) {
				append_node( unimplement_node ) ;
			}
			if( delay_slot_lnode ) {/* free delay_slot_lnode
						 * before free lnode because
						 * in some cases lnode is in
						 * delay_slot_lnode depend_list.
						 */
				free_depend( delay_slot_lnode );
			}
			free_depend( lnode );
			delay_slot_lnode = (LIST_NODE *)NULL;
		} else {
			if( number_of_nodes == 2 && branch_node ) {
				if( NODE_IS_NOP( branch_node->n_next ) &&
					exchangable(branch_node, lnode->nodep) ) {
						/* delete the NOP first */
						delete_node( branch_node->n_next );
						append_node( branch_node );
						append_node( lnode->nodep );
				} else {
					append_node( lnode->nodep );
					delay_np = branch_node->n_next;
					append_node( branch_node );
					append_node( delay_np );
					if( NODE_IS_NOP( delay_np ) ) {
					/* come back after re-scheduled the whole proc. */
						(void)insert_lnode(branch_node,
									&branches_with_nop);
					}	
				}
				free_depend( lnode );/* BEFORE reset branch_node */
				branch_node = (Node *)NULL;
			} else {
				append_node( lnode->nodep );
				free_depend( lnode );
			}
		}		
	}
	if( branch_node ) {
		delay_np = branch_node->n_next;
		append_node( branch_node );
		append_node( delay_np );
		if( NODE_IS_NOP( delay_np ) ) {
		/* come back after re-scheduled the whole proc. */
			(void)insert_lnode( branch_node, &branches_with_nop );
		}
	
	}
}


static LIST_NODE *
best_node() {
	int perfect;
	register LIST_NODE *lnode;
	register int cycles;
	static LIST_NODE *best_fpu_node(), *best_iu_node();
	static void update_fpu_status();
	
	perfect = MAX_CYCLES;
	if( free_fpu_nodes ) {
		lnode = best_fpu_node( &perfect ); /* modify "prefect" as side effect */
	}

	if( ( perfect != 0 ) && free_iu_nodes ) {    /* no PREFECT fpu node */
		lnode = best_iu_node();
		if( NODE_IS_LOAD( lnode->nodep ) ) {
			interlock_reg = lnode->nodep->n_operands.op_regs[O_RD];
		} else {
			interlock_reg = 0;
		}
		if( fpu_status[0].cycles ) {     /* The fpu is at work */
			if( NODE_IS_CALL( lnode->nodep ) ||
			    NODE_IS_TRAP( lnode->nodep ) ) {
				cycles = MAX_CYCLES;
			} else {
				if( NODE_IS_LOAD( lnode->nodep ) ) {
					cycles = IU_LOAD_CYCLES;
				} else {
					if( NODE_IS_STORE( lnode->nodep ) ) {
						cycles = IU_STORE_CYCLES;
                                                st_buf_status = (lnode->nodep->n_opcp->o_func.f_rd_width == 8) ?
						                STORE_DOUBLE_INTERLOCK : 
								STORE_SINGLE_INTERLOCK;
					} else {
						cycles = IU_ARITH_CYCLES;
					}
				}
				if (!NODE_IS_STORE(lnode->nodep)) st_buf_status -= cycles;
				update_fpu_status( (Node *)NULL, cycles );
			}
		}
		return lnode;
	} else { /* return an FPU inst. */
		interlock_reg = 0;
		if( NODE_IS_LOAD( lnode->nodep ) ) {
			cycles = NODE_IS_DOUBLE_FLOAT(lnode->nodep) ?
			         (IU_LOAD_CYCLES + 1) : IU_LOAD_CYCLES;
		} else if( NODE_IS_STORE( lnode->nodep ) ) {
			if ( NODE_IS_DOUBLE_FLOAT(lnode->nodep) ) {
				cycles = IU_STORE_CYCLES + 1;
				st_buf_status = STORE_DOUBLE_INTERLOCK;
			}
			else {
				cycles = IU_STORE_CYCLES;
				st_buf_status = STORE_SINGLE_INTERLOCK;
			}				
		} else if (NODE_IS_FBRANCH(lnode->nodep) || NODE_IS_FCMP(lnode->nodep)){
			cycles = FPU_CMP_CYCLES;
		} else cycles = 1;

		if (!NODE_IS_STORE(lnode->nodep)) st_buf_status -= cycles; 
		if (NODE_BYPASS_FPU(lnode->nodep)) 
			update_fpu_status( (Node *)NULL, cycles + perfect );
		else
			update_fpu_status( lnode->nodep, cycles + perfect );
		return lnode;
	}
}

/*
 * return the "best choice" among all the free fpu nodes.
 * set *flagp to the cycles the fpu node that returns 
 * has to wait before it can start executing.
 */
static LIST_NODE *
best_fpu_node( flagp ) int *flagp;
{
	register LIST_NODE *best_choice, *lp;
	static LIST_NODE *better_fpu_node();

	best_choice = (LIST_NODE *)NULL;

	for( lp = free_fpu_nodes; lp; lp = lp->next ) {
		best_choice = better_fpu_node( best_choice, lp, flagp );
	}
	return best_choice;
}

/* better_fpu_node()
 * return the "better choice" of the two fpu nodes and set the *flagp.
 * Algorithm (select in this order):
 *	1. If there is only one free fpu node then return it.
 *      2. Return the one that has to wait less. If waits are equal:
 *           a) return the one that invloves the fpu if only one does
 *           b) return the one that invloves a chain if only one does
 *           c) return a "store" if only one is a "store" (spread out stores for cache buffer)
 *           d) return (if both are loads) the one that appeared first in the code (hack: see below)
 *	     e) return the one with most of FPU dependences.
 *	     f) return the one with most of IU dependences.
 *	Set *flagp to the cycles the return node has to wait.
 */
static LIST_NODE *
better_fpu_node( lp1, lp2, flagp )
register LIST_NODE *lp1, *lp2;
register int *flagp;
{
	register Node *np1, *np2;
	register LIST_NODE *choice;
	register int wait_1, wait_2;
	Bool chain_1, chain_2;
	static int has_to_wait();

	choice = (LIST_NODE *)NULL;
	
	if( lp1 == (LIST_NODE *)NULL ) {
		choice = lp2;
	} 
	if( lp2 == (LIST_NODE *)NULL ) {
		choice = lp1;
	}
	if( choice ) {  /* one of the nodes is NULL; return choice and set flagp */
		if( (fpu_status[0].cycles == 0) && (st_buf_status <= 0) ) *flagp = 0;
		else *flagp = has_to_wait( choice->nodep, &chain_1 );
		return choice;
	}
	
	/* We have two nodes, let's see which is better */
	np1 = lp1->nodep;
	np2 = lp2->nodep;
	chain_1 = FALSE;
	chain_2 = FALSE;
	/* set up PARAMETER to CHOSE */

	wait_1 = has_to_wait( np1, &chain_1 );
	wait_2 = has_to_wait( np2, &chain_2 );

	/* Return the shorter wait (if any) */
	if (wait_1 < wait_2){ 
		*flagp = wait_1;
		return lp1;
	}
	else if (wait_2 < wait_1) {
		*flagp = wait_2;
		return lp2;
	}
	/* Equal wait. */ 
	else {
		*flagp = wait_1;         /* could be wait_2 since they're the same */

		/* Return the one that involves the FPU (not a ldf or stf) 
		 * (if only one does).
		 */
		if ( NODE_BYPASS_FPU(np1) ^ NODE_BYPASS_FPU(np2) ) {
			if ( NODE_BYPASS_FPU(np2) )
				return lp1;
			else
				return lp2;
		}

		/* Return the one that involves a chain (if only one does) */
		if (chain_1 ^ chain_2) {
			if( chain_2 )
				return lp2;
			else
				return lp1;
		}
	
		/* both lp1 and lp2 can chain, or neither can chain */
		if ( NODE_IS_STORE(np1) ^ NODE_IS_STORE(np2) ) 
		{
			if ( NODE_IS_STORE(np2) )
			        return lp2;
			else
                                return lp1;
		}
		/* This is a kludge.  Whn dealing with an unrolled loop we need to "terminate"
		 * an "iteration" of the loop asap so we can "store" the result and thus separate
		 * the stores.  In case  (1)ld;(2)add;(3)ld;(4)mul;(5)st;(6)ld;(7)add....
		 * this hack picks (3) rather than (6).
		 */
		else if ( NODE_IS_LOAD(np1) && NODE_IS_LOAD(np2) )
		{
			if ( np1->n_lineno < np2->n_lineno ) { return lp1; }
			return lp2;
		}

		if ( np1->inst_fpu_dep > np2->inst_fpu_dep ) { return lp1; } 
		if ( np2->inst_fpu_dep > np1->inst_fpu_dep ) { return lp2; }
		
		if ( np1->inst_iu_dep > np2->inst_iu_dep ) { return lp1;}
		return lp2;
	}
}	


/*
 * Return the number of cyclces that np has to wait.  Also set chain to TRUE if
 * the wait involves chainning.
 */
static int
has_to_wait( np, chain ) 
register Node *np;
register Bool *chain;
{
	Bool regset_empty();
	register_set regset_intersect();
	register Node *fnode;
	register int wait, new_wait, cycles, i;
	
	

	wait = 0;
	
	for (i = 0 ; i < MAX_FPU_NODES ; i++){
		if (fpu_status[i].fnode != (Node *)NULL){
			fnode = fpu_status[i].fnode;
			cycles = fpu_status[i].cycles;
			if (!regset_empty(regset_intersect(REGS_READ(np),REGS_WRITTEN(fnode)))) {
				if ( (i == 0) && CHAINNABLE(np) && CHAINNABLE(fnode) ){
					wait = MAX ( wait, (cycles - FPU_CHAIN_CYCLES - FPU_SETUP_CYCLES) );
					*chain = TRUE;
				}
				else {
					new_wait = NODE_IS_STORE(np) ?
						   MAX ( wait, cycles ) :
						   MAX ( wait, (cycles - FPU_SETUP_CYCLES) );
					if (new_wait > wait){
						*chain = FALSE;
						wait = new_wait;
					}
				}
			}
		}
	}
	
	if ( (fpu_nodes == 3) )   /* Resource dependency.  3 fpu nodes */
		wait = MAX ( wait, fpu_status[0].cycles );

	/* Resource dependency. 2 fpu nodes w/one div or sqrt */
	else if ((fpu_nodes == 2) && 
		 ((OP_FUNC(fpu_status[0].fnode).f_subgroup == FSG_FDIV) ||
		  (OP_FUNC(fpu_status[0].fnode).f_subgroup == FSG_FSQT) ||
		  (OP_FUNC(fpu_status[1].fnode).f_subgroup == FSG_FDIV) ||
		  (OP_FUNC(fpu_status[1].fnode).f_subgroup == FSG_FSQT)))
		wait = MAX ( wait, fpu_status[0].cycles - FPU_SETUP_CYCLES );

	if ( NODE_IS_STORE(np) ) wait = MAX ( wait, st_buf_status ); /* Store buffer status */

	return wait;
}


/*
 * Decrements the remainning cycles of each currently executing fpu instruction
 * by "cycles".  If fnode is not NULL it is an fpu instruction and we will set
 * it as the last fpu instruction and the "old-last" will becom next-to-last etc.
 * and the cycles for the last instruction will be determined.
 */
static void
update_fpu_status( fnode, cycles)
	register Node *fnode;
	register int cycles;
{
	register int busy;
	int i;

	if ( fnode != (Node *)NULL ){     /* It is an fpu instruction */

		switch (OP_FUNC(fnode).f_subgroup){  /* Figure out how long its
						      * going to take.
						      */
			
		      case FSG_FADD:
		      case FSG_FSUB:
		      case FSG_FMOV:
		      case FSG_FNEG:
		      case FSG_FABS:
		      case FSG_FCVT:
			busy = FPU_SIMPLE_CYCLES;
			break;
			
		      case FSG_FMUL:
			busy = NODE_IS_DOUBLE_FLOAT(fnode) ?
				FPU_MULD_CYCLES : FPU_MULS_CYCLES;
			break;
			
		      case FSG_FDIV:
			busy = NODE_IS_DOUBLE_FLOAT(fnode) ?
				FPU_DIVD_CYCLES : FPU_DIVS_CYCLES;
			break;
			
		      case FSG_FSQT:
			busy = NODE_IS_DOUBLE_FLOAT(fnode) ?
				FPU_SQRTD_CYCLES : FPU_SQRTS_CYCLES;
			break;

		      case FSG_FCMP:
			busy = FPU_CMP_CYCLES;
			break;
			
		      default:
			busy = 1;
			break;
		}

	        /* "move" all the fpu nodes "up" */
		for ( i = (MAX_FPU_NODES - 1) ; i > 0 ; i-- ){
			fpu_status[i].fnode = fpu_status[i-1].fnode;
			if (fpu_status[i-1].cycles == 0) fpu_status[i].cycles = 0;
			else fpu_status[i].cycles = fpu_status[i-1].cycles - 1;
		}
		/* set the last fpu node */
		fpu_status[0].fnode = fnode;
		fpu_status[0].cycles = busy;
	} else { /* its an integer intstruction (or a ld/st) */
		for ( i = 0 ; i < MAX_FPU_NODES ; i++ ){
			if ( fpu_status[i].cycles <= cycles ){
				fpu_status[i].cycles = 0;
				fpu_status[i].fnode = (Node *)NULL;
			} else {
				fpu_status[i].cycles -= cycles;
			}
		}
	}

	/* If the last or next to last node are null and the
	 * node following is not then "move" down all fpu_nodes
	 */
	for ( i = 0 ; i < (MAX_FPU_NODES - 1) ; i++ ){
		if ( (fpu_status[i].cycles == 0) &&
		     (fpu_status[i+1].cycles != 0) ){
			fpu_status[i] = fpu_status[i+1];
			fpu_status[i+1].cycles = 0;
			fpu_status[i+1].fnode = (Node *)NULL;
		}
	}

	fpu_nodes = 0;
	/* Count the number of nodes in the fpu */
	for ( i = 0 ; i < MAX_FPU_NODES ; i++ ) {
		if (fpu_status[i].cycles == 0) break;
		else fpu_nodes++;
		     
	}
}
		

/*
 * best_iu_node() :
 *	This routine return the "best choice" among all the free IU nodes
 * for the next instruction.
 * "best CHOSE" is defined in the order of the following pirorities :
 * 	1. No REGISTER interlock with previous instruction.
 *	2. Fill CALL delay slots with "useful" instructions.
 *	3. Fill BRANCH delay slot with "useful" instruction.
 *
 */
static LIST_NODE *
best_iu_node() {
	register LIST_NODE *best_choice, *lp;
	static LIST_NODE *better_iu_node();
	
	best_choice = (LIST_NODE *)NULL;

	for( lp = free_iu_nodes; lp; lp = lp->next ) {
		best_choice = better_iu_node( best_choice, lp );
	}

	if( best_choice->nodep->delay_candidate ) {
		/* See if any of the CALL in cti_node could be */
		/* scheduled before schedule the best_node */
		for( lp = best_choice->nodep->cti_node; lp; lp = lp->next ) {
			if( NODE_IS_CALL( lp->nodep ) /* not a BRANCH */ &&
			   lp->nodep->dependences == 1 /* only depend on best_node now */ ) {
			/* return the CALL node and let the best_node */
			/* be the one in the delay slot */
				delay_slot_lnode = best_choice;
				best_choice = lp;
				break;
			}
		}
	}
	return best_choice;
}

/* better_iu_node()
 * return the "better choice" of the two iu nodes.
 *
 * Algorithm:
 *	1. If there is only one free IU node then return it.
 *	2. If there is only one free IU node that will NOT
 *	   causing register interlock with the previous instruction
 *	   then return it.
 *	3. return one has most of FPU dependences because FPU instructions
 *	   are slow than IU and need more IU to fill the "inter-dependences".
 *	4. return LOAD instruction.  Scheduling LOAD as early as possible, so
 *	   have better chance to avoid interlock.
 *	5. return CALL/TRAP instruction.
 *	6. return one has most of IU dependences.
 *	7. return one has most of "delay_candidate"
 *	8. if both no dependences and there is branch node then save the one
 *	   can in the branch delay slot.
 */
static LIST_NODE *
better_iu_node( lp1, lp2 ) register LIST_NODE *lp1, *lp2;
{
	register Node *np1, *np2;

	if( lp1 == (LIST_NODE *)NULL ) {
		return lp2;
	} else {
		if( lp2 == (LIST_NODE *)NULL ) {
			return lp1;
		}
	}
	
	np1 = lp1->nodep;
	np2 = lp2->nodep;
	
	if( interlock_reg != 0 ) {/* previous instruction is a LOAD */
				  /* instruction and defines interlock_reg */
		if( REG_IS_READ( np1, interlock_reg ) ) {/* np1 interlock */
			if( ! REG_IS_READ( np2, interlock_reg ) ) {/* no interlock for np2 */
				return lp2;
			} /* else, both causing interlock */
		} else { /* no interlock for np1 */
			if( REG_IS_READ( np2, interlock_reg ) ) { /* np2 interlock */
				return lp1;
			} /* else, nither interlock */
		}
	}

	if( np1->inst_fpu_dep > np2->inst_fpu_dep ) { return lp1; } 

	if( np2->inst_fpu_dep > np1->inst_fpu_dep ) { return lp2; }

	if( NODE_IS_LOAD( np1 ) && (! NODE_IS_LOAD( np2 ) ) ) { return lp1; }

	if( NODE_IS_LOAD( np2 ) && (! NODE_IS_LOAD( np1 ) ) ) { return lp2; }

	if( NODE_IS_CALL( np1 ) || NODE_IS_TRAP( np1 ) ) { return lp1; }

	if( NODE_IS_CALL( np2 ) || NODE_IS_TRAP( np2 ) ) { return lp2; }

	if( np2->inst_iu_dep > np1->inst_iu_dep ) { return lp2; }

	if( np1->inst_iu_dep > np2->inst_iu_dep ) { return lp1; }

	if( np1->delay_candidate > np2->delay_candidate ) { return lp1; }

	if( np2->delay_candidate > np1->delay_candidate ) { return lp2; }

	if( branch_node && NODE_IS_NOP (branch_node->n_next) ) {
		if( np1->depend_list == (LIST_NODE *)NULL ) { /* boths are 0 */
			if( exchangable( branch_node, np1 ) ) {
				return lp2;
			}
		}
	}
	return lp1;
}

static LIST_NODE *
best_delay_node( np ) register Node *np;
{
/*
 * np must be a CALL.  return the best free node
 * that can be put in the delay slot of the np,
 * or return NULL if there is no such node exit.
 */
	register LIST_NODE *best_choice, *lp;
	Bool exchangable();
	static LIST_NODE *better_iu_node();
	
	best_choice = (LIST_NODE *)NULL;
	for( lp = free_iu_nodes; lp; lp = lp->next ) {
		if( exchangable( np, lp->nodep ) ) {
			best_choice = better_iu_node( best_choice, lp );
		}
	}
	return( best_choice );
}

static void
free_depend( lp ) LIST_NODE *lp;
{
/*
 * Decrement the dependences of the node that depend on the np.
 * Put the node into the free list if the node becomes "free".
 */

	register LIST_NODE *depend_node;
	register Node *np, *node;
	Bool insert_lnode();
	
	np = lp->nodep;
	
	for( depend_node = np->depend_list; depend_node; depend_node = depend_node->next ) {
		node = depend_node->nodep;
		node->dependences -= 1;
		if( node->dependences==0 && node != branch_node) {
			/*
			 * do NOT put the branch_node  into
			 * free_iu_nodes, but append at end after
			 * relocate all the other nodes.
			 */
			if( NODE_ACCESSES_FPU( node ) ) {
				(void) insert_lnode( node,&free_fpu_nodes );
			} else {
				(void) insert_lnode( node,&free_iu_nodes );
			}
			/* compute dependences only when its really FREE */
			compute_dependences( node, ++free_node_count );
		}
	}
	delete_free( lp, (NODE_ACCESSES_FPU(np) ? TRUE : FALSE) );
	--number_of_nodes;
}

void
copy_target_to_delay_slot( branch, delay ) register Node *branch, *delay;
{
/*
 * This is an another way to fill the branch''s delay slot.
 * delay is pointing to the nop that follows branch.
 * This routine will try to copy the target of the branch
 * to the delay and branch to the next statement of the
 * target instead.
 */
	register Node *target_label, *target_inst, *new_label,
			*np, *next_np;
	Node *next_instruction(), *unique_optimizer_label_node();
	struct opcode *branch_with_toggled_annul_opcp();
	void copy_node(), delete_node(), change_branch_target(),
	     insert_after_node();
	Bool exchangable(), branch_is_last_reference_to_target_label(),
	     can_fall_into(), regset_empty();
	register_set regset_intersect();
	     
	if( !NODE_IS_BRANCH( branch ) ) {      /* JMP ? */
		return;
	}
	target_label = branch->n_operands.op_symp->s_def_node;
	target_inst = next_instruction(target_label);
	if( (!NODE_HAS_DELAY_SLOT( target_inst ) )  && ( !NODE_IS_NOP( target_inst ) )
#ifdef CHIPFABS
		&& ( !( iu_fabno == 1 && NODE_IS_LOAD( target_inst ) ) )
#endif
	) {
		copy_node( target_inst, delay, OPT_SCHEDULING );

		/* Following code is copied from opt_fill_delay.c */
		/* Change the branch target to just AFTER the node 
		 * just copied.
		 */
		if (NODE_IS_LABEL(target_inst->n_next)) {
			/* How convenient!
			 * A label is already there for us to use!
			 */
			new_label = target_inst->n_next;
		} else {
			/* Create our own label node, to have one to
			 * branch to.
			 */
			new_label = unique_optimizer_label_node( target_inst->n_lineno, 
								OPT_SCHEDULING );
			/* Insert it in the list of nodes */
			insert_after_node(target_inst, new_label );
		}

		/* If we can''t "fall through" to execute code at
		 * the old target label ("Lx" in the pattern above) and
		 * this was the only branch (reference) to it, then we
		 * can just delete it and the node in its delay slot
		 * (which we just copied into the conditional branch''s
		 * delay slot).
		 *
		 * If that is the case, we only need to delete the
		 * delay-slot node here, as "change_branch_target()"
		 * will take care of deleting the label node.
		 */

/*
 * The following test does NOT work well.
 * Becasue this routine will generate two back-to-back
 * labels by deleting code.  We can clean up the duplicated
 * labels, dead code, simple jumps and useless nops after
 * the instruction scheduling.
 *
 *		if ( branch_is_last_reference_to_target_label(branch) &&
 *					(!can_fall_into(target_label)) ) {
 *			delete_node(target_inst);
 *		}
 */

		/* Change the branch node''s reference from the old
		 * target label to the new one.
		 * (If this was the only reference to the old label,
		 * this will cause that label node to be deleted, too)
		 */
		change_branch_target(branch, new_label->n_operands.op_symp);

		if ( BRANCH_IS_CONDITIONAL(branch) ) {
			/* Make it into a Branch-with-Annul. */
			branch->n_opcp = branch_with_toggled_annul_opcp(branch->n_opcp);
		}
	} else {/* ELSE, can NOT copy it */
		/*
		 * Try the following pattern :
		 *
		 * 	bcc	Lxxx		<-- branch
		 *	nop			<-- delay
		 *	INST			<-- next_np
		 *
		 * if
		 *	1. INST is NOT a CTI.
		 *	2. INST is NOT a LABEL. -- to make AS happy
		 *	3. INST does NOT take more then one cycle. -- could be slower
		 *	4. INST does NOT modify any resource that LIVE at Lxxx.
		 *
		 * then we can make code into : ( save one cycle in fell thur case )
		 *
		 *	bcc	Lxxx
		 *	INST
		 */
		if( ! BRANCH_IS_CONDITIONAL(branch) ) {
			return;
		}
		
		next_np = next_instruction( delay );
		/* next_instruction() skips LABELs so has to check */
		/* see if we step into another basic block */
		for( np = delay; np != next_np; np = np->n_next ) {
			if( NODE_IS_LABEL( np ) ) {
				return;
			}
		}

		if( NODE_HAS_DELAY_SLOT( next_np ) || 
				NODE_IS_LOAD( next_np ) || NODE_IS_STORE ( next_np ) ||
				NODE_ACCESSES_FPU( next_np ) ) {
			return;
		}
		if( ! (  regset_empty( regset_intersect( REGS_WRITTEN(next_np),
					REGS_LIVE( target_label ) ) ) ) ) {
			return;
		}
		copy_node( next_np, delay, OPT_SCHEDULING );
		delete_node( next_np );
	}
}


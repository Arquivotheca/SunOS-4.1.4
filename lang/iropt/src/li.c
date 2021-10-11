#ifndef lint
static	char sccsid[] = "@(#)li.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include "iropt.h"
#include "page.h"
#include "misc.h"
#include "loop.h"
#include "iv.h"
#include "ln.h"
#include "make_expr.h"
#include "recordrefs.h"
#include "lu.h"
#include "li.h"
#include "bit_util.h"
#include <stdio.h>

/*
**	code motion transformations :
**	visit each loop, outermost ones first; make a list of all invariant
**	triples in the loop, go through this list and move all root triples
**	to the preheader
*/

/* global */
int preloop_ntriples, preloop_nleaves;

/* local */
static BLOCK	*from_bp;
static void	analyze_loop(/*loop, do_iv, do_unroll*/);
static BOOLEAN	isconst_triple(/*tp,bp,loop,always_executed*/);
static BOOLEAN	isconst_operand1(/*tp,loop*/);
static BOOLEAN	isconst_operand2(/*tp,loop*/);
static BOOLEAN	isconst_def(/*def_tp, def_bp, loop*/);
static void	mark_tree(/*parent*/);
static void	move_trees_backwards(/*loop*/);
static void	adjust_var_refs(/*tp, to_bp*/);
static void	move_triple(/*tp,to_bp*/);

#if TARGET == SPARC
static LIST	*const_leaf_list = LNULL;
static void	mark_const_leaf(/*tp, leafp*/);
static void	move_const_leaves(/*loop*/);
#endif

void
scan_loop_tree(node,do_iv,do_unroll)
	LOOP_TREE *node;
	BOOLEAN do_iv, do_unroll;
{
	register LOOP_TREE *ltp2;
	register char *cp;

	if( node == (LOOP_TREE *) NULL ) return;
	analyze_loop(node->loop,do_iv,do_unroll);
	for(ltp2=loop_tree_tab,cp = node->children;
		cp < &node->children[nloops]; cp++,ltp2++) {
		if (*cp) {
			scan_loop_tree(ltp2,do_iv,do_unroll);
		}   
	}
}

static void
analyze_loop(loop, do_iv, do_unroll)
	register LOOP *loop;
	BOOLEAN do_iv, do_unroll;
{
	register LIST *lp_temp, *invariant_tail;
	register TRIPLE *tp;
	register BLOCK *bp;
	BOOLEAN  always_executed;	/* true => block dominates all exits */

	if (loop == (LOOP*) NULL) return;
	/*
	**	the triple's visited flag is used to mark invariant triples:
	**	if non-zero it record the block the triple is in before being moved
	**	initialize all triples in the region to not constant
	**	the block's visited flag is used
	**	to avoid calculating exit dominators unless necessary
	*/
	LFOR(lp_temp, loop->blocks) {
		bp = (BLOCK*) lp_temp->datap;
		bp->visited = IR_FALSE;
		TFOR(tp,bp->last_triple) {
			tp->visited = IR_FALSE;
		}
	}
	loop->invariant_triples = invariant_tail = LNULL;
	/*
	**	traverse the blocks in the loop in dfo order
	**	ie parents before children. Check that a block is
	**	in the loop and that it has triples; make a list
	**	of the eligible loop constants then move them to the 
	**	preheader
	*/
	if(do_iv == IR_TRUE || do_unroll == IR_TRUE) iv_init();
	for(bp=entry_block;bp;bp=bp->dfonext) {
		if(INLOOP(bp->blockno,loop) && bp->last_triple) {
			always_executed = is_exit_dominator(bp, loop);
			TFOR(tp,bp->last_triple->tnext) {
				if(isconst_triple(tp,bp,loop,always_executed)) {
					/*if the triple is invariant remember which block it's in */
					tp->visited =  (BOOLEAN) bp;
					lp_temp = NEWLIST(loop_lq);
					(TRIPLE*) lp_temp->datap = tp;
					/*
					 * For a single link list, build the
					 * list in the reverse order of the
					 * LAPPEND.
					 */
					
					if( loop->invariant_triples == LNULL ) {
						loop->invariant_triples = invariant_tail = lp_temp;
					} else {
						lp_temp->next = loop->invariant_triples;
						loop->invariant_triples = invariant_tail->next = lp_temp;
					}
				}
			}
		}
	}
	if(SHOWCOOKED == IR_TRUE) {
		print_loop(loop);
	}
	if(do_iv == IR_TRUE || do_unroll == IR_TRUE) {
		clean_save_temp();
		LFOR(lp_temp, loop->blocks) {
			bp = (BLOCK*) lp_temp->datap;
			TFOR(tp,bp->last_triple) {
				tp->visited = IR_FALSE;
			}
		}
		if(do_iv == IR_TRUE)
			do_induction_vars(loop);
		else
			do_loop_unrolling(loop);
	} else { /* code motion */
#if TARGET == SPARC
		if( const_leaf_list != LNULL ) {
			move_const_leaves(loop);
		}
#endif
		move_trees_backwards(loop);
	}
#if TARGET == SPARC
	const_leaf_list = LNULL;
#endif
/*
 * 	if(SHOWCOOKED == IR_TRUE) {
 *		dump_cooked(loop->loopno);
 *	}
 */
}

/*
**	is a triple invariant in this loop ? a value triple is invariant
**	if it always yields the same value, a mod triple is invariant
**	if the definition is always the same (see isconst_def...)
*/
static BOOLEAN
isconst_triple(tp,bp,loop,always_executed)
	TRIPLE *tp;
	BLOCK *bp;
	LOOP *loop;
	BOOLEAN always_executed;
{
	LIST *lp;
	BOOLEAN const_operand1=IR_TRUE, const_operand2=IR_TRUE;
	TRIPLE *implicit;

	if( ! (ISOP(tp->op, VALUE_OP) || ISOP(tp->op, MOD_OP) )  || 
				tp->tripleno >= preloop_ntriples) {
		return(IR_FALSE);
	}

	switch(tp->op) {
		case IR_IMPLICITDEF:
		case IR_IMPLICITUSE:
		case IR_ENTRYDEF:
		case IR_EXITUSE:
			return IR_FALSE;

		case IR_SCALL:
		case IR_FCALL:
			/*
			 * should analyze func_descr to decide
			 * whether call is movable.
			 */
			return IR_FALSE;

		case IR_ADDROF:
			return IR_TRUE;

	 	case IR_ASSIGN:
			const_operand1 = isconst_def(tp,bp,loop);
			const_operand2 = isconst_operand2(tp,loop);
			break;

	 	case IR_ISTORE:
			if(partial_opt == IR_TRUE)
				return (IR_FALSE);
			/* 
			**	first check that the address through which we are
			**	indirecting is a loop constant - if so check that all
			**	the definitions that can result from the istore are
			**	invariant
			*/
			const_operand1 = isconst_operand1(tp,loop);
			const_operand2 = isconst_operand2(tp,loop);
			if( const_operand1 == IR_FALSE){
				return(IR_FALSE);
			}
			if( !always_executed ) {
				return(IR_FALSE);
			}
			LFOR(lp, (LIST *)tp->implicit_def) {
				implicit = (TRIPLE*) lp->datap;
				if(isconst_def(implicit,bp,loop) == IR_FALSE) {
					return(IR_FALSE);
				}
			}
			break;

		case IR_IFETCH:
			if(partial_opt == IR_TRUE)
				return (IR_FALSE);
			const_operand1 = isconst_operand1(tp,loop);
			if( const_operand1 == IR_FALSE){
				return(IR_FALSE);
			}
			if( !always_executed ) {
				return(IR_FALSE);
			}
			LFOR(lp, (LIST *)tp->implicit_use) {
				implicit = (TRIPLE*) lp->datap;
				if(isconst_operand1(implicit,loop) == IR_FALSE) {
					return(IR_FALSE);
				}
			}
			break;

		default:
			if(ISOP(tp->op, USE1_OP)) {
				const_operand1 = isconst_operand1(tp,loop);
			}
			if(ISOP(tp->op, USE2_OP)) {
				const_operand2 = isconst_operand2(tp,loop);
			} else {
				const_operand2 = IR_TRUE;
			}
			if (const_operand1 && const_operand2) {
				if( may_cause_exception(tp)
				    && !always_executed ) {
					return(IR_FALSE);
				}
			}
			break;

	}
	return ( (BOOLEAN) (const_operand1 == IR_TRUE && const_operand2 == IR_TRUE) );
}

/*
**	is a left operand constant in this loop ?
*/
static BOOLEAN
isconst_operand1(tp,loop)
	register TRIPLE *tp;
	register LOOP *loop;
{
	register VAR_REF *rp;
	register LIST *lp;

	if(tp->left->operand.tag == ISTRIPLE) {
		/*
		 * because triples are a tree in post order,
		 * if the operand is a triple and invariant it will have been 
		 * marked invariant
		 */
		return(tp->left->triple.visited ? IR_TRUE : IR_FALSE );
	} else if(tp->left->operand.tag == ISLEAF) {
		if(ISCONST(tp->left)) {
#if TARGET == SPARC
			mark_const_leaf(tp, (LEAF*)tp->left);
#endif
			return(IR_TRUE);
		} else {
			if(tp->left->leaf.leafno >= preloop_nleaves)  {
				/* leaves created in the course of code motion have inaccurate
				** reachdef information - assume the worst
				*/
				return(IR_TRUE);
			} else if(partial_opt == IR_TRUE && 
					  tp->left->leaf.class == VAR_LEAF &&
					  EXT_VAR((LEAF*)tp->left)) {
				return(IR_FALSE);
			} else if(tp->reachdef1 == LNULL) {
				set_rc((LEAF*)tp->left);
				return(IR_TRUE);
			} else if(tp->reachdef1->next == tp->reachdef1) {
				/*
				**	one definition reaches:if it's outside loop the operand
				**	is invariant and a rc otherwise if the def site is invariant
				**	this operand is but it's not an rc
				*/
				rp = (VAR_REF*) tp->reachdef1->datap;
				if(	!(INLOOP(rp->site.bp->blockno,loop)) ) {
					set_rc((LEAF*)tp->left);
					return(IR_TRUE);
				} else if(rp->site.tp->visited) {
					return(IR_TRUE);
				} else {
					return(IR_FALSE);
				}
			} else {
				/*
				**	if there several definitions reach they must all
				**	come from outside the loop
				*/
				LFOR(lp,tp->reachdef1) {
					rp = (VAR_REF*) lp->datap;
					if(	INLOOP(rp->site.bp->blockno,loop) ) {
						return(IR_FALSE);
					}
				}
				set_rc((LEAF*)tp->left);
				return(IR_TRUE);
			}
		}
	} else {
		quita("isconst_operand1: illegal node type (%d)",
			(int)tp->left->operand.tag);
	}
	/*NOTREACHED*/
} 

static BOOLEAN
isconst_operand2(tp,loop)
	register TRIPLE *tp;
	register LOOP *loop;
{
	register VAR_REF *rp;
	register LIST *lp;

	if(tp->right->operand.tag == ISTRIPLE) {
		return(tp->right->triple.visited ? IR_TRUE : IR_FALSE );
	} else if(tp->right->operand.tag == ISLEAF) {
		if(ISCONST(tp->right)) {
#if TARGET == SPARC
			mark_const_leaf(tp, (LEAF*)tp->right);
#endif
			return(IR_TRUE);
		} else {
			if(tp->right->leaf.leafno >= preloop_nleaves)  {
				return(IR_TRUE);
			} else if(partial_opt == IR_TRUE && 
					  tp->right->leaf.class == VAR_LEAF &&
					  EXT_VAR((LEAF*)tp->right)) {
				return(IR_FALSE);
			} else if(tp->reachdef2 == LNULL) {
				set_rc((LEAF*)tp->right);
				return(IR_TRUE);
			} else if(tp->reachdef2->next == tp->reachdef2) {
				rp = (VAR_REF*) tp->reachdef2->datap;
				if(	!(INLOOP(rp->site.bp->blockno,loop)) ) {
					set_rc((LEAF*)tp->right);
					return(IR_TRUE);
				} else if(rp->site.tp->visited) {
					return(IR_TRUE);
				} else {
					return(IR_FALSE);
				}
			} else {
				LFOR(lp,tp->reachdef2) {
					rp = (VAR_REF*) lp->datap;
					if(	INLOOP(rp->site.bp->blockno,loop) ) {
						return(IR_FALSE);
					}
				}
				set_rc((LEAF*)tp->right);
				return(IR_TRUE);
			}
		}
	} else {
		quita("isconst_operand2: illegal node type (%d)",
			(int)tp->right->operand.tag);
	}
	/*NOTREACHED*/
} 


static BOOLEAN
isconst_def(def_tp, def_bp, loop)
	TRIPLE *def_tp;
	BLOCK *def_bp;
	LOOP *loop;
{
	TRIPLE *use_tp;
	BLOCK *use_bp;
	LIST *lp, *reachdef;
	LEAF *leafp;
	VAR_REF *rp;

	if(def_tp->left->operand.tag != ISLEAF ) {
		quit("isconst_def: lhs of a mod triple not a leaf");
	}
	leafp = (LEAF*) def_tp->left;
	if(leafp->class == VAR_LEAF && EXT_VAR(leafp))
		return IR_FALSE;

	/*	three conditions must be met for an assignment to be moved to the 
	**	preheader - from Dragon Book Section 13.4
	*/
	LFOR(lp,def_tp->canreach) {
		rp = (VAR_REF*) lp->datap;
		use_tp = rp->site.tp;
		use_bp = rp->site.bp;
		if(! INLOOP(use_bp->blockno,loop)) {
			/*
			**	If def_tp can reach outside the loop then  the
			**	assignment can be moved only if it's in a block
			**	which dominates all exits.
			**	The block's visited flag is used
			**	to avoid re-calculating exit dominators 
			*/
			if(is_exit_dominator(def_bp, loop)) {
				continue;
			} else {
				return IR_FALSE ;
			}
		}
	}

	rp = leafp->references;
	while(rp) {
		if(	INLOOP(rp->site.bp->blockno, loop) ) {
			switch (rp->reftype) {
				case VAR_DEF:
				case VAR_AVAIL_DEF:
					if( rp->site.tp != def_tp ) {
						/* condition 2: no more than 1 def in loop */
						return IR_FALSE;
					}
					break;

				case VAR_USE1:
				case VAR_EXP_USE1:
				case VAR_USE2:
				case VAR_EXP_USE2:
					/* condition 3
					**	if the use is inside the loop then def_tp must be the
					**	only triple to reach it
					*/
					use_tp = rp->site.tp;
					if(rp->reftype == VAR_USE1 || rp->reftype == VAR_EXP_USE1) {
						reachdef = use_tp->reachdef1;
					} else {
						reachdef = use_tp->reachdef2;
					}
					if(reachdef && reachdef->next == reachdef && 
							LCAST(reachdef,VAR_REF)->site.tp == def_tp ) {
					} else {
						return IR_FALSE ;
					}
					break;
			}
		}
		rp = rp->next_vref;
	}
	return IR_TRUE ;
}

static void
mark_tree(parent)
	register TRIPLE *parent;
{
	if( ISOP(parent->op, USE1_OP) && parent->left->operand.tag == ISTRIPLE ) {
		mark_tree((TRIPLE*)parent->left);
	} 
	if(ISOP(parent->op,USE2_OP) && parent->right->operand.tag==ISTRIPLE){
		mark_tree((TRIPLE*)parent->right);
	}
	(BLOCK*) parent->visited = (BLOCK*) NULL;
}

static void
move_trees_backwards(loop)
	register LOOP *loop;
{
	register LIST *lp, *last;
	register TRIPLE *tp;
	TRIPLE *root, *parent;
	LEAF *tmp;
	typedef struct invariant_tree {
		BLOCK *from_bp; 
		TRIPLE *root; 
		struct invariant_tree *next;
	} INV_TREE;
	INV_TREE *ivtp, *inv_tree_head, *inv_tree_tail;

	if(loop->invariant_triples == LNULL) return;
	inv_tree_head = inv_tree_tail = (INV_TREE*) NULL;
	last = loop->invariant_triples; /* invariant_triple is a "backword" list */
	LFOR( lp, last )
	{
		tp = (TRIPLE*) lp->datap;
		if(ISOP(tp->op,ROOT_OP)) {
			ivtp = (INV_TREE *) ckalloc(1,sizeof(INV_TREE));
			ivtp->from_bp = (BLOCK*) tp->visited;
			ivtp->root = (TRIPLE*) tp;
			ivtp->next = inv_tree_head;
			inv_tree_head = ivtp;
			if(inv_tree_tail ==(INV_TREE*) NULL) {
				inv_tree_tail = ivtp;
			}
			mark_tree(tp);
		}
	}
	last = loop->invariant_triples;
	LFOR( lp, last ) {
		tp = (TRIPLE*) lp->datap;
		if((BLOCK*) tp->visited == (BLOCK*) NULL ) {
			continue;
		}
		parent = find_parent(tp);
		if(tp->expr && tp->expr->recompute) {
			continue;
		}
		if(tp->expr && tp->expr->save_temp != (LEAF*) NULL ) { 
			/* temp already allocated*/
			tmp = tp->expr->save_temp;
		} else {
			tmp = new_temp(tp->type);
			if(tp->expr) {
				tp->expr->save_temp = tmp;
			}
		}
	   	if((TRIPLE*)parent->left == tp) {
		   (LEAF*) parent->left = tmp;
	   	} else {
		   (LEAF*) parent->right = tmp;
	   	}
		root = append_triple(tp, IR_ASSIGN,
			(IR_NODE*)tmp, (IR_NODE*)tp, tp->type);
		ivtp = (INV_TREE *) ckalloc(1,sizeof(INV_TREE));
		ivtp->from_bp = (BLOCK*) tp->visited;
		ivtp->root = root;
		ivtp->next = (INV_TREE*) NULL;

		if(inv_tree_head ==(INV_TREE*) NULL) {
			inv_tree_head = ivtp;
		}
		if(inv_tree_tail != (INV_TREE*) NULL) {
			inv_tree_tail->next = ivtp;
		}
		inv_tree_tail = ivtp;
		mark_tree(root);
	}	
	for(ivtp = inv_tree_head; ivtp ; ivtp = ivtp->next ) {
			from_bp = ivtp->from_bp;
			move_triple(ivtp->root,loop->preheader);
	}
}

/*
**	change the block pointer for all reference records associated with a
**	triple in preparation for moving the triple out of the block it's in
*/
static void
adjust_var_refs(tp, to_bp)
	register TRIPLE *tp;
	BLOCK *to_bp;
{
	register VAR_REF *vrp;

	if( tp->var_refs ) {
		vrp = tp->var_refs;
		while(vrp && vrp->site.tp == tp) {
			vrp->site.bp = to_bp;
			vrp = vrp->next;
		}
	}
	/* note that the var ref lists are no longer in the
	** same order as the triples within blocks 
	*/
}

static void
move_triple(tp,to_bp)
	TRIPLE *tp;
	BLOCK *to_bp;
{
	LIST *lp;

	if(tp->left->operand.tag == ISTRIPLE) {
		move_triple((TRIPLE*)tp->left,to_bp);
	}
	if(ISOP(tp->op,BIN_OP) && tp->right->operand.tag == ISTRIPLE) {
		move_triple((TRIPLE*)tp->right,to_bp);
	}

	adjust_var_refs(tp,to_bp);

	/* first move any associated implicit uses */
	LFOR(lp,tp->implicit_use) {
		move_triple((TRIPLE*)lp->datap, to_bp);
	}

	/* then the triple */
	tp->tprev->tnext = tp->tnext;
	tp->tnext->tprev = tp->tprev;
	if(from_bp->last_triple == tp) {
		quit("move_triple: block does not terminate with a branch");
	}
	tp->tprev = tp->tnext = tp;
	/*
	**	the last triple in the preheader is the goto to the header
	**	so use tprev
	*/
	TAPPEND(to_bp->last_triple->tprev,tp);

	/* then any implicit defs */
	LFOR(lp,tp->implicit_def) {
		move_triple((TRIPLE*)lp->datap, to_bp);
	}
}

#if TARGET == SPARC
static void
mark_const_leaf(tp, leafp) register TRIPLE *tp; register LEAF *leafp;
{
	register LIST *lp;
	register LEAF *addr_leaf;

	if( leafp->class == ADDR_CONST_LEAF ) {
		addr_leaf = leafp->addressed_leaf;
		if( addr_leaf->val.addr.seg != &seg_tab[BSS_LARGE_SEGNO] &&
			addr_leaf->val.addr.seg != &seg_tab[DATA_SEGNO] ) {
				return;
		}
	} else { /* CONST_LEAF */
		if( IS_SMALL_INT( leafp ) ) {
			return;
		}
	}
	
	if( leafp->references == (VAR_REF *) NULL ) { /* first reference of leafp*/
		lp = NEWLIST(loop_lq);
		(LEAF *)lp->datap = leafp;
		LAPPEND( const_leaf_list, lp );
	}
	lp = NEWLIST(loop_lq);
	(TRIPLE *)lp->datap = tp;
	/* use LEAF.references as a pointer to LIST of TRIPLES */
	LAPPEND( ((LIST *)leafp->references), lp );
}

static void
move_const_leaves(loop) LOOP *loop;
{
/*
 * It'll take two CPU cycles for Sparc to setup a 'big constant'
 * in a register.  move_const_leaves will save those constants
 * in the temperaries outside the loop and references to the
 * temperaries in the loop.
 */

	register LIST *lp1, *lp2;
	register LEAF *leafp, *new_leaf;
	register TRIPLE *tp;
	
	LFOR( lp1, const_leaf_list ) {
		leafp = LCAST( lp1, LEAF );
		LFOR( lp2, ((LIST *)leafp->references)) {
			tp = LCAST( lp2, TRIPLE );
			if( tp->visited ) { /* this triple will be moved to outside the loop */
				continue;
			}
			/* else, move it */
			if( leafp->visited ) { /* already allocated a tmp. for leafp */
				new_leaf = (LEAF *)(leafp->visited);
			} else {
				(LEAF *)(leafp->visited) = new_leaf = new_temp( leafp->type );
			}
			LFOR( lp2, ((LIST *)leafp->references)) {
			/* replace constant leaf by new tmp. */
			/* break out after done, so it is safe */
			/* to re-use lp2 and tp */
				tp = LCAST( lp2, TRIPLE );
				if( tp->left == (IR_NODE *)leafp ) {
					tp->left = (IR_NODE *)new_leaf;
				}
				if( ISOP( tp->op, BIN_OP ) &&
					tp->right == (IR_NODE *)leafp ) {
					tp->right = (IR_NODE *) new_leaf;
				}
			}
			/* initialized the new_leaf in pre-header */
			(void) append_triple(loop->preheader->last_triple->tprev,
				IR_ASSIGN, (IR_NODE*)new_leaf, (IR_NODE*)leafp,
				leafp->type );
			break;
		}
		leafp->references = (VAR_REF *)NULL;
	}
}
#endif

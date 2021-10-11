#ifndef lint
static	char sccsid[] = "@(#)resequence.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include "debug.h"
#include "resequence.h"
#include "misc.h"
#include <stdio.h>

/*
** Rearrange the triples in a block so they correspond to a post order walk
** of the ROOT_OP trees. The algorithm, due to A. Fyfe, is :
**
** temporary vars
**    iuse     {list of IMPLICIT_USES }
**    use      {list of triples that belong to a given tree}
**    idef_before {list of IMPLICIT_DEFS due to side effects of FCALLs;
**		   these are applied before the root triple }
**    idef_after  {list of other IMPLICIT_DEFS;
**		   these are applied after the root triple }
**
**  { note :the ordering iuse, use, idef_before, ROOT, idef_after corresponds 
**    to implicit.c }
**
** foreach  ROOT_OP
**    visit left, right, then self
**    unlink all IMPLICIT_DEFS due to this node from the 
**            tnext list
**    if the DEF was due to an FCALL
**            append it to idef_before
**    else 
**            append it to idef_after
**    unlink all IMPLICIT_USES due to this node from the 
**            tnext list and append them to iuse 
**    if the node is not a root op
**            unlink the node from the tnext list and append it to use
**    else
**            concatenate iuse use and idef_before and prepend them to root
**            append idef_after to root
** endfor
**            
*/

/* local */

static TRIPLE *iuse, *use, *idef_before, *idef_after;
static void resequence_root(/*tp*/);
static void visit(/*node*/);
static void unlink_triple(/*tp*/);
static TRIPLE *tconcat(/*tp, item*/);
static void remove_tree(/*tp, bp*/);

void
resequence_triples()
{
	TRIPLE *tp, *first;
	BLOCK *bp;
	int tripleno;

	tripleno = 0;
	for (bp = entry_block; bp != NULL; bp = bp->next) {
		if(bp->last_triple == TNULL)
			continue;
		/*
		 * mark all HOUSE_OPs, ROOT_OPs and their subtrees
		 * as 'visited'.  All other ops must either be
		 * salvaged or discarded, depending on whether
		 * or not they have potential side effects.
		 */
		first = bp->last_triple->tnext;
		idef_before = idef_after = iuse = use = TNULL;
		TFOR(tp, first) {
			if (ISOP(tp->op, HOUSE_OP)) {
				tp->visited = IR_TRUE;
			} else {
				tp->visited = IR_FALSE;
			}
			if ( ISOP(tp->op, ROOT_OP) )  {
				resequence_root(tp);
			}
		}
		/*
		 * scan backwards for trees without roots
		 */
		for (tp=bp->last_triple; tp!=first; tp=tp->tprev) {
			if (!tp->visited) {
				remove_tree(tp, bp);
			}
		}
		/*
		 * reassign triple numbers (e.g., for iv)
		 */
		first = bp->last_triple->tnext;
		TFOR(tp, first) {
			tp->tripleno = tripleno++;
		}
	}
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
resequence_root(tp)
	TRIPLE *tp;
{
	visit((IR_NODE*)tp);
	iuse = tconcat(iuse, use);
	iuse = tconcat(iuse, idef_before);
	(void) tconcat(iuse, tp);
	if(idef_after) {
		idef_after = idef_after->tprev;
		TAPPEND(tp,idef_after);
	}
	idef_before = TNULL;
	idef_after = TNULL;
	iuse = TNULL;
	use = TNULL;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
visit(node)
	IR_NODE *node;
{
	TRIPLE *tmp, *here, *ntuple;
	LIST *lp;

	if(node == NULL || node->operand.tag != ISTRIPLE ) {
		return;
	}
	here = (TRIPLE*) node;
	here->visited = IR_TRUE;
	if ( ISOP(here->op, (USE1_OP|MOD_OP)) ) {
		visit(here->left);
	}
	if( ISOP( here->op, USE2_OP ) ) {
		visit(here->right);
	}
	if( ISOP( here->op, NTUPLE_OP ) ) {
		ntuple = (TRIPLE*)here->right;
		TFOR(tmp, ntuple) {
			/* the ordering of auxiliary triples is not important */
			visit(tmp->left);
		}
	}
	LFOR(lp, here->implicit_def) {
		tmp = LCAST(lp, TRIPLE);
		tmp->visited = IR_TRUE;
		unlink_triple(tmp);
		if(tmp->right->triple.op == IR_FCALL) {
			idef_before = tconcat(idef_before,tmp);
		} else {
			idef_after = tconcat(idef_after,tmp);
		}
	}
	LFOR(lp, here->implicit_use) {
		tmp = LCAST(lp, TRIPLE);
		tmp->visited = IR_TRUE;
		unlink_triple(tmp);
		iuse = tconcat(iuse, tmp);
	}
	if ( !ISOP(here->op, ROOT_OP) ) {
		unlink_triple(here);
		use = tconcat(use, here);
	}
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/* debug routine to print triples in sequence */

static void
tpr_seq(tlp)
	TRIPLE *tlp;
{
	TRIPLE *tp;

	TFOR(tp, tlp) {
		tpr(tp);
	}
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
unlink_triple(tp)
	TRIPLE *tp;
{
	/* no need to update bp->last_triple since we never unlink root triples
	** and last_triple must always be a branch =>ROOT
	*/
	tp->tprev->tnext = tp->tnext;
	tp->tnext->tprev = tp->tprev;
	tp->tprev = tp->tnext = tp;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * concatenate (item) to the end of (tp).
 * Example:
 *	-> (tconcat '(a b c) '(x y z))
 *	(a b c x y z)
 */
static TRIPLE *
tconcat(tp, item)
	TRIPLE *tp, *item;
{
	TRIPLE *tmp;

	if (tp == TNULL)
		return item;
	if (item == TNULL)
		return tp;
	tmp = tp->tprev;
	item = item->tprev;
	TAPPEND(tmp, item);
	return tp;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * remove_tree(tp, bp)
 *	removes a tree, or as much of one as we can get away with.
 *	The algorithm is a postorder tree-walk, which may be pre-empted
 *	by the discovery of potential side effects in a subtree.
 */
static void
remove_tree(tp, bp)
	TRIPLE *tp;
	BLOCK *bp;
{
	TRIPLE *ntuple,*tmp;
	LIST *lp;

	/*
	 * check for possible side effects.
	 */
	if (!can_delete(tp) || may_cause_exception(tp)) {
		/*
		 * this tree gets its own root
		 */
		tp = append_triple(tp, IR_FOREFF,
			(IR_NODE*)tp, (IR_NODE*)NULL, undeftype);
		resequence_root(tp);
		return;
	}
	/*
	 * traverse subtrees
	 */
	if (ISOP(tp->op, (USE1_OP|MOD_OP))
	    && tp->left->operand.tag == ISTRIPLE) {
		remove_tree((TRIPLE*)tp->left, bp);
	}
	if (ISOP(tp->op, USE2_OP)
	    && tp->right->operand.tag == ISTRIPLE) {
		remove_tree((TRIPLE*)tp->right, bp);
	}
	if( ISOP( tp->op, NTUPLE_OP ) ) {
		ntuple = (TRIPLE*)tp->right;
		TFOR(tmp, ntuple) {
			remove_tree((TRIPLE*)tmp->left, bp);
		}
	}
	if(tp->implicit_def != NULL) {
		quita("remove_tree: deleting triple with implicit defs\n");
		/*NOTREACHED*/
	}
	LFOR(lp, tp->implicit_use) {
		tmp = LCAST(lp, TRIPLE);
		(void)free_triple(tmp, bp);
	}
	/*
	 * delete root (not a ROOT_OP)
	 */
	(void)free_triple(tp, bp);
}

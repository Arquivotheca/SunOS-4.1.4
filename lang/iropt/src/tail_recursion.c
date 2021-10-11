#ifndef lint
static  char sccsid[] = "@(#)tail_recursion.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * tail recursion elimination
 */

#include <stdio.h>
#include "iropt.h"
#include "page.h"
#include "misc.h"
#include "lu.h"
#include "tail_recursion.h"

static char	*pname;
static TYPE	undef_type;
static void	find_recursion(/*bp*/);
static void	right_pattern(/*bp, tp, leafp*/);
static void	make_iterate(/*call,bp*/);

void
do_tail_recursion()
{
	SEGMENT *segp;
	BLOCK *bp;
	LIST *lp;

	/* check if there is any local variables or parameters aliasing */
	for (segp = seg_tab; segp != NULL; segp = segp->next_seg) {
		if (segp->descr.class == AUTO_SEG) {
			LFOR(lp, segp->leaves) {
				if(((LEAF*)lp->datap)->no_reg == IR_TRUE){
					return;
				}
			}
		}
	}
	LFOR (lp, seg_tab[ARG_SEGNO].leaves){
		if(((LEAF*)lp->datap)->no_reg == IR_TRUE){
			return;
		}
	}
	pname = hdr.procname;
	for (bp = entry_block; bp; bp = bp->next){
		bp->visited = IR_FALSE;
	}
	find_recursion(exit_block);
}

/*
 * Trace back from the exit_block until find a root_op in a block which
 * has only *one* successor, if
 *   1. if it is fval or scall, check if it a tail recursion
 *   2. if it is istore or assign, stop
 *   3. else ignore
 */
static void
find_recursion(bp)
	BLOCK *bp;
{
	TRIPLE *tp;
	LIST *lp;
	LEAF *leafp;

	bp->visited = IR_TRUE;
	for(tp = bp->last_triple; tp != bp->last_triple->tnext; tp = tp->tprev){
		switch(tp->op){
		case IR_ASSIGN:
		case IR_ISTORE:
			return;
		case IR_SCALL:
			if (((LEAF*)tp->left)->class == CONST_LEAF &&
			    !strcmp(((LEAF*)tp->left)->val.const.c.cp, pname)){
				make_iterate(tp, bp);
			}
			return;
		case IR_FVAL:
			leafp = (LEAF*)tp->left;
			right_pattern(bp, tp, leafp);
			return;
		default:;
		}
	}
	LFOR (lp, bp->pred){
		BLOCK *pred_bp;

		pred_bp = (BLOCK *)lp->datap;
		if (pred_bp->succ != pred_bp->succ->next ||
		    pred_bp->visited == IR_TRUE)
			continue;
		find_recursion(pred_bp);
	}
}

/*
 * See if it is ok to do the tail recursion elimination
 * We like to find
 *   T[]  fcall procname
 *        assign L[] T[]
 *        fval L[]        <--- tp
 */
static void
right_pattern(bp, tp, leafp)
	BLOCK *bp;
	TRIPLE *tp;
	LEAF *leafp;
{
	LIST *lp;

	bp->visited = IR_TRUE;
	for (tp = tp->tprev; tp != bp->last_triple; tp = tp->tprev){
		if (ISOP(tp->op, VALUE_OP))
			return;
		else if (tp->op == IR_FCALL || tp->op == IR_SCALL || 
			 tp->op == IR_ISTORE || tp->op == IR_FVAL)
			return;
		else if (tp->op == IR_ASSIGN){
			if ((LEAF*)tp->left != leafp || 
			    tp->right->operand.tag != ISTRIPLE ||
			    (tp = (TRIPLE*)tp->right)->op != IR_FCALL ||
			    ((LEAF*)tp->left)->class != CONST_LEAF ||
			    strcmp(((LEAF*)tp->left)->val.const.c.cp, pname))
				return;
			else {
				make_iterate(tp,bp);
				return;
			}
		} else
			continue;

	}
	LFOR (lp, bp->pred){
		BLOCK *pred_bp;

		pred_bp = (BLOCK *)lp->datap;
		if (pred_bp->succ != pred_bp->succ->next ||
		    pred_bp->visited == IR_TRUE)
			continue;
		right_pattern(pred_bp, pred_bp->last_triple, leafp);
	}
}

/*
 * Make it iterate
 *   1. assign all the actual parameters to temps
 *   2. assign all the temps to formal parameters
 *   3. goto block 1
 */
static void
make_iterate(call,bp)
	TRIPLE *call;
	BLOCK *bp;
{
	TRIPLE *tp, *param_tp, *ref_tp, *seq;
	LIST *lp;
	LEAF *param, *tmp, *arg;
	BLOCK *begin;
	int nparam, narg;

	undef_type.tword = UNDEF;
	undef_type.size = 0;
	undef_type.align = 0;
	begin = (BLOCK*)entry_block->succ->datap;

	/* check if the same number of parameters */
	tp = call;
	if(call->right != NULL){
		nparam = 1;
		for(param_tp = call->right->triple.tnext;
		    param_tp != (TRIPLE*)(call->right);
		    param_tp = param_tp->tnext)
			nparam++;
	} else
		nparam = 0;
	narg = 0;
	LFOR (lp, seg_tab[ARG_SEGNO].leaves){
		narg++;
	}
	if (nparam != narg )
		return;
	if (narg == 0)
		/* no argument, go ahead */
		goto doit;

	/* check the same type */
	param_tp = call->right->triple.tnext;
	LFOR (lp, seg_tab[ARG_SEGNO].leaves){
		arg = (LEAF*)lp->datap;
		param = (LEAF*)param_tp->left;
		if (same_irtype(arg->type, param->type) == IR_FALSE)
			return;
		param_tp = param_tp->tnext;
	}

	tp = call;
	param_tp = call->right->triple.tnext;
	LFOR (lp, seg_tab[ARG_SEGNO].leaves){
		arg = (LEAF*)lp->datap;
		param = (LEAF*)param_tp->left;
		tmp = new_temp(param->type);
		tp = append_triple(tp, IR_ASSIGN,
			(IR_NODE*)tmp, (IR_NODE*)param, param->type);
		param_tp = param_tp->tnext;
	}
	seq = call;
	param_tp = call->right->triple.tnext;
	LFOR (lp, seg_tab[ARG_SEGNO].leaves){
		seq = seq->tnext;
		arg = (LEAF*)lp->datap;
		param = (LEAF*)param_tp->left;
		tmp = (LEAF*)seq->left;
		tp = append_triple(tp, IR_ASSIGN,
			(IR_NODE*)arg, (IR_NODE*)tmp, arg->type);
		param_tp = param_tp->tnext;
	}
doit:
	if(SHOWCOOKED == IR_TRUE){
		(void)fprintf(stderr,"* tail recursion elimination in %s *\n", pname);
	}
	ref_tp = append_triple(TNULL, IR_LABELREF, (IR_NODE*)begin,
			(IR_NODE*)ileaf(0), undef_type);
	tp = append_triple(tp, IR_GOTO, (IR_NODE*)NULL, (IR_NODE*)ref_tp, 
			undef_type);
	(void)connect_block(begin->pred, bp);
	(void)connect_block(bp->succ, begin);
	(void)free_triple(call, bp);
	for (tp = tp->tnext; tp != bp->last_triple->tnext; tp = tp->tnext)
		tp = free_triple(tp, bp);
}

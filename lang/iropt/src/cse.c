#ifndef lint
static  char sccsid[] = "@(#)cse.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include "page.h"
#include "misc.h"
#include "make_expr.h"
#include "recordrefs.h"
#include "cse.h"
#include <stdio.h>
#include <ctype.h>

/* 
 * This file contains the major function of doing common subexpression
 * elimination.  Right now, it will do the CSE locally (within the basic block)
 * first and then, after the global data flow information has been re-computed,
 * it will do the CSE globally (cross the basic block).
 */

/* local */
static TRIPLE	*find_local_base();
static void	eliminate_local_cs(/*cs_tp, reachdef*/);
static TRIPLE	*find_local_base(/*reachdef*/);
static void	eliminate_global_cs(/*bp, cs_triple, reachdef*/);


void
cse_init()
{
	register BLOCK *bp;
	register TRIPLE *tp, *last;

	if(cse_lq == NULL) cse_lq = new_listq();
	for(bp=entry_block; bp; bp=bp->next) {
		last = bp->last_triple;
		TFOR(tp,last) {
			(TRIPLE*) tp->visited = TNULL;
		}
	}
}

void
do_local_cse()
{ /* 
   * This procedure will be called before the global data flow information
   * has been computed.  It will detect ALL the local common subexpression
   * and then call to the eliminate_local_cs.
   */

	register BLOCK *bp;
	register TRIPLE *tp, *tlp;

	for( bp = entry_block; bp; bp = bp->next )
	{ /* walk the triple backward, so it can catch the */
	  /* largest common supexpression first. */
		tp = bp->last_triple;
		tlp = TNULL;
		
		do
		{
			if( tp->reachdef1 /* can be reached */
			    && tp->left->operand.tag == ISTRIPLE
			    && tp->reachdef1->next != tp->reachdef1 )
				eliminate_local_cs( (TRIPLE*)tp->left,
				    tp->reachdef1 );
			if( tp->reachdef2 /* can be reached */
			    && tp->right->operand.tag == ISTRIPLE
			    && tp->reachdef2->next != tp->reachdef2 )
				eliminate_local_cs( (TRIPLE*)tp->right,
				    tp->reachdef2 );
			if( tlp ) { /* we are scanning parameter list */
				tp = tp->tnext;
				if( tp == (TRIPLE *)tlp->right ) {/* done with PARM */
					tp = tlp;
					tlp = TNULL;
				}
			} else { 
				tp = tp->tprev;
				if( ( tp->op == IR_SCALL || tp->op == IR_FCALL ) && ( tp->right ) ) {
					tlp = tp;
					tp = (TRIPLE *) tlp->right;
				}
			}
		}while( tp != bp->last_triple );
	}
}

static void
eliminate_local_cs( cs_tp, reachdef ) 
	TRIPLE *cs_tp;
	LIST *reachdef;
{
	TRIPLE *base_triple, *tp;
	LIST *lp, *can_reach;

	if (cs_tp->expr != NULL && cs_tp->expr->recompute )
		return;
	base_triple = find_local_base( reachdef ); /* base_triple is the one */
					           /* that going to be saved */
						   /* and re-used */
	can_reach = copy_list(base_triple->canreach,cse_lq);
	LFOR( lp, can_reach )
	{
		tp = LCAST(lp, EXPR_REF)->site.tp;
		if( base_triple->expr != (EXPR*)NULL &&
			((TRIPLE *)tp->left)->expr == base_triple->expr )
		{
			tp->left = (IR_NODE *)base_triple;
			fix_reachdef( IR_FALSE, tp, tp->reachdef1, base_triple );
			tp->reachdef1 = LNULL;
		}

		if( ISOP( tp->op, BIN_OP ) && base_triple->expr != (EXPR*)NULL &&
			((TRIPLE *)tp->right)->expr == base_triple->expr )
		{
			tp->right = (IR_NODE *)base_triple;
			fix_reachdef( IR_FALSE, tp, tp->reachdef2, base_triple );
			tp->reachdef2 = LNULL;
		}
	}
}

static TRIPLE *
find_local_base( reachdef ) register LIST *reachdef;
{ /* Find the base for the local common subexpression.
   * For those common subexpressions in a single basic block, the base
   * triple is one that has the longest canreach chain.
   */
	register LIST *lp, *list_ptr;
	register TRIPLE *tp, *base_triple;
	register max_count = 0;
	register count;

	LFOR( lp, reachdef )
	{
		tp = LCAST(lp, EXPR_REF)->site.tp;
		count = 0;
		LFOR(list_ptr, tp->canreach)
		{
			++count;
		}
		if( count > max_count )
		{
			max_count = count;
			base_triple = tp;
		}
	}
	if( max_count == 0 )
			quit("can not find local base: find_local_base");

	return( base_triple );
}

void
do_global_cse()
{
	BLOCK *bp;
	TRIPLE *tp, *tp1, *tlp;
	LIST *lp, *lp1;
	LEAF *new_leaf;
	VAR_REF *var_rp;

	for( bp = entry_block; bp; bp = bp->next )
	{ /* for each basic block */
		tp = bp->last_triple;
		tlp = TNULL;
		
		do
		{ /* for each triple */
			if( !tp->inactive ) {
				if( tp->reachdef1 /* can be reached */
				    && tp->left->operand.tag == ISTRIPLE
				    && tp->reachdef1->next != tp->reachdef1
				    && tp->left->triple.expr != (EXPR*)NULL)
					eliminate_global_cs( bp,
					    (TRIPLE*)tp->left, tp->reachdef1 );
				if( tp->reachdef2 /* can be reached */
				    && tp->right->operand.tag == ISTRIPLE
				    && tp->reachdef2->next != tp->reachdef2
				    && tp->right->triple.expr != (EXPR*)NULL)
					eliminate_global_cs( bp,
					    (TRIPLE*)tp->right, tp->reachdef2 );
			}
			if( tlp ) { /* we are scanning parameter list */
				tp = tp->tnext;
				if( tp == (TRIPLE *)tlp->right ) {/* done with PARM */
					tp = tlp;
					tlp = TNULL;
				}
			} else { 
				tp = tp->tprev;
				if( ( tp->op == IR_SCALL || tp->op == IR_FCALL ) && ( tp->right ) ) {
					tlp = tp;
					tp = (TRIPLE *) tlp->right;
				}
			}
		}while( tp != bp->last_triple );
	}
	for( bp = entry_block; bp; bp = bp->next )
	{ /* for each basic block */
		tlp = TNULL;
		
		TFOR( tp, bp->last_triple->tnext )
		{ /* for each triple */
			if( tlp /* scanning parameter list */ && tp == (TRIPLE *)tlp->right ) {
				/* done with parameter list, go back to normal list */
				tp = tlp;
				tlp = TNULL;
			} else {
				if( ( tp->op == IR_SCALL || tp->op == IR_FCALL ) && tp->right ) {
					tlp = tp;
					tp = (TRIPLE *)tlp->right;
				}
			}
			if (tp->inactive)
				continue;
			if( tp->op == IR_ASSIGN )
			{
				if( tp->canreach == LNULL )
				{
					if( can_delete( tp ) )
					{
						delete_triple( IR_TRUE, tp, bp );
					}
					continue;
				}
				/*
				 * check for the following pattern:
				 *	T[m] ....	canreach T[n]
				 *	...
				 *	T[n] ASSIGN L[..], T[m] canreach T[m]
				 * This is an assignment whose result is used only by its
				 * own rhs, which in turn is used only by the assignment.
				 * Note that canreach == NULL for expressions with
				 * incomplete data flow information (i.e., if global
				 * variables are used and partial_opt == TRUE).
				 */
				if( tp->canreach == tp->canreach->next )
				{ /* one usage of this assignment */
					TRIPLE *reachp;
					reachp = (TRIPLE *)tp->right;
					if( reachp == LCAST(tp->canreach, EXPR_REF)->site.tp
						&& reachp->canreach != NULL
						&& reachp->canreach == reachp->canreach->next)
					{ /* only usage is to re-define itself */
						if( can_delete( tp ) )
						{
							delete_triple( IR_TRUE, tp, bp );
						}
					}
				}
				continue;
			}	
	
			if( ! ISOP(tp->op, VALUE_OP) || tp->expr == (EXPR*)NULL)
				continue;

			if( tp->canreach )
			{
				if( ( ( tp->canreach != tp->canreach->next ) ||
					( LCAST(tp->canreach, EXPR_REF)->site.bp != bp ) ) &&
							! tp->expr->recompute &&
							! tp->visited )
				{ /* be used more than one place or be used in another
				  /* block and has not been saved yet. */
					if( tp->expr && tp->expr->save_temp ) 
					{ /* already allocated */
						new_leaf = (LEAF *)tp->expr->save_temp;
					}
					else
					{
						new_leaf = new_temp(tp->type);
						if(tp->expr)
						{
							tp->expr->save_temp = new_leaf;
						}
					}
					(TRIPLE*)tp->visited = append_triple(
						tp, IR_ASSIGN,
						(IR_NODE*)new_leaf,
						(IR_NODE*)tp, tp->type);
					/* build canreach chain */
					var_rp = new_var_ref(IR_FALSE);
					var_rp->reftype = VAR_DEF;
					var_rp->site.tp = (TRIPLE *)tp->visited;
					var_rp->site.bp = bp;
					
					((TRIPLE*)tp->visited)->canreach =
						copy_list( tp->canreach, df_lq );

					tp->canreach = NEWLIST( df_lq );
					(VAR_REF *)tp->canreach->datap = var_rp;
					
					LFOR(lp, ((TRIPLE *)tp->visited)->canreach )
					{ /* replace with the new leaf */
						tp1 = LCAST(lp,EXPR_REF)->site.tp;
						if( tp1->left == (IR_NODE *)tp )
						{
							tp1->left = (IR_NODE *)new_leaf;
							LFOR( lp1, tp1->reachdef1 )
							{
								if(LCAST(lp1,EXPR_REF)->site.tp == tp )
								{
									(VAR_REF *)lp1->datap = var_rp;
									break;
								}
							}
						}
						if( ISOP(tp1->op, BIN_OP) && 
								( tp1->right == (IR_NODE *)tp ) )
						{
							tp1->right = (IR_NODE *)new_leaf;
							LFOR(lp1, tp1->reachdef2)
							{
								if( LCAST(lp1,EXPR_REF)->site.tp == tp )
								{
									(VAR_REF *)lp1->datap = var_rp;
									break;
								}
							}
						}
					}
				}
			}
			else
			{ /* VALUE_OP but reaches nowhere */
				if( can_delete( tp ) )
				{
					delete_triple( IR_TRUE, tp, bp );
				}
			}
		} /*TFOR*/
	}
	remove_dead_triples();
	empty_listq(cse_lq);
}

static void
eliminate_global_cs( bp, cs_triple, reachdef )
	BLOCK *bp;
	TRIPLE *cs_triple;
	LIST *reachdef;
{
	TRIPLE *base_triple, *tp;
	LIST *can_reach, *lp1, *lp2, *reach_def;

	if (cs_triple->expr->recompute)
		return;
	reach_def = copy_list(reachdef,cse_lq);

	/* replace the expression reference in the triple by the base_triple */
	LFOR( lp1, reach_def )
	{
		if( ( base_triple = LCAST(lp1, EXPR_REF)->site.tp ) == cs_triple )
			continue;

		if( LCAST(lp1,EXPR_REF)->site.bp == bp || 
		    base_triple->expr == (EXPR*)NULL)
		/* assume local CSE has been done */
			continue;

		/* found a non-local base_triple */
		/* pick up any non-local base_triple should work */
		can_reach = copy_list( cs_triple->canreach ,cse_lq);

		/* delete the near by one */
		delete_triple( IR_TRUE, cs_triple, bp );

		LFOR(lp2, can_reach )
		{
			tp = LCAST(lp2, EXPR_REF)->site.tp;

			if( ( (TRIPLE *)tp->left ) == cs_triple )
			{
				tp->left = (IR_NODE *)base_triple;

				/* make the base_triple be the only one on */
				/* tp"s reachdef1 chain, so the cse won"t over do it */
				tp->reachdef1 = NEWLIST( df_lq );
				tp->reachdef1->datap = lp1->datap;
			}

			if( ( ISOP( tp->op, BIN_OP ) ) && ((TRIPLE *)tp->right == cs_triple ))
			{
				tp->right = (IR_NODE *)base_triple;
				tp->reachdef2 = NEWLIST( df_lq );
				tp->reachdef2->datap = lp1->datap;
			}
		}
		break;
	}
}


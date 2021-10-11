#ifndef lint
static	char sccsid[] = "@(#)copy_ppg.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

# include "iropt.h"
# include "page.h"
# include "misc.h"
# include "bit_util.h"
# include "recordrefs.h"
# include "copy_ppg.h"
# include "copy_df.h"
# include <stdio.h>

/* global */
SET_PTR	copy_kill, copy_gen, copy_in, copy_out;
int	copy_set_wordsize;

/* local */
static BOOLEAN still_in(/*var, ref, triple*/);
static void replace(/* var_use, tp, by_triple */);
static void global_replace(/*bp, triple, var_use, by_copy */);
static void delete_define_at(/* tp, cp */);
static void fix_copy_in(/* bp, cp, base_copy*/);
static BOOLEAN both_local_var(/* leafp1, leafp2 */);


static BOOLEAN
still_in(var, ref, triple)
	LEAF *var;
	EXPR_REFERENCE_TYPE ref;
	TRIPLE *triple;
{
	LIST *lp, *reachdef;
	TRIPLE *tp;

	if (ref == EXPR_EXP_USE1)
		reachdef = triple->reachdef1;
	else /* ref = EXPR_EXP_USE2 */
		reachdef = triple->reachdef2;

	LFOR(lp, reachdef)
	{
		tp = LCAST(lp, VAR_REF)->site.tp;
		if ((LEAF*)tp->right != var)
			return(IR_FALSE);
	}
	return(IR_TRUE);
}

void
do_local_ppg()
{ /* do copy propagation locally -- within the basic block */

	register BLOCK *block_ptr;
	register LIST *lp;
	register TRIPLE *tp1, *tp2;
	VAR_REF  *var_use;
	
	if(copy_lq == NULL) copy_lq = new_listq();

	for( block_ptr = entry_block; block_ptr; block_ptr = block_ptr->next )
	{
		TFOR( tp1, block_ptr->last_triple->tnext )
		{
			if( tp1->op == IR_ASSIGN && interesting_copy( tp1 ) )
			{
				LFOR(lp, tp1->canreach )
				{
					var_use = LCAST(lp, VAR_REF);
					tp2 = var_use->site.tp;
					if( tp2->op == IR_IMPLICITUSE ||
						tp2->op == IR_EXITUSE )
						continue;

					if( no_change(tp1->tnext,
					    tp2, (LEAF*)tp1->right ) )
						replace( var_use, tp2, tp1 );
				}
			}
		}
	}
}

void
do_global_ppg()
{
	LIST **lpp, *lp1, *lp2, *lp3, *can_reach, *copy_list();
	int copyno;
	LEAF *right;
	COPY *cp;
	TRIPLE *tp, *triple;
	BLOCK *bp;
	VAR_REF *var_use;

	if(copy_lq == NULL) copy_lq = new_listq();

	/* compute the data flow infomation */
	entable_copies();
	if( ncopies == 0 ) /* no interested copy */
		return;
	copy_set_wordsize = ( roundup((unsigned)ncopies,BPW) ) / BPW;
	copy_gen = new_heap_set(nblocks, ncopies);
	copy_kill = new_heap_set(nblocks, ncopies);
	copy_in = new_heap_set(nblocks, ncopies);
	copy_out = new_heap_set(nblocks, ncopies);
	build_copy_kill_gen();
	build_copy_in_out();
	for( lpp = copy_hash_tab; lpp < &copy_hash_tab[COPY_HASH_SIZE]; lpp++ )
	{ /* for all the interested copies */
		LFOR(lp1, *lpp)
		{ /* for each interested copy */

			cp = (COPY *)lp1->datap;
			cp->visited = IR_TRUE;	/* been processed */
			copyno = cp->copyno;
			right = cp->right;
			LFOR(lp2, cp->define_at)
			{ /* for each occurs */
				tp = LCAST(lp2, COPY_REF)->site.tp;
				can_reach = copy_list( tp->canreach, copy_lq );
				LFOR(lp3, can_reach)
				{ /* for all the triple it can reach */
					var_use = LCAST(lp3, VAR_REF);
					triple = var_use->site.tp;
					if( triple->op == IR_IMPLICITUSE ||
						triple->op == IR_EXITUSE )
						continue;

					bp = var_use->site.bp;
					if(bit_test(copy_in, bp->blockno, copyno) )
					{ /* in the IN[] SET */
						if( no_change(bp->last_triple->tnext,
								triple,	right) &&
						    still_in(right, 
								var_use->reftype, triple))
						{ /* no local change */
							global_replace( bp,
									triple,
									var_use, cp );
						}
					}
				}
			}
		}
	}

	for( bp = entry_block; bp; bp = bp->next )
	{ /* for each basic block */
		if( bp->last_triple )
		{
			TFOR( triple,  bp->last_triple->tnext )
			{ /* clean up */
				if( triple->canreach == LNULL
				    && triple->op == IR_ASSIGN
				    && triple->left->operand.tag == ISLEAF
				    && triple->right->operand.tag == ISLEAF
				    && can_delete( triple ) ) {
					if( ! partial_opt
					    || both_local_var((LEAF*)triple->left,
						    (LEAF*)triple->right)){
						remove_triple( triple, bp );
					}
				} else {
					if( triple->op == IR_ASSIGN &&
							triple->left == triple->right ) {
					/* FIX THE CANREACH FIRST */
						LFOR( lp1, triple->reachdef2 ) {
							tp = LCAST(lp1,VAR_REF)->site.tp;
							lp2 = copy_list(triple->canreach, df_lq);
							tp->canreach = merge_lists(tp->canreach,lp2);
						}
						LFOR( lp1, triple->implicit_def)
						{ /* delete the implicit def with the triple */
							remove_triple( (TRIPLE*)lp1->datap, bp );
						}
						remove_triple( triple, bp );
					}
				}
			}
		}
	}
	empty_listq(copy_lq);
	free_heap_set(copy_gen);
	free_heap_set(copy_kill);
	free_heap_set(copy_in);
	free_heap_set(copy_out);
}

/*
 * propagate the rhs of an assignment to a reached use
 * within another triple, in the same block.  The correct
 * operand is determined by a VAR_REF representing the reached
 * use.
 */
static void
replace( var_use, tp, by_triple )
	VAR_REF *var_use;
	register TRIPLE *tp, *by_triple;
{
	switch(var_use->reftype) {
	case VAR_USE1:
	case VAR_EXP_USE1:
		if (tp->left != by_triple->left)
			break;
		tp->left = by_triple->right;
		return;
	case VAR_USE2:
	case VAR_EXP_USE2:
		if (tp->right != by_triple->left)
			break;
		tp->right = by_triple->right;
		return;
	default:
		break;
	}
	quita("illegal replacement in local_ppg");
	/*NOTREACHED*/
}

/*
 * propagate the rhs of an assignment to a reached use
 * within another triple, possibly in a different block.
 * The correct operand is determined by a VAR_REF
 * representing the reached use.  Update the canreach and
 * reachdefs lists.  We are very close to the abyss here.
 */
static void
global_replace(bp, triple, var_use, by_copy ) 
	BLOCK *bp; 
	TRIPLE *triple; /* triple->will be modified */
	VAR_REF *var_use;	/* reached use from the set canreach(by_copy) */
	COPY *by_copy;
{
	TRIPLE *tp1, *tp2;
	LIST *lp1, *lp2, *lp3, *reachdef, *copy_list();
	void fix_copy_in(), delete_define_at();
	BLOCK *block_ptr;

	switch(var_use->reftype) {
	case VAR_USE1:
	case VAR_EXP_USE1:
		if (triple->left != (IR_NODE*)by_copy->left) {
			quita("global_replace: VAR_USE1 mismatch");
			/*NOTREACHED*/
		}
		triple->left = (IR_NODE *)by_copy->right;
		reachdef = copy_list( triple->reachdef1, copy_lq );
		triple->reachdef1 = (LIST *) NULL;
		
		/* modify the canreach and reachdef */

		LFOR( lp3, reachdef ) {/* for all the defination points */
			tp2 = LCAST( lp3, VAR_REF )->site.tp;
			LFOR( lp1, tp2->reachdef2 ) {
				tp1 = LCAST( lp1, VAR_REF )->site.tp;
				lp2 = NEWLIST( df_lq );
				lp2->datap = (LDATA*)var_use;
				LAPPEND(tp1->canreach, lp2);
				lp2 = NEWLIST( df_lq );
				lp2->datap = lp1->datap;
				LAPPEND(triple->reachdef1, lp2);
			}
		}
		fix_reachdef( IR_FALSE, triple, reachdef, TNULL );
		return;

	case VAR_USE2:
	case VAR_EXP_USE2:
		if( !ISOP( triple->op, BIN_OP )
		    || triple->right != (IR_NODE*)by_copy->left ) {
			quita("global_replace: VAR_USE2 mismatch");
			/*NOTREACHED*/
		}
		triple->right = (IR_NODE *)by_copy->right;
		reachdef = copy_list(triple->reachdef2, copy_lq);
		triple->reachdef2 = (LIST *)NULL;
		
		/* modify the canreach */
		LFOR( lp3, reachdef ) {/* for all the defination points */
			tp2 = LCAST( lp3, VAR_REF )->site.tp;
			LFOR( lp1, tp2->reachdef2 ) {
				tp1 = LCAST( lp1, VAR_REF )->site.tp;
				lp2 = NEWLIST( df_lq );
				lp2->datap = (LDATA*)var_use;
				LAPPEND(tp1->canreach, lp2);
				lp2 = NEWLIST( df_lq );
				lp2->datap = lp1->datap;
				LAPPEND(triple->reachdef2, lp2);
			}
		}
		fix_reachdef( IR_FALSE, triple, reachdef, TNULL );

		/* replace the right leaf, will require more work */
		/* because triple could be another interested copy */
		
		if( triple->op == IR_ASSIGN && triple->expr && 
					( ! ((COPY*)triple->expr)->visited ))
		{ /* this triple 'generate' a copy
		   * and has not been processed.
		   * delete triple from the copy->define_at list
		   */
		   	delete_define_at( triple, (COPY *)triple->expr );
			fix_copy_in( bp, (COPY*)triple->expr, by_copy );
			for( block_ptr = entry_block; 
					block_ptr; 
						block_ptr = block_ptr->next )
			{
				block_ptr->visited = IR_FALSE;
			}
		}
		return;

	default:
		break;
	}
	quita("illegal replacement in global_ppg");
	/*NOTREACHED*/
}

static void
delete_define_at( tp, cp ) TRIPLE *tp; COPY *cp;
{
	LIST *lp;

	LFOR( lp, cp->define_at ) {
		if( tp == LCAST( lp, COPY_REF)->site.tp ) {
			delete_list( &cp->define_at, lp );
			break;
		}
	}
}

static void
fix_copy_in( bp, cp, base_copy)
	BLOCK *bp;
	COPY *cp, *base_copy;
{ /*
   * Right now, we will only try to fix the IN[] set for the old
   * COPY.  The right way to do it is to re-compute the data flow 
   * information, but for a fast fix, we use the equation as following :
   * for all the successor of bp set the corresponding bit of the old
   * copy iff both bits for the old and the base copies are set.
   */

	LIST *lp;

	bp->visited = IR_TRUE;
	LFOR(lp, bp->succ)
	{
		BLOCK *blp = LCAST(lp, BLOCK);
		if( !blp->visited )
			fix_copy_in( blp, cp, base_copy );
	}

	if( ! bit_test( copy_in, bp->blockno, base_copy->copyno ) )
		bit_reset( copy_in, bp->blockno, cp->copyno );

}

static BOOLEAN
both_local_var( leafp1, leafp2 ) LEAF *leafp1, *leafp2;
{
	/*
	 * return IR_TRUE iff neither leafp1 nor leafp2 is exteranl
	 * or HEAP variable.
	 */
	register union leaf_value *rv, *lv;

	lv = &leafp1->val;
	if( lv->addr.seg->descr.external == EXTSTG_SEG
	    || lv->addr.seg->descr.class == HEAP_SEG ) {
		return IR_FALSE;
	}
	if( ! ISCONST( leafp2 ) ) {
		rv = &leafp2->val;
		if( rv->addr.seg->descr.external == EXTSTG_SEG
		    || rv->addr.seg->descr.class == HEAP_SEG ) {
			return IR_FALSE;
		}
	}
	return IR_TRUE;
}

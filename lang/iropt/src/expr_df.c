#ifndef lint
static	char sccsid[] = "@(#)expr_df.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include "page.h"
#include "misc.h"
#include "bit_util.h"
#include "recordrefs.h"
#include "make_expr.h"
#include "expr_df.h"
#include "var_df.h"
#include "debug.h"
#include <stdio.h>

/* export */

SET_PTR expr_gen, expr_kill, expr_in;
SET_PTR exprdef_gen, exprdef_kill, exprdef_in;
SET_PTR exprdef_mask, all_exprdef_mask;
int exprdef_set_wordsize;
int expr_set_wordsize;
int navailexprdefs;

/* locals */

static int expr_avail_defno;
static LIST *blowup_exprdef_in_bits(/*ep,bp*/);

void
build_expr_sets(setbit_flag)
	BOOLEAN setbit_flag;
{
/* This routine will build expr_gen, expr_kill, exprdef_gen and the */
/* exprdef_mask.  After the expr_kill for each block and the exprdef_mask */
/* have been built, it will call build_exprdef_kill to build exprdef_kill*/

	register EXPR_REF *rp, *first_rp;
	register EXPR *ep, **epp;
	LIST *gen_list;
	EXPR_KILLREF *last_kill;
	int this_block, blockno;
	TRIPLE *tp;
	BOOLEAN killed;
	SET_PTR killed_in;	/* set of blocks in which expr is killed */
	register int exprno, i, n;

	if(setbit_flag == IR_TRUE) {
		killed_in = new_heap_set(1, nblocks);
	} else {
		killed_in = NULL;
	}
	if(entry_block->blockno != 0) quit("build_exprsets: FIXME");
	expr_avail_defno = 0;
	gen_list = LNULL;
	last_kill = (EXPR_KILLREF*) NULL;
	/*
	**	examine all expressions
	*/
	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) 
	{
		for( ep = *epp; ep; ep = ep->next_expr )
		{
			if(ep->op == IR_ERR || ep->references == (EXPR_REF*) NULL) 
			{
				continue;
			}


			if( setbit_flag == IR_TRUE )
			{
				n = SET_SIZE(killed_in);
				for(i=0; i < n; i++)
				{
					killed_in->bits[i] = 0;
				}
			}

			exprno = ep->exprno;
			first_rp = ep->references;

			/*
			**	for each expression examine all references
			*/
			while(first_rp)
			{ /* for this expression */
				killed = IR_FALSE;
				if(first_rp->reftype == EXPR_KILL) {
					this_block = ((EXPR_KILLREF*)first_rp)->blockno;
				} else {
					this_block =  first_rp->site.bp->blockno;
				}
				gen_list = LNULL;
				last_kill = (EXPR_KILLREF*) NULL;
				empty_listq(tmp_lq);
				rp = first_rp;

				do
				{ /* for this basic block */
					switch(rp->reftype) 
					{
						case EXPR_GEN:
							(void)insert_list(&gen_list, (LDATA*)rp, tmp_lq);
							last_kill = (EXPR_KILLREF*) NULL;
							break;

						case EXPR_KILL:
							killed = IR_TRUE;
							/*
							** kill all definitions
							*/
							last_kill = (EXPR_KILLREF*) rp;
							gen_list = LNULL;
							break;

						case EXPR_USE1:
							tp = rp->site.tp;
							/*
							**	put all live definitions
							**	on the reachdef list
							*/
							tp->reachdef1 = 
							    copy_list(gen_list,df_lq);

							if( ! killed )
							{
								rp->reftype = 
								  EXPR_EXP_USE1;
							}
							break;

						case EXPR_USE2:
							tp = rp->site.tp;
							/*
							**	put all live definitions
							**	on the reachdef list
							*/
							tp->reachdef2 = 
							    copy_list(gen_list,df_lq);
	
							if( ! killed )
							{
								rp->reftype = 
								  EXPR_EXP_USE2;
							}
							break;
					}
					rp = rp->next_eref;
					if(rp) {
						if(rp->reftype == EXPR_KILL) {
							blockno = ((EXPR_KILLREF*)rp)->blockno;
						} else {
							blockno =  rp->site.bp->blockno;
						}
					} else {
						blockno = -1;
					}
				} while( blockno == this_block) ;
				first_rp = rp;
				/*
				** if any kills of this expr were found 
				** in this block note so
				*/
				if( setbit_flag == IR_TRUE )
				{
					if( killed == IR_TRUE )
					{
						set_bit(killed_in->bits,
							this_block);
					}

					if( gen_list )
					{ /* mark the expr_gen, expr_def_gen */
						build_expr_gen( gen_list, exprno);
					}
					else
					{
						if( last_kill )
						{ /*	there is a lasting kill of this expr */
						  /*	 mark it in the expr_kill set */
							bit_set( expr_kill, 
									last_kill->blockno, ep->exprno );
						}
					}
				}
			}
			if( setbit_flag == IR_TRUE )
				build_exprdef_kill( killed_in, exprno );
		}
	}
	empty_listq(tmp_lq);
	free_expr_killref_space();
	if(setbit_flag == IR_TRUE && expr_avail_defno != navailexprdefs) {
		quita("build_expr_sets: error: expr_avail_defno=%d navailexprdefs=%d",
			expr_avail_defno,navailexprdefs);
	}
	if (killed_in != NULL) {
		free_heap_set(killed_in);
	}
}

LOCAL
build_expr_gen( gen_list , exprno ) 
	LIST *gen_list;
	register int exprno;
{
	register LIST *lp;
	register EXPR_REF *rp;
	register SET mask;

	rp = LCAST( gen_list, EXPR_REF );
	bit_set( expr_gen, rp->site.bp->blockno, exprno );
	mask = ROW_ADDR(exprdef_mask, exprno);

	LFOR( lp, gen_list )
	{
		rp = LCAST( lp, EXPR_REF );
		rp->reftype = EXPR_AVAIL_GEN;
		rp->defno = expr_avail_defno++;
		bit_set( exprdef_gen, rp->site.bp->blockno, rp->defno);
		set_bit( mask, rp->defno );
	}
}

/*
 * Build the exprdef_kill sets for an expression E.
 * Algorithm:
 *	for each block B,
 *	    if E is killed in B,
 *		exprdef_kill[B] |= { all definitions of E };
 *	    end if;
 *	end for;
 */
LOCAL
build_exprdef_kill(killed_in, exprno)
    SET_PTR killed_in;
    int exprno;
{
    int blockw, blockbit, blockno;
    int killed_in_setsize;
    SET exprdef_kill_bits;
    SET exprdef_mask_bits;
    int i, exprdef_setsize;

    /*
     * for each block B,
     */
    killed_in_setsize = SET_SIZE(killed_in);
    for (blockw = 0; blockw < killed_in_setsize; blockw++) {
	/*
	 * if E is killed in B,
	 */
	if (killed_in->bits[blockw] != 0) {
	    for (blockbit = 0; blockbit < BPW; blockbit++) {
		blockno = blockw*BPW + blockbit;
		if (test_bit(killed_in->bits, blockno)) {
		    /*
		     * exprdef_kill[B] |= { all definitions of E };
		     */
		    exprdef_kill_bits = ROW_ADDR(exprdef_kill, blockno);
		    exprdef_mask_bits = ROW_ADDR(exprdef_mask, exprno);
		    exprdef_setsize = SET_SIZE(exprdef_kill);
		    for (i = 0; i < exprdef_setsize; i++) {
			exprdef_kill_bits[i] |= exprdef_mask_bits[i];
		    }
		}
	    }
	}
    }
}

/*
 * compute IN and OUT for available expressions problem;
 * use the algorithm:
 *	do 
 *	    change = FALSE;
 *	    for each block B do
 *		newin = ~{};
 *		for each predecessor P of B do
 *		    newin *= (in[P] - kill[P]) + gen[P];
 *		end;
 *		if (in[B] != newin) then
 *		    change = TRUE;
 *		    in[B] = newin;
 *		end if;
 *	    end for;
 *	while (change == TRUE);
 */
void
compute_avail_expr()
{
	SET gen_bits, kill_bits, in_bits;
	SET gen, kill, in, newin;
	register LIST *lp;
	register BLOCK *bp;	/* steps through blocks */
	BOOLEAN	 change;	/* set if any changes propagated */
	register int i;		/* loop control */
	register int setsize;	/* loop control */
	SET_PTR expr_newin;	/* temp sets for detecting changes */
	int b;			/* bit array index for block B */
	int p;			/* bit array index for predecessors of B */
	int passno;		/* pass counter */

	expr_newin = new_heap_set(1, nexprs);
	in_bits    = expr_in->bits;
	gen_bits   = expr_gen->bits;
	kill_bits  = expr_kill->bits;
	newin      = expr_newin->bits;
	setsize    = expr_set_wordsize;
	do {
		change = IR_FALSE;
		passno = 0;
		/*
		 * for each block B do
		 */
		for (bp=entry_block->next;bp;bp=bp->next) {
			/*
			 * newin = ~{};
			 */
			for (i=0; i<setsize; i++) {
				newin[i] = ~0;
			}
			/*
			 * for each predecessor P of B do
			 */
			LFOR(lp,bp->pred) {
				p = LCAST(lp,BLOCK)->blockno*expr_set_wordsize;
				gen  = &gen_bits[p];
				kill = &kill_bits[p];
				in   = &in_bits[p];
				for (i=0; i < setsize; i++) {
					newin[i] &= (in[i] &~ kill[i]) | gen[i];
				}
			}
			/*
			 * if (in[B] != newin) then
			 */
			b = bp->blockno * expr_set_wordsize;
			in = &in_bits[b];
			for (i=0; i < setsize; i++) {
				if(in[i] != newin[i]) {
					break;
				}
			}
			if (i < setsize) {
				for (; i < setsize; i++) {
					in[i] = newin[i];
				}
				change = IR_TRUE;
			}
		} /* for */
		passno++;
		if(SHOWDF) {
			printf("====> avail_exprs() PASS %d\n", passno);
			dump_set(expr_in);
		}
	} while (change == IR_TRUE);
	free_heap_set(expr_newin);
}

/*
 * compute IN and OUT for reaching definitions on expressions;
 * use the algorithm:
 *	do 
 *	    change = FALSE;
 *	    for each block B do
 *		newin = {};
 *		for each predecessor P of B do
 *		    newin += (in[P] - kill[P]) + gen[P];
 *		end;
 *		if (in[B] != newin*all_exprdef_mask[B]) then
 *		    change = TRUE;
 *		    in[B] = newin*all_exprdef_mask[B];
 *		end if;
 *	    end for;
 *	while (change == TRUE);
 */
void
compute_exprdef_in()
{
	SET_PTR  exprdef_newin;	/* temp sets for detecting changes */
	SET gen_bits, kill_bits, in_bits, mask_bits;
	SET gen, kill, in, mask, newin;
	register LIST *lp;
	register BLOCK *bp;	/* steps through blocks */
	BOOLEAN	 change;	/* set if any changes propagated */
	register int i;		/* loop control */
	register int setsize;	/* loop control */
	int passno;		/* pass counter */
	int b;			/* bit array index for block B */
	int p;			/* bit array index for predecessors of B */

	exprdef_newin = new_heap_set(1, navailexprdefs);
	in_bits  =  exprdef_in->bits;
	gen_bits =  exprdef_gen->bits;
	kill_bits = exprdef_kill->bits;
	mask_bits = all_exprdef_mask->bits;
	newin    =  exprdef_newin->bits;
	setsize  =  exprdef_set_wordsize;
	passno = 0;
	do {
		change = IR_FALSE;
		/*
		 * for each block B do
		 */
		for (bp=entry_block->next;bp;bp=bp->next) {
			/*
			 * newin = {};
			 */
			for (i=0; i<setsize; i++) {
				newin[i] = 0;
			}
			/*
			 * for each predecessor P of B do
			 *     newin += (in[P] - kill[P]) + gen[P];
			 * end for;
			 */
			LFOR(lp,bp->pred) {
				p = LCAST(lp,BLOCK)->blockno*setsize;
				in   = &in_bits[p];
				kill = &kill_bits[p];
				gen  = &gen_bits[p];
				for (i=0; i<setsize; i++) {
					newin[i] |= (in[i] &~ kill[i]) | gen[i];
				}
			}
			/*
			 * if in[B] != (newin & all_exprdef_mask[B]) then
			 *     change = TRUE;
			 *     in[B] = newin & all_exprdef_mask[B];
			 * end if;
			 */
			b = bp->blockno * setsize;
			in   = &in_bits[b];
			mask = &mask_bits[b];
			for ( i = 0; i < setsize; ++i ) {
				if (in[i] != (newin[i]&mask[i])) {
					break;
				}
			}
			if ( i < setsize ) {
				change = IR_TRUE;
				for ( ; i < setsize; i++) {
					in[i] = newin[i] & mask[i];
				}
			}
		} /* for each B */
		passno++;
		if(SHOWDF) {
			printf("====> exprdefs PASS %d\n", passno);
			dump_set(exprdef_in);
		}
	} while (change == IR_TRUE);
	free_heap_set(exprdef_newin);
}

/*
**	go through all exprs
**	and for each examine all references block, by block
**	set exposed uses to exprdef_forward_in[b] for that block
*/
void
expr_reachdefs()
{
	register EXPR *ep, **epp;
	register EXPR_REF *rp, *first_rp;
	LIST *in_list;
	BLOCK *this_block;

	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		for( ep = *epp; ep; ep = ep->next_expr )
		{
			if(ep->op == IR_ERR || ep->references == (EXPR_REF*) NULL) {
				continue;
			}
			first_rp = ep->references;
			while(first_rp) {
				this_block = first_rp->site.bp;
				rp = first_rp;
				in_list = blowup_exprdef_in_bits(ep,this_block);
				do {
					if(in_list) {
						if(rp->reftype == EXPR_EXP_USE1) {
							if(rp->site.tp->reachdef1) {
								rp->site.tp->reachdef1 = merge_lists(rp->site.tp->reachdef1,copy_list(in_list,df_lq));
							} else {
								rp->site.tp->reachdef1 = copy_list(in_list,
																	df_lq);
							}
						} else if(rp->reftype == EXPR_EXP_USE2){
							if(rp->site.tp->reachdef2) {
								rp->site.tp->reachdef2 = merge_lists(rp->site.tp->reachdef2,copy_list(in_list,df_lq));
							} else {
								rp->site.tp->reachdef2 = copy_list(in_list,
																df_lq);
							}
						}
					}
					rp = rp->next_eref;
				} while(rp && rp->site.bp == this_block);
				first_rp = rp;
			}
			empty_listq(tmp_lq);
		}
	}
}

static LIST *
blowup_exprdef_in_bits(ep,bp)  
	EXPR *ep;
	BLOCK *bp;
{
	LIST *list;
	register EXPR_REF *rp;
	register SET in;

	list = LNULL;
	in = ROW_ADDR(exprdef_in, bp->blockno);
	for(rp = ep->references; rp ; rp = rp->next_eref) {
		if(rp->reftype == EXPR_AVAIL_GEN
		    && test_bit(in, rp->defno)==IR_TRUE){
			(void)insert_list(&list,  (LDATA*)rp, tmp_lq);
		}
	}
	return(list);
}

/*
 * An exprdef D(E) is available on entry to a basic block B,
 * if the corresponding expression is available on entry to B.
 * The set all_exprdef_mask[B] is the set of all expression defs
 * available on entry to B.
 *
 * Algorithm:
 *	for each block B,
 *	    for each expression E in avail[B],
 *	        all_exprdef_mask[B] |= exprdef_mask[E];
 *	    end for;
 *	end for;
 */

void
build_exprdef_mask()
{
    BLOCK *bp;			/* for iterating through blocks */
    int exprw, exprbit, j;	/* for bit-scanning avail[B] */
    SET all_exprdef_bits;	/* defs of all exprs in avail[B] */
    SET exprdef_bits;	 /* defs of an expression in avail[B] */
    SET expr_in_bits;	 /* {expressions available on entry to B} */
    int exprdef_setsize; /* size of exprdef_mask[B], in words */
    int expr_in_size;	 /* size of expr_in[B] in words */
    int exprno;		 /* expression #, for indexing expr_in[] */

    /*
     * for each block B,
     */
    for (bp = entry_block->next; bp; bp = bp->next) {
	/*
	 * for each expression E in avail[B],
	 */
	int blockno = bp->blockno;
	expr_in_bits = ROW_ADDR(expr_in, blockno);
	expr_in_size = SET_SIZE(expr_in);
	for (exprw = 0; exprw < expr_in_size; exprw++) {
	    /*
	     * the set of expressions available on entry to B is
	     * typically sparse, so we can iterate more quickly
	     * through the set by skipping zeros a word at a time.
	     */
	    if (expr_in_bits[exprw] != 0) {
		for (exprbit = 0; exprbit < BPW; exprbit++) {
		    exprno = exprw*BPW + exprbit;
		    if (test_bit(expr_in_bits, exprno)) {
			/*
			 * all_exprdef_mask[B] |= exprdef_mask[E];
			 */
			all_exprdef_bits = ROW_ADDR(all_exprdef_mask, blockno);
			exprdef_bits = ROW_ADDR(exprdef_mask, exprno);
			exprdef_setsize = SET_SIZE(exprdef_mask);
			for (j = 0; j < exprdef_setsize; ++j) {
			    all_exprdef_bits[j] |= exprdef_bits[j];
			}
		    }
		}
	    }
	}
    }
}

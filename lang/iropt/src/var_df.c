#ifndef lint
static	char sccsid[] = "@(#)var_df.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include "recordrefs.h"
#include "page.h"
#include "misc.h"
#include "bit_util.h"
#include "var_df.h"
#include "make_expr.h"
#include "expr_df.h"
#include "live.h"
#include "debug.h"
#include <stdio.h>

/* export */
SET_PTR save_reachdefs;	 /* out[] sets from reaching defs */
int var_avail_defno;	 /* counter for available var definitions */

/* locals */
static int vardef_set_wordsize;	
static SET_PTR var_gen, var_kill, var_in, var_out;
static void compute_var_df(/*do_global,do_livedefs*/);
static void compute_expr_df(/*do_global*/);
void dump_cooked(/*point*/);
static void build_kill(/*alldef_list*/);
static void compute_var_in_out();
static void compute_var_in();
static LIST *avail_def_list(/*leafp*/);
static LIST *blowup_in_bits(/*def_list,bp*/);  
static void var_reachdefs();
static void do_canreach();

/*
	obtain data flow information for the computation graph:  first
	build a table of variable references; then go through triples
	list building the gen, kill, def and use sets; lastly solve in
	and out for both the forward and backward problems
*/

void
compute_df(do_vars,do_exprs,do_global,do_livedefs) 
	BOOLEAN do_vars,do_exprs,do_global,do_livedefs;
{
	/* build the expr symbol table then
	** build the var and expr ref tables and 
	** use them to form the references lists for exprs 
	** and leaves 
	*/
	if(do_exprs == IR_TRUE) {
		entable_exprs();
		record_refs(IR_TRUE);
		if(SHOWCOOKED == IR_TRUE) {
			dump_exprs();
			dump_expr_refs();
			dump_var_refs();
		}
		free_depexpr_lists();
	} else {
		record_refs(IR_FALSE);
		if(SHOWCOOKED == IR_TRUE) {
			dump_var_refs();
		}
	}
   /*      the set wordsizes depend on the number of avail defs
   */
	vardef_set_wordsize =  ( roundup((unsigned)navailvardefs,BPW) ) / BPW;
	expr_set_wordsize = ( roundup((unsigned)nexprs,BPW) ) / BPW;
	exprdef_set_wordsize = ( roundup((unsigned)navailexprdefs,BPW) ) / BPW;

	if(do_exprs == IR_TRUE && nexprdefs > 0) {
		compute_expr_df(do_global);
	}
	
	if(do_vars == IR_TRUE && vardef_set_wordsize > 0) {
		compute_var_df(do_global,do_livedefs);
	}
	
	if(vardef_set_wordsize > 0 || exprdef_set_wordsize > 0) {
		do_canreach();
	}
}

static void
compute_var_df(do_global,do_livedefs)
	BOOLEAN do_global;
	BOOLEAN do_livedefs;
{

	if(do_global == IR_TRUE) {
		if (do_livedefs) {
			/* need out sets from reachdefs later */
			var_out = new_heap_set(nblocks,navailvardefs);
			var_in = new_heap_set(nblocks,navailvardefs);
			var_gen = new_heap_set(nblocks,navailvardefs);
			var_kill = new_heap_set(nblocks,navailvardefs);
			build_var_sets(IR_TRUE); 
			compute_var_in_out();
			save_reachdefs = var_out;
		} else {
			var_in = new_heap_set(nblocks,navailvardefs);
			var_gen = new_heap_set(nblocks,navailvardefs);
			var_kill = new_heap_set(nblocks,navailvardefs);
			build_var_sets(IR_TRUE); 
			compute_var_in();
		}
		var_out = NULL;
		free_heap_set(var_gen); var_gen = NULL;
		free_heap_set(var_kill); var_kill = NULL;
		var_reachdefs();
		free_heap_set(var_in); var_in = NULL;
	} else {
		build_var_sets(IR_FALSE); 
	}
}

static void
compute_expr_df(do_global)
	BOOLEAN do_global;
{
	if(do_global == IR_TRUE && navailexprdefs > 0 ) {
		/*
		 * for a given expr, exprdef_mask keeps track of all
		 * expr def bits that define it
		 */
		exprdef_gen = new_heap_set(nblocks,navailexprdefs);
		exprdef_kill = new_heap_set(nblocks,navailexprdefs);
		exprdef_mask = new_heap_set(nexprs,navailexprdefs);
		expr_in = new_heap_set(nblocks,nexprs);
		expr_gen = new_heap_set(nblocks,nexprs);
		expr_kill = new_heap_set(nblocks,nexprs);
		build_expr_sets(IR_TRUE);
		compute_avail_expr();
		free_heap_set(expr_gen); expr_gen = NULL;
		free_heap_set(expr_kill); expr_kill = NULL;
		all_exprdef_mask = new_heap_set(nblocks,navailexprdefs);
		build_exprdef_mask();
		free_heap_set(expr_in); expr_in = NULL;
		free_heap_set(exprdef_mask); exprdef_mask = NULL;
		exprdef_in = new_heap_set(nblocks,navailexprdefs);
		compute_exprdef_in();
		free_heap_set(exprdef_gen); exprdef_gen = NULL;
		free_heap_set(exprdef_kill); exprdef_kill = NULL;
		free_heap_set(all_exprdef_mask); all_exprdef_mask = NULL;
		expr_reachdefs();
		free_heap_set(exprdef_in); exprdef_in = NULL;
	} else {
		build_expr_sets(IR_FALSE);
	}
}

void
dump_cooked(point)
	int point;
{
	register BLOCK *p;
	register LIST *lp;
	register TRIPLE *tlp;

	dump_segments();
	dump_leaves();
	printf("\n\tPOINT %s\n", debug_nametab[point]);
	for(p=entry_block;p;p=p->next) {

			printf("\nBLOCK [%d] %s label %d next %d loop_weight %d",p->blockno,
				(p->entryname ? p->entryname : ""), p->labelno,
				(p->next ? p->next->blockno : -1 ), p->loop_weight);
			printf(" pred: ");
			if(p->pred) LFOR(lp,p->pred->next) {
				printf("%d ",  ((BLOCK *)lp->datap)->blockno );
			}
			printf("succ: ");
			if(p->succ) LFOR(lp,p->succ->next) {
				printf("%d ",  ((BLOCK *)lp->datap)->blockno );
			}
			if(p->dfonext) {
				printf("dfonext %d ",  p->dfonext->blockno);
			}
			if(p->dfoprev) {
				printf("dfoprev %d ",  p->dfoprev->blockno);
			}
			printf("\n");
			if(p->last_triple) TFOR(tlp,p->last_triple->tnext) {
				print_triple(tlp,1);
			}
			show_live_after_block(p);
			printf("\n");
	}
	(void)fflush(stdout);
}

/*
**	go through the references list of each leaf; for each basic
**	block in the list find the locally available definition (if
**	any) by scanning backwards, and the locally exposed uses by scanning
**	forwards. If a locally available definition is found for basic block b set
**	the corresponding bit in GEN(b) and  set KILL(c) for all bb c != b
*/
void
build_var_sets(setbit_flag)
	BOOLEAN setbit_flag;
{
	LIST *all_avail_defs;	/* all available definitions of a variable*/
	LEAF *leafp;		/* steps through the leaf table */
	VAR_REF *rp;		/* steps through the references list */
	VAR_REF *def_rp;
	BLOCK *this_block;
	TRIPLE *tp;
	LIST **lpp, *hash_link, *def_list;

	var_avail_defno=0;
	for(lpp = leaf_hash_tab; lpp < &leaf_hash_tab[LEAF_HASH_SIZE]; lpp++) {
		LFOR(hash_link, *lpp) {
			leafp = (LEAF*) hash_link->datap;
			rp = leafp->references;
			all_avail_defs = LNULL;
			empty_listq(tmp_lq);
			while( rp )
			{ /* for this leaf */
				this_block = rp->site.bp;
				def_list = LNULL;
				do 
				{ /* for this block */
					tp = rp->site.tp;
					if(rp->reftype == VAR_USE1) 
					{
						if(def_list == LNULL) 
						{	
							rp->reftype = VAR_EXP_USE1;
						} 
						else 
						{
							tp->reachdef1 = copy_list(def_list,df_lq);
						}
					} 
					else
						if(rp->reftype == VAR_USE2) 
						{
							if(def_list == LNULL) 
							{	
								rp->reftype = VAR_EXP_USE2;
							}
							else 
							{
								tp->reachdef2 = copy_list(def_list,df_lq);
							}
						}
						else 
						{
					/* 
					 * after a definition is encountered any remaining uses are not
					 * exposed
					 */
							if(def_list == LNULL) 
							{
								def_list = NEWLIST(tmp_lq);
							}
							(VAR_REF*) def_list->datap = rp;
						}
				rp = rp->next_vref;
				} while( rp && rp->site.bp == this_block );
				if( def_list )
				{ /* there is a defination in this block */
					def_rp = (VAR_REF *)def_list->datap;
					def_rp->reftype = VAR_AVAIL_DEF;
					def_rp->defno = var_avail_defno++;
					if( setbit_flag == IR_TRUE )
					{
						bit_set(var_gen, this_block->blockno, def_rp->defno);
						LAPPEND(all_avail_defs, def_list);
					}
				}
			}

			if( setbit_flag == IR_TRUE && all_avail_defs != LNULL )
			{
				build_kill(all_avail_defs);
			}
		}
	}
	empty_listq(tmp_lq);
	if(var_avail_defno != navailvardefs) {
		quita("build_var_sets: error: var_avail_defno=%d navailvardefs=%d",
			var_avail_defno,navailvardefs);
	}
}

static void
build_kill(alldef_list)
	register LIST *alldef_list;
{
	register LIST *lp, *lp2;
	register BLOCK *this_block;
	register SET kill;

	LFOR(lp,alldef_list) {
		this_block = LCAST(lp,VAR_REF)->site.bp;
		kill = ROW_ADDR(var_kill, this_block->blockno);
		for(lp2 = lp->next; lp2 != lp; lp2 = lp2->next) {
			set_bit(kill, LCAST(lp2,VAR_REF)->defno);
		}
	}
}

/* compute IN and OUT for forward/union flow problem */

static void
compute_var_in_out()
{
	BIT_INDEX in,out,gen,kill,newin;
	/* point to sets for a specific block*/
	BIT_INDEX in_index,out_index,kill_index,gen_index;	
	BLOCK *bp;
	BOOLEAN	change, block_change;	/* note if any changes propagated */
	SET_PTR var_newin;		/* temp for detecting changes */
	int passn = 0;
	register int b;		/* start of a block's bits within a set */
	register int temp;
	register int i;		/* loop control */
	register int setsize;

	var_newin = new_heap_set(1, navailvardefs);
	gen = var_gen->bits;
	kill = var_kill->bits;
	in = var_in->bits;
	out = var_out->bits;
	newin = var_newin->bits;

	/* initialize IN[b] = 0 and OUT[b] = GEN[b] */
	setsize = vardef_set_wordsize;
	temp = nblocks * setsize;
	{	register BIT_INDEX in_ptr = in, out_ptr=out, gen_ptr=gen;
		for (i=0; i<temp; i++) {
			*in_ptr++ = 0;
			*out_ptr++ = *gen_ptr++;
		}
	}

	change = IR_TRUE;
	while (change == IR_TRUE) {
		passn++;
		if(SHOWDF== IR_TRUE) {
			printf("compute_var_in_out PASS %d\ni in[i],out[i]\n",
				passn);
		}
		change = IR_FALSE;
		for (bp=entry_block;bp;bp=bp->dfonext) {
			b=(int)bp->blockno*(short)setsize;
			in_index = &in[b];
			kill_index = &kill[b];
			gen_index = &gen[b];
			/*
			 * compute NEWIN: for each of block b's predecessors 
			 * newin is set to the or of the predecessors' OUTs
			 */
			{
				register BIT_INDEX newin_ptr = newin;
				for (i=0; i<setsize; i++) {
					*newin_ptr++ = 0;
				}
			}
			if(bp->pred) {
				register LIST *lp, *pred;
				pred = bp->pred;
				LFOR(lp,pred) {
					out_index = &out[(int)LCAST(lp,BLOCK)->blockno*(short)setsize];
					{
						register BIT_INDEX newin_ptr;
						register BIT_INDEX out_ptr;
						newin_ptr = newin;
						out_ptr = out_index;
						for (i=0; i<setsize; i++) {
							*newin_ptr++ |= *out_ptr++;
						}
					}
				}
			}
			/* test if IN eq NEWIN */
			block_change = IR_FALSE;
			{ register BIT_INDEX in_ptr = in_index, newin_ptr = newin;
				for (i=0; i<setsize; i++) {
					if(*in_ptr++ != *newin_ptr++) {
						block_change = IR_TRUE;
						break;
					}
				}
			}
			out_index = &out[b];
			if(block_change == IR_TRUE) {
				/* set IN[b] to NEWIN[b] */
				/* and OUT[b]=IN[b] - KILL[b] U GEN[b] */
				{ register BIT_INDEX in_ptr = in_index, newin_ptr = newin,
										out_ptr = out_index, kill_ptr = kill_index,
										gen_ptr = gen_index;
					for (i=0; i<setsize; i++) {
						*in_ptr = *newin_ptr++;
						*out_ptr++ = (*in_ptr++ & ~*kill_ptr++) | *gen_ptr++;
					}
				}
				change = IR_TRUE;
			}
			if(SHOWDF==IR_TRUE) {
				for (i=0; i<setsize; i++) {
					printf("%d\t%X\t%X\n",bp->blockno,in_index[i],out_index[i]);
				}
			}
		}
	}
	if(SHOWDF==IR_TRUE) {
		dump_set(var_in);
		printf("compute_var_in_out: %d passes to converge\n",passn);
	}
	free_heap_set(var_newin);
}

/*
 * compute IN only for the reaching definitions problem,
 * using the following:
 *	repeat
 *	    changed = FALSE;
 *	    for each block B do
 *		newin = {};
 *		foreach predecessor P of B do
 *		    newin += (in[P] - kill[P]) + gen[P];
 *		end for;
 *		if (in[B] != newin) then
 *		    changed = TRUE;
 *		    in[B] = newin;
 *		end if;
 *	    end for;
 *	until !changed;
 */
static void
compute_var_in()
{
	SET  in_bits, gen_bits, kill_bits; /* pointers to all bits of sets */
	SET  in, gen, kill, newin;   /* pointers to individual rows of sets */
	BLOCK *bp;		/* for iterating through basic blocks */
	BOOLEAN	change;		/* note if any changes propagated */
	SET_PTR var_newin;	/* temp for detecting changes */
	register int b;		/* start of a block's bits within a set */
	register int i;		/* loop control */
	register int setsize;	/* size of set[B], in words */
	register LIST *lp, *pred;  /* for iterating through predecessors */

	var_newin = new_heap_set(1, navailvardefs);
	gen_bits = var_gen->bits;
	kill_bits = var_kill->bits;
	in_bits = var_in->bits;
	newin = var_newin->bits;
	setsize = vardef_set_wordsize;
	do {
		change = IR_FALSE;
		/*
		 * for each block B do
		 */
		for (bp=entry_block;bp;bp=bp->dfonext) {
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
			pred = bp->pred;
			LFOR(lp, pred) {
				int p = LCAST(lp,BLOCK)->blockno*setsize;
				gen  = &gen_bits[p];
				kill = &kill_bits[p];
				in   = &in_bits[p];
				for (i=0; i<setsize; i++) {
					newin[i] |= (in[i] &~ kill[i]) | gen[i];
				}
			}
			/*
			 * if (in[B] != newin) then
			 *     change = TRUE;
			 * end if;
			 */
			b = bp->blockno * setsize;
			in = &in_bits[b];
			for (i=0; i<setsize; i++) {
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
	} while (change);
	free_heap_set(var_newin);
}

static LIST *
avail_def_list(leafp)
	LEAF *leafp;
{
	LIST *list, *new_l;
	register VAR_REF *rp;

	list = LNULL;
	for(rp = leafp->references; rp ; rp = rp->next_vref ) {
		if(rp->reftype == VAR_AVAIL_DEF) {
			new_l = NEWLIST(tmp_lq);
			(VAR_REF *)new_l->datap = rp;
			LAPPEND(list, new_l);
		}
	}
	return list;
}

static LIST *
blowup_in_bits(def_list,bp)  
	LIST *def_list;
	BLOCK *bp;
{
	LIST *lp, *list, *new_l;
	register VAR_REF *rp;
	register SET in;

	list = LNULL;
	in = ROW_ADDR(var_in, bp->blockno);
	LFOR(lp, def_list) {
		rp = LCAST(lp, VAR_REF);
		if ( test_bit(in, rp->defno) == IR_TRUE ) {
			new_l = NEWLIST(tmp_lq);
			(VAR_REF *)new_l->datap = rp;
			LAPPEND(list, new_l);
		}
	}
	return(list);
}

static void
var_reachdefs()
{
	LEAF *leafp;
	BLOCK *this_block;
	LIST *in_list;
	LIST *def_list;
	VAR_REF *rp;
	LIST **lpp, *hash_link;

/*
**	go through all leaves
**	and for each list examine all references block, by block
**	set exposed uses to var_in[b] for that block
*/

	for(lpp = leaf_hash_tab; lpp < &leaf_hash_tab[LEAF_HASH_SIZE]; lpp++) {
		LFOR(hash_link, *lpp) {
			leafp = (LEAF*) hash_link->datap;
			rp = leafp->references;
			in_list = LNULL;
			empty_listq(tmp_lq);
			def_list = avail_def_list(leafp);
			while(rp) {
				this_block = rp->site.bp;
				in_list = blowup_in_bits(def_list,this_block);
				do {
					if(rp->reftype == VAR_EXP_USE1) {
						rp->site.tp->reachdef1 = copy_list(in_list,df_lq);
					} else if(rp->reftype == VAR_EXP_USE2){
						rp->site.tp->reachdef2 = copy_list(in_list,df_lq);
					}
					rp = rp->next_vref;
				} while(rp && rp->site.bp == this_block);
			}
		}
	}
	empty_listq(tmp_lq);
}

static void
do_canreach()
{
	EXPR *ep, **epp;
	VAR_REF *vsink_rp;
	EXPR_REF *esink_rp;
	TRIPLE *sink_tp;
	register BOOLEAN istriple; 
	register LIST *new_l, *sink_lp;
	register TRIPLE *source_tp;

	for( vsink_rp = var_ref_head; vsink_rp; vsink_rp = vsink_rp->next ) {
		if(ISUSE(vsink_rp->reftype)) { 
			sink_tp = (TRIPLE*) vsink_rp->site.tp;
			if(vsink_rp->reftype==VAR_USE1 || 
						vsink_rp->reftype==VAR_EXP_USE1)
			{ 	register LIST *reachdef1 = sink_tp->reachdef1;

				istriple = 
					(sink_tp->left->operand.tag == ISTRIPLE ? IR_TRUE : IR_FALSE );
				LFOR(sink_lp,reachdef1) {
					if(istriple) {
						source_tp = LCAST(sink_lp,EXPR_REF)->site.tp;
					} else { 	/*sink_tp->left->operand.tag == ISLEAF*/
						source_tp = LCAST(sink_lp,VAR_REF)->site.tp;
					}
					new_l = NEWLIST(df_lq);
					(VAR_REF*) new_l->datap = vsink_rp;
					LAPPEND(source_tp->canreach,new_l);
				}
			} 
			else
			{ 	register LIST *reachdef2 = sink_tp->reachdef2;

				istriple = 
					(sink_tp->right->operand.tag == ISTRIPLE ? IR_TRUE : IR_FALSE );
				LFOR(sink_lp,reachdef2) {
					if(istriple) {
						source_tp = LCAST(sink_lp,EXPR_REF)->site.tp;
					} else {
						source_tp = LCAST(sink_lp,VAR_REF)->site.tp;
					}
					new_l = NEWLIST(df_lq);
					(VAR_REF*) new_l->datap = vsink_rp;
					LAPPEND(source_tp->canreach,new_l);
				}
			}
		}
	}

	if(nexprs)
		for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		for( ep = *epp; ep; ep = ep->next_expr ) {
			for(esink_rp = ep->references; 
								esink_rp ; esink_rp = esink_rp->next_eref) {
				if( ISEUSE(esink_rp->reftype) ) {
					sink_tp = (TRIPLE*) esink_rp->site.tp;
					if(esink_rp->reftype==EXPR_USE1 || 
							esink_rp->reftype==EXPR_EXP_USE1)
					{ 	register LIST *reachdef1 = sink_tp->reachdef1;

						istriple = 
							(sink_tp->left->operand.tag == ISTRIPLE ? IR_TRUE : IR_FALSE );
						LFOR(sink_lp,reachdef1) {
							if(istriple) {
								source_tp = LCAST(sink_lp,EXPR_REF)->site.tp;
							} else {
								source_tp = LCAST(sink_lp,VAR_REF)->site.tp;
							}
							new_l = NEWLIST(df_lq);
							(EXPR_REF*) new_l->datap = esink_rp;
							LAPPEND(source_tp->canreach,new_l);
						}
					} 
					else 
					{ 	register LIST *reachdef2 = sink_tp->reachdef2;

						istriple = 
							(sink_tp->right->operand.tag == ISTRIPLE ? IR_TRUE : IR_FALSE );
						LFOR(sink_lp,reachdef2) {
							if(istriple) {
								source_tp = LCAST(sink_lp,EXPR_REF)->site.tp;
							} else {
								source_tp = LCAST(sink_lp,VAR_REF)->site.tp;
							}
							new_l = NEWLIST(df_lq);
							(EXPR_REF*) new_l->datap = esink_rp;
							LAPPEND(source_tp->canreach,new_l);
						}
					}
				}
			}
		}
	}
}


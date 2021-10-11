#ifndef lint
static	char sccsid[] = "@(#)loop.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
#include "iropt.h"
#include "page.h"
#include "bit_util.h"
#include "misc.h"
#include "setup.h"
#include "loop.h"
#include "debug.h"
#include <stdio.h>

/* global */
LIST	*loops;
int	nloops;

/* local */

static int	block_set_wordsize;
static SET_PTR	dominate_set;
static SET_PTR	loop_in_blocks;
static SET_PTR	loop_exit_dominators;
static void	new_header(/*edgep,blocks*/);
static void	make_loop(/*back_edge*/);
static void	make_loop_tab();
static void	connect_preheaders();
static void	redirect_flow(/*from_bp,old_to_bp,new_to_bp*/);

/* we build one loop for each header regardless of how many back edges come
** into it. the loop build goes as follows : recognize a back edge, build
** its natural loop, add this to other loops that share the same header
** (struct loop_list) then build a loop struct for export to other routines
*/
static struct loop_list {
	BLOCK *header, *preheader;
	LIST *blocks, *edges;
	LOOP *loop;
	struct loop_list *next;
} *headers;


/*
** dragon book algorithm 13.2
*/
void
find_dominators()
{
	register BIT_INDEX dominates_index, mask;
	SET_PTR mask_ptr;
	register BLOCK *pred_bp, *bp;
	register LIST *pred;
	register int i;
	register int setsize;
	BLOCK *after_entry;
	BOOLEAN change;

	block_set_wordsize = ( roundup((unsigned)nblocks, BPW) ) / BPW;
	setsize = block_set_wordsize;
	if (dominate_set == NULL) {
		dominate_set = new_heap_set(nblocks,nblocks);
	} else if (dominate_set->nrows != nblocks) {
		free_heap_set(dominate_set);
		dominate_set = new_heap_set(nblocks,nblocks);
	}
	mask_ptr = new_heap_set(1,nblocks);
	mask = mask_ptr->bits;

	/*
	**	D(n0) = n0
	*/
	set_bit(mask,entry_block->blockno);
	dominates_index = dominate_set->bits + 
				(entry_block->blockno*setsize);
	for(i=0;i<setsize;i++) {
		dominates_index[i] = mask[i];
	}

	/*
	**	for n - in N-n0 do D(n) = N
	*/
	after_entry = entry_block->dfonext;
	for(bp=after_entry;bp;bp=bp->next) {
		dominates_index = dominate_set->bits + (bp->blockno*setsize);
		for(i=0;i<setsize;i++) {
			dominates_index[i] = ~0;
		}
	}
	change = IR_TRUE;

	while (change == IR_TRUE) {
		change = IR_FALSE;
		for(bp=after_entry;bp;bp=bp->dfonext) {
			/*
			**	mask = {n} union (intersection D(p) for all p pred of bp)
			*/
			for(i=0;i<setsize;i++) {
				mask[i] = ~0;
			}
			LFOR(pred,bp->pred) {
				pred_bp = (BLOCK*) pred->datap;
				dominates_index = dominate_set->bits + 
						(pred_bp->blockno*setsize);
				for(i=0;i<setsize;i++) {
					mask[i] &= dominates_index[i];
				}
			}
			set_bit(mask,bp->blockno);

			dominates_index = dominate_set->bits + 
							(bp->blockno*setsize);
			for(i=0;i<setsize;i++) {
				if(mask[i] != dominates_index[i]) {
					change = IR_TRUE;
					break;
				}
			}
			if (change) {
				for(; i<setsize; i++) {
					dominates_index[i] = mask[i];
				}
			}
		}
	}
	free_heap_set(mask_ptr);
}

void
find_loops()
{
	BLOCK *bp;
	int i;
	BIT_INDEX dominates_index;
	register LIST *lp;
	EDGE *ep;

	loops = LNULL;
	nloops = 0;
	headers = (struct loop_list *) NULL;
	if(loop_lq == NULL) loop_lq = new_listq();
	find_dominators();
	if(SHOWDF == IR_TRUE) {
		printf(" DOMINATOR df bits\n");
		for(bp=entry_block;bp;bp=bp->next) {
			printf(" %d\n",bp->blockno);
			dominates_index = dominate_set->bits + bp->blockno;
			for(i=0;i<block_set_wordsize;i++) {
				printf(" %d %X\n",i,dominates_index[i]);
			}
		}
	}

	LFOR(lp,edges) {
		ep = (EDGE*) lp->datap;
		if(ep->edgetype == RETREATING) {
			if(dominates(ep->to->blockno, ep->from->blockno)){
				ep->edgetype = RETREATING_BACK;
				make_loop(ep);
			}
		}
	}
	if(SHOWDF == IR_TRUE) {
		dump_edges();
	}

	if(headers) {
		connect_preheaders();
		/*	now that we know how many new blocks we'll need ... */
		make_loop_tab();
		/*	dfo and dominator information needs to be recomputed
		**	to account for preheader blocks
		*/
		find_dfo();
		find_dominators();
	}
}

static void
new_header(edgep,blocks)
	EDGE *edgep;
	LIST *blocks;
{
	register LIST *new_edge, *lp;
	register struct loop_list *hp;
	register BLOCK *header = edgep->to;

	LFOR(lp,indirgoto_targets) {
		/* if a header is the target of an indirect goto ignore it -
		** otherwise we would have to change all the places where the headers's
		** label was assigned and this seems more trouble than its worth
		*/
		if(header == (BLOCK*) (LCAST(lp,TRIPLE)->left) ) {
			return;
		}
	}

	for(hp=headers; hp; hp=hp->next) {
		if(hp->header == header) break;
	}
	if(!hp) {
		hp  = (struct loop_list *) ckalloc(1,sizeof(struct loop_list));
		hp->header = header;
		hp->next = headers;
		headers = hp;
	}
	new_edge = NEWLIST(loop_lq);
	(EDGE*) new_edge->datap = edgep;
	LAPPEND(hp->edges,new_edge);
	LFOR(lp,blocks) {
		(void)insert_list(&(hp->blocks), (LDATA*)lp->datap, loop_lq);
	}
}

/* 
**	tests whether block number 1 dominate block number 2 
** 
*/
BOOLEAN
dominates(bn1,bn2)
	int bn1, bn2;
{
	return(bit_test(dominate_set,bn2,bn1));
}

void
print_loop(loop)
	LOOP *loop;
{
	register LIST *lp, *first;
	register TRIPLE *tp;

	printf("loop %d preheader %d ", 
		loop->loopno, loop->preheader->blockno);
	LFOR(lp,loop->back_edges) {
		printf(" %d->%d, ", 
			LCAST(lp,EDGE)->from->blockno,
			LCAST(lp,EDGE)->to->blockno);
	}
	LFOR(lp,loop->blocks) {
		printf("B[%d]  ", LCAST(lp,BLOCK)->blockno);
	}
	printf("\n\t");
	if(loop->invariant_triples) {
		first = loop->invariant_triples;
		LFOR(lp,first) {
			tp = (TRIPLE*) lp->datap;
			printf("T[%d]%c  ", tp->tripleno, 
			    ((ISOP(tp->op,ROOT_OP)) ? 'r ' : ' '));
		}
	}
	printf("\n");
}

void
dump_edges()
{
	static char *edgetypes[] = {
		"RETREATING",
		"RETREATING_BACK",
		"ADVANCING",
		"ADVANCING_TREE",
		"CROSS",
		"RETREATORCROSS"
	};
	register LIST *lp;
	EDGE *ep;

	LFOR(lp,edges) {
		ep = (EDGE*) lp->datap;
		printf("%d -> %d %s\n",
		ep->from->blockno,ep->to->blockno,edgetypes[(int)ep->edgetype]);
	}
}

/* Algorithm 13.1 */
static void
make_loop(back_edge)
	EDGE *back_edge;
{
	LIST *loop_blocks = LNULL;
	register LIST *lp, *lp2;
	BLOCK *block, *m;
	LIST *stack;

	stack = LNULL;
	block = back_edge->to;
	block->loop_weight = LOOP_WEIGHT_FACTOR;/*later adjusted for nesting depth*/
	lp = NEWLIST(loop_lq);
	(BLOCK*) lp->datap = block;
	LAPPEND(loop_blocks,lp);
	
	block = back_edge->from; /* INSERT(n) */
	if( insert_list(&loop_blocks, (LDATA*)block, loop_lq) == IR_TRUE ) {
		block->loop_weight = LOOP_WEIGHT_FACTOR;
		lp = NEWLIST(tmp_lq);
		(BLOCK*) lp->datap = block;
		LAPPEND(stack,lp);
	}

	while(stack != LNULL) {
		m = (BLOCK*) stack->datap;
		delete_list(&stack,stack);
		LFOR(lp2,m->pred) { /* foreach pred p do INSERT(p) */
			block = (BLOCK*) lp2->datap;
			if(insert_list(&loop_blocks,(LDATA*)block,loop_lq) == IR_TRUE) {
				block->loop_weight = LOOP_WEIGHT_FACTOR;
				lp = NEWLIST(tmp_lq);
				(BLOCK*) lp->datap = block;
				LAPPEND(stack,lp);
			}
		}
	} 
	
	new_header(back_edge,loop_blocks);

	empty_listq(tmp_lq);
}

static void
make_loop_tab()
{
	register BLOCK *bp, *succ;
	register LIST *lp, *lp2, *lp3, *tmp_lp;
	register SET in_blocks;
	LOOP *loop, **loop_vec, *loop1, *loop2;
	BLOCK *header1, *preheader1;
	register int i;

	/*
	 * initialize the (loops * blocks) map
	 */
	loop_vec = (LOOP**)heap_tmpalloc((unsigned)sizeof(LOOP*)*nloops);
	loop_in_blocks = new_heap_set(nloops, nblocks);
	loop_exit_dominators = new_heap_set(nloops, nblocks);
	LFOR(lp,loops) {
		loop = (LOOP*) lp->datap;
		loop_vec[loop->loopno] = loop;
		in_blocks = ROW_ADDR(loop_in_blocks, loop->loopno);
		loop->in_blocks = in_blocks;
		LFOR(lp2,loop->blocks) {
			bp = (BLOCK*) lp2->datap;
			set_bit(in_blocks, bp->blockno);
		}
	}

	/*
	 * Decide which loops a preheader block belongs to.  Note: we
	 * should do this before looking for loop exit blocks (below).
	 * Otherwise, the blocks whose successors are preheaders will
	 * appear to be exit blocks.
	 */
	LFOR(lp,loops) {
		loop1 = (LOOP*) lp->datap;
		preheader1 = loop1->preheader;
		header1 = LCAST(loop1->back_edges,EDGE)->to;
		/*
		 * preheader1 belongs to all the loops header1 belongs
		 * to except loops whose header is header1
		 */
		for(i=0; i < nloops; i++) {
			loop2 = loop_vec[i];
			if(!INLOOP(header1->blockno, loop2)) {
				/* header1 not in loop2 */
				continue;
			}
			if( LCAST(loop2->back_edges,EDGE)->to == header1 ) {
				continue;
			}
			set_bit(loop2->in_blocks, preheader1->blockno);
			tmp_lp = NEWLIST(loop_lq);
			(BLOCK*) tmp_lp->datap = preheader1;
			LAPPEND(loop2->blocks,tmp_lp);
		}
	}
	/*
	 * for each loop, mark those blocks in the loop that are
	 * exit blocks, i.e., those that have a successor outside
	 * the loop
	 */
	LFOR(lp,loops) {
		loop = (LOOP*) lp->datap;
		LFOR(lp2,loop->blocks) {
			bp = (BLOCK*) lp2->datap;
			LFOR(lp3,bp->succ) {
				succ = (BLOCK*) lp3->datap;
				if( !INLOOP(succ->blockno,loop)) {
					(void)insert_list(&loop->exit_blocks,
						(LDATA*)bp, loop_lq);
					break;
				}
			}
		}
	}
	if(SHOWDF) {
		dump_set(loop_in_blocks);
	}
	heap_tmpfree((char*)loop_vec);
}

static void
connect_preheaders()
{
	TRIPLE *ref_triple;
	TYPE type;
	LIST *lp, *lp2, *lp3, *pred_list;
	LOOP *loop;
	BLOCK *bp, *header, *preheader;
	struct loop_list *hp;
	EDGE *ep;

	/* for each header allocate a loop then */
	/* connect preheaders to headers and vice versa */
	for(hp = headers; hp; hp = hp->next ) {
		hp->preheader = preheader = new_block();
		move_block(hp->header->prev, preheader); /* insert in front of the header */
		preheader->labelno = new_label();
		loop = (LOOP*)	ckalloc(1,sizeof(LOOP));
		hp->loop = loop;
		loop->back_edges = hp->edges;
		loop->blocks = hp->blocks;
		loop->loopno = nloops++;
		loop->preheader = preheader;
		lp = NEWLIST(loop_lq);
		(LOOP*) lp->datap = loop;
		LAPPEND(loops,lp);

		header = hp->header;
		lp2 = NEWLIST(proc_lq);
		(BLOCK*) lp2->datap = header;
		LAPPEND(preheader->succ,lp2);

		lp2 = NEWLIST(proc_lq);
		(BLOCK*) lp2->datap = preheader;
		LAPPEND(header->pred,lp2);

		type.tword = UNDEF;
		type.size = 0;
		type.align = 0;
		preheader->last_triple = append_triple(preheader->last_triple,
			IR_LABELDEF, (IR_NODE*)preheader, (IR_NODE*) NULL,type);
		ref_triple = append_triple(TNULL,
			IR_LABELREF, (IR_NODE*)header, (IR_NODE*)ileaf(0),type);
		preheader->last_triple = append_triple(preheader->last_triple,
			IR_GOTO, (IR_NODE*)NULL, (IR_NODE*)ref_triple, type);
	}

	/*
	**	then look at all predecessors of the header and make those not in
	**	the loop (other than the preheader) connect to the preheader
	*/
	for(hp = headers; hp; hp = hp->next ) {
		header = hp->header;
		preheader = hp->preheader;
		pred_list = LNULL; /* build a new list of predecessors */
		LFOR(lp,header->pred) {
			bp = (BLOCK*) lp->datap;
			if(bp == preheader) { /* the preheader is always on the pred_list*/
				lp3 = NEWLIST(proc_lq);
				(BLOCK*) lp3->datap = bp;
				LAPPEND(pred_list,lp3);
				goto next_pred;	
			} else { /* test if it's a back edge */
				loop = hp->loop;
				LFOR(lp2,loop->back_edges) {
					ep = (EDGE*) lp2->datap;
					if(bp == ep->from)  {
						/* if this edge is one of the back edges coming into the
						** loop leave it alone
						*/
						lp3 = NEWLIST(proc_lq);
						(BLOCK*) lp3->datap = bp;
						LAPPEND(pred_list,lp3);
						goto next_pred;	
					}
				}
				/* it's an edge to be redirected */
				redirect_flow(bp,header,preheader);
			}
		next_pred: ;
		}
		header->pred = pred_list;
	}
}

static void
redirect_flow(from_bp,old_to_bp,new_to_bp)
	BLOCK *from_bp, *old_to_bp, *new_to_bp;
{
	LIST *lp, *lp2;
	TRIPLE *labelref, *last;
	BLOCK *bp;

	LFOR(lp,from_bp->succ) {
		bp = (BLOCK*) lp->datap;
		if(bp == old_to_bp)  {
			(BLOCK*) lp->datap = new_to_bp;
			last = from_bp->last_triple;
			if(ISOP(last->op,BRANCH_OP)) {
				switch(last->op) {
					case IR_SWITCH :
					case IR_CBRANCH :
					case IR_INDIRGOTO :
					case IR_REPEAT:
						TFOR(labelref,(TRIPLE*)last->right) {
							if( (BLOCK*)labelref->left == old_to_bp) {
								(BLOCK*)labelref->left = new_to_bp;
							}
						}
						break;
			
					case IR_GOTO :
						labelref = (TRIPLE*) last->right;
						if( (BLOCK*)labelref->left == old_to_bp) {
							(BLOCK*)labelref->left = new_to_bp;
						} else {
							quit("redirect_flow(): bad labelref triple");
						}
						break;
				}
			} else {
				quita("redirect_flow(): connected by op >>%s<<",
					op_descr[ORD(last->op)].name);
			}
			lp2 = NEWLIST(proc_lq);
			(BLOCK*) lp2->datap =  from_bp;
			LAPPEND(new_to_bp->pred,lp2);
		}
	}
}

/*
 * returns TRUE if a block B dominates all exits of a given loop.
 * 
 * Side effect: the loop_exit_dominators information for this block
 * is valid only if bp->visited is set to TRUE.  This is an ancient
 * hack to avoid recomputation of exit dominators.
 */
BOOLEAN
is_exit_dominator(bp, loop)
    BLOCK *bp;
    LOOP *loop;
{
    register LIST *lp;
    register BLOCK *exit_bp;
    register blockno;

    if (!bp->visited) {
	bp->visited = IR_TRUE;
	blockno = bp->blockno;
	LFOR(lp,loop->exit_blocks) {
	    exit_bp = LCAST(lp, BLOCK);
	    if(!dominates(blockno, exit_bp->blockno)) {
		/* this block does not dominate all exits */
		return IR_FALSE;
	    }
	}
	bit_set(loop_exit_dominators, loop->loopno, blockno);
    }
    return (BOOLEAN)bit_test(loop_exit_dominators, loop->loopno, bp->blockno);
}

/*
 * Deallocate loop sets.
 * Must be called at heap_reset time.
 */
void
loop_cleanup()
{
    if (dominate_set) {
	free_heap_set(dominate_set);
	dominate_set = NULL;
    }
    if (loop_in_blocks) {
	free_heap_set(loop_in_blocks);
	loop_in_blocks = NULL;
    }
    if (loop_exit_dominators) {
	free_heap_set(loop_exit_dominators);
	loop_exit_dominators = NULL;
    }
}


/*
 * dbx print routine
 */

/*LINTLIBRARY*/

void
dump_loops()
{
	register LIST *lp;
	register LOOP *loop;

	printf("LOOPS:\n");
	LFOR(lp,loops) {
		loop = (LOOP*) lp->datap;
		print_loop(loop);
	}
}


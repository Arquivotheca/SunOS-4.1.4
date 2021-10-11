#ifndef lint
static	char sccsid[] = "@(#)setup.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include "page.h"
#include "misc.h"
#include "loop.h"
#include "setup.h"
#include <stdio.h>

/* global */
LIST	*edges;
BLOCK	*first_block_dfo;

/* local */
static BLOCK *dfonext;
static int *dfnumber1, dfcount1;    /* 0 -> n-1 : order in which visited */
static int *dfnumber2, dfcount2;    /* n-1 -> 0  : how "far" from init node */
static void dfo_search(/*bp*/);


void
find_dfo()
{
	register BLOCK *bp, *from, *to;
	register LIST *lp;
	register EDGE *ep;
	int i;

        edges = LNULL;
        for(bp=entry_block;bp;bp=bp->next) {
                bp->visited = IR_FALSE;
        }
	dfnumber1 = (int*)heap_tmpalloc((unsigned)nblocks*sizeof(int));
	dfnumber2 = (int*)heap_tmpalloc((unsigned)nblocks*sizeof(int));
        dfcount1 = 0;
        dfcount2 = nblocks-1;
        entry_block->dfoprev = (BLOCK*) NULL;
        dfonext = (BLOCK*) NULL;
 
        dfo_search(entry_block);
        /*
        **      distinguish retreating from cross edges
        **      for retreating edges the dfs distance number of
        **      the from is >= than that of the to, ie to is an ancestor
        **      of from in the DFST. The remaining edges are cross by elimination
        */
        LFOR(lp,edges) {
                ep = (EDGE*) lp->datap;
                if(ep->edgetype == RETREATORCROSS) {
                        from = ep->from;
                        to = ep->to;
                        if(dfnumber2[from->blockno] >= dfnumber2[to->blockno]) {
                                ep->edgetype = RETREATING;
                        } else {
                                ep->edgetype = CROSS;
                        }
                }
        }
        if(SHOWDF == IR_TRUE) {
                printf(" block visited all_children_visited\n");
                for(i=0;i<nblocks;i++) {
                        printf(" %d %d %d\n",i,dfnumber1[i],dfnumber2[i]);

                }
        }
	heap_tmpfree((char*)dfnumber1);
	heap_tmpfree((char*)dfnumber2);
}

static void
dfo_search(bp)
	register BLOCK *bp;
{
	register LIST *succ, *lp;
	register BLOCK *next;
	register EDGE *ep;

        bp->visited = IR_TRUE;
        dfnumber1[bp->blockno] = dfcount1;
        dfcount1 += 1;
        LFOR(succ,bp->succ) {
                next = (BLOCK*) succ->datap;
                ep = (EDGE*) ckalloc(1,sizeof(EDGE));
                ep->from = bp;
                ep->to = next;
                lp = NEWLIST(proc_lq);
                (EDGE*) lp->datap = ep;
                LAPPEND(edges,lp);
                if(next->visited == IR_FALSE) {
                        /*
                        **      all visited to unvisited vertix edges are in the DFST
                        */
                        ep->edgetype = ADVANCING_TREE;
                        dfo_search(next);
                } else {
                        /*
                        **      if we've visited this node and its predecessor
                        **      dfnumber 1s have been assigned to both vertices
                        **      if the from is less than to it's an advancing edge
                        **      else it could be a retreating or cross edge
                        **      these will be distinguished later
                        */
                        if(dfnumber1[next->blockno] > dfnumber1[bp->blockno]) {
                                ep->edgetype = ADVANCING;
                        } else {
                                ep->edgetype = RETREATORCROSS;
                        }
                }
        }
        if(dfonext) {
                dfonext->dfoprev = bp;
        } else {
                first_block_dfo = bp;
        }
        dfnumber2[bp->blockno] = dfcount2;
        dfcount2--;
        bp->dfonext = dfonext;
        dfonext = bp;
}

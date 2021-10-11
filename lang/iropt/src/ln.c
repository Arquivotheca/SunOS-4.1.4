#ifndef lint
static	char sccsid[] = "@(#)ln.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
#include "iropt.h"
#include "page.h"
#include "misc.h"
#include "cost.h"
#include "loop.h"
#include "ln.h"
#include <stdio.h>

/* global */
LOOP_TREE *loop_tree_tab;
LOOP_TREE *loop_tree_root;

/* local */

static unsigned n_tree_nodes;
static tree_depth;
static BOOLEAN is_loop_nested(/*inner,outer*/);
static void visit_loop_tree(/*ltp*/);
static void assign_loop_weight(/*ltp, loop_weight*/);


/*
**	the loop tree encodes the loop nesting structure
**	ie loop1 is a child of loop2 if loop1 is entirely contained in loop2
**	the algorithm is to make a tree with all loops as children of a (fake)
**	root and then successively change siblings to children
**	overlapping loops ie neither nested nor disjoint are treated as disjoint
*/

void
make_loop_tree()
{
	register LIST *lp;
	LOOP *loop;
	register char * child_pt, *cp;
	LOOP_TREE *loop_tree_pt;

	/*
	**	allocate space for the tree in one swoop since the number of
	**	nodes doesn't change, children are encoded
	**	in a byte vector
	*/

	if(nloops <= 0) return;
	n_tree_nodes = nloops+1;
	loop_tree_tab = (LOOP_TREE*)ckalloc(n_tree_nodes,sizeof(LOOP_TREE));
	child_pt = (char*) ckalloc(n_tree_nodes*n_tree_nodes,sizeof(char));

	/*
	**	root node initialized to no siblings and all nodes other than
	**	itself as children
	*/
	loop_tree_root = &loop_tree_tab[n_tree_nodes-1];
	loop_tree_root->children = child_pt;
	for(cp = child_pt; cp < &child_pt[nloops]; cp++) {
		*cp = 1;
	}
	child_pt += n_tree_nodes;

	/*
	**	all other nodes initialized to no children and all nodes other
	**	than the root as siblings
	*/
	loop_tree_pt = loop_tree_tab;
	LFOR(lp,loops) {
		loop = (LOOP*) lp->datap;
		loop_tree_pt->loop = loop;
		loop_tree_pt->parent = loop_tree_root;
		loop_tree_pt->children = child_pt;
		child_pt += n_tree_nodes;
		loop_tree_pt++;
	}

	visit_loop_tree(loop_tree_root);
	tree_depth = -1; 	/* initialize to -1 to account for the dummy root*/
	assign_loop_weight(loop_tree_root, (COST)1);
}

static BOOLEAN
is_loop_nested(inner,outer)
	register LOOP *inner, *outer;
{
	register EDGE *ep;
	register LIST *lp;

	/*
	**	if, for all of inner's back edges,both vertices of the edge are in outer
	**	inner is contained in outer;
	*/
	LFOR(lp,inner->back_edges) {
		ep = (EDGE*) lp->datap;
		if( (!INLOOP(ep->from->blockno,outer))  ||
			(!INLOOP(ep->to->blockno,outer)) ) {
			return IR_FALSE;
		}	
	}
	return IR_TRUE;
}

static void
visit_loop_tree(ltp)
	LOOP_TREE *ltp;
{
	register LOOP *loop1, *loop2;
	register int i, ii;
	register LOOP_TREE *ltp1, *ltp2;

	for(i=0; i<nloops; i++) if(ltp->children[i]) {
		ltp1 = loop_tree_tab + i;
		loop1 = ltp1->loop;
		for(ii=0; ii<nloops; ii++) if(ltp->children[ii] && i != ii) {
			ltp2 = loop_tree_tab + ii;
			loop2 = ltp2->loop;
			if(	is_loop_nested(loop2,loop1) == IR_TRUE ) {
				ltp2->parent = ltp1;
				ltp->children[ii] = 0;
				ltp1->children[ii] = 1;
			}
		}
	}

	for(i=0; i<nloops; i++) if(ltp->children[i]) {
		visit_loop_tree(loop_tree_tab+i);
	}
}

#define max(x,y) (((x) > (y)) ? (x) : (y))

static void
assign_loop_weight(ltp, loop_weight)
	LOOP_TREE *ltp;
	COST loop_weight;
{
	register LOOP_TREE *ltp2;
	register LIST *lp;
	register BLOCK *block;
	register LOOP *loop;
	register char *cp;

	tree_depth++;
	for(ltp2=loop_tree_tab,cp = ltp->children; 
		cp < &ltp->children[nloops]; cp++,ltp2++) {
		if (*cp) {
			assign_loop_weight(ltp2,
			    mul_costs(loop_weight, (COST)LOOP_WEIGHT_FACTOR));
		}
	}
	loop = ltp->loop; 
	if(loop) {
		if(SHOWDF == IR_TRUE) {
			printf("loopno %d depth %d\n",loop->loopno,tree_depth);
		}
		LFOR(lp,loop->blocks) {
			block = (BLOCK*) lp->datap;
			if((COST)block->loop_weight < loop_weight)
				block->loop_weight = (int)loop_weight;
		}
	}
	tree_depth --;
}

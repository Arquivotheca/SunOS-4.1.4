/*      @(#)opt_context.c  1.1     94/10/31     Copyr 1986 Sun Microsystems */

#include <stdio.h>
#include "structs.h"
#include "registers.h"
#include "bitmacros.h"
#include "node.h"
#include "optimize.h"
#include "opcode_ptrs.h"
#include "node.h"
#include "opt_utilities.h"
#include "opt_statistics.h"
#include "opt_progstruct.h"
#include "opt_regsets.h"
#include "opt_context.h"
#include "alloc.h"

/*
 * discover facts about the basic blocks of a program.
 * These include:
 *	names referenced
 *	register contents on exit.
 *	window registers totally dead in this block
 * We'll stash this information in the context structure,
 * for the use of higher-level optimizations.
 * We'll also do some optimizations as we go.
 * In particular, we want to avoid re-forming the same name within
 * a block.
 * In the 68k c2 optimizer, this is known as content analysis.
 */

/******************************************************************************/
/***		context packing, unpacking, & printing			    ***/
/******************************************************************************/

static void
save_context( bbp, maxnames )
	struct basic_block * bbp;
{
	struct regtable_entry *	rtp;
	struct name  *		np;
	struct namenode *	nnp;
	int 			nnames,
				nregs;
	struct context *	ctp;
	Regno			rn;
	
	/* count the resources we'll need */
	for ( nnp = name_array, nnames = 0;
	      nnp != &name_array[maxnames];
	      nnp += 1 )
	      	if (nnp->np != NULL) nnames += 1;
	for ( rn = RN_G0, nregs = 0; rn < NREGS ; rn += 1 ){
		if (regtable[rn][VALUE].valid)  nregs += 1;
		if (regtable[rn][MEMORY].valid) nregs += 1;
	}
	/* allocate one of the suckers */
	if (bbp->bb_context != NULL)
		as_free(bbp->bb_context);
	ctp = (struct context *)as_malloc( sizeof(struct context) + nregs*sizeof(struct regval) + nnames*sizeof(struct name) );
	 
	/* fill it in */
	ctp->ctx_deadregs = totally_dead;
	ctp->ctx_nnames = nnames;
	ctp->ctx_names = (struct name *)(ctp+1);
	ctp->ctx_nregs = nregs;
	ctp->ctx_regs  = (struct regval *)(ctp->ctx_names+nnames);
	for ( nnp = name_array, nnames = 0;
	     nnp != &name_array[maxnames];
	     nnp += 1 )
	     	if (nnp->np != NULL){
	    		ctp->ctx_names[nnames++] = nnp->name;
	     	}
	for ( rn = RN_G0, nregs = 0; rn < NREGS ; rn += 1 ){
		if (regtable[rn][VALUE].valid){
			ctp->ctx_regs[nregs].rv_reg = rn;
			ctp->ctx_regs[nregs].rv_vm = VALUE;
			ctp->ctx_regs[nregs].rv_rte = regtable[rn][VALUE];
			nregs += 1;
		}
		if (regtable[rn][MEMORY].valid){
			ctp->ctx_regs[nregs].rv_reg = rn;
			ctp->ctx_regs[nregs].rv_vm = MEMORY;
			ctp->ctx_regs[nregs].rv_rte = regtable[rn][MEMORY];
			nregs += 1;
		}
	}
	/* finally, hook it to the bb structure */
	bbp->bb_context = ctp;
}

/*
 * merge the contexts of the predecessors of this basic block.
 * we use the naive approach.
 */
static void
merge_contexts( bbp )
	struct basic_block * bbp;
{
	int pred_n, n;
	struct context *ctp;
	register struct regval *rvp;
	register struct regtable_entry *old;
	Bool	valid[NREGS][MEMORY+1], vv;
	
	regtable_forget_all();
	if (bbp->bb_npred == 0 ) return; /* thats all, folks! */
	
	/* unconditionally install the first one */
	ctp = bbp->bb_predlist[0]->bb_context;
	for ( rvp = ctp->ctx_regs, n = ctp->ctx_nregs; n!=0; rvp++, n--){
		regtable[rvp->rv_reg][rvp->rv_vm] = rvp->rv_rte;
	}
	/* from now on, it gets tougher */
	for ( pred_n = 1; pred_n < bbp->bb_npred; pred_n += 1 ){
		ctp = bbp->bb_predlist[pred_n]->bb_context;
		for ( n = 0 ; n < NREGS; n += 1 ){
			valid[n][VALUE]=FALSE;	valid[n][MEMORY]=FALSE;
		}
		for ( rvp = ctp->ctx_regs, n = ctp->ctx_nregs; n!=0; rvp++, n--){
			/* compare with whats already there */
			old = & regtable[rvp->rv_reg][rvp->rv_vm];
			vv = TRUE;
			if (!old->valid) break; /* forget it */
			if ( !(old->v.nm_symbol == rvp->rv_rte.v.nm_symbol
			    && old->v.nm_addend == rvp->rv_rte.v.nm_addend))
				vv = FALSE; /* no way */
			switch(rvp->rv_vm){
			case VALUE:
				if (old->hipart != rvp->rv_rte.hipart)
					vv = FALSE;
				break;
			case MEMORY:
				if (old->size != rvp->rv_rte.size)
					vv = FALSE;
				if (old->v.nm_symbol == NULL
				&& (old->basereg != rvp->rv_rte.basereg))
					vv = FALSE;
				break;
			}
			valid[rvp->rv_reg][rvp->rv_vm] = vv;
		}
		/* now, only retain the things we marked as ok */
		for ( n = 0 ; n < NREGS; n += 1 ){
			regtable[n][VALUE].valid = valid[n][VALUE];
			regtable[n][MEMORY].valid = valid[n][MEMORY];
		}
	}
}
	
/* this is for debugging only */
void
print_context( ctp )
	struct context * ctp;
{
	int		n;
	struct name *	np;
	struct regval *	rvp;
	extern char *regnames[];
	
	if (ctp->ctx_nnames != 0 ){
		printf("	NAMES:\n");
		for ( np = ctp->ctx_names, n = ctp->ctx_nnames; n!=0; np++, n--){
			printf("\t\t");
			if (np->nm_symbol!= NULL){
				printf("%s+",np->nm_symbol->s_symname);
			}
			printf("%d\n",np->nm_addend);
		}
	}
	if (ctp->ctx_nregs != 0 ){
		printf("	REGS:\n");
		for ( rvp = ctp->ctx_regs, n = ctp->ctx_nregs; n!=0; rvp++, n--){
			printf("\t\t%s\t", regnames[rvp->rv_reg]);
			if (rvp->rv_vm == MEMORY)
				printf("[");
			print_regentry( &(rvp->rv_rte), rvp->rv_vm );
			if (rvp->rv_vm == MEMORY)
				printf("]");
			printf("\n");
		}
	}
}
			
/******************************************************************************/
Bool
propagate_constants_and_register_contents( marker_np )
	Node *marker_np;
{
	struct basic_block * bbp, *bl;
	int nnames;
	Bool changed_bb_struct = FALSE;
	Bool did_something = FALSE;
	Bool keep_going, assigned_goto_found;

	if ( ! do_opt[ OPT_CONSTPROP] ) return FALSE;
	bl = make_blocklist( marker_np, &assigned_goto_found );
	bbp = bl;
	while (bbp != 0){
		nnames = find_names( bbp->bb_dominator, bbp->bb_trailer );
		did_something |= share_names( nnames, bbp  );
		regtable_forget_all();
		did_something |= regtable_scan_block( bbp, &changed_bb_struct );
		save_context( bbp, nnames );
		if (did_something) /* dummy lines */
			did_something = TRUE;
		if (changed_bb_struct) {
			free_blocklist( bl );
			bl = make_blocklist( marker_np, &assigned_goto_found );
			bbp = bl;
			changed_bb_struct = FALSE;
		} else bbp = bbp->bb_chain;
	}
	keep_going = ! assigned_goto_found;
	while( keep_going){
		keep_going = FALSE;
		bbp = bl;
		while (bbp != 0){
			merge_contexts( bbp );
			keep_going |= regtable_scan_block( bbp, &changed_bb_struct );
			nnames = find_names( bbp->bb_dominator, bbp->bb_trailer );
			save_context( bbp, nnames );
			did_something |= keep_going; /* dummy line */
			/*
			 * If changed_bb_struct we have to start all over again.
			 * We might as well bail out and finish the work next
			 * time around.
			 */
			if (changed_bb_struct){
				free_blocklist( bl );
				return TRUE;
			}
			else bbp = bbp->bb_chain;
		}
	}
	free_blocklist( bl );
	return did_something;
}

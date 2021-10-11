/*      @(#)opt_progstruct.c  1.1     94/10/31     Copyr 1986 Sun Microsystems */

#include <stdio.h>
#include "structs.h"
#include "bitmacros.h"
#include "alloc.h"
#include "optimize.h"
#include "opt_utilities.h"
#include "opt_statistics.h"
#include "opt_progstruct.h"


/* opcode function description: has f_group, f_subgroup, &c  */
#define OP_FUNC(np)	((np)->n_opcp->o_func)

#define new_block()	((struct basic_block *)as_calloc( sizeof (struct basic_block), 1))

static void
add_bbpred( b, p)
	struct basic_block *b, *p;
{
	if (b->bb_npred == 0){
		b->bb_predlist = (struct basic_block **)as_malloc( sizeof(struct basic_block*));
		*(b->bb_predlist) = p;
		b->bb_npred =1;
	} else {
		b->bb_predlist = (struct basic_block **)as_realloc( b->bb_predlist, (b->bb_npred+1)*sizeof(struct basic_block*));
		b->bb_predlist[(b->bb_npred)++] = p;
	}
}

static void
add_bbsucc( b, p)
	struct basic_block *b, *p;
{
	if (b->bb_nsucc == 0){
		b->bb_succlist = (struct basic_block **)as_malloc( sizeof(struct basic_block*));
		*(b->bb_succlist) = p;
		b->bb_nsucc =1;
	} else {
		b->bb_succlist = (struct basic_block **)as_realloc( b->bb_succlist, (b->bb_nsucc+1)*sizeof(struct basic_block*));
		b->bb_succlist[(b->bb_nsucc)++] = p;
	}
}

static struct basic_block *
symblock( sp )
	struct symbol *sp;
{
	/* given a symbol, return the corresponding bb, creating it if necessary */
	if (sp->s_blockname == NULL ){
		sp->s_blockname = new_block();
	}
	return sp->s_blockname;
}

/*
 * look at our program and make from it a list of basic blocks.
 * these blocks have one entry (the dominator -- usually a label)
 * and multiple exits. They are chains of nodes. If there are multiple
 * uses of the label, then there may be multiple
 * predecessors. If the block contains or ends with conditional or
 * computed branches, then the block may have multiple successors.
 * If we find a computed goto, then we cannot reliably propegate
 * flow information, so we notify our caller of this situation.
 */

struct basic_block *
make_blocklist( marker_np, assigned_goto_found_p )
	Node *marker_np;
	Bool *assigned_goto_found_p;
{
	register Node * np, *dominator;
	Node * next;
	struct basic_block *listhead = NULL;
	struct basic_block **listtail = &listhead;
	struct basic_block *bbp, *destbp, *thisbp;
	Bool fallthrough = FALSE;
	int    nodenumber;

	nodenumber = 1;
	*assigned_goto_found_p = FALSE;
	for (dominator = marker_np->n_next; dominator != marker_np; dominator = np ){
		/* have a dominator. see if the block already exists */
		if (OP_FUNC(dominator).f_group == FG_LABEL){
			/* which is very likely...*/
			*listtail = thisbp = symblock( dominator->n_operands.op_symp );
		} else {
			*listtail = thisbp = new_block();
		}
		listtail = &(thisbp->bb_chain);
		/*
		 * if the last block ended with a conditional branch, it could be
		 * a predecessor, even though it didn't explicitly name our dominator
		 */
		if (fallthrough){
			/* here, bbp still points to the previous, falling-through block */
			add_bbpred( thisbp, bbp);
			add_bbsucc( bbp, thisbp );
			fallthrough = FALSE;
		}
		bbp = thisbp;
		bbp->bb_dominator = dominator;
		bbp->bb_name = nodenumber++;
		/* now iterate through this block, finding the end and finding successors */
		for (np = dominator; np != marker_np; np = np->n_next ){
			switch(OP_FUNC(np).f_group){
			case FG_BR:
				/* branch */
				if ( BRANCH_IS_CONDITIONAL(np)){
					/* conditional branch */
					fallthrough = TRUE;
				}
				/* hook to destination */
				destbp = symblock( np->n_operands.op_symp );
				add_bbpred( destbp, bbp );
				add_bbsucc( bbp, destbp );
				/* this is the end of this block
				 * not counting the delay slot
				 * ... and any trailing comments
				 */
				np = next_code_generating_node_or_label(np->n_next);
				bbp->bb_trailer = np->n_prev;
				break; /* out of switch */

			case FG_JMP:
				/* this had better be a return or switch */
				next = next_code_generating_node(np);
				if ( OP_FUNC(next).f_group     == FG_ARITH
				&&   OP_FUNC(next).f_subgroup  == FSG_REST ){
					np = next;
				} else if ( is_jmp_at_top_of_cswitch(np) ){
					/* switch has many successors */
					for (np = np->n_next->n_next->n_next;
					OP_FUNC(np).f_group == FG_CSWITCH;
				        np = np->n_next ){
						struct symbol *sp;
						sp = np->n_operands.op_symp;
						if (sp != NULL){
							if (sp->s_opertype != OPER_NONE) {
								/*
								 * Another PIC kludge ... see
								 * symbol_expression.c for the
								 * grim details
								 */
								Node *def_node = sp->s_def_node;
								if (def_node != NULL) {
									/*
									 * get correct symbol
									 */
									sp = def_node->n_operands.op_symp;
								} else {
									/* 
									 * symbol undefined; bail out
									 */
									*assigned_goto_found_p = TRUE;
								}
							}
							destbp = symblock(sp);
							add_bbpred( destbp, bbp );
							add_bbsucc( bbp, destbp );
						}
					}
					/* back up one some next_code...() will work */
					np= np->n_prev;
				} else {
					*assigned_goto_found_p = TRUE;
				}
				/* suck up trailing comments */
				np = next_code_generating_node_or_label(np);
				bbp->bb_trailer = np->n_prev;
				/* this is the beginning of the next block */
				/* it SHOULD be a label */
				/* in any case, we can't get there from here */
				break; /* out of switch */

			case FG_LABEL:
				/* a label is the beginning of the next block */
				/* unless its the beginning of this one! */
				if (np != dominator){
					/* if the symbol is global we have a
					 * FORTRAN multiple entry case and we
					 * shouldn't fall through.
					 */
					fallthrough = !BIT_IS_ON(np->n_operands.op_symp->s_attr, SA_GLOBAL);
					break; /* out of switch, then out of loop */
				}
				/* FALLTHROUGH!! */

			default:
				bbp->bb_trailer = np;
				continue;
			}	/* end switch */
			break;	/* out of loop */
		}
	}
	return listhead;
}


void
free_blocklist( bbp )
	struct basic_block* bbp;
{
	struct basic_block* next;

	while (bbp != 0 ){
		next = bbp->bb_chain;
		if (bbp->bb_predlist)
			as_free( bbp->bb_predlist );
		if (bbp->bb_succlist)
			as_free( bbp->bb_succlist );
		if (bbp->bb_context)
			as_free( bbp->bb_context );
		if (OP_FUNC(bbp->bb_dominator).f_group == FG_LABEL)
			bbp->bb_dominator->n_operands.op_symp->s_blockname = NULL;
		as_free( bbp );
		bbp = next;
	}
}

/* this is only for debugging */

void
print_blocklist( bbp )
	struct basic_block *bbp;
{
	struct basic_block **bbnp;
	Node *np;
	int n;

	/* here we go */
	for( bbp; bbp != 0; bbp = bbp->bb_chain ){
		/* haven't visited here yet */
		printf("node %d:\n", bbp->bb_name );
		/* print out pred & succ lists */
		if (bbp->bb_npred != 0){
			printf("\tpred:");
			for( n=bbp->bb_npred, bbnp=bbp->bb_predlist;
			n != 0;
			n -=1, bbnp+=1){
				printf(" %d", (*bbnp)->bb_name);
			}
			printf("\n");
		}
		if (bbp->bb_nsucc != 0){
			printf("\tsucc:");
			for( n=bbp->bb_nsucc, bbnp=bbp->bb_succlist;
			n != 0;
			n -=1, bbnp+=1){
				printf(" %d", abs((*bbnp)->bb_name));
			}
			printf("\n");
		}
		/* print out contents of block */
		for ( np = bbp->bb_dominator;
		    np != bbp->bb_trailer->n_next;
		    np = np->n_next)
			disassemble_node( np );
		/* print out context */
		if (bbp->bb_context != NULL )
			print_context( bbp->bb_context );
	}
}


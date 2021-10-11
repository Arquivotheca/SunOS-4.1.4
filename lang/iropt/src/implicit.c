#ifndef lint
static	char sccsid[] = "@(#)implicit.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include "recordrefs.h"
#include "page.h"
#include "misc.h"
#include "implicit.h"
#include <stdio.h>
#include <ctype.h>

/* local */
static LIST	*externals, *statics, *heap_leaves, *arg_leaves, *fvals;
static void	insert_externals(/*newleaf*/);
static void	lookfor_implicit();
static TRIPLE	*note_side_eff(/*op, operand, site, after*/);

/* inserts a leaf in the externals list for C's SCALL and FCALL */

static void
insert_externals( newleaf)
	LEAF	*newleaf;
{
	LIST	*acc_list;

	acc_list = NEWLIST(tmp_lq);
	(LEAF *)acc_list->datap = newleaf;
	LAPPEND(externals, acc_list);
}


void
insert_implicit()
{
	register LIST *lp, *lp1;
	SEGMENT *sp;
	register TRIPLE *tp, *first, *tp1;
	LIST *real_entries;
	BLOCK *bp;
	LEAF  *leafp, *lf;

	statics = externals = heap_leaves = arg_leaves = LNULL;
	for(sp=seg_tab; sp != NULL; sp = sp->next_seg) {
		if(sp->descr.external == EXTSTG_SEG && sp->descr.class != HEAP_SEG &&
					sp->leaves != LNULL) {
			if( partial_opt ) { /* do NOT trace the external variables. */
				continue;
			}
			lp = (LIST*) copy_list(sp->leaves,tmp_lq);
			LAPPEND(externals,lp);
		} else if((sp->descr.class == BSS_SEG || sp->descr.class == DATA_SEG) &&
							sp->leaves != LNULL) {
			lp = (LIST*) copy_list(sp->leaves,tmp_lq);
			LAPPEND(statics,lp);
		} else if( sp->descr.class == ARG_SEG && sp->leaves != LNULL) {
			/* no exit_use of arguments */
			lp = (LIST*) copy_list(sp->leaves, tmp_lq);
			LAPPEND(arg_leaves,lp);
		} else if(sp->descr.class == HEAP_SEG && sp->leaves != LNULL) {
			if( partial_opt ) {
				continue;
			}
			lp = (LIST*) copy_list(sp->leaves,tmp_lq);
			LAPPEND(heap_leaves,lp);
		}
	}

	/* look for real entry points and insert IR_ENTRYDEFs at each one */
	real_entries = LNULL;
	LFOR(lp, ext_entries) {
		bp = (BLOCK*) lp->datap;
		if(bp->labelno > 0 ) {
			LIST *lp2;

			lp2 = NEWLIST(tmp_lq);
			(BLOCK*) lp2->datap = bp;
			LAPPEND(real_entries,lp2);
		}
	}

	LFOR(lp, real_entries) {
		bp = (BLOCK*) lp->datap;
		first = bp->last_triple->tnext;
		tp = first;
		TFOR(tp1, first) {
			if (tp1->op == IR_PASS)
				tp = tp1;
		}
/*
		if(ISOP(tp->op,BRANCH_OP)) {
			tp = tp->tprev;
		}
*/
		tp = add_implicit_list(IR_ENTRYDEF, externals, TNULL, tp);
		tp = add_implicit_list(IR_ENTRYDEF, heap_leaves, TNULL, tp);
		tp = add_implicit_list(IR_ENTRYDEF, statics, TNULL, tp);
		tp = add_implicit_list(IR_ENTRYDEF, arg_leaves, TNULL, tp);
/*
		if(!ISOP(bp->last_triple->op,BRANCH_OP)) {
			entry_block->last_triple = tp;
		}
*/
	}
	if (hdr.lang != FORTRAN && seg_tab[HEAP_SEGNO].leaves &&
		partial_opt != IR_TRUE) {
		leafp = (LEAF *) seg_tab[HEAP_SEGNO].leaves->datap;
		insert_externals(leafp);	
		LFOR (lp, leafp->overlap)
			if (LCAST(lp, LEAF)->val.addr.seg->descr.external != EXTSTG_SEG) {
				lf = LCAST(lp, LEAF);
				insert_externals(lf);
				LFOR(lp1, lf->overlap) 
					insert_externals(LCAST(lp1, LEAF));
			}
		externals = order_list(externals, tmp_lq);
	}
	lookfor_implicit();

	LFOR (lp, fvals) {
		/* 
		** EXITUSEs are used by reg_alloc to restore the memory
		** version of variables used outside the routine. They
		** should really occur after the return value has been
		** computed. However on SPARC, there is a conflict between
		** use of o0 to hold the return value and use of o0 to
		** effect the register store. Hence EXITUSEs immediately
		** precede FVALs
		*/
		tp = (TRIPLE *) lp->datap;
		tp = tp->tprev;
		tp = add_implicit_list(IR_EXITUSE, externals, TNULL, tp);
		if (!DO_ALIASING(hdr))
			tp = add_implicit_list(IR_EXITUSE, heap_leaves, TNULL, tp);
		tp = add_implicit_list(IR_EXITUSE, statics, TNULL, tp);
	}
	if (fvals == LNULL) {
       		tp = exit_block->last_triple;
       		if(ISOP(tp->op,BRANCH_OP)) {
               		tp = tp->tprev;
       		}
       		tp = add_implicit_list(IR_EXITUSE, externals, TNULL, tp);
       		if (!DO_ALIASING(hdr))
               		tp = add_implicit_list(IR_EXITUSE, heap_leaves, TNULL, tp);
       			tp = add_implicit_list(IR_EXITUSE, statics, TNULL, tp);
       			if(!ISOP(entry_block->last_triple->op,BRANCH_OP)) {
               			exit_block->last_triple = tp;
       			}
	}

	empty_listq(tmp_lq);
}

/*
**	scan all blocks and insert implicit uses and def triples for
**	leaves that are affected but do not explicitly appear.
**	For calls that are not intrinsic or support functions
**	externals are used and killed
**
** for expressions that contain several calls only 
** insert one set of external uses/defs around
** the surrounding root triples
** in x=f(i)+x the def of x implied by the
** call does not reach its use within the same 
** expression. this is legal because of order of
** evaluation freedom 
**
*/

static void
lookfor_implicit()
{
	register BLOCK *bp;
	TRIPLE *tp, *last_root, *argp, *before, *after, *first, *call_site;
	LIST *lp, *lp1, *newl;
	LEAF *leafp;
	FUNC_DESCR func_descr;
	TRIPLE *external_uds = TNULL;	/* call triple to which external */
					/* uses and defs are credited*/
	/*
	 * KLUDGE: use var_refs to hold (leaf,triple) pairs for
	 * implicit defs within an expression containing calls.
	 * Free the var_refs at ROOT_TRIPLE boundaries.  This
	 * assumes that there are no real var_refs at the moment.
	 *
	 * This is a (choke) interim patch to banish the use of
	 * ckalloca(), prior to banishing the use of implicit u/d
	 * triples, which presumably will get rid of this entire
	 * file.  My apologies to the reader if this didn't happen.
	 */
	VAR_REF *rp;
	TYPE type;

	type.tword = UNDEF;
	type.size = 0;
	type.align = 0;
	fvals = LNULL;
	if (var_ref_head != NULL) {
		quit("lookfor_implicit: unexpected var_refs in use\n");
		/*NOTREACHED*/
	}
	for(bp=entry_block; bp; bp=bp->next)    {
		if(bp->last_triple==TNULL) continue;

		first = bp->last_triple->tnext;
		last_root = TNULL;
		TFOR(tp,first) {
			if (tp->op == IR_FVAL) {
				lp1 = NEWLIST(tmp_lq);
				(TRIPLE*) lp1->datap = tp;
				LAPPEND(fvals, lp1);
			}
			if(	tp->op == IR_IMPLICITDEF || tp->op == IR_IMPLICITUSE ||
				tp->op == IR_ENTRYDEF || tp->op == IR_EXITUSE ) {
				continue;
			}

			if( ISOP(tp->op,ROOT_OP ) ){
				/* 	new root: check if we have any pending implicit u/d and 
				**	insert uses after the previous root and defs before this 
				**	root
				*/
				if(external_uds != TNULL) {
					if (last_root == TNULL) {
						quit("lookfor_implicit: rootless function call");
						/*NOTREACHED*/
					}
					while(	last_root->tnext->op == IR_IMPLICITUSE &&
							last_root->tnext->right == (IR_NODE*)last_root){
						last_root = last_root->tnext;
					}
					last_root = add_implicit_list(IR_IMPLICITUSE, externals,  
										external_uds, last_root);
				}

				before = tp->tprev;
				for(rp = var_ref_head; rp ; rp = rp->next ) {
					call_site = rp->site.tp;
					if (partial_opt && EXT_VAR(rp->leafp)) {
						continue;
					}
					before = append_triple(before, IR_IMPLICITDEF, 
							(IR_NODE*)rp->leafp, (IR_NODE*)call_site,type);
					newl = NEWLIST(proc_lq);
					(TRIPLE*) newl->datap = before;
					LAPPEND(call_site->implicit_def,newl);
				}
				if (var_ref_head != NULL)
					(void)new_var_ref(IR_TRUE);

				if(external_uds != TNULL) {
					before = add_implicit_list(IR_IMPLICITDEF, externals,  
										external_uds, before);
					external_uds = TNULL;
				}
				last_root = tp;
			}

			before = tp->tprev;
			after = tp;
			switch(tp->op) {
			
				case IR_SCALL:
					if(tp->left->operand.tag == ISLEAF) {
						func_descr = tp->left->leaf.func_descr;
					} else {
						func_descr = EXT_FUNC;
					}
					before = note_side_eff(IR_IMPLICITUSE, tp->left,tp, before);
					TFOR(argp, (TRIPLE*) tp->right) {
						before =note_side_eff(IR_IMPLICITUSE,argp->left,tp,before);
						if(argp->can_access != LNULL) {
							before = add_implicit_list(IR_IMPLICITUSE, 
												argp->can_access, tp, before);
							if(func_descr !=INTR_FUNC){
								after = add_implicit_list(IR_IMPLICITDEF, 
													argp->can_access,tp, after);
								LFOR(lp,argp->can_access) {
									leafp = (LEAF*) lp->datap;
									after = note_side_eff(IR_IMPLICITDEF,
										(IR_NODE*)leafp, tp, after);
								}
							}
						}
					}
					if(func_descr == EXT_FUNC) {
						before = add_implicit_list(IR_IMPLICITUSE, externals,tp,before);
						after = add_implicit_list(IR_IMPLICITDEF, externals, tp, after);
					}
					break;

				case IR_FCALL:
					if(tp->left->operand.tag == ISLEAF) {
						func_descr = tp->left->leaf.func_descr;
					} else {
						func_descr = EXT_FUNC;
					}
					before = note_side_eff(IR_IMPLICITUSE, tp->left,tp, before);
					TFOR(argp, (TRIPLE*) tp->right) {
						before =note_side_eff(IR_IMPLICITUSE,argp->left,tp,before);
						if(argp->can_access != LNULL) {
							before = add_implicit_list(IR_IMPLICITUSE, 
												argp->can_access, tp, before);
							if(func_descr !=INTR_FUNC){
								LFOR(lp,argp->can_access) {
									LIST *llp, *overlap;
									rp = new_var_ref(IR_FALSE);
									rp->leafp = (LEAF*) lp->datap;
									rp->site.tp = tp;
									overlap = rp->leafp->overlap;
									LFOR(llp, overlap) {
										rp = new_var_ref(IR_FALSE);
										rp->leafp = (LEAF*)llp->datap;
										rp->site.tp = tp;
									}
								}
							}
						}
					}
					if( tp->left->operand.tag == ISLEAF &&
						tp->left->leaf.class == CONST_LEAF &&
						tp->left->leaf.func_descr != EXT_FUNC
					){
						/* then it's a support function */	
					} else {
						/* avoid putting out multiple copies of 
						**	external ud for an expression like 
						**	f1()+f2(). 
						**	The external uds get credited to f2
						*/
						external_uds = tp;
					}
					break;

				case IR_IFETCH:
					before = note_side_eff(IR_IMPLICITUSE, tp->left,tp, 
									before);
					before = add_implicit_list(IR_IMPLICITUSE, tp->can_access, tp,
									before);
					LFOR(lp,tp->can_access) {
						leafp = (LEAF*) lp->datap;
						before = note_side_eff(IR_IMPLICITUSE,
							(IR_NODE*)leafp, tp, before);
					}
					break;

				case IR_ISTORE:
					before = note_side_eff(IR_IMPLICITUSE, tp->left,tp, 
									before);
					before = note_side_eff(IR_IMPLICITUSE, tp->right,tp, 
									before);
					before = add_implicit_list(IR_IMPLICITUSE, tp->can_access, tp,
									before);
					after = add_implicit_list(IR_IMPLICITDEF, tp->can_access, tp,
									after);
					LFOR(lp,tp->can_access) {
						leafp = (LEAF*) lp->datap;
						before = note_side_eff(IR_IMPLICITUSE,
							(IR_NODE*)leafp,tp, before);
						after = note_side_eff(IR_IMPLICITDEF,
							(IR_NODE*)leafp,tp, after);
					}
					break;

				default:
					if( ISOP(tp->op, USE1_OP)) {
						(void)note_side_eff(IR_IMPLICITUSE,
							tp->left, tp, before);
					}
					if( ISOP(tp->op, USE2_OP)) {
						(void)note_side_eff(IR_IMPLICITUSE,
							tp->right, tp, before);
					}
					if( ISOP(tp->op, MOD_OP)) {
						(void)note_side_eff(IR_IMPLICITDEF,
							tp->left, tp, after);
					}
					break;

			}
		}
		if(external_uds != TNULL) {
			 quit("lookfor_implicit: function value(s) not used");
		}
	}
	(void)new_var_ref(IR_TRUE);
}

/* 
** find the set of leaves that can be affected (used or defined) along with
** the operand explicitly named in a triple. If the operand is a leaf then
** the access list could be all leaves statically aliased with it; if its
** an indirection then the access list is the union of the overlap lists
** of all leaves on the indirect nodes's can_access list
*/

static TRIPLE *
note_side_eff(op, operand, site, after)
	IR_OP op;
	IR_NODE *operand;
	TRIPLE *site;
	TRIPLE *after;
{
	LIST *acc_list = LNULL;
	register LIST *lp, *lp2;
	LEAF *leafp;
	TYPE type;
	SEGMENT *segp;

	type.tword = UNDEF;
	type.size = 0;
	type.align = 0;
	if(operand->operand.tag == ISLEAF && operand->leaf.overlap) {
		acc_list = operand->leaf.overlap = 
					order_list(operand->leaf.overlap,proc_lq);
		LFOR(lp,acc_list) {
			leafp = (LEAF*) lp->datap;
			if (partial_opt) {
				/*
				 * if partial_opt is on, we only care about locals
				 */
				if (leafp->no_reg) {
					continue;
				}
				segp = leafp->val.addr.seg;
				if (segp != NULL && segp->descr.external == EXTSTG_SEG) {
					continue;
				}
			}
			if(leafp != (LEAF*) operand ) {
				LIST *lp3; TRIPLE *tp;
				if( op == IR_IMPLICITDEF ) {
					lp2 = site->implicit_def;
				} else {
					lp2 = site->implicit_use;
				}
				LFOR( lp3, lp2 ) {
					tp = LCAST(lp3, TRIPLE);
					if( tp->op == op && tp->left == (IR_NODE*)leafp ) {
						goto next_leaf;
					}
				}
				after = append_triple(after, op,
					(IR_NODE*)leafp, (IR_NODE*) site, type);
				lp2 = NEWLIST(proc_lq);
				(TRIPLE*) lp2->datap = after;
				if( op == IR_IMPLICITDEF ) {
					LAPPEND(site->implicit_def,lp2);
				}
				else {
					LAPPEND(site->implicit_use,lp2);
				}
			}
next_leaf:;
		}
	}
	return after ;
}

/*
** put out a implicit uses or defs after after for every leaf on list and 
** credit them to site
*/
TRIPLE *
add_implicit_list(op, list, site, after)
	IR_OP op;
	LIST *list;
	TRIPLE *after, *site;
{
	register LIST *lp, *lp2;
	register LEAF *leafp;
	TYPE type;
	SEGMENT *segp;

	type.tword = UNDEF;
	type.size = 0;
	type.align = 0;
	LFOR(lp,list) {
		leafp = (LEAF*) lp->datap;
		if (partial_opt) {
			/*
			 * if partial_opt is on, we only care about locals
			 */
			if (leafp->no_reg) {
				continue;
			}
			segp = leafp->val.addr.seg;
			if (segp != NULL && segp->descr.external == EXTSTG_SEG) {
				continue;
			}
		}
		if( op == IR_ENTRYDEF ) {
			after = append_triple(after, op,
				(IR_NODE*)leafp, (IR_NODE*)site, type);
			leafp->entry_define = after;
			continue;
		}
		if( op == IR_EXITUSE ) {
			after = append_triple(after, op,
				(IR_NODE*)leafp, (IR_NODE*)site, type);
			leafp->exit_use = after;
			continue;
		}
		if(site != TNULL) {
			LIST *lp3; TRIPLE *tp;
			if( op == IR_IMPLICITDEF ) {
				lp2 = site->implicit_def;
			} else {
				lp2 = site->implicit_use;
			}
			LFOR( lp3, lp2 ) {
				tp = LCAST(lp3, TRIPLE);
				if( tp->op == op && tp->left == (IR_NODE*)leafp ) {
					goto next_leaf;
				}
			}
			after = append_triple(after, op,
				(IR_NODE*)leafp, (IR_NODE*)site, type);
			lp2 = NEWLIST(proc_lq);
			(TRIPLE*) lp2->datap = after;
			if( op == IR_IMPLICITDEF ) {
				LAPPEND(site->implicit_def,lp2);
			} else {
				LAPPEND(site->implicit_use,lp2);
			}
		}
next_leaf:;
	}
	return(after);
}

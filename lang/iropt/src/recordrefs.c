#ifndef lint
static        char sccsid[] = "@(#)recordrefs.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include "iropt.h"
#include "recordrefs.h"
#include "page.h"
#include "misc.h"
#include "make_expr.h"

/*
 * counters for VAR_REF, EXPR_REF, EXPR_KILLREF records
 */
int nvardefs, nexprdefs;		/* #defs of variables and expressions */
int navailvardefs, navailexprdefs;	/* n of these available on bb exit */
int nvarrefs, nexprrefs;		/* #uses+defs of vars and exprs */
int nexprkillrefs;

/* chain of all var_refs for a procedure */
VAR_REF *var_ref_head;

/* locals */

static void	record_var_def(/*site, item*/);
static void	record_expr_def(/*site, item*/);
static void	record_use(/*site, item, use1or2*/);
static void	free_ref_space();
static int	count_avail_exprdefs();
static int	count_avail_vardefs();
static BOOLEAN	record_expr_refs;

/*
**	examine each triple and build the corresponding variable and/or expr 
**	reference entries distinguishing definitions, uses and killed exprs. 
**	other routines will later mark available definitions 
**	and exposed uses
*/

void
record_refs(do_exprs)
	BOOLEAN do_exprs;
{
	register TRIPLE *tp, *first;
	register LIST *lp;
	/* list of rparams to be defined at end of an expr*/
	LIST *param_defs = LNULL;
	BLOCK *bp;
	TRIPLE *argp;
	/* in expressions with imbedded definitions due to function calls 
	   (rparams and implicit defs) these are delayed until all uses
	   are recorded */
	BOOLEAN	delay_defs;	
	SITE site;

	record_expr_refs = do_exprs;
	free_ref_space();

	for(bp=entry_block; bp; bp=bp->next)	{
		if(bp->last_triple==TNULL) continue;
		first = bp->last_triple->tnext;
		site.bp=bp;
		TFOR(tp, first) {
			if(do_exprs == IR_FALSE) {
				tp->expr = (EXPR*) NULL;
			}
			site.tp = tp;
			if(tp->op == IR_SCALL || tp->op == IR_FCALL) {
				record_use(site,tp->left,1);
				/*
				** the order of uses and defs around call sites is sensitive.
				** the strategy is to mark use of the function name, mark
				** a use of all arguments and then (1) if the call is a
				** subroutine call mark a def of all rparams (2) else
				** save a list of all such defines and put it out after
				** all uses in this rhs have been recorded to
				** ensure the defs do not reach uses in the same
				** rhs ( or kill subexpressions). There is an implicit
				** assumption that arguments to rparam are leaves or
				** indirects of address calculations - other expressions
				** should have been assigned to temporaries in pass1
				*/
				TFOR(argp,(TRIPLE*) tp->right) {
					site.tp = argp;
					record_use(site,argp->left,1);
				}

				if(tp->op == IR_SCALL) { /* it's a subroutine*/
					delay_defs = IR_FALSE;
				} else { /* it's (part of) an expression */
					delay_defs = IR_TRUE;
					/* record a definition of the expression 
					*/
					if(record_expr_refs == IR_TRUE) {
						site.tp = tp;
						record_expr_def(site,tp);
					}
				}
			} else if(tp->op == IR_IMPLICITDEF) {
				if(delay_defs == IR_TRUE) {
					/* 	an implicit def amidst an expression that contains
					**	a function call is treated as a reference parameter
					**	ie the def is a side effect that occurs after the
					**	whole expr has been recorded
					*/
					lp = NEWLIST(tmp_lq);
					(TRIPLE*) lp->datap = tp;
					LAPPEND(param_defs,lp);
				} else {
					site.tp = tp;
					record_var_def(site, (LEAF*)tp->left);
				}
			} else {
				if( ISOP(tp->op, USE1_OP) && 
					( tp->left->operand.tag != ISLEAF ||
					  (!ISCONST(tp->left)) ) ){
						record_use(site,tp->left,1);
				}
				if( ISOP(tp->op, USE2_OP) && 
					(tp->right->operand.tag != ISLEAF ||
					  (!ISCONST(tp->right)) ) ){
						record_use(site,tp->right,2);
				}
				if( ISOP(tp->op, VALUE_OP) && record_expr_refs == IR_TRUE ) {
						record_expr_def(site,tp);
				} else {
					if(ISOP(tp->op,ROOT_OP)) {
						/*
						** defs due to rparams are put out at the end 
						** of an expression after all uses are recorded
						** eg for i = b[i]+f(&i) the two uses of i and the
						** use of the + triple are recorded before the
						** definition 
						*/
						if(param_defs != LNULL) {
							LFOR(lp,param_defs) {
								argp = (TRIPLE*) lp->datap;
								site.tp = argp;
								record_var_def(site, (LEAF*)argp->left);
							}
							param_defs = LNULL;
						}
						delay_defs = IR_FALSE;
					}
					/*	the modifications due to an istore are described
					**	by implicit def triples and thus are not recorded here
					*/
					if(ISOP(tp->op, MOD_OP) && tp->op != IR_ISTORE) {
						site.tp = tp;
						record_var_def(site, (LEAF*)tp->left);
					}
				}
			}
		}
		if(param_defs != LNULL) {
			quit("record_refs: function value(s) not used");
		}
		empty_listq(tmp_lq);
        /* find out how many leaves were defined in this block - since
		** there can be at most one reaching definition per leaf per block
		** this is also the number of reaching definitions in this block
		*/
	}
	if(record_expr_refs == IR_TRUE) {
		navailexprdefs = count_avail_exprdefs();
	}
	navailvardefs = count_avail_vardefs();
}

/*
** the item being defined is a single leaf - multiple definitions due to
** assignment through an indirect node or assignment to overlapped leaves
** have been expanded into separate implicit def triples
*/

static void
record_var_def(site,item)
	SITE site;
	LEAF *item;
{
	register LIST *lp;
	register VAR_REF *var_rp;
	register EXPR_KILLREF *expr_rp;
	EXPR *expr;

	if( ((IR_NODE*) item)->operand.tag != ISLEAF) {
		/* FIXME debug */
		quit("record_var_def: bad definition");
	}
	if(partial_opt == IR_TRUE && EXT_VAR(item))
		return;
	var_rp = new_var_ref(IR_FALSE);
	var_rp->reftype = VAR_DEF;
	var_rp->site = site;
		/* 
		** defno becomes the bit number of the definition if
		** build_var_sets determines the definition is available on exit 
		** from the block - here it's initialized to an illegal bit number
		** note that the leaf was defined in this block
		*/
	var_rp->defno = NULLINDEX;
	var_rp->leafp = item;

		/*
		**  add the mod var reference to the leaf's references list
		**  for all expressions on the leaf's dependent exprs list
		**		if the expr is computed (ie not a leaf)
		**  	make a killed expr reference and
		**  	add this reference to the expr's references list
		*/

	if (++nvardefs >= MAX_VARDEFS)
	{
		quita("too many vardefs; try optimizing at a lower level");
		/*NOTREACHED*/
	}

	/* build the references IN the ORDER of OCCURS */
	if( item->references == (VAR_REF *)NULL )
		item->ref_tail = item->references = var_rp;
	else
	{
		item->ref_tail->next_vref = var_rp;
		item->ref_tail = var_rp;
	}

	if(site.tp->var_refs == (VAR_REF*) NULL) {
		site.tp->var_refs = var_rp;
	}

	if(record_expr_refs == IR_TRUE) LFOR(lp,item->dependent_exprs) {
		expr = (EXPR*) lp->datap;
		if( expr->op == IR_ERR) { /* it's a 'LEAF' expr */
			continue;
		}
		/* if the last ref to this expr was in this block and it was a kill
		** there's no information gained by recording another kill
		*/
        if( expr->ref_tail != (EXPR_REF *)NULL &&
			((EXPR_KILLREF*)(expr->ref_tail))->reftype == EXPR_KILL &&
			((EXPR_KILLREF*)(expr->ref_tail))->blockno == site.bp->blockno)
		{
			continue;
		}
		expr_rp = new_expr_killref(IR_FALSE);
		expr_rp->reftype = EXPR_KILL;
		expr_rp->blockno = site.bp->blockno;

		if( expr->ref_tail == (EXPR_REF *)NULL )
			expr->ref_tail = expr->references = (EXPR_REF*) expr_rp;
		else
		{
			expr->ref_tail->next_eref = (EXPR_REF*) expr_rp;
			expr->ref_tail = (EXPR_REF*) expr_rp;
		}

	}

}

static void
record_expr_def(site,item)
	SITE site;
	TRIPLE *item;
{
	register EXPR_REF *expr_rp;
	register EXPR *expr;

	if((expr = item->expr) == (EXPR*)NULL)
		return;
	expr_rp = new_expr_ref(IR_FALSE);
	expr_rp->reftype = EXPR_GEN;
	expr_rp->site = site;
	expr_rp->defno = NULLINDEX;
	if( expr->ref_tail == (EXPR_REF *)NULL )
		expr->ref_tail = expr->references = expr_rp;
	else
	{
		expr->ref_tail->next_eref = expr_rp;
		expr->ref_tail = expr_rp;
	}
	if (++nexprdefs >= MAX_EXPRDEFS)
	{
		quita("too many exprdefs; try optimizing at a lower level");
		/*NOTREACHED*/
	}
}

static void
record_use(site,item, use1or2)
	SITE site;
	IR_NODE *item;
	int use1or2;
{
	register LEAF *leafp;
	register VAR_REF *var_rp;
	register EXPR_REF *expr_rp;
	register EXPR *expr;

	if(item->operand.tag == ISLEAF) {
		leafp = (LEAF*) item;
		if(ISCONST(leafp)) return;
		if(partial_opt == IR_TRUE && EXT_VAR((LEAF*)item))
			return;
		var_rp =  new_var_ref(IR_FALSE);
		var_rp->reftype = ( use1or2  == 1  ? VAR_USE1 : VAR_USE2 );
		var_rp->site = site;
		var_rp->leafp = leafp;
		if( leafp->references == (VAR_REF *)NULL )
			leafp->ref_tail = leafp->references = var_rp;
		else
		{
			leafp->ref_tail->next_vref = var_rp;
			leafp->ref_tail = var_rp;
		}

		if(site.tp->var_refs == (VAR_REF*) NULL) {
			site.tp->var_refs = var_rp;
		}
	} else if(item->operand.tag == ISTRIPLE) {
		if(record_expr_refs != IR_TRUE) return;
		if((expr = item->triple.expr) == (EXPR*)NULL)
			return;
		expr_rp = new_expr_ref(IR_FALSE);
		expr_rp->reftype = ( use1or2 == 1  ? EXPR_USE1 : EXPR_USE2 );
		expr_rp->site = site;

		if( expr->ref_tail == (EXPR_REF *)NULL )
			expr->ref_tail = expr->references = expr_rp;
		else
		{
			expr->ref_tail->next_eref = expr_rp;
			expr->ref_tail = expr_rp;
		}

	} else {
		quita("record_use: bad operand tag >%X<",item->operand.tag); 
	}
}


/*
**	go through all exprs
**	and eliminate kill references from the references list
**	then free the space used for the kill references
*/
void
free_expr_killref_space()
{
	register EXPR *ep, **epp;
	register EXPR_REF *rp0, *rp1;

	nexprkillrefs = 0;
	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		for( ep = *epp; ep; ep = ep->next_expr ) {
			rp0 = ep->references;
			while(	rp0 && rp0->reftype == EXPR_KILL ){
				rp0 = rp0->next_eref;
				nexprkillrefs++;
			}
			ep->references = ep->ref_tail = rp0;

			while(rp0) {
				ep->ref_tail = rp0;
				rp1 = rp0->next_eref;
				while(rp1 && rp1->reftype == EXPR_KILL) {
					rp1 = rp1->next_eref;
					nexprkillrefs++;
				}
				rp0->next_eref = rp1;
				rp0 = rp1;
			}
		}
	}
	(void)new_expr_killref(IR_TRUE);
}

static void
free_ref_space()
{
	register LIST **lpp, *hash_link;
	register LEAF *leafp;
	register BLOCK *bp;
	register TRIPLE *tp, *triple;
	TRIPLE *last, *right;
	LIST *head;

	for(lpp = leaf_hash_tab; lpp < &leaf_hash_tab[LEAF_HASH_SIZE]; lpp++) {
		head =  *lpp;
		if(head != LNULL) LFOR(hash_link, head) {
			leafp = (LEAF*) hash_link->datap;
			leafp->references = leafp->ref_tail =  (VAR_REF*) NULL;
		}
	}

	for(bp=entry_block; bp; bp=bp->next)	{
		last = bp->last_triple;
		if(last != TNULL) TFOR(tp,last) {
			if( ISOP(tp->op, NTUPLE_OP)) {
				right = (TRIPLE*) tp->right;
				if(right != TNULL) TFOR( triple, right ) {
					triple->reachdef1=triple->reachdef2=triple->canreach = NULL;
					triple->var_refs = (VAR_REF*) NULL;
				}
			}
			tp->reachdef1 = tp->reachdef2 = tp->canreach = NULL;
			tp->var_refs = (VAR_REF*) NULL;
		}
	}

	(void)new_expr_killref(IR_TRUE);
	(void)new_expr_ref(IR_TRUE);
	(void)new_var_ref(IR_TRUE);
	navailvardefs = navailexprdefs = nvardefs = nexprdefs = nvarrefs = nexprrefs = 0;
}

static int
count_avail_exprdefs()
{
	register EXPR *ep, **epp;
	register int blockgen, total;
	register EXPR_REF *rp;
	int this_block, blockno;

	total=0;
	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		for( ep = *epp; ep; ep = ep->next_expr ) {
			if(ep->op == IR_ERR) {
				continue;
			}
			rp = ep->references;
			while(rp) {
				if(rp->reftype == EXPR_KILL) {
					this_block = ((EXPR_KILLREF*)rp)->blockno;
				} else {
					this_block =  rp->site.bp->blockno;
				}
				blockgen=0;

				do { /* for this basic block */
					switch(rp->reftype) {
						case EXPR_GEN:
							blockgen++;
							break;

						case EXPR_KILL:
							blockgen = 0;
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
				total += blockgen;
			}
		}
	}
	return total;
}

static int
count_avail_vardefs()
{
	register LEAF *leafp;
	register VAR_REF *rp;
	register BLOCK *this_block;
	register int total = 0;
	register BOOLEAN defined;

	for(leafp=leaf_tab; leafp; leafp = leafp->next_leaf) {
		rp = leafp->references;
		while( rp ) {
			this_block = rp->site.bp;
			defined = IR_FALSE;
			do { /* for this block */
				if(rp->reftype == VAR_DEF) {
					defined = IR_TRUE;
				}
				rp = rp->next_vref;
			} while( rp && rp->site.bp == this_block );
			if(defined) { /* the last definition will be available on exit */
				total++;
			}
		}
	}
	return total;
}

#ifndef lint
static	char sccsid[] = "@(#)make_expr.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include "page.h"
#include "misc.h"
#include "recordrefs.h"
#include "make_expr.h"
#include <stdio.h>

/* global */
int nexprs=0;
EXPR *expr_hash_tab[EXPR_HASH_SIZE];

/* local */
static EXPR *lookup_hash_expr(/*index,exprp*/);
static void find_call_expr(/*exprp,tp*/);
static EXPR * lookup_hash_expr(/*index,exprp*/);
static BOOLEAN same_expr(/*ep1,ep2*/);
static void check_recompute(/*tp*/);

void
dump_exprs()
{
	register LIST *lp;
	register EXPR *ep, **exprparr, **epp;
	EXPR_REF *erp;

	exprparr = (EXPR**)heap_tmpalloc((unsigned)(nexprs+1) * sizeof(char*));
	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		for( ep = *epp; ep; ep = ep->next_expr )
		{
			exprparr[ep->exprno] = ep;
		}
	}

	exprparr[nexprs] = (EXPR*) NULL;
	printf("\nEXPRS\n");
	epp = exprparr;
	for(ep = *epp++; ep ; ep = *epp++) {
		if(ep->op == IR_ERR) {
			printf("[%d] leaf %s L[%d] ",ep->exprno, 
				(((LEAF*)ep->left)->pass1_id ? 
						((LEAF*)ep->left)->pass1_id : ""),
				((LEAF*)ep->left)->leafno);
		}else {
			printf("[%d] %s E[%d] ",
				ep->exprno, op_descr[ORD(ep->op)].name,
					ep->left->exprno);
			if(ISOP(ep->op,BIN_OP) && ep->op != IR_SCALL && ep->op != IR_FCALL ){
				printf("E[%d] ", ep->right->exprno);
			}
			if(ep->recompute) {
				printf(" (recompute) ");
			}
			if(ep->depends_on) {
				printf("\tdepends_on: ");
				LFOR(lp,ep->depends_on->next){
					printf("L[%d] ", LCAST(lp,LEAF)->leafno);
				}   
			}
			if(ep->references) {
				printf("references: ");
				for(erp = ep->references; erp; erp = erp->next_eref) {
					if (erp->reftype == EXPR_KILL) {
						printf("kill B[%d] ",
							((EXPR_KILLREF*)(erp))->blockno);
					} else {
						printf("%d  ", erp->refno);
					}
				}   
			}
		}
		printf("\n");
	}
	heap_tmpfree((char*)exprparr);
}

void
free_exprs() 
{
	register EXPR **epp;

	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		*epp = (EXPR *) NULL;
	}
	(void)new_expr(IR_TRUE);
	nexprs = 0;
}

void
free_depexpr_lists()
{
	register LIST **lpp, *hash_link;
	register EXPR *ep, **epp;
	LEAF *leafp;

	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		for( ep = *epp; ep; ep = ep->next_expr ) {
			ep->depends_on =  LNULL;

		}
	}
	for(lpp = leaf_hash_tab; lpp < &leaf_hash_tab[LEAF_HASH_SIZE]; lpp++) {
		LFOR(hash_link, *lpp) {
			leafp = (LEAF*) hash_link->datap;
			leafp->dependent_exprs = LNULL;
		}
	}
	empty_listq(dependentexpr_lq);
}

void
entable_exprs()
{
	register LIST *hash_link, **lpp;
	register BLOCK *bp;
	register TRIPLE *tp;
	LEAF *leafp;

	free_exprs();
	if(dependentexpr_lq == NULL) dependentexpr_lq = new_listq();
#ifdef DEBUG
	if(queuesize(tmp_lq) != 0)
		quit("entable_exprs: tmp_lq in use");
#endif

	for(lpp = leaf_hash_tab; lpp < &leaf_hash_tab[LEAF_HASH_SIZE]; lpp++) {
		LFOR(hash_link, *lpp) {
			leafp = (LEAF*)hash_link->datap;
			leafp->dependent_exprs = LNULL;
		}
	}
	for(bp=entry_block;bp;bp=bp->next) {
		TFOR(tp,bp->last_triple) {
				tp->expr = (EXPR*) NULL;
		}
	}
	for(bp=entry_block;bp;bp=bp->next) {
		TFOR(tp,bp->last_triple) {
			if( ISOP(tp->op,VALUE_OP)) {
				tp->expr = find_expr((IR_NODE*)tp);
			}
			check_recompute(tp);
		}
		empty_listq(tmp_lq);
	}
}

EXPR *
find_expr(np)
	IR_NODE *np;
{
	struct expr expr;
	int hash;
	register TRIPLE *tp;
	LEAF *leafp;
	EXPR *ep;
	LIST *lp, *head;

	if(np == (IR_NODE*) NULL) {
		return( (EXPR*) NULL);
	}
	expr.type = np->operand.type;
	/* screen out wierd types */
	switch(expr.type.tword) {
		case STRTY:
		case UNIONTY:
		case BITFIELD:
			return( (EXPR*) NULL);
		default:
			break;
	}
	switch(np->operand.tag) {
		case ISTRIPLE:
			tp = (TRIPLE*) np;
			if( ! ISOP(tp->op,VALUE_OP)) {
				quita("find_expr: non value triple >%x<",ORD(tp->op));
			}
			if(tp->expr)
				return (tp->expr);
			expr.op = tp->op;
			if( tp->op == IR_SCALL || tp->op == IR_FCALL ) {
				if(partial_opt == IR_TRUE)
					return (EXPR*)NULL;
				find_call_expr(&expr,tp);
			} else if(ISOP(tp->op,BIN_OP)) {
				if((expr.left = find_expr(tp->left)) == (EXPR*)NULL)
					return (EXPR*)NULL;
				if((expr.right = find_expr(tp->right)) == (EXPR*)NULL)
					return (EXPR*)NULL;
				expr.depends_on = 
					copy_and_merge_lists(expr.left->depends_on,
							expr.right->depends_on,tmp_lq);
			} else if(ISOP(tp->op,UN_OP)) {
				if(partial_opt && tp->op == IR_IFETCH)
					return (EXPR*)NULL;
				if((expr.left = find_expr(tp->left)) == (EXPR*)NULL)
					return (EXPR*)NULL;
				expr.right = (EXPR*) NULL;
				if(tp->op == IR_ADDROF) {
					expr.depends_on = LNULL;
				} else if(tp->op == IR_IFETCH) {
					/*
					** an indirection node depends on
					** the node's implicitly used variables
					*/
					expr.depends_on = copy_list(expr.left->depends_on,tmp_lq);
					LFOR(lp, tp->implicit_use) {
						(void)insert_list(&expr.depends_on,
							(LDATA*)(LCAST(lp,TRIPLE)->left), tmp_lq);
					}
				} else {
					expr.depends_on = copy_list(expr.left->depends_on,tmp_lq);
				}
			}
			hash = (ORD(tp->op) << 16 ) ^ (int) expr.left  ^ (int) expr.right; 
			hash %=  EXPR_HASH_SIZE;
			ep = lookup_hash_expr(hash,&expr);
			break;
			
		case ISLEAF:
			leafp = (LEAF*) np;
			if(partial_opt == IR_TRUE && leafp->class == VAR_LEAF &&
			   (EXT_VAR(leafp)))
				return (EXPR*)NULL;
			/* see if we already have an expression entry for this 
			** leaf (ep->op == IR_ERR). exploit the fact that this expr
			** will usually be the first one added to the dependent_exprs
			** list by lookup_hash_expr()
			*/
			if(leafp->dependent_exprs) {
				head = leafp->dependent_exprs->next;
				LFOR(lp,head) {
					ep = (EXPR*) lp->datap;
					if(ep->op == IR_ERR && ep->left == (EXPR*)leafp)
						return ep;
				}
			}
			expr.op = IR_ERR;
			expr.left = (EXPR*) leafp; /*leaf's address used as a unique expr */
			expr.right = (EXPR*) NULL;
			expr.depends_on = NEWLIST(tmp_lq);
			(LEAF*) expr.depends_on->datap =  leafp;
			hash = ((int)leafp >> 4) % EXPR_HASH_SIZE;
			ep = lookup_hash_expr(hash,&expr);
			break;

		default:
			quita("find_expr: bad operand tag >%x<",ORD(np->operand.tag));

		}
		return(ep);
}

static void
find_call_expr(exprp,tp)
	register EXPR * exprp;
	register TRIPLE *tp;
{
	static call_count = 0;

	/* 
	**	for now assume calls never yield the same expr
	**	should be something like if fun type not void and
	**  same fun and same arglist and args not defined and
	**  fun builtin and no side effects THEN same expr FIXME
	*/
	exprp->left = find_expr(tp->left);
	exprp->right = (EXPR*) ++call_count;
	exprp->depends_on = copy_list(exprp->left->depends_on,tmp_lq);
}

void
clean_save_temp()
{
	register EXPR *ep, **epp;

	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		for( ep = *epp; ep; ep = ep->next_expr )
			ep->save_temp = (LEAF *) NULL;
	}
}
	
static EXPR *
lookup_hash_expr(index,exprp)
	int index;
	EXPR *exprp;
{
	register LIST *lp1, *lp2;
	register EXPR *ep;
		
	for( ep = expr_hash_tab[index]; ep; ep = ep->next_expr )
	{
		if(same_expr(exprp, ep))
		{
			return((EXPR*) ep);
		}
	}
	ep = (EXPR*) new_expr(IR_FALSE);
	*ep = *exprp;
	ep->depends_on = copy_list(exprp->depends_on, dependentexpr_lq);
	ep->recompute = IR_FALSE;
	ep->exprno = nexprs++;
	ep->references = ep->ref_tail = (EXPR_REF*) NULL;
	ep->save_temp = (LEAF *) NULL;
	ep->next_expr = expr_hash_tab[index];
	expr_hash_tab[index] = ep;
	/*
	**	now for all leaves on which this expr depends do:
	**  	add the expr to the leaf's dependent exprs list
	*/
	LFOR(lp1,exprp->depends_on) {
		lp2 = NEWLIST(dependentexpr_lq);
		(EXPR*) lp2->datap = ep;
		LAPPEND(LCAST(lp1,LEAF)->dependent_exprs,lp2);
	}
	return( ep );
}

static BOOLEAN
same_expr(ep1,ep2)
	EXPR *ep1, *ep2;
{

	if(ep1->op != ep2->op) return(IR_FALSE);
	if( ep1->op == IR_ERR || ISOP(ep1->op,UN_OP) ) { 
		if(ep1->left == ep2->left) {
			return(same_irtype(ep1->type,ep2->type) );
		} else {
			return(IR_FALSE);
		}
	} 
	if(ISOP(ep1->op,BIN_OP)) {
		if(ep1->left == ep2->left && ep1->right == ep2->right) {
			return((same_irtype(ep1->type,ep2->type)));
		}
		if(ISOP(ep1->op,COMMUTE_OP)) {
			if(ep1->left == ep2->right &&ep1->right == ep2->left) {
				return((same_irtype(ep1->type,ep2->type)));
			}
		}
	}
	return(IR_FALSE);
}

/*
 * Look for expressions that are probably more trouble to save
 * than to recompute, given the likelihood of running out of
 * registers.
 */

static void
check_recompute(tp)
	TRIPLE *tp;
{
	IR_NODE *np;
	EXPR *ep;
	TWORD tword;

	switch(tp->op) {
	case IR_IFETCH:
	case IR_ISTORE:
		/*
		 * look for indirect memory access through
		 * a base+offset expression, where offset is
		 * in the range supported by the target machine
		 */
		np = tp->left;
		if (np->operand.tag == ISTRIPLE
		    && (np->triple.op == IR_PLUS || np->triple.op == IR_MINUS)){
			ep = np->triple.expr;
			np = np->triple.right;
			if (np->operand.tag == ISLEAF) {
				if (IS_SMALL_OFFSET(np) && ep != NULL) {
					ep->recompute = IR_TRUE;
				}
			}
		}
		break;
	case IR_PCONV:
	case IR_CONV:
		/*
		 * pointer-pointer conversions.  These are often
		 * introduced for maintaining type consistency among
		 * indirect references to structures and unions.
		 */
		np = tp->left;
		tword = np->operand.type.tword;
		if (ISPTR(tp->type.tword)
		    && (ISPTR(tword) || ISINT(tword))
		    && tp->expr != NULL) {
			/*
			 * ptr-ptr conversion --
			 * recompute if it's a local variable
			 * or a small constant
			 */
			if (np->operand.tag == ISLEAF
			    && np->leaf.class == VAR_LEAF
			    && !EXT_VAR((LEAF*)np)
			    || IS_SMALL_OFFSET(np)) {
				tp->expr->recompute = IR_TRUE;
			}
			break;
		}
		/*
		 * conversions of small integral constants
		 */
		if ((ISINT(tp->type.tword) || ISCHAR(tp->type.tword))
		    && ISCONST(np)
		    && (IS_SMALL_INT(np) || ISCHAR(np->leaf.type.tword))
		    && tp->expr != NULL) {
			tp->expr->recompute = IR_TRUE;
		}
		break;
	case IR_EQ:
	case IR_NE:
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
		/*
		 * relational expressions are more expensive to
		 * save than to recompute, at least on the machines
		 * we have to deal with.
		 */
		if (tp->expr != NULL) {
			tp->expr->recompute = IR_TRUE;
		}
		break;
	}
}

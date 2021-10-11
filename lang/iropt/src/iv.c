#ifndef lint
static	char sccsid[] = "@(#)iv.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Induction variable transformation - closely follows " An Algorithm for
 * Reduction of Operator Strength" Cocke & Kennedy - CACM  Nov 77, Vol 20 No 11
 */

#include "iropt.h"
#include "page.h"
#include "misc.h"
#include "loop.h"
#include "make_expr.h"
#include "recordrefs.h"
#include "iv.h"
#include <stdio.h>

/* global */
int	n_iv=0;
int	n_rc=0;
LIST	*iv_def_list=NULL;

/* locals */

static LIST	*cands;
static IV_INFO	*iv_info_list;
static IV_INFO	*free_iv_info_list;
static LEAF	**ivrc_tab;
static char	*afct_tab;
static IV_HASHTAB_REC	*iv_hashtab[IV_HASHTAB_SIZE];
static LIST	*new_iv_def_list;

static void	compute_afct();
static void	unset_iv(/*ivp*/);
static void	update_base_on(/*ivp, loop*/);
static void	check_reducible(/*ivp, loop*/);
static void	cleanup_iv_list(/*loop*/);
static void	cleanup_iv_def_list();
static void	find_cands(/*loop*/);
static void	tsort(/*tp*/);
static void	sort_iv_def_list();
static void	dotest_repl(/*loop*/);
static void	print_iv(/*ivp*/);
static void	dump_ivs();
static void	doiv_r23();
static void	doiv_r45(/*loop*/);
static IV_HASHTAB_REC	*hash(/*x,op,c*/);
static void	doiv_r67(/*loop*/);
static TRIPLE	*replace_operand(/*old_operand,with*/);
static void	doiv_r89(/*loop*/);
static void	update_plus_temp(/*hr_mult,hr_plus,cand_sibling*/);
static IV_HASHTAB_REC	*new_iv_hashtab_rec(/*x,op,c,loop*/);
static TRIPLE	*add_triple(/*after,op,left,right,type*/);
static IV_INFO	*new_iv_info();
static void	free_iv_info(/*ivp*/);

#define T(x,c) hash((LEAF*)(x),IR_MULT,(c))->t
#define TBAD ((TRIPLE*)-1)

static void
compute_afct()
{
	LEAF **ivp0, *targ;
	int indx, row_bytesize, row_wordsize;
	IV_INFO *ivp;
	TRIPLE *def_tp, *add_op;
	char *afct_p, *afct_i, *afct_j;
	int *tmp;
	BOOLEAN change;
	int n, i, j;
	LIST *lp;

	ivp0 = ivrc_tab = (LEAF**) ckalloc((unsigned)n_iv + n_rc,sizeof(LEAF*));
	indx = 0;
	for(ivp=iv_info_list; ivp; ivp = ivp->next ) {
		if(ivp->is_iv == IR_TRUE) {
			ivp->indx = indx++;
			*ivp0 = ivp->leafp;
			ivp0++;
		}
	}
	for(ivp=iv_info_list; ivp; ivp = ivp->next ) {
		if(ivp->is_rc == IR_TRUE) {
			ivp->indx = indx++;
			*ivp0 = ivp->leafp;
			ivp0++;
		}
	}

	row_bytesize = roundup((unsigned)(n_iv + n_rc) , sizeof(int)/sizeof(char));
	row_wordsize = row_bytesize/(sizeof(int)/sizeof(char));
	afct_tab = ckalloc((unsigned)n_iv, (unsigned)row_bytesize);
	
	LFOR(lp, iv_def_list) {
		def_tp = (TRIPLE*) lp->datap;	
		targ = (LEAF*) def_tp->left;
		ivp = IV(targ);
		if(!ivp->afct) {
			ivp->afct = afct_tab + (ivp->indx*row_bytesize);
		}
		afct_p = IV(targ)->afct;
		afct_p[IV(targ)->indx] = YES; /* step A1 */
		if(def_tp->right->operand.tag == ISTRIPLE) { /* A2 and A3 */
			add_op = (TRIPLE*) def_tp->right;
			afct_p[IV(add_op->left)->indx] = YES; 
			if(ISOP(add_op->op,BIN_OP)) {
				afct_p[IV(add_op->right)->indx] = YES; 
			}
		} else {
			afct_p[IV(def_tp->right)->indx] = YES; 
		}
	}

	change = IR_TRUE;
	tmp = (int*)heap_tmpalloc((unsigned)row_wordsize*sizeof(int));
	while(change == IR_TRUE) {
		change = IR_FALSE;
		for(i=0, afct_i = afct_tab; i < n_iv; afct_i += row_bytesize, i++) {
			for(n = 0; n< row_wordsize; n++ ) {
				tmp[n] = ((int*)afct_i)[n];
			}
			for(j=0, afct_j=afct_tab;j < n_iv;afct_j += row_bytesize, j++){
				if(afct_i[j] == YES) {
					for(n = 0; n<row_wordsize; n++) {
						tmp[n] |= ((int*)afct_j)[n];
					}
				}
			}
			for(n = 0; n< row_wordsize; n++ ) {
				if(tmp[n] != ((int*)afct_i)[n]) {
					change=IR_TRUE;
					break;
				}
			}
			if(change == IR_TRUE) {
				for(n = 0; n< row_wordsize; n++ ) {
					((int*)afct_i)[n] |= tmp[n];
				}
			}
		}
	}
	heap_tmpfree((char*)tmp);
}

/* check that a definition of a possible iv is one of the legitimate forms */

BOOLEAN
check_definition(def)
	register TRIPLE *def;
{
	register TRIPLE *add_op;

	if(def->op == IR_ASSIGN) {
		if(def->right->operand.tag == ISLEAF) {
			return IR_TRUE;
		} else if(def->right->operand.tag == ISTRIPLE) {
			add_op = (TRIPLE*) def->right;
			switch(add_op->op) {
				case IR_PLUS:
				case IR_MINUS:
					if(	add_op->left->operand.tag == ISLEAF && 
						add_op->right->operand.tag == ISLEAF) {
							return IR_TRUE;
					}
					break;

				case IR_NEG:
					if(	add_op->left->operand.tag == ISLEAF ) {
						return IR_TRUE;
					}
					break;
				}
		}
	}
	return IR_FALSE;
}

void
find_iv(loop)
	LOOP *loop;
{
	LIST *lp;
	LEAF *leafp;
	BOOLEAN change;
	TRIPLE *tp;
	IV_INFO *ivp;
	BLOCK *bp;
	VAR_REF *rp;

	/*
	** if triple is a sto, neg, add or sub add targ to IV
	** the rc set consists of all vars with no defs in the region and all
	** consts that appear in a "possible iv" defining instruction 
	** this is done to keep from lugging all const leaves around
	*/
	LFOR(lp, loop->blocks) {
		bp = (BLOCK*) lp->datap;
		TFOR(tp, bp->last_triple) {
			switch(tp->op) {
				case IR_ASSIGN:
					if(tp->right->operand.tag == ISLEAF && ISCONST(tp->right)) 
						set_rc((LEAF*)tp->right);
					if(partial_opt == IR_TRUE &&
					   tp->left->leaf.class == VAR_LEAF &&
					   EXT_VAR((LEAF*)tp->left))
						break;
					set_iv(tp, IR_TRUE, loop);
					break;
				case IR_PLUS:
				case IR_MINUS:
					if(	tp->left->operand.tag == ISLEAF && 
						tp->right->operand.tag == ISLEAF) {
						if(ISCONST(tp->left)) 
							set_rc((LEAF*)tp->left);
						if(ISCONST(tp->right))
							set_rc((LEAF*)tp->right);
						if(partial_opt == IR_TRUE &&
						   (tp->left->leaf.class == VAR_LEAF &&
						    EXT_VAR((LEAF*)tp->left) ||
						    tp->right->leaf.class == VAR_LEAF &&
						    EXT_VAR((LEAF*)tp->right)))
						    break;
						set_iv(tp, IR_TRUE, loop);
					}
					break;
				case IR_NEG:
					if(	tp->left->operand.tag == ISLEAF ) {
						if(ISCONST(tp->left)) 
							set_rc((LEAF*)tp->left);
						if(partial_opt == IR_TRUE &&
						   tp->left->leaf.class == VAR_LEAF &&
						   EXT_VAR((LEAF*)tp->left))
                            break;
						set_iv(tp, IR_TRUE, loop);
					}
					break;
			}
		}
	}

	/*
	**	ensure that all definitions in the block are of the allowed
	**	form
	*/
	LFOR(lp, iv_def_list) {
		tp = (TRIPLE*) lp->datap;
		if(tp->visited == IR_TRUE && tp->op == IR_ASSIGN && (ivp = IV(tp->left))){
			leafp = (LEAF*) tp->left;
			rp = leafp->references;
			while(rp) {
				if( (rp->reftype == VAR_DEF ||rp->reftype == VAR_AVAIL_DEF) &&
					(INLOOP(rp->site.bp->blockno, loop))
				) {
					if(check_definition(rp->site.tp) == IR_FALSE) {
						unset_iv(ivp);
						break;
					}
				}
				rp = rp->next_vref;
			}
		}
	}


	/*
	**	if a1 not in IV U RC or a2 not in IV U RC remove targ from IV
	*/
	change = IR_TRUE;
	while(change) {
		change = IR_FALSE;
		LFOR(lp, iv_def_list) {
			tp = (TRIPLE*) lp->datap;
			if(tp->visited == IR_FALSE) continue;
			switch (tp->op) {
				case IR_ASSIGN:
					if(!(ivp = IV(tp->left))) continue;
					if(tp->right->operand.tag == ISLEAF) {
						if((!IV(tp->right) || IV(tp->right)->is_rc )) {
						/* assign by a RC is a RC NOT an IV */
						/* a1 not in IV */
							change = IR_TRUE;
							unset_iv(ivp);
						}
					} else {
						if(((TRIPLE*)tp->right)->visited == IR_FALSE) {
							change = IR_TRUE;
							unset_iv(ivp);
						}
					}
					break;
				case IR_PLUS:
				case IR_MINUS:
					if(!IV(tp->left)) {
						change = IR_TRUE;
						tp->visited = IR_FALSE;
						break;
					}
					if(!IV(tp->right)) {
						change = IR_TRUE;
						tp->visited = IR_FALSE;
						break;
					}
					if(IV(tp->left)->is_iv == IR_TRUE &&
					   IV(tp->right)->is_iv == IR_TRUE){
						change = IR_TRUE;
						tp->visited = IR_FALSE;
					}
					break;
				case IR_NEG:
					if(!IV(tp->left)) {
						change = IR_TRUE;
						tp->visited = IR_FALSE;
					}
					break;
			}
		}
	}
	cleanup_iv_list(loop);
	cleanup_iv_def_list();
}

static void
unset_iv(ivp)
	register IV_INFO *ivp;
{
	if(ivp->is_iv) {
		IV(ivp->leafp) = NULL; 
		n_iv --; 
		ivp->is_iv = IR_FALSE;
	}
}

void
set_iv(def_tp, check_base_iv, loop)
	TRIPLE *def_tp;
	BOOLEAN check_base_iv;
	LOOP *loop;
{
	register LEAF *targ;
	register IV_INFO *ivp;
	register LIST *lp;

	/* only integer variables are considered to be iv */
	/* assignment doesn't have value!! */
	if(def_tp->op != IR_ASSIGN && !ISINT(def_tp->type.tword) &&
	   !ISPTR(def_tp->type.tword))
		return;

	/* mark visited -- a definition or a possible definition of an iv */
	def_tp->visited = IR_TRUE;
	lp = NEWLIST(iv_lq);
	(TRIPLE*) lp->datap = def_tp;
	LAPPEND(iv_def_list,lp);

	if(def_tp->op != IR_ASSIGN) 
		return;

	targ = (LEAF*) def_tp->left;

	/* iv = rc - iv is not an iv at all */
	if(((TRIPLE *)(def_tp->right))->op == IR_MINUS &&
	   ((TRIPLE *)(def_tp->right))->right == (IR_NODE *)targ){
		def_tp->visited = IR_FALSE;
		if(ivp = IV(targ)) 
			unset_iv(ivp);
		return;
	}
	/* iv = -iv is not iv either */
	if(def_tp->right->triple.op == IR_NEG && 
	   def_tp->right->triple.left == (IR_NODE*)targ){
		def_tp->visited = IR_FALSE;
		if(ivp = IV(targ)) 
			unset_iv(ivp);
		return;
	}

	if( !(ivp = IV(targ)) ) {
		ivp = new_iv_info();
		IV(targ) = ivp;
		ivp->leafp = targ;
		ivp->def_triple = def_tp;
		n_iv++;
		ivp->base_on = def_tp; /* just remember it for further use */
		/* check it is a base iv -- base on itself && there is only one def */
		if (check_base_iv == IR_TRUE) {
			BOOLEAN not_set = IR_TRUE;
			VAR_REF *rp;

			if(!(((TRIPLE *)(def_tp->right))->right == (IR_NODE *)targ) ^
				(((TRIPLE *)(def_tp->right))->left == (IR_NODE *)targ)) {
				not_set = IR_FALSE;	/* not depend on itself */
				ivp->is_iv = IR_TRUE;
				return;				/* no need to check single definition */
			}
			rp = targ->references;
			while(not_set && rp) {
				if(INLOOP(rp->site.bp->blockno, loop)) {
					switch(rp->reftype) {
						case VAR_DEF:
						case VAR_AVAIL_DEF:
							if(rp->site.tp != def_tp ) {
								not_set = IR_FALSE;
							}
							break;
					}
				}
				rp = rp->next_vref;
			}
			if (not_set == IR_TRUE) {
				ivp->base_on = TNULL;  /* it is a base iv */
			} else {
				ivp->base_on = TBAD; /* not a base iv */
			}
		}
	}
	ivp->is_iv =  IR_TRUE;
}

void
set_rc(leafp)
	LEAF *leafp;
{
	register IV_INFO *ivp;

	if( !(ivp = IV(leafp)) ) {
		ivp = new_iv_info();
		IV(leafp) = ivp;
		ivp->leafp = leafp;
		n_rc++;
	}
	ivp->is_rc =  IR_TRUE;
}

/* find what is the iv based on */
static void
update_base_on(ivp, loop)
	IV_INFO *ivp;
	LOOP *loop;
{
	register TRIPLE *use_tp;
	register VAR_REF *rp;
	register IR_NODE *np;
	register LIST *reachdef;

	if (ivp->base_on != TNULL && ivp->base_on != TBAD) {
		/* check if this iv only defined once in the loop &&
			no outside reachdefs */
		rp = ivp->leafp->references;
		while(rp) {
			if(INLOOP(rp->site.bp->blockno, loop)) {
				switch (rp->reftype) {
					case VAR_DEF:
					case VAR_AVAIL_DEF:
						if(rp->site.tp != ivp->base_on){
							ivp->base_on = TBAD;
							return;
						}
						break;
					case VAR_USE1:
					case VAR_EXP_USE1:
					case VAR_USE2:
					case VAR_EXP_USE2:
						use_tp = rp->site.tp;
						if (rp->reftype == VAR_USE1 || 
						    rp->reftype == VAR_EXP_USE1) {
							reachdef = use_tp->reachdef1;
						} else {
							reachdef = use_tp->reachdef2;
						}
						if (reachdef && reachdef->next == reachdef &&
							LCAST(reachdef, VAR_REF)->site.tp ==
							 ivp->base_on) {
						} else {
							ivp->base_on = TBAD;
							return;
						}
						break;
				}
			}
			rp = rp->next_vref;
		}
		/* set base_on to be the definition triple of the iv which
		   ivp based on */
		np = ivp->base_on->right;
		if (np->operand.tag == ISLEAF) {
			if(!(IV(np)->is_iv)) quit("update_base_on a non-iv");
			if(IV(np)->base_on != TBAD) {
				ivp->base_on = IV(np)->def_triple;
			} else {
				ivp->base_on = TBAD;
			}
		} else {
			if(IV(((TRIPLE *)np)->left)->is_iv == IR_TRUE) {
				ivp->base_on = IV(((TRIPLE *)np)->left)->def_triple;
			} else {
				ivp->base_on = IV(((TRIPLE *)np)->right)->def_triple;
			}
		}
	}
}


/* reset iv's base_on if the updating of base iv is between the
   definition of iv and the use of iv, if no, then the iv is 
   reducible */
   
static void
check_reducible(ivp, loop)
	IV_INFO *ivp;
	LOOP *loop;
{
	VAR_REF *iv_site, *base_site, *reach_site;
	register short iv_bp, base_bp, reach_bp, iv_tp, base_tp, reach_tp;
	LIST *lp;
	TRIPLE *tp;

	tp = ivp->def_triple;
	iv_site = tp->var_refs;
	while(iv_site == NULL){
		tp = tp->tprev;
		iv_site = tp->tprev->var_refs;
	}
	iv_bp = iv_site->site.bp->blockno;
	iv_tp = iv_site->site.tp->tripleno;
	tp = ivp->base_on;
	base_site = tp->var_refs;
	while(base_site == NULL){
		tp = tp->tprev;
		base_site = tp->tprev->var_refs;
	}
	base_bp = base_site->site.bp->blockno;
	base_tp = base_site->site.tp->tripleno;
	LFOR(lp, ivp->def_triple->canreach){
		reach_site = (VAR_REF *)lp->datap;
		if(INLOOP(reach_site->site.bp->blockno, loop)){
			reach_bp = reach_site->site.bp->blockno;
			reach_tp = reach_site->site.tp->tripleno;
			if(iv_bp == base_bp && base_bp == reach_bp){
			/* all in the same basic block */
				if(iv_tp < base_tp && base_tp < reach_tp){
					ivp->base_on = TBAD;
					break;
				}
			} else if(iv_bp == base_bp){
			/* the use is not in the same bb */
				if(iv_tp < base_tp){
					ivp->base_on = TBAD;
					break;
				} 
			} else if(iv_bp == reach_bp){
			/* the base iv def is not in the same bb */
				continue;
			} else if(base_bp == reach_bp){
			/* the iv def is not in the same bb */
				ivp->base_on = TBAD;
				break;
			} else {
			/* all in diff bb's and I save the effort to prove the leagality */
				ivp->base_on = TBAD;
				break;
			}
		}
	}
}

/* the iv_info list now contains leaves which were found not to */
/* be ivs - unlist them */
static void
cleanup_iv_list(loop)
	LOOP *loop;
{ 
	register IV_INFO *ivp, *next_ivp;
	register BOOLEAN change;

	for (ivp = iv_info_list; ivp; ivp = next_ivp) {
		next_ivp = ivp->next;
		if (ivp->is_rc == IR_FALSE && ivp->is_iv == IR_FALSE ) {
			free_iv_info(ivp);
		}
	}
	if (iv_info_list) {
		for(ivp = iv_info_list; ivp; ivp = ivp->next ) {
			if(ivp->is_iv) {
				update_base_on(ivp, loop);
			}
		}
		/* since the update_base_on is called according to the sequence
		   of iv_info_list, the base_on information may not be correct,
		   e.g. the dependency graph is K->J->I, I is a base iv, but J
		   has two definitions in the loop, then when set the base_on
		   infomation of K, it may not have figured out that J is not
		   a member of family of base iv */
		/* in the mean time, we try to find out cyclic dependency
		   to rule out the members like I<->J from family of base iv */
		change = IR_TRUE;
		while(change == IR_TRUE) {
			change = IR_FALSE;
			for (ivp = iv_info_list; ivp; ivp = ivp->next) {
				if(ivp->base_on != TNULL && ivp->base_on != TBAD){
					IV_INFO *ip;
					TRIPLE *itself;

					itself = ivp->base_on;
					ip = IV(ivp->base_on->left);
					while(ip && ip->base_on != TNULL && ip->base_on != TBAD) {
						if(ip->base_on == ivp->def_triple || 
						   ip->base_on == itself) {
							ip->base_on = TBAD;
							break;
						}
						ip = IV(ip->base_on->left);
					}
					if(ip == NULL) quit("update_base_on a non-iv");
					if(ip->base_on == TBAD) {
						ivp->base_on = TBAD;
						change = IR_TRUE;
						break;
					}
				}
			}
		}
		for (ivp = iv_info_list; ivp; ivp = ivp->next) {
			if(ivp->base_on != TNULL && ivp->base_on != TBAD){
				check_reducible(ivp, loop);
			}
		}
		for (ivp = iv_info_list; ivp; ivp = ivp->next) {
			TRIPLE *tp = ivp->base_on;
			if(tp != TNULL && tp != TBAD){
				while(IV(tp->left)->base_on != TNULL) {
					if(IV(tp->left)->base_on == TBAD){
						ivp->base_on = TBAD;
						break;
					}
					tp = IV(tp->left)->base_on;
				}
			}
		}
	}
}

static void
cleanup_iv_def_list()
{ 
	register TRIPLE *tp;
	register LIST *lp, *tmp, *new_list;
	register LEAF * targ;

	new_list = LNULL;
	LFOR(lp, iv_def_list) {
		if ((tp = (TRIPLE*)lp->datap)->visited == IR_FALSE || 
			tp->op != IR_ASSIGN) continue;
		targ = (LEAF*) tp->left;
		if( (!IV(targ)) || IV(targ)->is_rc == IR_TRUE) {
			continue;
		} else {
			tmp = NEWLIST(iv_lq);
			(TRIPLE*) tmp->datap = (TRIPLE*) lp->datap;
			LAPPEND(new_list,tmp);
		}
	}
	iv_def_list = new_list;
}

static void
find_cands(loop)
	LOOP *loop;
{
	register LIST *lp, *new_cand;
	register TRIPLE *tp, *new, *def;
	register BLOCK *bp;
	register int left, right;
	LEAF *tmp;
	int p;

	cands = LNULL;
	LFOR(lp, loop->blocks) {
		bp = (BLOCK*) lp->datap;
		TFOR(tp, bp->last_triple) {
			new = 0;
			if(tp->op == IR_LSHIFT && ISINT(tp->type.tword) &&
			   ISCONST(tp->right)){
				tp->op = IR_MULT;
				p = ((LEAF*)(tp->right))->val.const.c.i;
				tp->right = (IR_NODE*)ileaf_t(1 << p,
					((LEAF*)tp->right)->type);
			}
			if(tp->op == IR_MULT && ISINT(tp->type.tword) ) {
				if(	tp->left->operand.tag == ISLEAF ) {
					if(IV(tp->left)) {
						left = (IV(tp->left)->is_iv == IR_TRUE ? 1 : 0 );
					} else if(ISCONST(tp->left)) {
						left = 0;
					} else {
						continue;
					}
				} else {
					if(((TRIPLE*)tp->left)->visited == IR_TRUE){
						if(new) continue;
						left = 1;
						new = (TRIPLE*)tp->left;
					}else
						continue;
				}

				if(	tp->right->operand.tag == ISLEAF ) {
					if(IV(tp->right)) {
						right = (IV(tp->right)->is_iv == IR_TRUE ? 1 : 0 );
					} else if(ISCONST(tp->right)) {
						right = 0;
					} else {
						continue;
					}
				} else {
					if(((TRIPLE*)tp->right)->visited == IR_TRUE){
						if(new) continue;
						right = 1;
						new = (TRIPLE*)tp->right;
					}else
						continue;
				}

				if( left ^ right ) { /* left or right must be iv but not both */
					if(new) {
						IV_INFO *ip;
						BOOLEAN need_temp = IR_TRUE;
						/* we don't need new temp if it is a member in base
							iv family */
						if(IV(new->left)->is_iv == IR_TRUE
						    && IV(new->left)->base_on != TBAD) {
							need_temp = IR_FALSE;
						} else if (new->right != NULL
						    && IV(new->right)->is_iv == IR_TRUE
						    && IV(new->right)->base_on != TBAD) {
							need_temp = IR_FALSE;
						}
						/* a new temp is generated to be an iv */
						if(need_temp == IR_FALSE &&
							new->expr && new->expr->save_temp) {
							tmp = new->expr->save_temp;
						} else {
							tmp = new_temp(new->type);
							if(new->expr) {
								new->expr->save_temp = tmp;
							}
							def = add_triple(new, IR_ASSIGN,
								(IR_NODE*)tmp, (IR_NODE*)new, new->type);
							set_iv(def, IR_FALSE, loop);
							/* set base_on for the inserted iv */
							(IV(def->left))->base_on =
									((new->right) && IV(new->right)->is_iv) ?
									IV(new->right)->def_triple : 
									IV(new->left)->def_triple;
							ip = IV(IV(def->left)->base_on->left);
							while(ip && ip->base_on != TNULL && 
										ip->base_on != TBAD) {
								ip = IV(ip->base_on->left);
							}
							if(ip->base_on == TBAD) {
								(IV(def->left))->base_on = TBAD;
							}
						}
						if((TRIPLE *)tp->left == new){
							(LEAF *)tp->left = tmp;
						} else {
							(LEAF *)tp->right = tmp;
						}
					}
					new_cand = NEWLIST(iv_lq);
					(TRIPLE*) new_cand->datap = tp;
					LAPPEND(cands,new_cand);
					if(ISCONST(tp->left))
						set_rc((LEAF*)tp->left);
					if(ISCONST(tp->right))
						set_rc((LEAF*)tp->right);
				}
			}
		}
	}
}

/*
 * disgusting routine to initialize the "visited" field
 * of every record in the LEAF table, which is used to
 * stash pointers of various types.  Here it is used to
 * hold pointers to descriptors to iv's and rc's.
 *
 * This should always be called before calling scan_loop_tree()
 * at the outer level.
 */
void
iv_init_visited()
{
	LEAF *lp;
	for (lp = leaf_tab; lp != NULL; lp = lp->next_leaf) {
		IV(lp) = NULL;
	}
}

void
iv_init()
{
	IV_INFO *ivp, *next_ivp;

	for(ivp=iv_info_list; ivp; ivp = next_ivp ) {
		next_ivp = ivp->next;
		free_iv_info(ivp);
	}
	iv_def_list = LNULL;
	iv_info_list = (IV_INFO*) NULL;
	n_iv = n_rc = 0;
	if(iv_lq){
		empty_listq(iv_lq);
	}
	(void)bzero((char*)iv_hashtab, sizeof(iv_hashtab));
}

/*
 * called at heap_reset time
 * to flush the iv_info and free_iv_info
 * lists.
 */
void
iv_cleanup()
{
	iv_init();
	free_iv_info_list = NULL;
}

static void
tsort(tp)
	TRIPLE *tp;
{
	LIST *tmp;
	IV_INFO *info;

	info = IV(tp->left);
	if (info->def_triple == TNULL)
		return;
	if (!(info->base_on == TBAD || info->base_on == TNULL ||
	    IV(((TRIPLE*)(info->base_on))->left)->def_triple == TNULL)){
		tsort(info->base_on);
	}
	tmp = NEWLIST(iv_lq);
	(LIST*)tmp->datap = (LIST*)tp;
	if (iv_def_list == LNULL)
		iv_def_list = tmp;
	LAPPEND(new_iv_def_list, tmp);
	info->def_triple = TNULL;
}

static void
sort_iv_def_list()
{
	LIST *tmp, *lp, *lp1, *old_list;

	if (iv_def_list == LNULL)
		return;
	old_list = iv_def_list;
	iv_def_list = LNULL;
	new_iv_def_list = LNULL;

	LFOR (lp, old_list){
		tsort((TRIPLE*)lp->datap);
	}
	LFOR (lp, old_list){
		LFOR (lp1, iv_def_list){
			if (lp1->datap == lp->datap)
				break;
		}
		if (lp1 == LNULL){
			tmp = NEWLIST(iv_lq);
			(LIST*)tmp->datap = (LIST*)lp->datap;
			LAPPEND(new_iv_def_list, tmp);
		}
	}
}

void
do_induction_vars(loop)
	LOOP *loop;
{
	find_iv(loop);
	find_cands(loop);
	sort_iv_def_list();
	compute_afct();
	doiv_r23();
	if(SHOWCOOKED == IR_TRUE) {
		dump_ivs();
	}
	doiv_r45(loop);
	doiv_r67(loop);
	doiv_r89(loop);
	dotest_repl(loop);
}

static void
dotest_repl(loop)
	LOOP *loop;
{
	register LIST *lp;
	LEAF *c, *x, *k, **xp, **kp, *base;
	TRIPLE *tp, *def_tp;
	register BLOCK *bp;
	IV_HASHTAB_REC *htrp;

	if (loop->blocks == LNULL || loop->blocks->next != loop->blocks) {
		/* avoid loops with > 1 basic block */
		return;
	}
	LFOR(lp, loop->blocks) {
		bp = (BLOCK*) lp->datap;
		TFOR(tp, bp->last_triple) {
			switch(tp->op) {
				case IR_EQ:
				case IR_NE:
				case IR_LE:
				case IR_LT:
				case IR_GE:
				case IR_GT:
					if( tp->left->operand.tag == ISLEAF &&
						tp->right->operand.tag  == ISLEAF &&
						ISINT(tp->left->leaf.type.tword) &&
						ISINT(tp->right->leaf.type.tword) 
					) {
						xp = (LEAF**) &(tp->left);
						kp = (LEAF**) &(tp->right);
						if(IV(*xp) && IV(*xp)->is_iv == IR_TRUE) {
							x= *xp; k = *kp;
						} else if(IV(*kp) && IV(*kp)->is_iv == IR_TRUE) {
							xp = (LEAF**) &(tp->right);
							kp = (LEAF**) &(tp->left);
							x= *xp; k = *kp;
						} else {
							continue;
						}
						if(	((IV(k) && IV(k)->is_rc== IR_TRUE)) || (ISCONST(k)) ) {
							if(IV(x)->clist == LNULL) {
								continue;
							}
							if( IV(x)->plus_temps != LNULL) { /*FIXED*/
								htrp=(IV_HASHTAB_REC*)IV(x)->plus_temps->datap;
								*xp = htrp->t; 
								base = htrp->c;
								c = (LEAF*)IV(x)->plus_temps->next->datap;

							/* the temp for c*k will be modified to c*k +base.
							** To ensure it's not used anywhere else, a new temp
							** is created 
							*/
								htrp = new_iv_hashtab_rec(c,IR_MULT,k,loop);
								def_tp = htrp->def_tp;
								if(def_tp != TNULL) {
									def_tp->right = (IR_NODE*)
										add_triple(
											(TRIPLE*)def_tp->right,
											IR_PLUS, 
											(IR_NODE*)def_tp->right,
											(IR_NODE*)base, c->type);
									*kp = (LEAF*)def_tp->left;
								} else { /* c*k is folded */
									TYPE type;
									TRIPLE *after;
									LEAF *t;

									type.tword = UNDEF;
									type.size = 0;
									type.align = 0;
									after = loop->preheader->last_triple->tprev;
									after = add_triple(after, IR_PLUS,
										(IR_NODE*)htrp->t, (IR_NODE*)base,
										c->type);
									if(after->expr && after->expr->save_temp) {
										t = after->expr->save_temp;
									} else {
										t = new_temp(after->type);
										if(after->expr) {
											tp->expr->save_temp = t;
										}
									}
									after = add_triple(after,IR_ASSIGN,
										(IR_NODE*)t, (IR_NODE*)after, type);
									htrp->def_tp = after;
									*kp = t;
								}
							} else {
								c = (LEAF*) IV(x)->clist->datap;
								*xp = (LEAF*)T(x,c);
								if(	(htrp = hash(c,IR_MULT,k)) == 
											(IV_HASHTAB_REC*) NULL &&
									(htrp = hash(k,IR_MULT,c)) == 
											(IV_HASHTAB_REC*) NULL
								) {
									htrp = new_iv_hashtab_rec(c,IR_MULT,k,loop);
								}
								*kp = htrp->t;
							}
							/* if const is a negative number, flip the comparison */
							if (c->class == CONST_LEAF && c->val.const.c.i < 0){
								switch(tp->op) {
									case IR_LE:
										tp->op = IR_GE; break;
									case IR_LT:
										tp->op = IR_GT; break;
									case IR_GE:
										tp->op = IR_LE; break;
									case IR_GT:
										tp->op = IR_LT; break;
								}
							}
						}
					}
					break;
			}
		}
	}
}
     
static void
print_iv(ivp)
	IV_INFO *ivp;
{
	char *cp;
	int i;
	LIST *lp, *clist;

	printf("%d L[%d] %s %s next %d", ivp->indx, ivp->leafp->leafno,
		(ivp->is_rc ? "is_rc" : "" ) , (ivp->is_iv ? "is_iv" : "" ) , 
		(ivp->next ? ivp->next->indx : -1 )
	);
	if(ivp->is_iv == IR_TRUE){
		if(ivp->base_on == TNULL)
			printf(" base_on nil ");
		else if(ivp->base_on == TBAD)
			printf(" base_on bad ");
		else
			printf(" base_on T[%d] ", ivp->base_on->tripleno);
	}

	if(ivp->afct) {
		printf("afct ");
		for(i=0,cp=ivp->afct; i< (n_iv+n_rc); i++,cp++) {
			if(*cp == YES) printf("%d ",i);
		}
	}
	if(ivp->clist != LNULL) {
		printf("clist ");
		clist = ivp->clist;
		LFOR(lp,clist) {
			printf("L[%d] ",LCAST(lp,LEAF)->leafno); 
		}
	}
	(void)putchar('\n');
}

static void
dump_ivs()
{
	LIST **lpp, *hash_link, *lp;
	LEAF *leafp;
	IV_INFO *ivp;
	TRIPLE *cand;

	printf("IV summary\n");
	for(lpp = leaf_hash_tab; lpp < &leaf_hash_tab[LEAF_HASH_SIZE]; lpp++) {
		LFOR(hash_link, *lpp) {
			leafp = (LEAF*)hash_link->datap;
			if(ivp = IV(leafp)){
				print_iv(ivp);
			}
		}
	}
	if(cands != LNULL) {
		printf("cands:");
		LFOR(lp,cands) {
			cand = (TRIPLE*) lp->datap;
			printf(" T[%d](L[%d]*L[%d]) ",
				cand->tripleno, cand->left->leaf.leafno,
				cand->right->leaf.leafno);
		}
		(void)putchar('\n');
	}
	if(iv_def_list != LNULL) {
		printf("iv_def_list:");
		LFOR(lp,iv_def_list) {
			printf(" T[%d]",LCAST(lp,TRIPLE)->tripleno);
		}
	}
	(void)putchar('\n');
}

static void
doiv_r23()
{
	register LIST *lp;
	register char *cp;
	register int i, lim;
	LEAF *x, *y, *c;
	TRIPLE *cand;

	lim = n_iv + n_rc;
	LFOR(lp,cands) {
		cand = (TRIPLE*) lp->datap;
		if(IV(cand->left)->is_iv == IR_TRUE) {
			x = (LEAF*) cand->left;
			c = (LEAF*) cand->right;
		} else {
			x = (LEAF*) cand->right;
			c = (LEAF*) cand->left;
		}

		for(i=0, cp = IV(x)->afct; i < lim ;  i++, cp ++) {
			if(*cp == YES) {
				y = ivrc_tab[i];
				(void)insert_list(&(IV(y)->clist), (LDATA*)c, iv_lq);
			}
		}
	}
}

static void
doiv_r45(loop)
	LOOP *loop;
{
	LEAF *x, *c;
	int i, lim;
	LIST *cx, *lp;

	/*
	**	Steps R4 and R5 : For each x in IV U RC; for each c in C(x) :
	**	append temp(x,c) = x * c to the loop preheader and enter
	**	the temp in a hash table where it can be addressed by (x,c)
	*/
	lim = n_iv + n_rc;
	for(i=0; i< lim; i++) {
		x = ivrc_tab[i];
		cx = IV(x)->clist;
		LFOR(lp,cx) {
			c = (LEAF*) lp->datap;
			(void)new_iv_hashtab_rec(x,IR_MULT,c,loop);
		}
	}
}

static IV_HASHTAB_REC *
hash(x,op,c)
	register LEAF *x, *c;
	IR_OP op;
{
	register IV_HASHTAB_REC *htrp;
	register int key;

	key = ( ((int)x ^ (int)c) & ( 0xff00 ) ) >> 8;
	for(htrp = iv_hashtab[key]; htrp; htrp = htrp->next) {
		if(htrp->x == x && htrp->c == c && htrp->op == op) {
			return htrp;
		}
	}
	return (IV_HASHTAB_REC*) NULL;
}

static void
doiv_r67(loop)
	LOOP *loop;
{
	LEAF *c, *targ, *inc;
	TRIPLE *def_tp, *tp, *after, *after1;
	LIST *clist, *lp, *lp2, *new_l;
	TYPE type;
	IV_HASHTAB_REC *tc_hr;
	IR_OP update_op;
	BOOLEAN up;

	type.tword = UNDEF;
	type.size = 0;
	type.align = 0;
	LFOR(lp,iv_def_list) {
		def_tp = (TRIPLE*) lp->datap;
		targ = (LEAF*) def_tp->left;
		if(IV(targ)->clist == LNULL) {
			continue;	
		}
		clist = IV(targ)->clist;
		/* set after, which is the place to put the iv updating in the loop,
		   after1, which is the place to put the initialization of the iv 
		   in the preheader of the loop */
		if (IV(targ)->base_on == TBAD || IV(targ)->base_on == TNULL) {
			after1 = TNULL;
			after = def_tp;
		} else {
			after1 = loop->preheader->last_triple->tprev;
			tp = IV(targ)->base_on;
			up = IR_TRUE;
			if(def_tp->right->operand.tag == ISTRIPLE){
				if(((TRIPLE*)def_tp->right)->op == IR_NEG ||
				   ((TRIPLE*)def_tp->right)->op == IR_MINUS &&
				   IV(((TRIPLE*)def_tp->right)->right)->is_iv == IR_TRUE){
					up = IR_FALSE;
				}
			}
			/* look up for the base iv and determine either increment or
			   decrement */
			while(IV(tp->left)->base_on != TNULL) {
				if (IV(tp->left)->base_on == TBAD)
					quit("doiv_r67: couldn't find the base iv depends on");
				if(tp->right->operand.tag == ISTRIPLE){
					if(((TRIPLE*)tp->right)->op == IR_NEG ||
					   ((TRIPLE*)tp->right)->op == IR_MINUS &&
					   IV(((TRIPLE*)tp->right)->right)->is_iv == IR_TRUE){
						if(up == IR_TRUE)
							up = IR_FALSE;
						else
							up = IR_TRUE;
					}
				}
				tp = IV(tp->left)->base_on;
			}
			update_op = ((TRIPLE*)tp->right)->op;
			/* determine the operator used to do the updating */
			if(up == IR_FALSE){
				if(update_op == IR_PLUS)
					update_op = IR_MINUS;
				else
					update_op = IR_PLUS;
			}
			if(tp->left == ((TRIPLE*)tp->right)->left) {
				inc = (LEAF *)((TRIPLE*)tp->right)->right;
			} else {
				inc = (LEAF *)((TRIPLE*)tp->right)->left;
			}
			after = tp;
		}
		LFOR(lp2, clist) {
			TRIPLE *add_op;
			add_op = (TRIPLE*) def_tp->right;

			c = (LEAF*) lp2->datap;
			tc_hr = hash(targ,IR_MULT,c);
			if(def_tp->right->operand.tag == ISLEAF) {
				if(after1) {
					after1 = add_triple(after1,
						IR_ASSIGN, (IR_NODE*)tc_hr->t,
						(IR_NODE*)T(def_tp->right,c), type);
					after = add_triple(after,
						update_op, (IR_NODE*)tc_hr->t,
						(IR_NODE*)T(inc, c), add_op->type);
					after = add_triple(after,
						IR_ASSIGN, (IR_NODE*)tc_hr->t,
						(IR_NODE*)after, type);
				} else {
					after = add_triple(after, IR_ASSIGN, 
						(IR_NODE*)tc_hr->t,
						(IR_NODE*)T(def_tp->right,c), type);
				}
			} else if(def_tp->right->operand.tag == ISTRIPLE) {
				switch(add_op->op) {
					case IR_PLUS:
					case IR_MINUS:
						if(after1) {
							after1 = add_triple(after1, add_op->op,
										(IR_NODE*)T(add_op->left,c),
										(IR_NODE*)T(add_op->right,c),
										add_op->type);
							after1 = add_triple(after1, IR_ASSIGN,
										(IR_NODE*)tc_hr->t,
										(IR_NODE*)after1, type);
							after = add_triple(after, update_op,
										(IR_NODE*)tc_hr->t,
										(IR_NODE*)T(inc, c),
										add_op->type);
							after = add_triple(after, IR_ASSIGN,
										(IR_NODE*)tc_hr->t,
										(IR_NODE*)after, type);
							break;
						}
						after = add_triple(after, add_op->op, 
									(IR_NODE*)T(add_op->left,c),
									(IR_NODE*)T(add_op->right,c),
									add_op->type);
						after = add_triple(after, IR_ASSIGN, 
									(IR_NODE*)tc_hr->t,
									(IR_NODE*)after, type);
						break;

					case IR_NEG:
						if(after1) {
							after1 = add_triple(after1, IR_NEG, 
										(IR_NODE*)T(add_op->left,c),
										(IR_NODE*)NULL,
										add_op->type);
							after1 = add_triple(after1, IR_ASSIGN, 
										(IR_NODE*)tc_hr->t,
										(IR_NODE*)after1, type);
							after = add_triple(after, update_op, 
										(IR_NODE*)tc_hr->t,
										(IR_NODE*)T(inc, c), add_op->type);
							after = add_triple(after, IR_ASSIGN, 
										(IR_NODE*)tc_hr->t,
										(IR_NODE*)after, type);
							break;
						}
						after = add_triple(after, IR_NEG, 
									(IR_NODE*)T(add_op->left,c),
									(IR_NODE*) NULL, add_op->type);
						after = add_triple(after, IR_ASSIGN, 
									(IR_NODE*)tc_hr->t,
									(IR_NODE*)after, type);
						break;

					default:
						quit("doiv_r67: bad op in iv_def_list");
				}
			}
			new_l = NEWLIST(iv_lq);
			(TRIPLE*) new_l->datap = after;
			LAPPEND(tc_hr->update_triples,new_l);
		}
	}
}

static TRIPLE *
replace_operand(old_operand,with)
	IR_NODE *old_operand;
	LEAF *with;
{
	TRIPLE *parent, *tp;

	parent = find_parent((TRIPLE*)old_operand);
	if(ISOP(parent->op,UN_OP)) {
		(LEAF*) parent->left = with;
	} else if(parent->left == old_operand) {
		(LEAF*) parent->left = with;
	} else {
		(LEAF*) parent->right = with;
	}
	if(old_operand->operand.tag == ISTRIPLE) {
		tp = (TRIPLE*) old_operand;
		tp->tprev->tnext = tp->tnext;
		tp->tnext->tprev = tp->tprev;
	}
	return parent;
}

static void
doiv_r89(loop)
	LOOP *loop;
{
	LEAF *c, *x, *cand_sibling;
	TRIPLE *cand, *parent;
	LIST *lp, *new_l;
	IV_HASHTAB_REC *hr_mult, *hr_plus;

	LFOR(lp,cands) {
		cand = (TRIPLE*) lp->datap;
		if(IV(cand->left)->is_iv == IR_TRUE) {
			x = (LEAF*) cand->left;
			c = (LEAF*) cand->right;
		} else {
			x = (LEAF*) cand->right;
			c = (LEAF*) cand->left;
		}
		hr_mult = hash(x,IR_MULT,c);
		parent = replace_operand((IR_NODE*)cand, hr_mult->t);
		if(parent->op == IR_PLUS) {
			if(parent->left == (IR_NODE*) hr_mult->t) {
				if(	parent->right->operand.tag == ISLEAF &&
					( 
						((IV(parent->right) && IV(parent->right)->is_rc== IR_TRUE))
						||
					    (ISCONST(parent->right)) 
					)){
					cand_sibling = (LEAF*) parent->right;
				} else {
					continue;
				}
			} else {
				if(	parent->left->operand.tag == ISLEAF &&
					( 
						((IV(parent->left) && IV(parent->left)->is_rc== IR_TRUE))
						||
					    (ISCONST(parent->left))
					)){
					cand_sibling = (LEAF*) parent->left;
				} else {
					continue;
				}
			}
			if( ((hr_plus = hash(hr_mult->t,IR_PLUS,cand_sibling)) == 
											(IV_HASHTAB_REC*) NULL)
			) {
				hr_plus = new_iv_hashtab_rec(hr_mult->t,IR_PLUS,cand_sibling,loop);
				if(ISCONST(c) && c->val.const.c.i != 0 && !IV(x)->plus_temps){
				/* record c and the new iv for dotest_repl to use */
					new_l = NEWLIST(iv_lq);
					(LEAF*)new_l->datap = c;
					LAPPEND(IV(x)->plus_temps,new_l);
					new_l = NEWLIST(iv_lq);
					(IV_HASHTAB_REC*) new_l->datap = hr_plus;
					LAPPEND(IV(x)->plus_temps,new_l);
				}
				/* if no new temp generated, no update */
				if(hr_plus->updated == IR_FALSE) 
					update_plus_temp(hr_mult,hr_plus,cand_sibling);
			}
			(void)replace_operand((IR_NODE*)parent,hr_plus->t);
		}
	}
}

/* insert updates for a new plus temp (C1+X*C2) everywhere (X*C2) is updated */
static void
update_plus_temp(hr_mult,hr_plus,cand_sibling)
	IV_HASHTAB_REC *hr_mult, *hr_plus;
	LEAF *cand_sibling;
{
	LIST *lp, *update_list;
	TRIPLE *def_tp, *after;
	TYPE type;
	LEAF *incr;

	type.tword = UNDEF;
	type.size = 0;
	type.align = 0;
	hr_plus->updated = IR_TRUE;
	update_list = hr_mult->update_triples;
	LFOR(lp, update_list) {
		after = def_tp = (TRIPLE*) lp->datap;
		if(def_tp->right->operand.tag == ISLEAF) { 
			/* t1 <- t2  : recompute (t1+L1) */
			after = add_triple(after, IR_PLUS,
				(IR_NODE*)hr_mult->t,
				(IR_NODE*)cand_sibling, cand_sibling->type);
			after = add_triple(after, IR_ASSIGN, 
				(IR_NODE*)hr_plus->t, (IR_NODE*)after, type);
		} else if( def_tp->right->operand.tag == ISTRIPLE) {
			TRIPLE *add_op;
			add_op = (TRIPLE*) def_tp->right;
			switch(add_op->op) {
				case IR_PLUS:
				case IR_MINUS:
					if(hr_mult->t == (LEAF*) add_op->left) {
						/* t1 <- t1 + k  : update (t1+L1) */
						incr = (LEAF*) add_op->right;
					} else if(hr_mult->t == (LEAF*) add_op->right) {
						incr = (LEAF*) add_op->left;
					} else {
						/* t1 <- t2 + k  : recompute (t1+L1) */
						after = add_triple(after, IR_PLUS,
							(IR_NODE*)hr_mult->t,
							(IR_NODE*)cand_sibling, cand_sibling->type);
						after = add_triple(after, IR_ASSIGN, 
							(IR_NODE*)hr_plus->t,
							(IR_NODE*)after, type);
						break;
					}
					after = add_triple(after, add_op->op, 
								(IR_NODE*)hr_plus->t,
								(IR_NODE*)incr, add_op->type);
					after = add_triple(after, IR_ASSIGN, 
								(IR_NODE*)hr_plus->t,
								(IR_NODE*)after, type);
					break;

				case IR_NEG:
					if(hr_mult->t == (LEAF*) add_op->left) {
						/* t1 <- -t1 : change sign of (t1+L1) */
						after = add_triple(after, IR_NEG, 
								(IR_NODE*)hr_plus->t,
								(IR_NODE*) NULL, add_op->type);
						after = add_triple(after, IR_ASSIGN, 
								(IR_NODE*)hr_plus->t,
								(IR_NODE*)after, type);
					} else {
						/* t1 <- -t2 : recompute (t1+L1) */
						after = add_triple(after, IR_PLUS,
								(IR_NODE*)hr_mult->t,
								(IR_NODE*)cand_sibling,
								cand_sibling->type);
						after = add_triple(after, IR_ASSIGN, 
								(IR_NODE*)hr_plus->t,
								(IR_NODE*)after, type);
					}
					break;

				default:
					quit("update_plus_temp: bad op in update_triples list");
			}
		}
	}
}

TRIPLE *
find_parent(def_tp) 
	register TRIPLE *def_tp;
{
	register TRIPLE *use_tp, *right, *argp;

	TFOR(use_tp,def_tp) {
		if(ISOP(use_tp->op, USE1_OP) && (TRIPLE*) use_tp->left == def_tp) {
			return use_tp;
		}else if(ISOP(use_tp->op,USE2_OP) && (TRIPLE*)use_tp->right == def_tp){
			return use_tp;
		} else if( ISOP(use_tp->op, NTUPLE_OP)) {
			right = (TRIPLE*) use_tp->right;
			if(right != TNULL) TFOR( argp, right ) {
				if( (TRIPLE*) argp->left == def_tp) {
					return argp;
				}
			}
		}
	}
	/*
	 * couldn't find parent; make one up
	 */
	return append_triple(def_tp, IR_FOREFF,
		    (IR_NODE*)def_tp, (IR_NODE*)NULL, undeftype);
}

static IV_HASHTAB_REC *
new_iv_hashtab_rec(x,op,c,loop)
	IR_OP op;
	LEAF *x, *c;
	LOOP *loop;
{
	IV_HASHTAB_REC *htrp;
	LEAF *t;
	TRIPLE *after, *new_t;
	int val, key;
	TYPE type;

	htrp = (IV_HASHTAB_REC*) ckalloc(1,sizeof(IV_HASHTAB_REC));
	htrp->x = x;
	htrp->c = c;
	htrp->op = op;
	htrp->updated = IR_FALSE;
	if( ISCONST(x) && ISCONST(c) && ISINT(x->type.tword) && ISINT(c->type.tword) )
	{ /* can fold */
		if(op == IR_MULT) {
			val = x->val.const.c.i  * c->val.const.c.i;
		} else if(op == IR_PLUS) {
			val = x->val.const.c.i  + c->val.const.c.i;
		} else {
			quit("new_iv_hashtab_rec: bad op");
		}
		htrp->t = ileaf(val);
		htrp->def_tp == TNULL;
	} else { /* really need a triple */
		after = loop->preheader->last_triple->tprev; /*before goto*/
		/* make sure the temp if of type pointer if any of its operand are */
		type = temp_type(x->type, c->type);
		new_t = add_triple(after, op,
			(IR_NODE*)x, (IR_NODE*)c, type);
		if(new_t->expr && new_t->expr->save_temp) {
			t = new_t->expr->save_temp;
		} else {
			t = new_temp(new_t->type);
			if(new_t->expr) {
				new_t->expr->save_temp = t;
			}
		}
		htrp->t = t;
		htrp->def_tp = add_triple(new_t, IR_ASSIGN,
			(IR_NODE*)t, (IR_NODE*)new_t, type);
	}
	key = ( ((int)x ^ (int)c ) & ( 0xff00 ) ) >> 8;
	htrp->next = iv_hashtab[key];
	iv_hashtab[key] = htrp;
	return htrp;
}

/*
 * determine type of temp to hold l op r.
 * Assume that if one of the operands has pointer type,
 * so must the result. It is an error for either l or
 * r to have a non-scalar type.
 */
TYPE
temp_type(l,r)
	TYPE l,r;
{
	/*
	 * if one of the operands has pointer type, the
	 * other must have integral type.
	 */
	if( ISPTR(l.tword) ) {
		if ( !ISINT(r.tword) )
			quit("illegal pointer combination in temp_type");
		return l;
	}
	if( ISPTR(r.tword) ) {
		if ( !ISINT(l.tword) )
			quit("illegal pointer combination in temp_type");
		return r;
	}
	/*
	 * if neither has pointer type, both must have integral type.
	 */
	if(!ISINT(l.tword) || !ISINT(r.tword) ) {
		quit("illegal type combination in temp_type");
	}
	/*
	 * both types are integral; they should agree in size
	 */
	if( type_size(l) != type_size(r) ) {
		quita("size mismatch in temp_type (l.size = %d, r.size = %d)",
			l.size, r.size);
		/*NOTREACHED*/
	}
	return l;
}


/*make sure that whenever we generate a new triple we update the expr hash tab*/
static TRIPLE *
add_triple(after,op,left,right,type)
	register TRIPLE *after;
	register IR_OP op;
	register IR_NODE *left, *right;
	TYPE type;
{
	register TRIPLE *new_triple;

	new_triple = append_triple(after,op,left,right,type);
	if( ISOP(op,VALUE_OP) ) {
		new_triple->expr = find_expr((IR_NODE*)new_triple);
	}
	return new_triple;
}

/*
 * Allocate new IV_INFO record.
 * Insert at the beginning of iv_info_list,
 * which is doubly linked for efficient
 * deletion.
 */
static IV_INFO *
new_iv_info()
{
	IV_INFO *ivp;

	if (free_iv_info_list == NULL) {
		ivp = (IV_INFO*) ckalloc(1,sizeof(IV_INFO));
	} else {
		ivp = free_iv_info_list;
		free_iv_info_list = ivp->next;
	}
	ivp->next = iv_info_list;
	ivp->prev = NULL;
	if (iv_info_list != NULL)
		iv_info_list->prev = ivp;
	iv_info_list = ivp;
	return(ivp);
}

/*
 * Delete an IV_INFO record from the iv_info list.
 * Recycle it on the free_iv_info_list.
 */
static void
free_iv_info(ivp)
	IV_INFO *ivp;
{
	if (ivp->prev != NULL) {
		ivp->prev->next = ivp->next;
	}
	if (ivp->next != NULL) {
		ivp->next->prev = ivp->prev;
	}
	if (iv_info_list == ivp) {
		iv_info_list = ivp->next;
	}
	if (ivp->leafp) {
		IV(ivp->leafp) = NULL;
	}
	(void)bzero((char*)ivp, sizeof(IV_INFO));
	ivp->next = free_iv_info_list;
	free_iv_info_list = ivp;
}

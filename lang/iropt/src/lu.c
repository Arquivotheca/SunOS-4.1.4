#ifndef lint
static  char sccsid[] = "@(#)lu.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include "page.h"
#include "misc.h"
#include "loop.h"
#include "recordrefs.h"
#include "iv.h"
#include "lu.h"
#include <stdio.h>

/* 
 * LOOP UNROLLING TRANSFORMATION
 *
 *
 *	For a single basic block loop like:
 *
 *		..
 *		..
 *	loop:
 *		..
 *		..
 *		Body of the loop, update i by i = i + c
 *		..
 *		..
 *		if( i <= u ) goto loop else goto end
 *	end:
 *
 *	where i is an induction variable, u is the upper bound, and both 
 *	u, and c are region constants, i.e. the values do not change 
 *	during the loop iteration.
 *
 *
 *	To unroll the loop n times, the transformation replaces the loop 
 *	as:
 *
 *		..
 *		..
 *		if( i + (n-1)c > u ) goto testing else goto loop
 *	loop:
 *		..
 *		..
 *		Body of the loop, update i by i = i + c
 *		..
 *		..
 *
 *		   (n-2 copies of the loop body)
 *
 *		..
 *		..
 *		Body of the loop, update i by i = i + c
 *		..
 *		..
 *
 *		if( i + (n-1)c <= u ) goto loop else goto testing
 *	testing:
 *		if( i <= u ) goto odd else goto end
 *	odd:
 *		..
 *		..
 *		Body of the loop, update i by i = i + c
 *		..
 *		..
 *
 *		goto testing
 *	end:
 *
 * The functionality of two arithmetic units, which run concurrently,
 * may change from one implementation to another --> unrollable 
 * condition needs update
 *
 * The transformation doesn't count the exceptional cases --
 * if there is overflow or underflow, there is no guarantee to
 * generate the same effect code.
 */

#define TBAD ((TRIPLE*)-1)
#define MAXTRIPLES 40
#define TINYLOOP 20
#define FLOAT_UNROLL_FACTOR 2
#define TINYLOOP_UNROLL_FACTOR 4
#define FLOAT_INTENSIVE 8	/* loops with >= 1/8 floating point */
#define MEM_INTENSIVE   8	/* loops with >= 1/8 indirect loads/stores */

/* local variables */

static TRIPLE	*cntrl_tp;
static LEAF	*cntrl_iv;
static LEAF	*final;
static TRIPLE	*dup_start, *dup_end;
static BLOCK	*body, *ending;
static IR_OP	cmp_op;
static TYPE	undef_type, iv_type;
static BOOLEAN	left_is_rc;
static IR_OP	update_op;
static LEAF	*inc;
static TRIPLE	*cbranch_tp;
static LIST	*implicit_info[31];
static BOOLEAN	no_alias = IR_TRUE;
static BOOLEAN	loop_unroll_option_flag = IR_FALSE; /* true if -l<n> passed */
static int	loop_unroll_option_factor = 1;	/* specified unrolling factor */

/* local functions */

static BOOLEAN	pattern_ok(/*loop*/);
static BOOLEAN	is_reg_var(/*leafp*/);
static BOOLEAN	unrollable(/*loop,ntriples, nfloatops, nmemops, ndeadops*/);
static void	reg_aliaser();
static void	unroll(/*loop, ntriples, nfloatops, nmemops, ndeadops*/);
static IR_NODE	*new_var(/*x, c, op, after*/);
static TRIPLE	*copy_triple(/*tp, after, new_tp_ptr*/);
static TRIPLE	*dup_body(/*after*/);
static void	sub_iv(/*tp, targ, op, inc, n*/);

static BOOLEAN	define_tp(/*tp, targ*/);
static void	replace_iv();

#if TARGET == MC68000
static char *a[] = {
	"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"};
#endif

#if TARGET == SPARC
static char *a[] = {
	"%g0", "%g1", "%g2", "%g3", "%g4", "%g5", "%g6", "%g7",
	"%o0", "%o1", "%o2", "%o3", "%o4", "%o5", "%o6", "%o7",
	"%l0", "%l1", "%l2", "%l3", "%l4", "%l5", "%l6", "%l7",
	"%i0", "%i1", "%i2", "%i3", "%i4", "%i5", "%i6", "%i7"};
#endif

/*
 * set the value of the -l<factor> option,
 * which specifies a loop unrolling factor.
 * Note that a factor of 1 means do nothing.
 */
void
set_unroll_option(factor)
	int factor;
{
	loop_unroll_option_flag = IR_TRUE;
	loop_unroll_option_factor = (factor >= 1 ? factor : 1);
}

/*
 * return TRUE if loop unrolling is enabled.  Unrolling is on
 * by default at levels -O3,-O4, and may be enabled at any level
 * by the -l<factor> option.  This function is external so that
 * we can avoid a data flow pass if we aren't going to unroll
 * anything.
 */
BOOLEAN
loop_unroll_enabled()
{
	if (loop_unroll_option_flag) {
		return (BOOLEAN)(loop_unroll_option_factor >= 2);
	}
	return (BOOLEAN)(!(partial_opt));
}

/*
 * return TRUE if we are compiling code for a schedulable FPU.
 * On SPARC, this is always true; on m68k, it is true for the
 * 68882, and the FPA (provided the FPA instruction scheduler
 * in /lib/c2 is hacked to use the alias/noalias information)
 */
BOOLEAN
schedulable_fpu()
{
#	if TARGET == SPARC
	    return IR_TRUE;
#	endif
#	if TARGET == MC68000
	    /*
	     * note we assume "68881" means "68882" here
	     */
	    return (BOOLEAN)(use68881 || usefpa);
#	endif
#	if TARGET == I386
	    /* this will have to change for the Weitek 3167 */
	    return IR_FALSE;
#	endif
}

/*
 * return the loop unrolling factor, i.e., the number of times
 * eligible loops are to be unrolled.  The default unrolling
 * factor may be overridden by specifying the -l<factor> option.
 *
 * There are currently 2 situations where unrolling is a win,
 * assuming you're willing to give up the extra code space:
 *
 * 1. very small memory-bound loops, all architectures.  Reason:
 *    if the loop is small enough, branch overhead is a significant
 *    part of the execution time.
 *
 * 2. FPU-intensive loops.  Reason: unrolling helps schedule
 *    CPU/FPU activity concurrently.  This only pays off if the
 *    peephole optimizer knows about 'alias/noalias'.
 *
 * Making these notions quantitative inevitably involves some guesswork:
 *
 * 1. As a rough guess, we say a loop is "very small" and "memory-intensive"
 *    if its body contains no more than 20 intermediate code instructions,
 *    of which at least 10% must be indirect memory references.
 *
 * 2. As a rough guess, we say a loop is FPU-intensive if at least 10%
 *    of its instructions involve floating point.  [On sunrise, double
 *    precision add and multiply take 15 cycles; on sunray, add and
 *    multiply are 6 and 7 cycles, respectively.]
 *
 * 3. Loops with no indirect memory references are very unlikely to
 *    contain any interesting opportunities for instruction scheduling.
 *    Either the computation is inherantly serial, or else it is redundant,
 *    in which case it would have been moved out of the loop by earlier
 *    phases.  [However, note that assignments reaching only use sites
 *    outside the loop body yield dead code after unrolling.]
 */
static int
loop_unroll_factor(ntriples, nfloatops, nmemops, ndeadops)
	int ntriples;
	int nfloatops;
	int nmemops;
	int ndeadops;
{
	if (loop_unroll_option_flag) {
		/*
		 * -l<factor> specified.  Use it.
		 */
		return loop_unroll_option_factor;
	}
	if (nmemops == 0 && ndeadops == 0) {
		/*
		 * no point to unrolling this loop,
		 * at least on current machines.
		 */
		return 1;
	}
	if (ntriples <= TINYLOOP
	    && ntriples <= nmemops*MEM_INTENSIVE
	    && (nfloatops == 0 || schedulable_fpu())) {
		/*
		 * small memory-bound loop -- remember that
		 * floating point ops mean subroutine calls
		 * on machines without floating point hardware
		 */
		return TINYLOOP_UNROLL_FACTOR;
	}
	if (nfloatops > 0 && ntriples <= MAXTRIPLES
	    && ntriples <= nfloatops*FLOAT_INTENSIVE
	    && schedulable_fpu()) {
		/*
		 * floating point loop
		 */
		return FLOAT_UNROLL_FACTOR;
	}
	return 1;
}

void
do_loop_unrolling(loop)
	LOOP *loop;
{
	int ntriples;
	int nfloatops;
	int nmemops;
	int ndeadops;

	if (unrollable(loop, &ntriples, &nfloatops, &nmemops, &ndeadops)) {
		reg_aliaser();
		unroll(loop, ntriples, nfloatops, nmemops, ndeadops);
	}
}

/* same as ileaf, but look for const with the same type */
static LEAF *
const_ileaf(type, val)
TYPE type;
{
	struct constant const;
 
	const.isbinary = IR_FALSE;
	const.c.i=val;
	return (leaf_lookup(CONST_LEAF,type,(LEAF_VALUE *)&const));
}

static BOOLEAN
pattern_ok(loop)
	LOOP *loop;
{
	register TRIPLE *tp, *after;

	cntrl_iv = (LEAF *)NULL;
	cntrl_tp = TNULL;
	final = (LEAF *)NULL;
	dup_start = TNULL;
	dup_end = TNULL;
	left_is_rc = IR_FALSE;

	/* 
	 * final testing must be iv comparing with rc
	 */
	if(body->last_triple->op == IR_CBRANCH){
		cbranch_tp = body->last_triple;
		tp = body->last_triple->tprev;
		if((tp->op == IR_LE || tp->op == IR_LT || tp->op == IR_GE || 
			tp->op == IR_GT)
		  && ISINT(tp->type.tword)
		  && !ISUNSIGNED(tp->left->operand.type.tword)
		  && (TRIPLE*)tp->tnext->left == tp ){

			if(tp->left->operand.tag == ISLEAF){
				if(ISCONST(tp->left) || 
				   IV(tp->left) && IV(tp->left)->is_rc == IR_TRUE){
					if(tp->right->operand.tag == ISLEAF &&
					   IV(tp->right) && IV(tp->right)->is_iv == IR_TRUE){
						left_is_rc = IR_TRUE;
						cntrl_iv = (LEAF *)tp->right;
						final = (LEAF *)tp->left;	/* iv cmp rc */
					} else
						return IR_FALSE;			/* non-iv cmp rc */
				} else if(IV(tp->left) && IV(tp->left)->is_iv){
					if(tp->right->operand.tag == ISLEAF && 
					   (ISCONST(tp->right) ||
						IV(tp->right) && IV(tp->right)->is_rc == IR_TRUE)){
						final = (LEAF *)tp->right;
						cntrl_iv = (LEAF *)tp->left;/* rc cmp iv */
					} else
						return IR_FALSE;			/* non-rc cmp iv */
				} else
					return IR_FALSE;				/* not iv nor rc */
			} else 
				return IR_FALSE;					/* not iv-triple */

			cmp_op = tp->op;
			dup_end = tp->tprev;
			while(!(ISOP(dup_end->op, ROOT_OP))) {
				dup_end = dup_end->tprev;
			}
			if(dup_end == cbranch_tp)
				quit("loop_unroll: no dup_end triple");
		}
	}

	/* 
	 * transfor repeat node to cbranch 
	 */
	if((tp = body->last_triple)->op == IR_REPEAT){
		cntrl_iv = (LEAF*)tp->left;
		if(!ISINT(cntrl_iv->type.tword))
			return IR_FALSE;
		final = ileaf(0);
		dup_end = tp->tprev;
		cmp_op = IR_LT;
		after = append_triple(tp->tprev, IR_PLUS,
			(IR_NODE*)tp->left, 
			(IR_NODE*)const_ileaf(tp->left->leaf.type,-1),
			tp->left->leaf.type);
		after = append_triple(after, IR_ASSIGN,
			(IR_NODE*)tp->left, (IR_NODE*)after,
			tp->left->leaf.type);
		dup_end = after;
		set_iv(after, IR_FALSE, loop);
		IV(tp->left)->base_on = TNULL;
		after = append_triple(after, IR_LT,
			(IR_NODE*)tp->left, (IR_NODE*)ileaf(0),
			tp->left->leaf.type);
		tp->op = IR_CBRANCH;
		tp->left = (IR_NODE*)after;
		cbranch_tp = tp;
	}

	if(dup_end == TNULL) return(IR_FALSE);			/* pattern not found */
	return IR_TRUE;
}

/*
 * return TRUE if leafp represents a register variable.
 * Why?  Because the peephole optimizers can delete dead
 * register assignments, but not dead memory assignments.
 * [We're talking kludge here -- normally, iropt does dead
 * code elimination, but unrolling takes place too late to
 * take advantage of this.]
 */

static BOOLEAN
is_reg_var(leafp)
	LEAF *leafp;
{
	switch(leafp->val.addr.seg->descr.class) {
#if TARGET == MC68000
	case DREG_SEG:
	case AREG_SEG:
	case FREG_SEG:
		return IR_TRUE;
#endif
#if TARGET == SPARC || TARGET == I386
	case DREG_SEG:
	case FREG_SEG:
		return IR_TRUE;
#endif
	default:
		break;
	}
	return IR_FALSE;
}

/*
 * Return TRUE for a loop that is eligible for unrolling.
 * For eligible loops, counts of triples of various types are
 * returned in the result parameters ntriples, nfloatops, nmemops,
 * ndeadeops.  The values of these parameters are undefined for
 * ineligible loops.
 */
static BOOLEAN
unrollable(loop, ntriples, nfloatops, nmemops, ndeadops)
	LOOP *loop;
	int *ntriples;
	int *nfloatops;
	int *nmemops;
	int *ndeadops;
{
	register LIST *lp;
	register TRIPLE *tp;
	int nblocks;	/* #blocks in loop */
	int triples;	/* #triples in loop body */
	int floatops;	/* #floating point ops in loop body */
	int memops;	/* #memory ops in loop body */
	int deadops;	/* #(potentially) dead stores in loop body */

	/*
	 * initialize result parameters
	 */
	*ntriples = 0;
	*nfloatops = 0;
	*nmemops = 0;
	*ndeadops = 0;
	/*
	 * count blocks in loop
	 */
	nblocks = 0;
	LFOR(lp, loop->blocks){
		if (++nblocks > 1) {
			return IR_FALSE;	/* not single bb */
		}
	}
	/*
	 * count triples in loop body
	 */
	find_iv(loop);
	triples = 0;
	floatops = 0;
	memops = 0;
	deadops = 0;
	body = LCAST(loop->blocks, BLOCK);
	TFOR(tp, body->last_triple) {
		if(ISOP(tp->op, (VALUE_OP|MOD_OP|USE1_OP|USE2_OP))
		    && !ISOP(tp->op, HOUSE_OP)) {
			triples++;
			if (triples > MAXTRIPLES) {
				break;
			}
		}
		if(ISREAL(tp->type.tword)) {
			/* look for potentially concurrent FPU ops */
			floatops++;
		}
		if((tp->op == IR_IFETCH || tp->op == IR_ISTORE)
		    && tp->left->operand.tag == ISLEAF
		    && IV(tp->left) != NULL ) {
			/* look for block move loops and the like */
			memops++;
		}
		if (tp->op == IR_ASSIGN && can_delete(tp)
		    && is_reg_var((LEAF*)tp->left)) {	/*KLUDGE*/
			LIST *lp;
			BOOLEAN used_in_body;
			used_in_body = IR_FALSE;
			LFOR(lp, tp->canreach) {
				if (LCAST(lp, VAR_REF)->site.bp == body) {
					used_in_body = IR_TRUE;
					break;
				}
			}
			if (!used_in_body) {
				/*
				 * an assignment that reaches only sites
				 * outside the loop body yields dead code
				 * after unrolling.
				 */
				deadops++;
			}
		}
	}
	/*
	 * set result parameters to return counts
	 */
	*ntriples = triples;
	*nfloatops = floatops;
	*nmemops = memops;
	*ndeadops = deadops;
	/*
	 * if this loop isn't worth unrolling, give up
	 */
	if (loop_unroll_factor(triples, floatops, memops, deadops) < 2) {
		return IR_FALSE;
	}

	if(pattern_ok(loop) == IR_FALSE) {
		return IR_FALSE;
	}

	/* 
	 * find the init value -- through the ref records --- the definition
	 * outside of the loop
	 * calculate the number of iterations if possible
	 */

	/* 
	 * the cntrl_iv must be a base iv.  Why? -- after the iv optimization,
	 * it makes least sense to generalize the restriction
	 */
	if(IV(cntrl_iv)->base_on != TNULL) return(IR_FALSE);
	iv_type = cntrl_iv->type;

	if(body->last_triple->tnext->op == IR_LABELDEF){
		dup_start = body->last_triple->tnext->tnext;
	} else {
		quit("block starts with no labeldef");
	}
	return IR_TRUE;
}

/* 
 * find the aliasing information between registers for
 * instruction scheduler to generate more efficient code
 */
static void
reg_aliaser()
{
	TRIPLE *tp;
	LEAF *reg, *affect_leafp;
	LIST *lp, *lpi, *lpj;
	int i, j;
	BOOLEAN good;
	char buff[32];
	LEAF *new_leaf;

	for(i = 0; i <= 31; i++)
		implicit_info[i] = LNULL;
	TFOR(tp, body->last_triple){
		if((tp->op == IR_IFETCH || tp->op == IR_ISTORE) &&
		   tp->left->operand.tag == ISLEAF){
			reg = (LEAF*)tp->left;
#if TARGET == MC68000
			if(reg->val.addr.seg->descr.class != AREG_SEG){
#endif
#if TARGET ==  SPARC || TARGET == I386
			if(reg->val.addr.seg->descr.class != DREG_SEG){
#endif
				break;
			}
			/* record information */
			LFOR(lpi, tp->can_access){
				affect_leafp = (LEAF*)lpi->datap;
				lp = NEWLIST(proc_lq);
				lp->datap = (LDATA*)affect_leafp;
				LAPPEND(implicit_info[reg->val.addr.offset], lp);
			}
		}
	}
	/* 
	 * four level nested loops!! Should work fine with most fortran
	 * programs.  If we really want to unroll C programs, change it 
	 * to bit array
	 */
	for(i = 0; i <= 31; i++){
		for(j = i+1; j <=31; j++){
			good = IR_TRUE;
			LFOR(lpi, implicit_info[i]){
				if(good == IR_FALSE)
					break;
				LFOR(lpj, implicit_info[j]){
					if(lpi->datap == lpj->datap){
						good = IR_FALSE;
						break;
					}
				}
			}
			if(implicit_info[i] != LNULL && implicit_info[j] != LNULL &&
			   good == IR_TRUE){
				no_alias = IR_FALSE;
				/* put out |#NOALIAS# (ri, rj) */
#if TARGET == MC68000
				(void)sprintf(buff, "|#NOALIAS# %s, %s", a[i], a[j]);
#endif
#if TARGET == SPARC
				(void)sprintf(buff, "\t.noalias\t%s, %s", a[i], a[j]);
#endif
#if TARGET == I386
				(void)sprintf(buff, "/# NOALIAS (r%d, r%d)", i, j);
#endif
				new_leaf = cleaf(buff);
				(void)append_triple(dup_start->tprev, IR_PASS,
					(IR_NODE*)new_leaf, (IR_NODE*)NULL,
					undef_type);
			}
		}
	}
	/* delete implicit_info */
}

LIST *
connect_block(list, block)
	LIST *list;
	BLOCK *block;
{
	register LIST *lp;

	lp = NEWLIST(proc_lq);
	lp->datap = (LDATA *)block;
	LAPPEND(list, lp);
	return list;
}

static void
unroll(loop, ntriples, nfloatops, nmemops, ndeadops)
	LOOP *loop;
	int ntriples;
	int nfloatops;
	int nmemops;
	int ndeadops;
{
	register BLOCK *preheader, *testing, *odd;
	register TRIPLE *after, *after1;
	TRIPLE *tp, *def_tp, *ref_triple;
	IR_NODE *tmp;
	int i;
	TRIPLE *dummy;
	char buff[16];
	LEAF *new_leaf;
	int n_unroll;

	n_unroll = loop_unroll_factor(ntriples, nfloatops, nmemops, ndeadops);
	if(SHOWCOOKED == IR_TRUE)
		/* print out the alising information */
		(void)fprintf(stderr, "--unroll loop %d--\n", loop->loopno);

	tp = (TRIPLE*)body->last_triple->right;
	if((BLOCK*)tp->left == body){
		ending = (BLOCK *)tp->tnext->left;
	} else {
		ending = (BLOCK *)tp->left;
	}

	def_tp = IV(cntrl_iv)->def_triple;
	if(def_tp->op == IR_ASSIGN && def_tp->right->operand.tag == ISTRIPLE){
		update_op = ((TRIPLE *)(def_tp->right))->op;
		if(def_tp->left == def_tp->right->triple.left)
			inc = (LEAF*)def_tp->right->triple.right;
		else
			inc = (LEAF*)def_tp->right->triple.left;
	} else
		quit("def triple for a base iv is wrong");

	preheader = loop->preheader;

	testing = new_block();
	testing->labelno = new_label();
	move_block(body,testing);

	odd = new_block();
	odd->labelno = new_label();
	move_block(testing, odd);

	testing->pred = connect_block(testing->pred, body);
	(void)connect_block(testing->pred, odd);
	testing->succ = connect_block(testing->succ, ending);
	(void)connect_block(testing->succ, odd);
	odd->pred = connect_block(odd->pred, preheader);
	(void)connect_block(odd->pred, testing);
	odd->succ = connect_block(odd->succ, testing);
	(void)connect_block(preheader->succ, odd);
	(void)connect_block(ending->pred, testing);
	(void)connect_block(body->succ, testing);
	/* get rid of ending from body->succ
				  body from ending->pred */

	undef_type.tword = UNDEF;
	undef_type.size = 0;
	undef_type.align = 0;

	/* 
	 * fix preheader -- check if goto odd or body -------------------
	 */
	if(preheader->last_triple->op == IR_GOTO){
		tp = preheader->last_triple;
		preheader->last_triple = tp->tprev;
		unlist_triple(IR_FALSE,tp,preheader);	/* delete goto, but not labeldef */
		tmp = new_var(inc, const_ileaf(inc->type,n_unroll-1), IR_MULT, 
			preheader->last_triple);
		after = append_triple(preheader->last_triple, update_op, 
			(IR_NODE*)cntrl_iv, (IR_NODE*)tmp, iv_type);
		if(left_is_rc){
			after = append_triple(after, cmp_op,
			(IR_NODE*)final, (IR_NODE*)after, iv_type);
		} else {
			after = append_triple(after, cmp_op,
			(IR_NODE*)after, (IR_NODE*)final, iv_type);
		}
		tp = copy_triple(cbranch_tp, after, &dummy);
		tp->left = (IR_NODE*)after;
		if(tp->right->triple.left == (IR_NODE*)body){
			tp->right->triple.tnext->left = (IR_NODE*)odd;
		} else {
			tp->right->triple.left = (IR_NODE*)odd;
		}
		preheader->last_triple = tp;
	} else
		quit("last triple of a preheader is not goto");

	/* 
	 * fix testing -- check if goto odd or ending ----------------------
	 */
	testing->last_triple = append_triple(testing->last_triple, IR_LABELDEF,
		(IR_NODE*)testing, (IR_NODE*)NULL, undef_type);
	if(left_is_rc == IR_TRUE){
		after = append_triple(testing->last_triple, cmp_op,
		(IR_NODE*)final, (IR_NODE*)cntrl_iv, iv_type);
	} else {
		after = append_triple(testing->last_triple, cmp_op,
		(IR_NODE*)cntrl_iv, (IR_NODE*)final, iv_type);

	}
	tp = copy_triple(cbranch_tp, after, &dummy);
	tp->left = (IR_NODE*)after;
	if(tp->right->triple.left == (IR_NODE*)ending){
		tp->right->triple.tnext->left = (IR_NODE*)odd;
	} else {
		tp->right->triple.left = (IR_NODE*)odd;
	}
	testing->last_triple = tp;

	/* 
	 * fix odd -- duplicate body and goto testing ----------------------
	 */
	odd->last_triple = append_triple(odd->last_triple, IR_LABELDEF,
		(IR_NODE*)odd, (IR_NODE*)NULL, undef_type);
	after = dup_body(odd->last_triple);
	ref_triple = append_triple(TNULL, IR_LABELREF,
		(IR_NODE*)testing, (IR_NODE*)ileaf(0), undef_type);
	odd->last_triple= append_triple(after, IR_GOTO,
		(IR_NODE*)NULL, (IR_NODE*)ref_triple, undef_type);

	/* 
	 * fix body -- duplicate n_unroll times and replace iv's updating
	 * by inserting iv+rc triples ------------------------------------
	 */
	after = dup_body(dup_end);
	for(i = 3; i <= n_unroll; i++){
		after = dup_body(after);
	}
	tmp = new_var(inc, const_ileaf(inc->type,n_unroll-1), IR_MULT,
		body->last_triple->tprev);
	after = append_triple(body->last_triple->tprev, update_op,
		(IR_NODE*)cntrl_iv, tmp, iv_type);
	if(left_is_rc){
		after = append_triple(after, cmp_op,
		(IR_NODE*)final, (IR_NODE*)after, iv_type);
	} else {
		after = append_triple(after, cmp_op,
		(IR_NODE*)after, (IR_NODE*)final, iv_type);
	}
	after1 = after;
	if(no_alias == IR_FALSE){
#if TARGET == MC68000
		(void)sprintf(buff, "|#ALIAS#");
#endif
#if TARGET == SPARC
		(void)sprintf(buff, "\t.alias");
#endif
#if TARGET == I386
		(void)sprintf(buff, "/# ALIAS");
#endif
		new_leaf = cleaf(buff);
		after1 = append_triple(after, IR_PASS,
			(IR_NODE*)new_leaf, (IR_NODE*)NULL, undef_type);
		no_alias = IR_TRUE;
	}
	tp = copy_triple(cbranch_tp, after1, &dummy);
	tp->left = (IR_NODE*)after;
	if(tp->right->triple.left == (IR_NODE*)body){
		tp->right->triple.tnext->left = (IR_NODE*)testing;
	} else {
		tp->right->triple.left = (IR_NODE*)testing;
	}
	/* delete old cbrach tp */
	delete_triple(IR_FALSE, body->last_triple, body);
	body->last_triple = tp;
#if TARGET == MC68000
	/*
	 * on this machine, most of the time we're better
	 * off not doing replace_iv, because it inhibits
	 * autoincrement addressing.  However, autoincrement
	 * addressing gets in the way of FPU scheduling, so...
	 */
	if (nfloatops && schedulable_fpu()) {
		replace_iv(n_unroll);
	}
#else
	replace_iv(n_unroll);
#endif
}

static IR_NODE*
new_var(x, c, op, after)
	LEAF *x, *c;
	IR_OP op;
	TRIPLE *after;
{
	int val;
	TYPE type;
	TRIPLE *new_t;

	if( ISCONST(x) && ISCONST(c) &&	ISINT(x->type.tword) &&	ISINT(c->type.tword)) {
		/* can fold */
		if(op == IR_MULT) {
			val = x->val.const.c.i * c->val.const.c.i;
		} else if(op == IR_PLUS) {
			val = x->val.const.c.i + c->val.const.c.i;
		} else if(op == IR_MINUS) {
			val = x->val.const.c.i - c->val.const.c.i;
		} else
			quit("new_var: bad op");
		return (IR_NODE *)(const_ileaf(x->type,val));
	} else { /* need a triple */
		type = temp_type(x->type, c->type);
		new_t = append_triple(after,op, (IR_NODE*)x, (IR_NODE*)c, type);
		return (IR_NODE *)new_t;
	}
}

/*
 * This routine is customized for this file only
 */
static TRIPLE *
copy_triple(tp, after, new_tp_ptr)
	TRIPLE *tp, *after;
	TRIPLE **new_tp_ptr;
{
	IR_NODE *left, *right;
	register TRIPLE *tlp;
	TRIPLE *lk, *lk1;

	if(ISOP(tp->op, HOUSE_OP) && tp->op != IR_LABELREF)
			quit("wrong operation: copy_triple");
	if(tp->op == IR_LABELDEF)
			return after;
	if(SHOWCOOKED == IR_TRUE){
			printf("	copy_triple: number %d\n", tp->tripleno);
	}
	left = right = (IR_NODE*)NULL;
	if(tp->left) switch(tp->left->operand.tag) {
		case ISTRIPLE:
			/* we don't wnat to copy the boolean expression */
			if(tp->op == IR_CBRANCH) break;
			after = copy_triple((TRIPLE*)tp->left, after,
				(TRIPLE**)&left);
			break;
		case ISLEAF:
		case ISBLOCK:
			left = tp->left;
			break;
		default:
			quit("bad left operand: copy_triple");
	}
	if(tp->right) switch(tp->right->operand.tag) {
		case ISTRIPLE:
			/* we don't copy FCALL */
			if(tp->op == IR_PARAM) break;
			if(ISOP(tp->op, BRANCH_OP) || 
			   tp->op == IR_SCALL || tp->op == IR_FCALL) {
				after = copy_triple((TRIPLE*)tp->right, after,
					(TRIPLE**)&right);
				lk = (TRIPLE *)right;
				for(tlp = tp->right->triple.tnext; 
					tlp != (TRIPLE *)tp->right; tlp = tlp->tnext) {
					after = copy_triple(tlp, after,
						(TRIPLE**)&lk1);
					TAPPEND(lk, lk1);
					lk = lk1;
				}
			} else {
				after = copy_triple((TRIPLE*)tp->right, after,
					(TRIPLE**)&right);
			}
			break;
		case ISLEAF:
			right = tp->right;
			break;
		default:
			quit("bad right operand: copy_triple");
	}
	if(tp->op == IR_PARAM || tp->op == IR_LABELREF){
		*new_tp_ptr = append_triple(TNULL, tp->op, left, right, tp->type);
		return after;
	}
	*new_tp_ptr = append_triple(after, tp->op, left, right, tp->type);
	if(tp->op == IR_FCALL || tp->op == IR_SCALL){
		TFOR(tlp, (TRIPLE*)((*new_tp_ptr)->right)){
			tlp->right = (IR_NODE*)*new_tp_ptr;
		}
	}
	return *new_tp_ptr;
}

static TRIPLE *
dup_body(after)
	TRIPLE *after;
{
	TRIPLE *tp;
	TRIPLE *dummy;

	for(tp = dup_start; tp != dup_end; tp = tp->tnext){
		if(ISOP(tp->op, ROOT_OP)){
			after = copy_triple(tp, after, &dummy);
		}
	}
	/* copy the last ROOT_OP */
	after = copy_triple(dup_end, after, &dummy);
	return after;
}


static BOOLEAN
define_tp(tp, targ)
	TRIPLE *tp;
	LEAF *targ;
{
	if(tp->op != IR_ASSIGN || tp->left != (IR_NODE*)targ ||
	   tp->right->operand.tag != ISTRIPLE)
		return IR_FALSE;
	if(tp->left == tp->right->triple.right ||
	   tp->left == tp->right->triple.left)
		return IR_TRUE;
	else
		return IR_FALSE;
}


static void
sub_iv(tp, targ, op, inc, n)
	TRIPLE *tp;
	LEAF *targ;
	IR_OP op;
	LEAF *inc;
	int n;
{
	TRIPLE *tlp;
	IR_NODE *c, *tmp;

	c = new_var(inc, const_ileaf(inc->type,n), IR_MULT, tp->tprev);
	if(tp->left) switch(tp->left->operand.tag) {
		case ISTRIPLE:
			sub_iv((TRIPLE*)tp->left, targ, op, inc, n);
			break;
		case ISLEAF:
			if(tp->left == (IR_NODE*)targ){
				if(tp->op == IR_PARAM) {
					tmp = new_var(targ, (LEAF*)c,
						op, tp->right->triple.tprev);
				} else {
					tmp = new_var(targ, (LEAF*)c,
						op, tp->tprev);
				}
				tp->left = (IR_NODE*)tmp;
			}
			break;
	}
	if(tp->right) switch(tp->right->operand.tag) {
		case ISTRIPLE:
			if(tp->op == IR_PARAM) break;
			if(tp->op == IR_SCALL || tp->op == IR_FCALL) {
				sub_iv((TRIPLE*)tp->right, targ, op, inc, n);
				for(tlp = tp->right->triple.tnext;
				    tlp != (TRIPLE*)tp->right;
					tlp = tlp->tnext){
					sub_iv(tlp, targ, op, inc, n);
				}
			} else {
				sub_iv((TRIPLE*)tp->right, targ, op, inc, n);
			}
			break;
		case ISLEAF:
			if(tp->right == (IR_NODE*)targ){
				tmp = new_var(targ, (LEAF*)c, op, tp->tprev);
				tp->right = (IR_NODE*)tmp;
			}
			break;
	}
}

static void
replace_iv(n_unroll)
	int n_unroll;
{
	TRIPLE *def_tp, *tp;
	LEAF *targ, *inc;
	int n;
	IR_NODE *tmp;
	LIST *lp;
	IR_OP update_op;

	LFOR(lp, iv_def_list){
		def_tp = (TRIPLE *)lp->datap;
		targ = (LEAF *)def_tp->left;
		/* only interested in base iv */
		if(IV(targ) && IV(targ)->base_on != TNULL)
			continue;

		if(def_tp->left == def_tp->right->triple.left)
			inc = (LEAF*)def_tp->right->triple.right;
		else
			inc = (LEAF*)def_tp->right->triple.left;
		if(!ISCONST(inc)) continue;
		update_op = def_tp->right->triple.op;

		n = 0;
		for(tp = IV(targ)->def_triple; n < n_unroll; tp = tp->tnext){
			if(define_tp(tp, targ) == IR_TRUE){
				n++;
				if(n == n_unroll){
					tmp = new_var(inc, const_ileaf(inc->type,n_unroll), IR_MULT, tp->tprev);
					if(targ == (LEAF*)tp->right->triple.left){
						tp->right->triple.right = (IR_NODE*)tmp;
					} else {
						tp->right->triple.left = (IR_NODE*)tmp;
					}
					break;
				}
				delete_triple(IR_FALSE, tp, body);
			} else if(ISOP(tp->op, ROOT_OP)){
				(void)sub_iv(tp, targ, update_op, inc, n);
			}
		}
	}
}

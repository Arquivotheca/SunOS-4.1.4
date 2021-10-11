#ifndef lint
static	char sccsid[] = "@(#)optim2.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#include "cpass2.h"

extern int force_used;		/* 0 => function returns nothing */

static  void 
mulshift( amount, regname )
    char *regname;
{
    printf( "	sll	%s,%d,%s\n", regname, amount, regname );
}

/*
 * determine how many instructions we will need to do
 * multiply by this constant in line. 
 * Cost estimator is: # single bits + 2* # of multi bit
 * runs. If this exceeds 5, then it is more compact to use a 
 * multiply instruction or routine.
 *
 * This routine is called by hardops() in util2.c. It belongs here
 * to be near the conmul routine, of which it is conceptually a part.
 */
int
can_conmul( cnst )
	long cnst;
{
	int run, nbits;
	if (cnst<0){
	    cnst = - cnst;
	}
	run = cnst & (cnst>>1); /* each run shortened by one bit */
	run = cntbits( run ^ (run>>1) ) + (run&1); /* 2*# multibit runs */
	nbits = cntbits( cnst ^ (cnst>>1)) +(cnst&1); /* 2* #total runs*/
	run += nbits; /* 2* estimator */
	/* if the 2*estimator > 10, then should call multiply routine */
	return (run>10)?0:1;
}

void
genimmediate(v, reg)
	int v, reg;
{
	char * r = rnames[reg];
	if (IN_IMMEDIATE_RANGE(v)){
		printf("	mov	%d,%s\n", v, r);
	} else {
		printf("	sethi	%%hi(%d),%s\n", v, r);
		printf("	add	%s,%%lo(%d),%s\n", r, v, r);
	}
}

/*
 * Multiply a value by a constant.
 * Multiplies are slow on the Sparc, so do some shifting and
 * adding here.
 * For each 1-bit in the constant the value is shifted
 * by n where 2**n equals the bit.
 * The shifted value is then added into an accumulated value.
 * One tricky part is for runs of three or more consecutive
 * bits.  For a number like 7, it is cheaper to calculate
 * (8 - 1), than it is (3 + 2 + 1)
 */
conmul(p, cookie)
	register NODE	*p;
{
	register cnst;		/* The constant value */
	register int	curpos;	/* Position of last one bit */
	register int	power;	/* The bit being tested */
	int	run;		/* Num of consecutive one bits */
	int	nbits;		/* Num of bits since the last one bit */
	int	negconst;	/* Is the constant <0 ? */
	int	negmaxint;	/* Is abs(cnst) representable in 2's comp? */
	char *  destreg;	/* name of "AL" register */
	char *  helper;		/* name of "A1" register */

	helper = rnames[ resc[0].tn.rval ];
	cnst = p->in.right->tn.lval;
	negmaxint = 0;
	if (cnst<0){
	    negconst = 1;
	    cnst = - cnst;
	    if (cnst < 0) {
		/* abs(cnst) not representable in 2's complement */
		negmaxint = 1;
	    }
	} else 
	    negconst = 0;
	nbits = ffs( cnst )-1;	
	power = 0;
	destreg  = rnames[ p->in.left->tn.rval];
	if (negmaxint){
	    mulshift( nbits, destreg );
	    return;
	}
	if (negconst){
	    expand( p->in.left, INAREG|INTAREG, "	sub	%g0,AL,AL\n");
	}
	if (nbits>0){
	    mulshift( nbits, destreg );
	    cnst >>= nbits;
	}
	if (cnst==1) return;
	/* 
	 * the first time is special. If it is a run,
	 * then we move, negate, shift add. Otherwise, we don't negate.
	 */
	run = ffs( ~cnst) -1;
	expand(p, cookie, "	mov	AL,A1\n");
	switch (run){
	case 2:
		mulshift( 1, helper );
		expand( p, cookie, "\tadd\tAL,A1,AL\n");
		/* fall through */
	case 1: 
		curpos = 0;
		break;
	default:
		mulshift( run, helper );
		expand( p, INAREG|INTAREG, "	sub	A1,AL,AL\n");
		curpos = 1;
		break;
	}
	for ( cnst >>= run; cnst >>= (power=ffs(cnst))-1; cnst >>= run){
		run = ffs(~cnst)-1;
		switch(run) {
		case 1:
			mulshift( power-curpos, helper );
			expand( p, cookie, "\tadd\tAL,A1,AL\n");
			curpos = 0;
			break;
		case 2:
			mulshift( power-curpos, helper );
			expand( p, cookie, "\tadd\tAL,A1,AL\n");
			mulshift( 1, helper );
			expand( p, cookie, "\tadd\tAL,A1,AL\n");
			curpos = 0;
			break;
		default:
			mulshift( power-curpos, helper);
			expand( p, cookie, "\tsub\tAL,A1,AL\n");
			mulshift( run, helper);
			expand( p, cookie, "\tadd\tAL,A1,AL\n");
			curpos = 1;
			break;
		}
	}
}

/*
 * Divide a signed number by a positive constant power of two.
 * The code generated below computes x/N, where N=2**K, k=0..30,
 * using the algorithm
 *	div_N(int x) {
 *	    return ( x + (x < 0 ? N-1 : 0) ) >> K;
 *	}
 * where >> is an arithmetic right shift, and the correction N-1
 * is added before shifting in the case (x < 0).  This may be computed
 * on machines where shifts are single cycle and branches are not, as
 * follows:
 *	div_N(int x) {
 *	    int adjust = (x >> (K-1));
 *	    adjust = ((unsigned) adjust) >> (31 - (K-1));
 *	    return (x + adjust) >> K;
 *	}
 */
void
condiv( p, cookie ) register NODE *p;
{
    register cnst;	/* divisor N = (2**k) */
    int shiftcount;	/* shift count = k */
    int lab1f;
    char * lname = rnames[p->in.left->tn.rval];
    char * tname = rnames[resc[0].tn.rval]; 

    cnst = p->in.right->tn.lval;
    shiftcount = ffs(cnst)-1;
    if (shiftcount > 1) {
	printf("	sra	%s,%d,%s\n", lname, shiftcount-1, tname );
	printf("	srl	%s,%d,%s\n", tname, 31-(shiftcount-1), tname );
    } else {
	printf("	srl	%s,%d,%s\n", lname, 31-(shiftcount-1), tname );
    }
    printf("	add	%s,%s,%s\n", tname, lname, lname );
    printf("	sra	%s,%d,%s\n", lname, shiftcount, lname );
}


/*
 * Take the remainder of an integer value divided by a positive power
 * of 2.   For signed integers, the algorithm is
 *	rem_N(int x) {
 *	    return x - (div_N(x) * N);
 *	}
 * where N is a positive power of 2, and div_N(x) is computed as above.
 * For unsigned integers, the remainder is computed by masking with N-1.
 */
void
conrem( p, cookie ) register NODE *p;
{
    register cnst;	/* divisor N = (2**k) */
    int shiftcount;	/* shift count = k */
    char *lname;	/* string form of lhs */
    char *tname;	/* string form of temp operand, which varies */
    char const_str[8];	/* for constant value [used only in unsigned case] */

    lname = rnames[p->in.left->tn.rval];
    cnst = p->in.right->tn.lval;
    if (ISUNSIGNED(p->in.left->in.type)) {
	/*
	 * unsigned case: return (x & (N-1));
	 */
	if (IN_IMMEDIATE_RANGE(cnst-1)) {
	    sprintf(const_str, "%d", cnst-1);
	    tname = const_str;
	} else {
	    genimmediate( cnst-1, resc[0].tn.rval );
	    tname = rnames[resc[0].tn.rval];
	}
	printf("	and	%s,%s,%s\n", lname, tname, lname);
    } else {
	/*
	 * signed case:  return x - (div_N(x) * N);
	 */
	shiftcount = ffs(cnst)-1;
	tname = rnames[resc[0].tn.rval];
	if (shiftcount > 1) {
	    printf("	sra	%s,%d,%s\n", lname, shiftcount-1, tname );
	    printf("	srl	%s,%d,%s\n", tname, 31-(shiftcount-1), tname );
	} else {
	    printf("	srl	%s,%d,%s\n", lname, 31-(shiftcount-1), tname );
	}
	printf("	add	%s,%s,%s\n", tname, lname, tname );
	printf("	sra	%s,%d,%s\n", tname, shiftcount, tname );
	printf("	sll	%s,%d,%s\n", tname, shiftcount, tname );
	printf("	sub	%s,%s,%s\n", lname, tname, lname );
    }
}

optim2r( p, level )
	register NODE *p; 
{
	register NODE *q;
	register NODE *r;
	register long v;
	register op;
	int w;
	int lsize, rsize;
	unsigned fsize, fshf;
	NODE *rl, *rr;
	TWORD t;

	/*
	 * reduction of strength on integer constant operands
	 * this is a practical, fruitful area of peephole code optimization,
	 * since there are so many easy things that can be done.
	 * we also check for possible single-o OREGs in a place where
	 * the oreg2() should but does not.
	 */
	/*
	 * we are keeping track of our level so that we can tell things
	 * that are for effect only (i.e. top level nodes) from things
	 * that are conceivably for value. We BELIEVE that assignments
	 * of little things in memory, for effect, can be done without
	 * formally changing the type of the RHS. This might be more
	 * appropriately in setasg(), when we can look at the cookie involved.
	 * I prefer not to muck up that routine.
	 */
	op = p->in.op;
	switch (optype(op)) {
	case LTYPE:
		break;
	case UTYPE:
		optim2r(p->in.left, level+1);
		break;
	case BITYPE:
		optim2r(p->in.left,  level+1);
		optim2r(p->in.right, level+1);
		break;
	}
	switch (op){
	case NAME:
		/*
		 * stupid but true: NAMEs are best rewritten as UNARY MULs
		 * over ICONS. Works with offstar.
		 */
		if (p->tn.rval == 0){
			if (p->tn.name[0] == '\0' && IN_IMMEDIATE_RANGE(p->tn.lval)){
				p->tn.op = OREG;
				p->tn.rval = RG0;
			} 
			/* else {
			/* 	rl = tcopy( p );
			/* 	p->in.op = UNARY MUL;
			/* 	rl->tn.type = INCREF(rl->tn.type);
			/* 	rl->tn.op = ICON;
			/* 	p->in.left = rl;
			/* }
			*/
		}
		break;

	case LS:
	case ASG LS:
	case RS:
	case ASG RS:
		/* one interesting case: <lhs> <op> 0  */
		if ((r=p->in.right)->tn.op != ICON ) break;
		if (r->tn.lval == 0 && r->tn.name[0] == '\0' ){
promoteleft:
		    t = p->in.type;
		    q = p->in.left;
		    *p = *q; /* paint over */
		    p->in.type = t;
		    q->in.op = r->in.op = FREE;
		}
		/* could test for count of 1, too, but this is easier to do later on */
		break;


	case PLUS:
	case ASG PLUS:
	case MINUS:
	case ASG MINUS:
		/*
		 * one interesting cases: <lhs> <op> 0.
		 * <address reg> + SCONV( <short> ) is a good one, too,
		 * but is best done later on.
		 */
		if (((r=p->in.right)->tn.op == ICON
		    && r->tn.lval == 0 && r->tn.name[0] == '\0' )
		    || (r->tn.op == FCON && r->fpn.dval == 0.0 )) {
			goto promoteleft;
		}
		break;

	case MUL:
		/*
		 * put constants on the right
		 */
		if (p->in.left->in.op == ICON && p->in.right->in.op != ICON) {
			r = p->in.right;
			p->in.right = p->in.left;
			p->in.left = r;
		}
		/* fall through */

	case ASG MUL:
	case DIV:
	case ASG DIV:
	case MOD:
	case ASG MOD:
		/*
		 * two interesting case: <lhs> <op> 1
		 * and: small multiplies that can be handled by 
		 *      the feeble instructions.
		 */
		if (((r=p->in.right)->tn.op == ICON
		    && r->tn.lval == 1 && r->tn.name[0] == '\0')
		    || (r->tn.op == FCON && r->fpn.dval == 1.0 )) {
			switch (op){
			case MOD:
			case ASG MOD:
			    /*
			     * x%1 == 0
			     * BUT x might have side effects, so...
			     * x%1 ==> (x,0)
			     */
			    p->in.op = COMOP;
			    if (r->tn.op==FCON)
				r->fpn.dval = 0.0;
			    else
				r->tn.lval = 0;
			    break;
			default:
			    goto promoteleft;
			}
			break;
		}
		if (op == DIV || op == ASG DIV){
		    /*
		     * a very particular case: a difference of two
		     * pointers, divided by a power of two should
		     * always give an integral result (this is
		     * pointer subtraction, with the implied divide
		     * by sizeof( *p ) ). So, we can change this
		     * into a shift, if we ever see it.
		     * AND...
		     * a less peculiar case: unsigned divide by a power of 2.
		     */
		    if (
		       (p->in.type == UNSIGNED && r->in.op ==ICON  )
		    || ((p->in.type == INT || p->in.type == UNSIGNED) 
			&& r->in.op == ICON && (q = p->in.left)->in.op == MINUS
			&& ISPTR(q->in.left->in.type)
			&& ISPTR(q->in.right->in.type))
		    ){
			if (cntbits( v=r->tn.lval )==1 && r->in.name[0]=='\0' ){
			    /* well, looks like we found one */
			    w = ffs( v ) - 1;
			    p->in.op = op += RS - DIV; /* keep ASG if present */
			    r->tn.lval = w;
			    break;
			}
		    }
		}
		/* the other good case is multiply by a power of two,
			and thats already being handled in the front end */
		break;
		
	case FLD:
		/*
		 * Bit fields have been "optimized" to allow packing them in
		 * rather tightly with non-field elements. Here we'll weaken
		 * the underlying type, to allow for shorter, and less-well-
		 * aligned accesses.
		 *
		 * We still have to leave it as a field, in order to avoid
		 * fouling up operand type compatibility.  Suitably aligned
		 * and sized fields are dealt with differently depending on
		 * whether we are in an rvalue or an lvalue context.  Rvalues
		 * are dealt with in rewfld(); lvalues are treated as special
		 * cases (SFLD8, SFLD16) in table.c.
		 */
		q = p->in.left;
		/* the type of p is unsigned. the type of q determines the access */
		t = q->in.type;
		v = p->tn.rval; /* as in tshape */
		fsize = UPKFSZ(v);
		fshf = UPKFOFF(v);/* different from tshape */
		/*
		 * fields that fill a whole long word must be handled
		 * as a special case, lest we err by depending on the
		 * unpredictable result of shifting by wordsize.
		 */
		if (fsize == SZINT) {
			*p = *q;
			p->in.type = UNSIGNED;
			q->in.op = FREE;
			break;
		}
		/*
		 * the rule is: if the field falls entirely within one X,
		 * we can change the access type to "unsigned X"
		 */
		 switch(  ((fshf+fsize-1)/SZCHAR) - (fshf/SZCHAR) ){
		 case 0 : /* same byte */
		 	t = UCHAR;
		 	incraddr( q, fshf/SZCHAR);
		 	fshf %= SZCHAR;
		 	break;
		 case 1 : /* ajacent bytes */
		 	/* but does it cross a halfword boundary? */
		 	switch( fshf/SZCHAR ){
		 	case 0:
		 	case 2: /* no, its a halfword access */
		 		t = USHORT;
		 		incraddr( q, fshf/SZCHAR);
		 		fshf %= SZSHORT;
		 		break;
		 	default: /* sorry, its still a long access */
		 		break;
		 	}
		 	break;
		 default: /* a 3- or 4-byte field is always long */
		 	break;
		 }
		 q->in.type = t;
		 p->tn.rval = PKFIELD( fsize, fshf );
		 break;
		 
	case ASSIGN:
		if ( level == 0 ){
			/*
			 * look for assignment of small types, with the
			 * RHS an SCONV from a larger, integral, type.
			 * The assignment must be for effect only, as the
			 * resulting value will be wrong. The small thing
			 * being assigned to must also be in memory.
			 * This optimization
			 * is very suspect.
			 */
			if (ttype(p->in.type, TCHAR|TUCHAR|TSHORT|TUSHORT)
			&& (r=p->in.right)->in.op== SCONV 
			&& !ISFLOATING((rl=r->in.left)->tn.type)
			&& rl->in.op != SCONV
			&& tlen(p) <= tlen(rl)
			&& p->in.left->tn.op != REG ){
				/* clobber SCONV */
				p->in.right = rl;
				r->in.op = FREE;
				break;
			}
		}
		/*
		 * translate (x = x op y) to (x op= y),
		 * reversing a translation done in the
		 * front end for mapping to IR.
		 */
		q = p->in.left;
		r = p->in.right;
		switch(r->in.op) {
		case PLUS:
		case MINUS:
		case AND:
		case OR:
		case ER:
		case MUL:
		case DIV:
			if (equal(q, r->in.left)) {
				tfree(r->in.left);
				p->in.op = ASG r->in.op;
				p->in.right = r->in.right;
				r->in.op = FREE;
			} else if (commuteop(r->in.op)
			    && equal(q, r->in.right)) {
				tfree(r->in.right);
				p->in.op = ASG r->in.op;
				p->in.right = r->in.left;
				r->in.op = FREE;
			}
		}
		break;

	case PCONV:
		/*
		 * pointer-pointer conversions are no-ops on this
		 * machine.  The operand merely inherits the type.
		 * PCONV was formerly stripped out in clocal(), but
		 * it is now retained until code generation time
		 * in order to keep iropt happy.
		 */
		q = p->in.left;
		t = p->in.type;
		if (tlen(p) != tlen(q)) {
		    cerror("operand of PCONV must be same size as target type");
		}
		*p = *q;
		q->in.op = FREE;
		p->in.type = t;
		break;

	case AND:
	case OR:
	case ER:
		/*
		 * if we have a complement on the left, exchange
		 * operands, the better to exploit {andn,orn,xnor}
		 */
		q = p->in.left;
		r = p->in.right;
		if (q->in.op == COMPL && r->in.op != COMPL) {
			p->in.right = q;
			p->in.left = r;
		}
		break;

	case COMPL:
		/*
		 * DiGiacomo's rule: ~(x^y) == (x^~y), which
		 * matches the pattern for the 'xnor' instruction.
		 */
		q = p->in.left;
		if (q->in.op == ER && p->in.type == q->in.type) {
			p->in.op = ER;
			p->in.left = q->in.left;
			q->in.left = q->in.right;
			q->in.right = NULL;
			q->in.op = COMPL;
			p->in.right = q;
		}
		break;

	case FORCE:
		/*
		 * This is a cheat to tell us if a function
		 * returns a result, which means we must generate
		 * a "mov %o0,%i0" instruction in the epilogue.
		 * If FORCE is used in other contexts, the instruction
		 * may be unnecessary, but is still safe.
		 */
		force_used = 1;
		break;

	case SCONV:
		/*
		 * the hazardous domain of redundant conversions;
		 * here we restrict ourselves to the case where
		 * operand type and result type are both integral.
		 */
		q = p->in.left;
		if (ISINTEGRAL(p->in.type) && ISINTEGRAL(q->in.type)) {
		    /*
		     * conversion of a signed int object to an unsigned int
		     * of the same size: delete the conversion and change
		     * the type of the object.  Don't do this to operators
		     * where the type determines the arithmetic to be used.
		     */
		    int compare_types = 0;
		    switch(q->in.op) {
		    case AND:
		    case UNARY AND:
		    case OR:
		    case UNARY OR:
		    case COMPL:
		    case UNARY MUL:
			compare_types = 1;
			break;
		    case FLD:
		    case REG:
			/*
			 * Remember that the upper bits of a register
			 * containing a small int *are* defined, thus
			 * conversions from small ints to small ints of
			 * different signedness are significant.
			 */
			if (q->in.type == p->in.type
			  || tlen(p) == SZINT/SZCHAR) {
			    compare_types = 1;
			}
			break;
		    default:
			if (optype(q->in.op) == LTYPE) {
			    compare_types = 1;
			}
			break;
		    }
		    if (compare_types) {
			TWORD tq;
			t = p->in.type;
			tq = q->in.type;
			if (ISUNSIGNED(t)) {
			    t = DEUNSIGN(t);
			}
			if (ISUNSIGNED(tq)) {
			    tq = DEUNSIGN(tq);
			}
			if (t == tq) {
			    /* same types except for unsignedness */
			    t = p->in.type;
			    *p = *q; /* paint over */
			    p->in.type = t;
			    q->in.op = FREE;
			    break;
			}
		    }
		    /*
		     * A widening conversion followed by a narrowing
		     * conversion to the original type is a noop.  Note
		     * that the reverse is not true -- truncation or
		     * rounding must be preserved.
		     */
		    t = p->in.type;
		    if (q->in.op == SCONV && (r=q->in.left)->in.type == t
		      && tlen(q) > tlen(r)) {
			/*
			 * p's type and r's type are the same;
			 * q's conversion and p's conversion leave
			 * r unchanged.
			 */
			*p = *r;
			q->in.op = FREE;
			r->in.op = FREE;
			break;
		    }
		} else if (p->in.type == q->in.type) {
			/*
			 * p's type and q's type are identical;
			 * the SCONV is redundant.
			 */
			*p = *q;
			q->in.op = FREE;
		}
		break;

	} /* switch */
}

/*
 * equal(p,q) : returns 1 if expressions p and q are equivalent,
 *	in the sense that they always return the same value given
 *	the same initial values of their operands.  Note that if
 *	either p or q contains operators producing side-effects,
 *	equal(p,q) = 0.
 */

int
equal(p,q)
    register NODE *p,*q;
{
    register char *pn,*qn;
    register o;

    if (p == q)
	return 1;
    if (p == NIL || q == NIL)
	return 0;
    o = p->in.op;
    if (o != q->in.op)
	return 0;
    if (p->in.type != q->in.type)
	return 0;
    switch(optype(o)) {
    case UTYPE:
	if (callop(o))
	    return 0;
	if (o == FLD && p->tn.rval != q->tn.rval)
	    return 0;
	return equal(p->in.left, q->in.left);
    case BITYPE:
	if (callop(o) || asgop(o))
	    return 0;
	if (equal(p->in.left, q->in.left))
	    return equal(p->in.right, q->in.right);
	return 0;
    default:
	/* leaf nodes */
	switch (o) {
	case ICON:
	case OREG:
	case NAME:
	    pn = p->tn.name;
	    qn = q->tn.name;
	    if (pn != NULL && qn != NULL) {
		if ( *pn == *qn 
		  && (*pn == '\0' || strcmp(pn,qn) == 0) ) {
		    return (p->tn.lval == q->tn.lval);
		}
	    } else if (pn == qn) {	/* == NULL */
		return ( p->tn.lval == q->tn.lval );
	    }
	    return 0;
	case REG:
	    return(p->tn.rval == q->tn.rval);
	case FCON:
	    return(p->fpn.dval == q->fpn.dval);
	}
    }
    return 0;
}

optim2( p ) NODE *p;
{
	optim2r( p, 0 );
}

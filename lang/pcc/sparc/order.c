#ifndef lint
static	char sccsid[] = "@(#)order.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


/*
 * order.c
 */
#include "cpass2.h"

int fltused = 0;
int floatmath = 0;

SUTYPE fregs;

extern int mina, maxa, minb, maxb; /* imported from allo.c */
extern int toff, maxtoff;

static SUTYPE zed = { 0,0 };


# define iscnode(p) ((p)->in.op==REG && iscreg((p)->tn.rval))

# define max(x,y) ((x)<(y)?(y):(x))
# define min(x,y) ((x)<(y)?(x):(y))


NODE *
double_conv( p ) register NODE *p;
{
	NODE *q    = talloc();
	q->in.op   = SCONV;
	q->in.type = DOUBLE;
	q->in.left = p;
	q->in.rall = p->in.rall;
	q->in.su.d = p->in.su.d;
	q->in.su.f = max( p->in.su.f, 2 ); /* double-floating takes 2 registers */
	return q;
}

setincr( p, cook ) register NODE *p; 
{
	int op;
	NODE *lp, *rp;
	NODE *fldexp;	/* copy of FLD expression, evaluated INAREG */
	NODE *asgexp;	/* tree for (FLD = REG + ICON) */
	TWORD t;	/* expression type */

	switch (p->in.op){
	case INCR:
		op = PLUS;
		break;
	case DECR:
		op = MINUS;
		break;
	default:
		cerror("setincr: expected {INCR,DECR}, got %s",
		    pcc_opname(p->in.op));
		/*NOTREACHED*/
	}
	if ( cook == FOREFF ){
		/*
		 * value doesn't matter: use +=, -= instead
		 */
		p->in.op = ASG op;
		return 1;
	}
	lp = p->in.left;
	rp = p->in.right;
	t = p->in.type;
	if (lp->in.op == FLD) {
		NODE *llp= lp->in.left;
		/*
		 * Make the field operand addressable.
		 */
		if (llp->in.op == NAME) {
			unname(llp);
		}
		if (llp->in.op == UNARY_MUL) {
			offstar(llp->in.left, llp->in.type);
			canon(llp);
		}
		/*
		 * copy the FLD expression and evaluate it.
		 */
		fldexp = tcopy(lp);
		order(fldexp, INAREG);
		/*
		 * compile (FLD = REG op ICON) from copies of spare parts.
		 */
		asgexp = build_in(ASSIGN, t, tcopy(lp),
			    build_in(op, t, tcopy(fldexp), tcopy(rp)));
		order(asgexp, FOREFF);
		/*
		 * recycle pieces of the original tree, and return
		 * fldexp in place of it.
		 */
		reclaim(lp, RNULL, FOREFF);
		reclaim(rp, RNULL, FOREFF);
		fldexp->in.rall = p->in.rall;
		*p = *fldexp;
		tfree(fldexp);
		return(1);
	}
	return( 0 );
}

niceuty( p ) register NODE *p; 
{
	return( p->in.op == UNARY MUL && p->in.type != FLOAT &&
		shumul( p->in.left) == STARNM );
}

setconv( p, cookie )
    NODE *p;
{
    TWORD t;
    
    t = p->in.left->tn.type;
    if (ISFLOATING( p->in.type ) ){
    	if (!ISFLOATING(t) && t != INT && t != UNSIGNED){
    	    /*
    	     * this turkey is not one of the types our machine knows
    	     * how to convert. "Convert" it into integer or unsigned
    	     * in an integer register before proceeding.
    	     */
    	    if (p->in.left->in.op != REG)
    	    	order( p->in.left, INTAREG );
    	    /* here's the "conversion" -- we just paint over the type */
    	    p->in.left->tn.type = ISUNSIGNED(t)? UNSIGNED : INT;
    	}
	order( p->in.left, INBREG|INTBREG );
    } else {
    	/*
    	 * may be converting from a floating type to a scalar type.
    	 */
	order( p->in.left, OPEREG( t ) );
    }
    return 1;
}

/*
 * deal properly with the operand of a unary operator.
 */
int
setunary(p, cookie)
	register NODE *p;
{
	NODE *lp;

	order(p->in.left, OPEREG(p->in.type) );
	return(1);
}

setbin( p, cookie ) register NODE *p; 
{
	register NODE *r, *l;
	NODE *p2, *q, *t;
	SUTYPE sur, sul;
	SUTYPE qcost, pcost;
	TWORD typ;
	extern short revrel[]; /* for logic ops */
	int  i;

	typ = p->in.type;
	if (logop(p->in.op))
	    typ = p->in.left->in.type;
	r = p->in.right;
	l = p->in.left;
	sur = r->in.su;
	sul = l->in.su;


	if (p->in.op == CHK){
	    /*
	     * special case: rhs is always a comma
	     * evaluate lhs, never rhs. We'll make that up as we go along
	     */
	    if (!istnode(l) || !isareg(l->tn.lval)){
		order( l, INAREG|INTAREG );
		return (1);
	    }
	    return 0;
	}
	/*
	 * hack for khb: x*2.0 == x+x, saving
	 * a constant memory reference and cheapening
	 * the multiply to an add.
	 */
	i = 0;
	if (p->in.op == MUL
	    && r->in.op == FCON
	    && r->fpn.dval == 2.0) {
		if (l->in.op != REG) {
			order( l, OPEREG(typ) );
		}
		r->in.op = FREE;
		p->in.right = tcopy(l);
		p->in.op = PLUS;
		return(1);
	}
	if (SUGT( sul, sur)){
		order( l, OPEREG(typ) );
		i = 1;
	}
	if (SUTEST( sur )){
		if (simpleop(p->in.op) && r->in.op == COMPL) {
		    /*
		     * check for logical op with negation
		     */
		    if (r->in.left->in.op != REG
		    || !isareg(r->in.left->tn.rval)) {
			order( r->in.left, OPEREG(typ) );
			i = 1;
		    } else {
			/* assert(tshape(r, SCOMPLR)); */
		    }
		} else {
		    order( r, OPEREG(typ) );
		    i = 1;
		}
	}
	if ( l->tn.op != REG ){
		if (logop(p->in.op)
		    && l->in.op == ICON
		    && l->tn.name[0] == '\0'
		    && IN_IMMEDIATE_RANGE(l->tn.lval)) {
			/* put constants on the right */
			p->in.left = p->in.right;
			p->in.right = l;
			p->in.op = revrel[p->in.op-EQ];
			l = p->in.left;
			r = p->in.right;
		} else {
			order( l, OPEREG(typ) );
		}
		i = 1;
	}
	switch ( r->tn.op ){
	case REG:
		break;
	case ICON:
		if (r->tn.name[0] == '\0' && IN_IMMEDIATE_RANGE(r->tn.lval))
			break;
		else if (p->in.op == MUL || p->in.op == DIV || p->in.op == MOD )
		    break;
		/* ELSE FALL THROUGH */
	default:
		if (simpleop(p->in.op) && r->in.op == COMPL
		&& r->in.left->in.op == REG
		&& isareg(r->in.left->tn.rval)) {
			/* assert(tshape(r, SCOMPLR)); */
			break;
		}
		order( r, OPEREG(typ) );
		i = 1;
	}
	if (i == 0){
	    /*
	     * look -- the next thing we do is turn this into an op=
	     * sort of thing. make sure the left operand is a temp reg.
	     */
	    if (!tshape( l, OPEREG(typ)&(INTAREG|INTBREG) )){
		order(l, OPEREG(typ)&(INTAREG|INTBREG) );
	    }
	}
	return i;
}

setstr( p, cook ) register NODE *p; 
{ 
	/* structure assignment */
#	define MAXCOST 0
#	define MINCOST 1
	NODE * subtree[MINCOST+1];
	int i, nfixed;

	nfixed = 0;
	switch ( p->in.op ){
	case STASG:
	    /* find most expensive subtree. */
	    if (SUGT( p->in.right->in.su, p->in.left->in.su)){
		subtree[MAXCOST] = p->in.right;
		subtree[MINCOST] = p->in.left;
	    } else {
		subtree[MAXCOST] = p->in.left;
		subtree[MINCOST] = p->in.right;
	    }
	    /* now do them in this order */
	    for ( i = MAXCOST; i <= MINCOST; i += 1 ){
		if (!tshape( subtree[i], INTAREG) ){
			order( subtree[i], INTAREG );
			nfixed += 1;
		} 
	    }
	    break;
	case STARG:
	    if (!tshape( p->in.left, INTAREG) ){
		    order( p->in.left, INTAREG );
		    nfixed += 1;
	    }
	    break;
	default:
		cerror("setstr called accidentally");
	}
#ifdef FORT
	/* Pascal does struct assignment of things that ain't structs!! */
	if (ISARY(p->in.type)){
	    p->in.type = STRTY;
	    nfixed += 1;
	}
#endif
#	undef MAXCOST
#	undef MINCOST
	return( nfixed!=0 );
}

/*
 * double-indexed OREGs can cause trouble if we need to
 * deal with them piecewise, because on SPARC, there is no
 * base+index+OFFSET addressing.  So we occasionally have
 * to change them into single-indexed OREGs.
 */

static NODE *
make_single_oreg( p )
    NODE *p;
{
    int basereg, indexreg;
    NODE *basenode, *indexnode;
    TWORD t, ptr_t;

    basereg = R2UPK1(p->tn.rval);
    indexreg = R2UPK2(p->tn.rval);
    t = p->in.type;
    ptr_t = INCREF(t);
    reclaim(p, RNULL, 0);
    basenode = build_tn(REG, ptr_t, "", 0, basereg);
    indexnode = build_tn(REG, INT, "", 0, indexreg);
    rbusy(basereg, ptr_t);
    rbusy(indexreg, INT);
    p = build_in(PLUS, ptr_t, basenode, indexnode);
    order(p, INAREG);
    p->in.op = OREG;
    p->in.type = t;
    return(p);
}


setasg( p , cookie ) 
	register NODE *p; 
{
	/* setup for assignment operator */
	register NODE *r, *l;
	SUTYPE sur, sul;
	int i, v, s;
		/* if the cookie expresses any useful preference, use it */
	int usecook = (cookie&(INAREG|INTAREG|INBREG|INTBREG))?
		      cookie:(INAREG|INTAREG|INBREG|INTBREG);

	r = p->in.right;
	l = p->in.left;
	sur = r->in.su;
	sul = l->in.su;
	i = 0;

	if (SUGT( sur, sul)){
		if (l->in.op == REG && ISFLOATING(l->in.type) && 
		    r->in.op == UNARY MUL) {
			offstar( r->in.left, r->in.type );
		} else {
			order( r, usecook);
		}
		return(1);
	}
	if (SUTEST( sul )){
		if (l->in.op == (UNARY MUL) ){
			offstar( l->in.left, l->in.type );
			i = 1;
		}  else if (l->in.op==FLD){
			if ( l->in.left->in.op==UNARY MUL ){
				NODE *ll = l->in.left;
				offstar( ll->in.left, ll->in.type );
				i = 1;
			} else if ( !tshape( l->in.left, MEMADDR ) ){
				/*
				 * a field that is not a UNARY MUL and
				 * is not an OREG. Must be a name, or maybe
				 * a special name. In any case, we have to
				 * take its address into a register, then
				 * turn it into a unary mul.
				 */
				unname( l->in.left );
				i = 1;
			}
		}
	}
	if ( l->tn.op == FLD && r->tn.op == ICON && r->tn.name[0] == '\0'){
		v = r->tn.lval;
		s = UPKFSZ( l->tn.rval );
		if ( v == -1 || v == (1<<s)-1 || v == 0 ){
			/*
			 * There are two values we can force into a field
			 * without much work: all ones and all zeros. The former
			 * has two common aliases: -1 and 2**n-1, e.g. 7 for a 
			 * 3-bit field. Discover these cases and avoid 
			 * evaluating the constant into a register.
			 */
			return i;
		}
	} /* otherwise, look for the more general case of 0 on the rhs */
	if ( r->tn.op != REG && 
	    !( r->tn.op == ICON && r->tn.name[0] == '\0' && r->tn.lval == 0)){
		order( r, usecook );
		i = 1;
	}
	/* FPU registers can't be stored into partword
	   memory locations.  CPU registers can. */
	if ( r->tn.op == REG && isbreg(r->tn.rval)
	    && ISINTEGRAL(l->tn.type) && tlen(l) < SZINT/SZCHAR ) {
		order( r, INAREG );
		i = 1;
	}
	/*
	 * if we're trying to store an odd register pair into a
	 * double-word double-oreg, make the double-oreg into
	 * a single-oreg
	 */
	if (r->in.op == REG
	    && szty(l->in.type) > 1
	    && l->in.op == OREG && R2TEST(l->tn.rval)) {
		p->in.left = make_single_oreg(l);
		i = 1;
	}
	return i;
}
			



setasop( p, cookie ) register NODE *p; 
{
	/* setup for =ops */
	register SUTYPE sul, sur;
	register NODE *r, *l;
	register NODE *q, *p2;
	SUTYPE pcost;
	int i;

	r = p->in.right;
	l = p->in.left;
	sul = l->in.su;
	sur = r->in.su;
	i =0;

	if (l->tn.op != REG) {
	    if (!SUGT( sur, sul )){
		if (l->tn.op == UNARY MUL){
		    offstar( l->in.left, l->in.type );
		    return 1;
		} else if (l->in.op == FLD &&
		    l->in.left->in.op == UNARY MUL){
			NODE *ll = l->in.left;
			offstar(ll->in.left, ll->in.type);
			return 1;
		}
	    }
	}
	switch ( r->tn.op ){
	case REG:
		break;
	case ICON:
		if (r->tn.name[0] == '\0' && IN_IMMEDIATE_RANGE(r->tn.lval))
			break;
		else if (p->in.op == ASG MUL || p->in.op == ASG DIV
		    || p->in.op == ASG MOD )
			break;
		/* ELSE FALL THROUGH */
	default:
		if (simpleop(p->in.op) && r->in.op == COMPL) {
		    /*
		     * check for logical op with negation
		     */
		    if (r->in.left->in.op == REG
		    && isareg(r->in.left->tn.rval)) {
			/* assert(tshape(r, SCOMPLR); */
		    } else {
			order( r->in.left, INTAREG );
			i = 1;
		    }
		    break;
		}
		order( r, ISFLOATING(p->in.type)?INTBREG:INTAREG );
		i = 1;
	}
	return i;

}

/*
 * stupid but true: NAMEs are sometimes best rewritten as UNARY MULs
 * over ICONS. For instance, as the operand of a FLD node. Called from
 * setasg.
 */
unname( p )
    NODE *p;
{
	NODE *pnew;
	TWORD t;
	pnew = tcopy( p );
	incref( pnew );
	p->in.op = UNARY MUL;
	p->in.left = pnew;
}

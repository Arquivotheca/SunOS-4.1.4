#ifndef lint
static	char sccsid[] = "@(#)su.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


# include "cpass2.h"


extern int mina, maxa, minb, maxb; /* imported from allo.c */
int failsafe;
extern toff, maxtoff;

# define iscnode(p) (p->in.op==REG && iscreg(p->tn.rval))

# define max(x,y) ((x)<(y)?(y):(x))
# define min(x,y) ((x)<(y)?(x):(y))

static SUTYPE zed = { 0,0 };

extern int chk_ovfl;


sucomp( p ) register NODE *p; 
{

	/* set the su field in the node to the sethi-ullman
	   number, or local equivalent */

	register o, t;
	register nr;
	register u, tt;
	register NODE *r, *l;
	int /* bool actually */ is_floating_op;
	SUTYPE sul, sur;
	SUTYPE tempsu;

	p->in.su = zed;
	switch(t = optype( o=p->in.op)) {
	case LTYPE:
		break;
	case UTYPE:
		sucomp(p->in.left);
		break;
	case BITYPE:
		sucomp(p->in.left);
		sucomp(p->in.right);
		r = p->in.right;
		sur = r->in.su ;
		break;
	}
	nr = szty( tt = p->in.type );
	is_floating_op = (tt==DOUBLE) || (tt==FLOAT);
	if (t != LTYPE) {
		l = p->in.left;
		sul = l->in.su;
	}
	switch ( o ){
	
	case ASSIGN:
		/* usually, compute rhs into a register, compute lhs, store */
		/* could do it backwards if lhs is hairier		    */
		if (is_floating_op){
		    if (SUSUM( sur ) > SUSUM( sul )){
			sur.f = max( sur.f, sul.f+nr );
		    }else{
			sur.f = max( sur.f+1, sul.f);
		    }
		    sur.d = max( sur.d, sul.d);
		} else {
		    if (SUSUM( sur ) > SUSUM( sul )){
			sur.d = max( sur.d, sul.d+nr );
		    }else{
			sur.d = max( sur.d+1, sul.d);
		    }
		    if (l->in.op == FLD) {
			/*
			 * 1-2 for fld address,
			 * 1 for fld value,
			 * 3 to do the assignment
			 */
			if (l->in.left->in.op == UNARY MUL
			  && l->in.left->in.left->in.op == PLUS) {
			    sur.d = max(sur.d, 6);
			} else {
			    sur.d = max(sur.d, 5);
			}
		    }
		    sur.f = max( sur.f, sul.f);
		}
		p->in.su = sur;
		return;

	case STASG:
		/* compute two addresses, then use up to four more registers */
		/* after calculate the max tree, need 1 reg to hold the result*/
		t = max( max( sur.d, sul.d ), 1 + min( sur.d, sul.d ) );
		sur.d = max( 4, t );
		sur.f = max(sur.f, sul.f);
		p->in.su = sur;
		return;

	case UNARY MUL:
		if (shumul(p->in.left))
			return;
		/* most other unary ops handled in default case */
		sul.d = max( sul.d, 1); /* 1 address register to * through */
		sur.f = max(sur.f, sul.f);
		p->in.su = sul;
		return;

	case CALL:
	case UNARY CALL:
	case STCALL:
	case UNARY STCALL:
		p->in.su = fregs;
		return;


	case ASG PLUS:
	case ASG MINUS:
	case ASG AND:
	case ASG ER:
	case ASG OR:
	case ASG RS:
	case ASG LS:
		/* if the rhs is not a small contant, this is a reg-reg op */
		if ( r->tn.op == ICON && IN_IMMEDIATE_RANGE( r->tn.lval ) ){
			/* looks like a unary op */
			if (l->in.op == FLD) {
				/*
				 * 1-2 for fld address,
				 * 1 for fld value,
				 * 3 to do the assignment
				 */
				if (l->in.left->in.op == UNARY MUL
				  && l->in.left->in.left->in.op == PLUS) {
				    sul.d = max(sul.d, 6);
				} else {
				    sul.d = max(sul.d, 5);
				}
			} else {
				sul.d += nr;
			}
			p->in.su = sul;
			return;
		}
		/* FALL THROUGH */
	case ASG MUL:
	case ASG MOD:
	case ASG DIV:
		if ( l->in.op == FLD ) {
			/*
			 * 1-2 for fld address,
			 * 1 for fld value,
			 * 3 to do the assignment
			 */
			if (l->in.left->in.op == UNARY MUL
			  && l->in.left->in.left->in.op == PLUS) {
			    sul.d = max(sul.d, 6);
			} else {
			    sul.d = max(sul.d, 5);
			}
		}
	/* compute rhs, compute lhs (or vice versa), get lhs value, op */
reg_reg_asg:
		t = SUSUM(sul)?1:0; /* number regs needed to store lhs address */
		/* want at least 2 regs + lhs address */
		/* if lhs is complicated, will always compute it into a reg FIRST */
		/* p->in.su = max( t+2*nr, min(t+sur, nr+sul)); */
/*		if (sul)
/*		    u = max( sul, t+sur );
/*		else
/*		    u = sur;
/*		p->in.su = max( u, t+2*nr);
*/
		if (SUSUM(sul)){
		    sur.d = max( sur.d+t, sul.d );
		} 
		if (is_floating_op){
		    sur.f = max( sur.f, sul.f );
		    sur.f = max( sur.f, 2*nr  );
		} else {
		    sur.d = max( sur.d , 2*nr );
		    sur.f = max( sur.f, sul.f );
		}
		p->in.su = sur;
		return;

	case EQ:
	case NE:
	case LE:
	case GE:
	case GT:
	case LT:
	case UGT:
	case UGE:
	case ULT:
	case ULE:
		    tt = l->in.type;
		    nr = szty( tt );
		    is_floating_op = (tt==DOUBLE) || (tt==FLOAT);
		    /* FALL THROUGH */
	case PLUS:
	case AND:
	case OR:
	case ER:
	case MINUS:
		goto reg_reg_op;
	case INCR:
	case DECR:
		if (l->in.op == FLD) {
			/*
			 * 1-2 for fld addr,
			 * 1 for fld value,
			 * 1 for fld value [+-] 1,
			 * 3 to do the assignment
			 */
			if (l->in.left->in.op == UNARY MUL
			  && l->in.left->in.left->in.op == PLUS) {
			    sul.d = max(sul.d, 7);
			} else {
			    sul.d = max(sul.d, 6);
			}
		}
		goto reg_reg_op;
	case RS:
	case LS:
		/* looks like a unary op if rhs is small */
		if ( r->tn.op == ICON && IN_IMMEDIATE_RANGE( r->tn.lval ) ){
			/* looks like a unary op */
			sul.d = max( sul.d, nr);
			p->in.su = sul;
			return;
		}
		/* FALL THROUGH */
	case MUL:
	case DIV:
	case MOD:
reg_reg_op:
		/* register-register ops */
		/* must do one and then the other */
		/* commutability not an issue here */
		/* need at least two register sets */
		if (is_floating_op){
			if ( SUGT( sul, sur ) )
				t = max( sul.f, sur.f+nr );
			else 
				t = max( sul.f+nr, sur.f );
			sul.f = max( 2*nr, t);
			sul.d = max( sul.d, sur.d );
		} else {
			if ( SUGT( sul, sur ) ){
				t = max( sul.d, sur.d+nr );
			} else {
				t = max( sul.d+nr, sur.d );
			}
			sul.d = max( 2*nr, t);
			sul.f = max( sul.f, sur.f );
		}
		p->in.su = sul;
		return;

	case CHK:
		/*
		 * this is actually a ternary operation:
		 *     the expression to be checked,
		 *     the lower bound,
		 *     the upper bound.
		 * If the upper and lower bounds are constants, then
		 * cost is the cost of the expression, which must 
		 * always be evaluated into an integer register. We
		 * may have to evaluate the bounds, too. This costs more.
		 */
		if (chk_ovfl){
		    sur.d = max( r->in.left->in.su.d, r->in.right->in.su.d);
		    sur.f = max( r->in.left->in.su.f, r->in.right->in.su.f);
		    p->in.su.d = max( sul.d, nr+sur.d);
		    p->in.su.f = max( sul.f, sur.f );
		} else {
		    p->in.su = sul;
		}
		break;

	case NAME:
	case REG:
	case OREG:
	case ICON:
		return; /* su is zero */

	case FCON:
		/*
		 * A floating point constant is stored in a labeled
		 * constant pool.  You need 1 or 2 BREGS to hold it,
		 * and an AREG to reach it.  Note that the "needs" field
		 * of the optab structure now has a new bit (NAPAIR)
		 * to distinguish cases like this from the ones that
		 * actually require an even-odd pair of AREGs.
		 */
		p->in.su.d = 1;
		p->in.su.f = nr;
		return;

	case SCONV:
		if (is_floating_op || ISFLOATING(l->in.type)) {
			/* floating conversion -- uses fp reg */
			if (ISUNSIGNED(l->in.type))
				nr *= 2;
			sul.f = max( sul.f, nr );
		} else {
			sul.d = max( sul.d, nr); /* normal unary */
		}
		p->in.su = sul;
		break;

	default:
		if ( t == BITYPE ){
			/* random binary operators */
			/* choose largest */
			t = max( sur.d, sul.d );
			sur.d = max( t, nr );
			sur.f = max( sur.f, sul.f );
			p->in.su = sur;
			return;
		}
		/* must be unary */
		if (dope[o]&INTRFLG) {
			/* fortran intrinsics */
			/* i.e., CISC-style floating point ops */
			sul.d = max(sul.d, nr);
			if (ISFLOATING(tt)) {
				sul.f = max(sul.f, nr);
			}
		} else {
			/* random unary op */
			sul.d = max( sul.d, nr);
			if (ISFLOATING(tt)) {
				sul.f = max( sul.f, nr);
			}
		}
		p->in.su = sul;
	}
}

int radebug = 0;

mkrall( p, r ) register NODE *p; 
{
	/* insure that the use of p gets done with register r; in effect, */
	/* simulate offstar */

	if( p->in.op == FLD ){
		p->in.left->in.rall = p->in.rall;
		p = p->in.left;
	}

	if( p->in.op != UNARY MUL ) return;  /* no more to do */
	p = p->in.left;
	if( p->in.op == UNARY MUL ){
		p->in.rall = r;
		p = p->in.left;
	}
	if( p->in.op == PLUS && p->in.right->in.op == ICON ){
		p->in.rall = r;
		p = p->in.left;
	}
	rallo( p, r );
}

rallo( p, down ) NODE *p; 
{
	/* do register allocation */
	register o, type, down1, down2, ty;
	NODE *p2;

again:
	if( radebug ) printf( "rallo( %o, %d )\n", p, down );

	down2 = NOPREF;
	p->in.rall = down;
	down1 = ( down &= ~MUSTDO );

	ty = optype( o = p->in.op );
	type = p->in.type;


	switch( o ) {
	case STARG:
	case UNARY MUL:
		down1 = NOPREF;
		break;

	case ASSIGN:	
		down1 = NOPREF;
		down2 = down;
		break;

	case NOT:
	case ANDAND:
	case OROR:
		down1 = NOPREF;
		break;

	case FORCE:	
		if (type == FLOAT || type == DOUBLE )
		    down1 = FR0|MUSTDO;
		else
		    down1 = RO0|MUSTDO;
		break;
	default:
		break;
	}

recur:
	if( ty == BITYPE ) rallo( p->in.right, down2 );
	else if( ty == LTYPE )  return;
	/* else do tail-recursion */
	p =  p->in.left;
	down = down1;
	goto again;

}

stoasg( p, o ) register NODE *p; 
{
	/* should the assignment op p be stored,
	   given that it lies as the right operand of o
	   (or the left, if o==UNARY MUL) */
/*
	if( p->in.op == INCR || p->in.op == DECR ) return;
	if( o==UNARY MUL && p->in.left->in.op == REG && !isbreg(p->in.left->tn.rval) ) SETSTO(p,INAREG);
 */
	return( shltype(p->in.left->in.op, p->in.left ) );
}

deltest( p ) register NODE *p; 
{
	/* should we delay the INCR or DECR operation p */
	/* we don't have the means to do side-effect arithmetic */

	/*
	 * NOTE: delay2() assumes the rhs of INCR or DECR is a
	 * constant; this isn't always true [e.g., with -pic],
	 * so we must check explicitly.
	 */
	NODE *r = p->in.right;
	p = p->in.left;
	return( (p->in.op == REG || p->in.op == NAME || p->in.op == OREG ) &&
		(r->in.op == ICON || r->in.op == FCON));
}

mkadrs(p) register NODE *p; 
{
	register o;

	o = p->in.op;

	if( asgop(o) ){
		if( !SUGT( p->in.right->in.su , p->in.left->in.su )){
			if( p->in.left->in.op == UNARY MUL ){
				if( SUTEST( p->in.left->in.su ) )
					SETSTO( p->in.left->in.left, INTEMP );
				else {
					if (SUTEST( p->in.right->in.su ) )
					    SETSTO( p->in.right, INTEMP );
					else 
					    cerror( "store finds both sides trivial" );
				}
			}
			else if( p->in.left->in.op == FLD 
			    && p->in.left->in.left->in.op == UNARY MUL ){
				SETSTO( p->in.left->in.left->in.left, INTEMP );
			} else { 
				/* should be only structure assignment */
				SETSTO( p->in.left, INTEMP );
			}
		}
		else SETSTO( p->in.right, INTEMP );
	} else {
		if( SUGT( p->in.left->in.su , p->in.right->in.su )){
			SETSTO( p->in.left, INTEMP );
		} else {
			SETSTO( p->in.right, INTEMP );
		}
	}
}


notoff( t, r, off, cp) CONSZ off; char *cp; 
{
	/* is it legal to make an OREG or NAME entry which has an
	/* offset of off, (from a register of r), if the
	/* resulting thing had type t */

	if ( IN_IMMEDIATE_RANGE(off) && (cp == NULL || *cp=='\0' || smallpic) )
		/* yes */
		return(0); 
	return(1); /* NO */
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


/*
 * Compile memory reference expressions, attempting to make the best
 * use of the target machine's addressing modes.  The overall plan
 * is to put the expression into a canonical form, then select and
 * compile a suitable subtree based on availability of registers,
 * offset limitations, and support from the architecture.  A goal
 * is not to evaluate any more of the tree than is necessary.
 *
 * On entry, we assume we have been given the left branch of a UNARY MUL,
 * not the UNARY MUL itself.
 *
 * When done, we must guarantee that the resulting tree can be turned
 * into an OREG.
 */

offstar( p, t )
	register NODE *p; 
	TWORD t;	 /* type of the memory operand to be dereferenced*/
{
	register NODE *lp, *rp;
	int right_ok, left_ok;
	TWORD  ttype = p->in.type;

	if ( (p->in.rall&MUSTDO)
	    || (p->in.op != PLUS && p->in.op != MINUS ) ) {
		if ( p->in.op != REG  ) {
			order(p, INTAREG|INAREG);
		}
		return;
	}
	/*
	 * exp is (<exp> [+-] <exp>)
	 */
	lp = p->in.left;
	rp = p->in.right;
	/*
	 * try to put the constant, if any, on the right
	 */
	if (lp->tn.op == ICON && p->in.op == PLUS){
		p->in.right = lp;
		p->in.left = rp;
		lp = p->in.left;
		rp = p->in.right;
	}
	/*
	 * map (<exp> - <icon>) to (<exp> + <-icon>)
	 */
	if (p->in.op == MINUS && NNCON(rp)) {
		p->in.op = PLUS;
		rp->tn.lval = -(rp->tn.lval);
	}
	/*
	 * At this point, any subtractions must be done explicitly.
	 */
	if ( p->in.op == MINUS ) {
		order(p, INTAREG|INAREG);
		return;
	}
	/*
	 * lhs in register
	 */
	left_ok = lp->tn.op == REG;
	/*
	 * rhs const or in register
	 */
	right_ok = (rp->tn.op == ICON && IN_IMMEDIATE_RANGE(rp->tn.lval)
	    && rp->tn.name[0] == '\0');
	if ( t == DOUBLE && !right_ok && !dalign_flag ){
		/* double-oreg not ok for double types */
		order( p, INTAREG|INAREG); /* [base] */
		return;
	} else 
		right_ok |= rp->tn.op == REG ;
		
	while ( !left_ok || !right_ok ){
	    if (!left_ok && !SUGT( rp->tn.su, lp->tn.su)){
		order( lp, INTAREG|INAREG);
		left_ok = 1;
	    }
	    if ( !right_ok){
		order( rp, INTAREG|INAREG);
		right_ok = 1;
		rp->tn.su = zed;
	    }
	}
	/*
	 * should be OREGable now
	 */
	return;

	/*
	 * "Either this man is dead, or my watch has stopped."
	 */
	/* order(p, INTAREG|INAREG); */	/* [base] */
}

/*
 * routines for recognizing double Oreg's 
 */
int
base( p )
	NODE *p;
{
	if (p->tn.op != REG) return -1;
	return p->tn.rval;
}

int
offset( p, type)
	NODE *p;
	int type; /* not used */
{
	return base( p );
}

/*
 * routine for making double Oreg's
 */
makeor2( p, trash, r1, r2 )
	NODE *p, *trash;
	int r1,r2;
{
	/* DO NOT ATTEMPT to make a double double Oreg */
	if (p->in.type == DOUBLE && !dalign_flag)
		return;
	trash = p->in.left;
	trash->in.left->tn.op = FREE;
	trash->in.right->tn.op = FREE;
	trash->in.op = FREE;
	p->tn.op = OREG;
	p->tn.rval = R2PACK( r1, r2, 0 );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define mkmask(reg,mask) (busy[reg] ? ((mask) | (1<<(reg))) : (mask))

argsize( p ) register NODE *p; 
{
	register t, s;
	t = 0;
	if( p->in.op == CM ){
		t = argsize( p->in.left );
		p = p->in.right;
	}
	switch (p->in.type){
	case FLOAT:
		SETOFF( t, ALFLOAT/SZCHAR );
		return( t+(SZFLOAT/SZCHAR) );
	case DOUBLE:
		SETOFF( t, ALDOUBLE/SZCHAR );
		return( t+(SZDOUBLE/SZCHAR) );
	default:
		SETOFF( t, ALINT/SZCHAR );
		return( t+(SZINT/SZCHAR) );
	}
}

#ifndef lint
static        char sccsid[] = "@(#)optim.c 1.1 94/10/31 SMI";
#endif

# if TARGET == I386
# include "mfile1.h"
# else
# include "cpass1.h"
# endif

# define SWAP(p,q) {sp=p; p=q; q=sp;}
# define RCON(p) (p->in.right->in.op==ICON)
# define RO(p) p->in.right->in.op
# define RV(p) p->in.right->tn.lval
# define LCON(p) (p->in.left->in.op==ICON)
# define LO(p) p->in.left->in.op
# define LV(p) p->in.left->tn.lval

int oflag = 0;

#ifndef LINT
extern int twopass;	/* some transformations result in invalid input to */
			/* the global optimizer */
#endif

NODE *
fortarg( p ) NODE *p; {
	/* fortran function arguments */

	if( p->in.op == CM ){
		p->in.left = fortarg( p->in.left );
		p->in.right = fortarg( p->in.right );
		return(p);
		}

	while( ISPTR(p->in.type) ){
		p = buildtree( UNARY MUL, p, NIL );
		}
	return( optim(p) );
	}

static NODE *range_compare();

	/* mapping relationals when the sides are reversed */
short revrel[] ={ EQ, NE, GE, GT, LE, LT, UGE, UGT, ULE, ULT };
NODE *
optim(p) register NODE *p; {
	/* local optimizations, most of which are probably machine independent */

	register o, ty;
	NODE *sp;
	int i;
	TWORD t;

	if( (t=BTYPE(p->in.type))==ENUMTY || t==MOETY ) enumconvert(p);
	if( oflag ) return(p);
	ty = optype( o=p->in.op);
	if( ty == LTYPE ) return(p);

	if( ty == BITYPE ) p->in.right = optim(p->in.right);
	p->in.left = optim(p->in.left);

	/* collect constants */

	switch(o){

	case SCONV:
	case PCONV:
		return( clocal(p) );

	case FORTCALL:
		p->in.right = fortarg( p->in.right );
		break;

	case UNARY AND:
# if TARGET == I386 && !defined(LINT)
		switch ( LO(p) )
		{
		case VAUTO:
		case VPARAM:
		case TEMP:
			return(p);
		}
# endif
		if( LO(p) != NAME ) cerror( "& error" );

		if( !andable(p->in.left) ) return(p);

		LO(p) = ICON;

		setuleft:
		/* paint over the type of the left hand side with the type of the top */
		p->in.left->in.type = p->in.type;
		p->in.left->fn.cdim = p->fn.cdim;
		p->in.left->fn.csiz = p->fn.csiz;
		p->in.op = FREE;
		return( p->in.left );

	case UNARY MUL:
		if( LO(p) != ICON ) break;
		/* optimizer doesn't like names without symbol table entries */
		if(p->in.left->tn.rval == NONAME
#ifndef LINT
			&& twopass
#endif
		)  break;
		
		LO(p) = NAME;
		goto setuleft;

	case MINUS:
		if( LCON(p) && RCON(p)
		    && p->in.left->tn.rval == p->in.right->tn.rval ) {
			/* symbols cancel out */
			p->in.left->tn.rval = NONAME;
			p->in.right->tn.rval = NONAME;
		}
		if( !nncon(p->in.right) ) break;
		RV(p) = -RV(p);
		o = p->in.op = PLUS;

	case MUL:
	case PLUS:
	case AND:
	case OR:
	case ER:
		/* commutative ops; for now, just collect constants */
		/* someday, do it right */
		if( nncon(p->in.left) || ( LCON(p) && !RCON(p) ) )
			SWAP( p->in.left, p->in.right );
		/* make ops tower to the left, not the right */
		if( o == RO(p) ) {
			NODE *t1, *t2, *t3;
			t1 = p->in.left;
			sp = p->in.right;
			t2 = sp->in.left;
			t3 = sp->in.right;
			/* now, put together again */
			p->in.left = sp;
			sp->in.left = t1;
			sp->in.right = t2;
			sp->in.type = p->in.type;
			p->in.right = t3;
			}
		if(o == PLUS && LO(p) == MINUS && RCON(p) && RCON(p->in.left) &&
		  conval(p->in.right, MINUS, p->in.left->in.right, 1)){
			zapleft:
			RO(p->in.left) = FREE;
			LO(p) = FREE;
			p->in.left = p->in.left->in.left;
		}
		if( RCON(p) && LO(p)==o && RCON(p->in.left) && conval( p->in.right, o, p->in.left->in.right, 1 ) ){
			goto zapleft;
			}
		else if( LCON(p) && RCON(p) && conval( p->in.left, o, p->in.right, 1 ) ){
			zapright:
			RO(p) = FREE;
			p->in.left = makety( p->in.left, p->in.type, p->fn.cdim, p->fn.csiz );
			p->in.op = FREE;
			return( clocal( p->in.left ) );
			}
		/*
		 * subscript reassociation:
		 * 	(i [+-] c)*k => (i*k)[+-](c*k)
		 * This makes CSE work better, but fouls up data
		 * dependence analysis for parallel optimization.
		 * If we're still using PCC when this becomes an
		 * issue, we've got problems.
		 */
		if (o == MUL
		    && p->in.type == INT
		    && p->in.left->in.type == INT
		    && p->in.right->tn.type == INT
		    && nncon(p->in.right)) {
			/*
			 * expression is (...) * k
			 */
			NODE *lp;
			lp = p->in.left;
			if ((lp->in.op == PLUS || lp->in.op == MINUS)
			    && lp->in.left->in.type == INT
			    && lp->in.right->tn.type == INT
			    && nncon(lp->in.right)) {
				/*
				 * expression is ( x [+-] c ) * k
				 * rewrite as ( x * k ) [+-] ( c * k )
				 * Note that although this may introduce
				 * overflows, they do not affect the value
				 * of the result.
				 */
				CONSZ c, k;
				o = p->in.op = lp->in.op;
				lp->in.op = MUL;
				k = RV(p);
				c = RV(lp);
				RV(lp) = k;
				RV(p) *= c;
				p->in.left = optim(lp);
				}
			}
		/*
		 * change muls to shifts; Leave multiplication by 1
		 * alone at this stage, in order to make operator
		 * strength reduction (iropt variety) work better.
		 */

		if( o==MUL && nncon(p->in.right) && (i=ispow2(RV(p)))>0){
			o = p->in.op = LS;
			p->in.right->in.type = p->in.right->fn.csiz = INT;
			RV(p) = i;
			}

		/* change +'s of negative consts back to - */
		if( o==PLUS && nncon(p->in.right)
		    && !ISPTR(p->in.right->in.type) && RV(p)<0 ){
			RV(p) = -RV(p);
			o = p->in.op = MINUS;
			}
		break;

	case DIV:
		if( nncon(p->in.right)
		    && ( p->in.right->tn.lval == 1
			|| nncon(p->in.left)
			    && conval(p->in.left, DIV, p->in.right, 0) ) )
			goto zapright;
		break;

	case EQ:
	case NE:
	case LT:
	case LE:
	case GT:
	case GE:
	case ULT:
	case ULE:
	case UGT:
	case UGE:
		if( LCON(p) ) {

			/* exchange operands */

			sp = p->in.left;
			p->in.left = p->in.right;
			p->in.right = sp;
			p->in.op = revrel[p->in.op - EQ ];
			o = p->in.op;
			}
		if( LCON(p) ) {
			if( nncon(p->in.right)
			    && conval(p->in.left, p->in.op, p->in.right, 1) )
				goto zapright;
			if( RCON(p)
			    && p->in.left->tn.rval == p->in.right->tn.rval ) {
				/*
				 * comparison of two constants with
				 * same symbol
				 */
				p->in.left->tn.rval = NONAME;
				p->in.right->tn.rval = NONAME;
				if (conval(p->in.left, p->in.op, p->in.right, 1))
					goto zapright;
				cerror("botched constant comparison");
				}
			}
		if (LO(p) == SCONV && nncon(p->in.right)) {
			return range_compare(p);
			}
		break;
		}

	return(p);
	}

ispow2( c ) CONSZ c; {
	register i;
	if( c <= 0 || (c&(c-1)) ) return(-1);
	for( i=0; c>1; ++i) c >>= 1;
	return(i);
	}

nncon( p ) NODE *p; {
	/* is p a constant without a name */
	return( p->in.op == ICON && p->tn.rval == NONAME );
	}

/*
 * check for out-of-range comparison,
 * e.g., short s; ... if (s == 0x8001) ...
 * in which case the condition can be evaluated
 * at compile-time (with a warning, since the
 * outcome is probably not what the user meant)
 */
struct tydescr {
    CONSZ lbound, ubound;
    TWORD type;
    char *tname;
} tydescrs[] = {
    {-0x80,	  0x7f,		CHAR, 	  "char" },
    { 0,	  0xff,		UCHAR,	  "unsigned char" },
    {-0x8000,	  0x7fff,	SHORT,	  "short" },
    { 0,	  0xffff,	USHORT,	  "unsigned short" },
    {-0x80000000, 0x7fffffff,	INT,	  "int" },
    { 0,	  0xffffffff,	UNSIGNED, "unsigned" },
    {-0x80000000, 0x7fffffff,	LONG,	  "long" },
    { 0,	  0xffffffff,	ULONG,	  "unsigned long" },
};

#define char_ty   &tydescrs[0]
#define uchar_ty  &tydescrs[1]
#define short_ty  &tydescrs[2]
#define ushort_ty &tydescrs[3]
#define int_ty    &tydescrs[4]
#define uint_ty   &tydescrs[5]
#define long_ty   &tydescrs[6]
#define ulong_ty  &tydescrs[7]


static int
lessthan(x, y, isunsigned)
    CONSZ x,y;
    int isunsigned;
{
    if (isunsigned)
	return ((unsigned long)x < (unsigned long)y);
    return ((long)x < (long)y);
}

/*
 * Look for the pattern
 *	<logop>
 *	    SCONV {integral type}
 *		(x) {ushort|uchar|short|char}
 *	    ICON #y {integral type}
 * and, if the value of #y is out of the range of possible
 * values of the type of SCONV{integral type}(x), evaluate the
 * comparison at compile time.  A warning is produced, since
 * the result is probably NOT what the user expected.
 */
static NODE*
range_compare(p)
    register NODE *p;
{
    register NODE *q,*r;
    struct tydescr *t, *target;
    int result;

    /*
     * match on the left:
     *	    SCONV {integral type}
     *		(x) {ushort|uchar|short|char}
     */
    q = p->in.left;
    r = p->in.right;
    if (q->in.op != SCONV) {
	return(p);
    }
    switch(q->in.type) {
    case CHAR:     target = char_ty;   break;
    case UCHAR:    target = uchar_ty;  break;
    case SHORT:    target = short_ty;  break;
    case USHORT:   target = ushort_ty; break;
    case INT:      target = int_ty;    break;
    case UNSIGNED: target = uint_ty;   break;
    case LONG:     target = long_ty;   break;
    case ULONG:    target = ulong_ty;  break;
    default:
	return(p);
    }
    switch(q->in.left->in.type) {
    case CHAR:   t = char_ty;   break;
    case UCHAR:  t = uchar_ty;  break;
    case SHORT:  t = short_ty;  break;
    case USHORT: t = ushort_ty; break;
    default:
	return(p);
    }
    /*
     * if type t is a proper subset of the SCONV
     * type, use it to determine the range of the lhs.
     */
    if (ISUNSIGNED(t->type) == ISUNSIGNED(target->type) && t < target) {
	target = t;
    }
    /*
     * given bounds on the possible values of lhs = SCONV(x),
     * and knowing the value of #y, see if #y is known to 
     * be outside of the set of possible values of the lhs.
     */
    if (lessthan(target->ubound, r->tn.lval, ISUNSIGNED(target->type))) {
	/*
	 * (lbound <= lhs <= ubound) && (ubound < rhs),
	 * so (lhs < rhs)
	 */
	switch(p->in.op) {
	case LT:
	case LE:
	case ULE:
	case ULT:
	case NE:
	    result = 1;
	    break;
	default:
	    result = 0;
	    break;
	}
    } else if (lessthan(r->tn.lval, target->lbound, ISUNSIGNED(target->type))) {
	/*
	 * (rhs < lbound) && (lbound <= lhs <= ubound)
	 * so (rhs < lhs)
	 */
	switch(p->in.op) {
	case GT:
	case UGT:
	case GE:
	case UGE:
	case NE:
	    result = 1;
	    break;
	default:
	    result = 0;
	    break;
	}
    } else {
	return(p);
    }
    werror("constant %d is out of range of %s comparison ",
	r->tn.lval, target->tname);
    werror("result of comparison is always %s",
	(result == 1 ? "true" : "false"));
    /* rewrite as (<expr>, <compile-time result>) */
    r->tn.type = INT;
    r->tn.lval = result;
    p->in.op = COMOP;
    return(p);
}

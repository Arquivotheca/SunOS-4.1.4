#ifndef lint
static	char sccsid[] = "@(#)reader.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */



# include "cpass2.h"

/*	some storage declarations */

# ifndef ONEPASS
static char namebuf[BUFSIZ];
char *filename = namebuf;
int ftnno;  /* number of current function */
int lineno;
# else
# define NOMAIN
#endif

/*
 * These are for machines with multiple register sets, for which su is
 * "more than just a number". They may be defined in machdep2.h.
 */
#ifndef SUTOOBIG
#    define SUTOOBIG( a  ) ( a > fregs )
#endif
#ifndef SUPRINT
#    define SUPRINT( s ) print_str_d_nl( ", SU= ", s )
#endif

int nrecur;
int lflag;
extern int Wflag;
int edebug = 0;
int xdebug = 0;
int udebug = 0;
int vdebug = 0;

OFFSZ tmpoff;  /* offset for first temporary, in bits for current block */
OFFSZ maxoff;  /* maximum temporary offset over all blocks in current ftn, in bits */
int maxtreg, maxfreg;

NODE *stotree;
int stocook;

OFFSZ baseoff = 0;
OFFSZ maxtemp = 0;

extern unsigned int offsz;
extern unsigned int caloff();

p2init( argc, argv ) 
	char *argv[];
{
	/* set the values of the pass 2 arguments */

	register int c;
	int files;
	char *cp;

	offsz = caloff();
	allo0();  /* free all regs */
	files = 0;

	for( c=1; c<argc; ++c ){
		if( *(cp=argv[c]) == '-' ){
			while( *++cp ){
				switch( *cp ){
				/* aelrstuvwxWX */

#ifdef BROWSER
				case 'c':
					break;
#endif
				case 'X':  /* pass1 flags */
					while( *++cp ) { /* VOID */ }
					--cp;
					break;

				case 'l':  /* linenos */
					++lflag;
					break;

				case 'e':  /* expressions */
					++edebug;
					break;

				case 'o':  /* orders */
					++odebug;
					break;

				case 'r':  /* register allocation */
					++rdebug;
					break;

				case 'a':  /* rallo */
					++radebug;
					break;

				case 'v':
					++vdebug;
					break;

				case 't':  /* ttype calls */
					++tdebug;
					break;

				case 's':  /* shapes */
					++sdebug;
					break;

				case 'u':  /* Sethi-Ullman testing (machine dependent) */
					++udebug;
					break;

				case 'x':  /* general machine-dependent debugging flag */
					++xdebug;
					break;

				case 'w':
				case 'W':  /* shut up warnings */

					++Wflag;
					break;

				default:
					myflags( *cp, &cp );
				}
			}
		} else 
			files = 1;  /* assumed to be a filename */
	}

	minit();  /* miscellaneous machine-dependent initialization */
	mkdope();
	setrew();
	return( files );

}

# ifndef NOMAIN

mainp2( argc, argv )
	char *argv[];
{
	register files;
	int temp, aoff;
	register c;
	register char *cp;
	register NODE *p;
	TWORD t;
	OFFSZ size;
	int align;

	files = p2init( argc, argv );
	tinit();

	reread:

	if( files ){
		while( files < argc && argv[files][0] == '-' ) {
			++files;
			}
		if( files >= argc ) return( nerrors );
		freopen( argv[files++], "r", stdin );
		}
	while( (c=getchar()) > 0 ) switch( c ){
	case ')':
	default:
		/* copy line unchanged */
		if ( c != ')' )
			PUTCHAR( c );  /*  initial tab  */
		while( (c=getchar()) > 0 ){
			PUTCHAR(c);
			if( c == '\n' ) break;
			}
		continue;

	case BBEG:
		/* beginning of a block */
		temp = rdin(10);  /* ftnno */
		aoff = rdin(10); /* autooff for block gives max offset of autos in block */
		TMP_BBEG; 
		maxtreg = rdin(10);
		maxfreg = rdin(10);
		if( getchar() != '\n' ) cerror( "intermediate file format error");

		if( temp != ftnno ){ /* beginning of function */
			TMP_BFN;
			ftnno = temp;
			bfcode2();
			}
		else {
			TMP_NFN;
			/* maxoff at end of ftn is max of autos and temps
			   over all blocks in the function */
			}
		setregs();
		continue;

	case BEND:  /* end of block */
		TMP_BEND;
		t = (TWORD)rdin(10);
		size = (OFFSZ)rdin(10);
		align = rdin(10);
		eobl2( t, size, align );
		while( (c=getchar()) != '\n' ){
			if( c <= 0 ) cerror( "intermediate file format eof" );
			}
		continue;

	case EXPR:
		/* compile code for an expression */
		lineno = rdin( 10 );
		for( cp=filename; (*cp=getchar()) != '\n'; ++cp ) ; /* VOID, reads filename */
		*cp = '\0';
		if( lflag ) lineid( lineno, filename );

		p = eread();

# ifndef BUG4
		if( edebug ) fwalk( p, eprint, 0 );
# endif

# ifdef MYREADER
		MYREADER(p);  /* do your own laundering of the input */
# endif

		nrecur = 0;
		delay( p );  /* expression statement  throws out results */
		reclaim( p, RNULL, 0 );

		allchk();
		tcheck();
		TMP_EXPR;  /* expression at top level reuses temps */
		continue;

	default:
		cerror( "intermediate file format error" );

		}

	/* EOF */
	if( files ) goto reread;
	return(nerrors);

}

# endif

# ifdef ONEPASS

p2compile( p )
	NODE *p;
{

	if( lflag ) lineid( lineno, filename );
	/* generate code for the tree p */
# ifndef BUG4
	if( edebug ) fwalk( p, eprint, 0 );
# endif

# ifdef MYREADER
	MYREADER(p);  /* do your own laundering of the input */
# endif
	nrecur = 0;
	delay( p );  /* do the code generation */
	reclaim( p, RNULL, 0 );
	allchk();
	/* can't do tcheck here; some stuff (e.g., attributes) may be around from first pass */
	/* first pass will do it... */
	TMP_EXPR;  /* expression at top level reuses temps */
}

p2bbeg( aoff, myreg, myfreg )
{
	static int myftn = -1;

	TMP_BBEG;
	maxtreg = myreg;
	maxfreg = myfreg;
	if( myftn != ftnno ){ /* beginning of function */
		TMP_BFN;
		myftn = ftnno;
		}
	else {
		TMP_NFN;
		/* maxoff at end of ftn is max of autos and temps over all blocks */
		}
	setregs();
}

p2bend(t, size, align, opt_level)
    TWORD t;
    OFFSZ size;
    int align;
    int opt_level;
{
	TMP_BEND;
	eobl2( t, size, align, opt_level );
}

# endif

delay( p )
	register NODE *p;
{
	/* look in all legal p
	/* note; don't delay ++ and -- within calls or things like
	/* getchar (in their macro forms) will start behaving strangely */
	register i;
	NODE *deltrees[DELAYS];
	int deli;

	/* look for visible, delayable ++ and -- */

	deli = 0;
	delay2( p, deltrees, &deli );
	codgen( p, FOREFF );  /* do what is left */
	for( i = 0; i<deli; ++i ) codgen( deltrees[i], FOREFF );  /* do the rest */
}

delay2( p, deltrees, pdeli )
	register NODE *p;
	register NODE *deltrees[];
	register *pdeli;
{

	/* look for delayable ++ and -- operators */

	register o, ty;
	o = p->in.op;
	ty = optype( o );

	switch( o ){

	case NOT:
	case QUEST:
	case ANDAND:
	case OROR:
	case CALL:
	case UNARY CALL:
	case STCALL:
	case UNARY STCALL:
	case FORTCALL:
	case UNARY FORTCALL:
	case COMOP:
	case CBRANCH:
		/* for the moment, don7t delay past a conditional context, or
		/* inside of a call */
		return;

#ifdef VAX
	case UNARY MUL:
		/* if *p++, do not rewrite */
		if( autoincr( p ) ) return;
		break;
#endif

	case INCR:
	case DECR:
		if( optype(p->in.right->in.op) == LTYPE && deltest( p ) ){
			if( (*pdeli) < DELAYS ){
				register NODE *q;
				deltrees[(*pdeli)++] = tcopy(p);
				q = p->in.left;
				p->in.right->in.op = FREE;  /* zap constant */
				ncopy( p, q );
				q->in.op = FREE;
				return;
				}
			}

		}

	if( ty == BITYPE ) delay2( p->in.right, deltrees, pdeli );
	if( ty != LTYPE ) delay2( p->in.left, deltrees, pdeli );
}

codgen( p, cookie )
	NODE *p;
{

	/* generate the code for p;
	   order may call codgen recursively */
	/* cookie is used to describe the context */

	for(;;){
		canon(p);  /* creats OREG from * if possible and does sucomp */
		stotree = NIL;
# ifndef BUG4
		if( edebug ){
			print_str( "store called on:\n" );
			fwalk( p, eprint, 0 );
			}
# endif
		store(p);
		if( stotree==NIL ) break;

		/* because it's minimal, can do w.o. stores */

#ifdef ORDER_STOTREE
		ORDER_STOTREE;
#else
		order( stotree, stocook );
#endif ORDER_STOTREE
		}

	order( p, cookie );

}

# ifndef BUG4
char *cnames[] = {
	"SANY",
	"SAREG",
	"STAREG",
	"SBREG",
	"STBREG",
	"SCREG",
	"STCREG",
	"SCC",
	"SNAME",
	"SCON",
	"SFLD",
	"SOREG",
# ifdef WCARD1
	"WCARD1",
# else
	"STARNM",
# endif
# ifdef WCARD2"
	"WCARD2",
# else
	"STARREG",
# endif
	"SWADD",
	"SPECIAL", /* should never be accessed... */
	"INTEMP",
	"FORARG",
	0,
	};

prcook( cookie )
{

	/* print a nice-looking description of cookie */

	int i, flag;

	if( cookie & SPECIAL ){
		if( cookie == SZERO ) print_str( "SZERO" );
		else if( cookie == SONE ) print_str( "SONE" );
		else if( cookie == SMONE ) print_str( "SMONE" );
		else print_str_d( "SPECIAL+", cookie & ~SPECIAL );
		return;
		}

	flag = 0;
	for( i=0; cnames[i]; ++i ){
		if( cookie & (1<<i) ){
			if( flag ) putchar('|');
			++flag;
			print_str( cnames[i] );
			}
		}

}
# endif

int odebug = 0;

order(p,cook)
	NODE *p;
{

	register o, ty, m;
	int m1;
	int cookie;
	NODE *p1, *p2;

	cookie = cook;
	rcount();
	canon(p);
	rallo( p, p->in.rall );
	goto first;
	/* by this time, p should be able to be generated without stores;
	   the only question is how */

	again:

	if ( p->in.op == FREE )
		return;		/* whole tree was done */
	cookie = cook;
	rcount();
	canon(p);
	rallo( p, p->in.rall );
	/* if any rewriting and canonicalization has put
	 * the tree (p) into a shape that cook is happy
	 * with (exclusive of FOREFF, FORREW, and INTEMP)
	 * then we are done.
	 * this allows us to call order with shapes in
	 * addition to cookies and stop short if possible.
	 */
	if( tshape(p, cook &(~(FOREFF|FORREW|INTEMP)))
	    && p->in.rall == NOPREF ) {
		return;
	}

	first:
# ifndef BUG4
	if( odebug ){
		printf( "order( 0x%lx, ", p );
		prcook( cookie );
		print_str( " )\n" );
		fwalk( p, eprint, 0 );
		}
# endif

	o = p->in.op;
	ty = optype(o);

	/* first of all, for most ops, see if it is in the table */

	/* look for ops */

	switch( m = p->in.op ){

	default:
		/* look for op in table */
		for(;;){
			if( (m = match( p, cookie ) ) == MDONE ) goto cleanup;
			else if( m == MNOPE ){
				if( !(cookie = nextcook( p, cookie ) ) ) goto nomat;
				continue;
				}
			else break;
			}
		break;

	case COMOP:
	case FORCE:
	case CBRANCH:
	case QUEST:
	case ANDAND:
	case OROR:
	case NOT:
	case UNARY CALL:
	case CALL:
	case UNARY STCALL:
	case STCALL:
	case UNARY FORTCALL:
	case FORTCALL:
		/* don't even go near the table... */
		;

		}
	/* get here to do rewriting if no match or
	   fall through from above for hard ops */

	p1 = p->in.left;
	if( ty == BITYPE ) p2 = p->in.right;
	else p2 = NIL;
	
# ifndef BUG4
	if( odebug ){
		printf( "order( 0x%lx, ", p );
		prcook( cook );
		print_str( " ), cookie " );
		prcook( cookie );
		print_str_str_nl( ", rewrite ", opst[m] );
		}
# endif
	switch( m ){
	default:
		nomat:
		cerror( "no table entry for op %s", opst[p->in.op] );

	case COMOP:
		delay( p1 );
		p2->in.rall = p->in.rall;
		codgen( p2, cookie );
		ncopy( p, p2 );
		p2->in.op = FREE;
		goto cleanup;

	case FORCE:
		/* recurse, letting the work be done by rallo */
		p = p->in.left;
		cook = OPEREG(p->in.type)&(INTAREG|INTBREG|INTCREG);
		goto again;

	case CBRANCH:
		o = p2->tn.lval;
		cbranch( p1, -1, o );
		p2->in.op = FREE;
		p->in.op = FREE;
		return;

	case QUEST:
		cbranch( p1, -1, m=getlab() );
		p2->in.left->in.rall = p->in.rall;
		codgen( p2->in.left, OPEREG(p2->in.left->in.type)&(INTAREG|INTBREG|INTCREG) );
		/* force right to compute result into same reg used by left */
		p2->in.right->in.rall = p2->in.left->tn.rval|MUSTDO;
		reclaim( p2->in.left, RNULL, 0 );
		branch(m1 = getlab());
		deflab( m );
		codgen( p2->in.right, INTAREG|INTBREG|INTCREG );
		deflab( m1 );
		p->in.op = REG;  /* set up node describing result */
		p->tn.lval = 0;
		p->tn.rval = p2->in.right->tn.rval;
		p->in.type = p2->in.right->in.type;
		tfree( p2->in.right );
		p2->in.op = FREE;
		goto cleanup;

	case ANDAND:
	case OROR:
	case NOT:  /* logical operators */
		/* if here, must be a logical operator for 0-1 value */
		cbranch( p, -1, m=getlab() );
		p->in.op = CCODES;
		p->bn.label = m;
		order( p, INTAREG );
		goto cleanup;

	case FLD:	/* fields of funny type */
		if ( p1->in.op == UNARY MUL ){
			offstar( p1->in.left, p1->in.type );
			goto again;
			}
		if (! (cook&INTAREG) ){
		    order( p, INAREG|INTAREG );
		    goto again;
		}

	case UNARY MINUS:
	case GOTO:
		if (setunary(p, cookie)) goto again;
		goto nomat;

	case SCONV:
		if (setconv( p, cookie)) goto again;
		goto nomat;

	case NAME:
		/* all leaves end up here ... */
		if( o == REG ) goto nomat;
		order( p, OPEREG( p->in.type ) );
		goto again;

	case INIT:
		uerror( "illegal initialization" );
		return;

	case UNARY FORTCALL:
		p->in.right = NIL;
	case FORTCALL:
		o = p->in.op = UNARY FORTCALL;
		if( genfcall( p, cookie ) ) goto nomat;
		goto cleanup;

	case UNARY CALL:
		p->in.right = NIL;
	case CALL:
		o = p->in.op = UNARY CALL;
		if( gencall( p, cookie ) ) goto nomat;
		goto cleanup;

	case UNARY STCALL:
		p->in.right = NIL;
	case STCALL:
		o = p->in.op = UNARY STCALL;
		if( genscall( p, cookie ) ) goto nomat;
		goto cleanup;

		/* if arguments are passed in register, care must be taken that reclaim
		/* not throw away the register which now has the result... */

	case UNARY MUL:
		if( cook == FOREFF ){
			/* do nothing */
			delay( p->in.left );
			p->in.op = FREE;
			return;
			}
		if( p->in.type == STRTY || p->in.type == UNIONTY ) {
			/* useless trash from buildtree */
			*p = *p1;
			p1->in.op = FREE;
			goto again;
			}
		offstar( p->in.left, p->in.type );
		goto again;

	case INCR:  /* INCR and DECR */
		if( setincr(p, cook) ) goto again;

		/* x++ becomes (x += 1) -1; */

		if( cook & FOREFF ){  /* result not needed so inc or dec and be done with it */
			/* x++ => x += 1 */
			p->in.op = (p->in.op==INCR)?ASG PLUS:ASG MINUS;
			goto again;
			}

		p1 = tcopy(p);
		reclaim( p->in.left, RNULL, 0 );
		p->in.left = p1;
		p1->in.op = (p->in.op==INCR)?ASG PLUS:ASG MINUS;
		p->in.op = (p->in.op==INCR)?MINUS:PLUS;
		if (p1->in.left->in.op == FLD) {
			int fieldsize = UPKFSZ(p1->in.left->tn.rval);
			NODE *p2 = talloc();
			*p2 = *p;
			p2->in.right = tcopy(p->in.right);
			p->in.op = AND;
			p->in.left = p2;
			p->in.right->tn.lval = bitmask(fieldsize);
			}
		goto again;

	case STASG:
		if( setstr( p, cook ) ) goto again;
		goto nomat;

	case ASG PLUS:  /* and other assignment ops */
		if( setasop(p, cook) ) goto again;

		/* there are assumed to be no side effects in LHS */

		p2 = tcopy(p);
		p->in.op = ASSIGN;
		reclaim( p->in.right, RNULL, 0 );
		p->in.right = p2;
		canon(p);
		rallo( p, p->in.rall );

# ifndef BUG4
		if( odebug ) fwalk( p, eprint, 0 );
# endif

		order( p2->in.left, OPEREG(p2->in.type) );
		order( p2, OPEREG(p2->in.type) );
		goto again;

	case ASSIGN:
		if( setasg( p, cook ) ) goto again;
		if (cook & FORCC){
			/* turn into ((lhs)=(rhs),(lhs)) */
			p2 = tcopy(p);
			p->in.op = COMOP;
			reclaim( p->in.right, RNULL, 0 );
			p->in.right  = p->in.left;
			p->in.left = p2;
			canon(p);
			goto again;
		}
		goto nomat;

	case BITYPE:
		if( setbin( p, cook ) ) goto again;
		/* try to replace binary ops by =ops */
		switch(o){

		case PLUS:
		case MINUS:
		case MUL:
		case DIV:
		case MOD:
		case AND:
		case OR:
		case ER:
		case LS:
		case RS:
			p->in.op = ASG o;
			goto again;
			}
		goto nomat;

		}

	cleanup:

	/* if it is not yet in the right state, put it there */

	if( cook & FOREFF ){
		reclaim( p, RNULL, 0 );
		return;
		}

	if( p->in.op==FREE ) return;

	if( tshape( p, cook ) ) return;

	if( (m=match(p,cook) ) == MDONE ) return;

	/* we are in bad shape, try one last chance */
	if( lastchance( p, cook ) ) goto again;

	goto nomat;
}

int callflag;

store( p )
	register NODE *p;
{

	/* find a subtree of p which should be stored */

	register o, ty;

	o = p->in.op;
	ty = optype(o);

	if( ty == LTYPE ) return;

	switch( o ){

	case UNARY CALL:
	case UNARY FORTCALL:
	case UNARY STCALL:
		++callflag;
		break;

	case UNARY MUL:
		if( asgop(p->in.left->in.op) ) stoasg( p->in.left, UNARY MUL );
		break;

	case CALL:
	case FORTCALL:
	case STCALL:
		store( p->in.left );
		stoarg( p->in.right, o );
		++callflag;
		return;

	case ANDAND:
	case OROR:
	case QUEST:
	case COMOP:
		markcall( p->in.right );
		if( SUTOOBIG(p->in.right->in.su )) SETSTO( p, INTEMP );
	case CBRANCH:   /* to prevent complicated expressions on the LHS from being stored */
	case NOT:
		constore( p->in.left );
		return;

		}

	if( ty == UTYPE ){
		store( p->in.left );
		return;
		}

	if( asgop( p->in.right->in.op ) ) stoasg( p->in.right, o );

	if( SUTOOBIG(p->in.su )){ /* must store */
		mkadrs( p );  /* set up stotree and stocook to subtree
				 that must be stored */
		}

	store( p->in.right );
	store( p->in.left );
}

constore( p )
	register NODE *p;
{

	/* store conditional expressions */
	/*
	   the point is, avoid storing expressions in conditional
	   contexts, i.e.,
		x && y,
		x || y,
		x ? y : z,
	   because the evaluation order is predetermined.  Since
	   this is also true for
		x , y
	   we include it in the same category.
	 */

	switch( p->in.op ) {

	case ANDAND:
	case OROR:
	case QUEST:
	case COMOP:
		markcall( p->in.right );
	case NOT:
		constore( p->in.left );
		return;

		}

	store( p );
}

markcall( p )
	register NODE *p;
{
	/* mark off calls below the current node */

	again:
	switch( p->in.op ){

	case UNARY CALL:
	case UNARY STCALL:
	case UNARY FORTCALL:
	case CALL:
	case STCALL:
	case FORTCALL:
		++callflag;
		return;

		}

	switch( optype( p->in.op ) ){

	case BITYPE:
		markcall( p->in.right );
	case UTYPE:
		p = p->in.left;
		/* eliminate recursion (aren't I clever...) */
		goto again;
	case LTYPE:
		return;
		}

}

stoarg( p, calltype )
	register NODE *p;
{
	/* arrange to store the args */

	if( p->in.op == CM ){
		stoarg( p->in.left, calltype );
		p = p->in.right ;
		}
	if( calltype == CALL ){
		STOARG(p);
		}
	else if( calltype == STCALL ){
		STOSTARG(p);
		}
	else {
		STOFARG(p);
		}
	callflag = 0;
	store(p);
# ifndef NESTCALLS
	if( callflag ){ /* prevent two calls from being active at once  */
		SETSTO(p,INTEMP);
		store(p); /* do again to preserve bottom up nature....  */
		}
#endif
}

cbranch( p, true, false )
	register NODE *p;
{
	/* evaluate p for truth value, and branch to true or false
	/* accordingly: label <0 means fall through */

	register o, lab, flab, tlab;
	register NODE *lp, *rp;
	int t;
	int mode;
	extern int negrel[];

	lab = -1;

	lp = p->in.left;
	rp = p->in.right;
	switch( o=p->in.op ){

	case ULE:
	case ULT:
	case UGE:
	case UGT:
	case EQ:
	case NE:
	case LE:
	case LT:
	case GE:
	case GT:
		if( true < 0 ){
			if (ISFLOATING(lp->in.type)) {
				/* !(a < b) is not the same as (a >= b) */
				p->bn.reversed = !p->bn.reversed;
			} else {
				o = p->in.op = negrel[o-EQ];
			}
			true = false;
			false = -1;
		}
#ifndef NOOPT
		/*
		 * WARNING: do not "extend" this code to deal
		 * with floating point comparisons against zero.
		 * On machines without floating point hardware, the cost
		 * of evaluating a floating point operand with cookie ==
		 * FORCC is greater for ops {LT,GT,LE,GE} than for {EQ,NE},
		 * and the machine-dependent routines may only expect to
		 * encounter the latter case.
		 */
		if( rp->in.op == ICON
		    && rp->tn.lval == 0
		    && rp->in.name[0] == '\0' ){
			switch( o ){

			case UGT:
			case ULE:
				o = p->in.op = (o==UGT)?NE:EQ;
			case EQ:
			case NE:
			case LE:
			case LT:
			case GE:
			case GT:
				if( logop(lp->in.op) ){
				    /*
				     * ((x <logop> y) <logop> 0)
				     * l.h.s. is always 1 or 0, so...
				     */
				    switch(o) {
				    case GE:
					/* always true */
					delay(lp);
					branch(true);
					break;
				    case LT:
					/* always false */
					delay(lp);
					break;
				    case NE:
				    case GT:
					/* compile as (x <logop> y) */
					cbranch(lp, true, false);
					break;
				    case EQ:
				    case LE:
					/* compile as (!(x <logop> y)) */
					cbranch(lp, false, true);
					break;
				    }
				    if (false > 0) branch(false);
				    rp->in.op = FREE;
				    p->in.op = FREE;
				    return;
				}
				codgen( lp, FORCC );
				cbgen( o, lp->tn.type, true, p->bn.reversed, lp->in.op );
				break;

			case UGE:
				delay(lp);
				branch( true ); 
				break;
			case ULT:
				delay(lp);
				}
			}
		else
#endif
			{
			p->bn.label = true;
			codgen( p, FORCC );
			}
		if( false>0 ) branch( false );
		reclaim( p, RNULL, 0 );
		return;

	case ANDAND:
		lab = false<0 ? getlab() : false ;
		cbranch( lp, -1, lab );
		cbranch( rp, true, false );
		if( false < 0 ) deflab( lab );
		p->in.op = FREE;
		return;

	case OROR:
		lab = true<0 ? getlab() : true;
		cbranch( lp, lab, -1 );
		cbranch( rp, true, false );
		if( true < 0 ) deflab( lab );
		p->in.op = FREE;
		return;

	case NOT:
		cbranch( lp, false, true );
		p->in.op = FREE;
		return;

	case COMOP:
		delay(lp);
		p->in.op = FREE;
		cbranch( rp, true, false );
		return;

	case QUEST:
		flab = false<0 ? getlab() : false;
		tlab = true<0 ? getlab() : true;
		cbranch( lp, -1, lab = getlab() );
		cbranch( rp->in.left, tlab, flab );
		deflab( lab );
		cbranch( rp->in.right, true, false );
		if( true < 0 ) deflab( tlab);
		if( false < 0 ) deflab( flab );
		rp->in.op = FREE;
		p->in.op = FREE;
		return;

	case ICON:
		if( p->tn.lval || p->in.name[0] ){
			/* addresses of C objects are never 0 */
			if( true>=0 ) branch( true );
		} else if( false>=0 ) {
			branch( false );
		}
		p->in.op = FREE;
		return;

	case FCON:
		if( p->fpn.dval != 0.0 ) {
			if( true>=0 ) branch( true );
		} else if( false>=0 ) {
			branch( false );
		}
		p->in.op = FREE;
		return;

	default:
		/* get condition codes */
		codgen( p, FORCC );
		if( true >= 0 ) {
			cbgen( NE, p->tn.type, true, 0, p->tn.op );
		}
		if( false >= 0 ) {
			if (true >= 0)
				branch(false);
			else
				cbgen( EQ, p->tn.type, false, 0, p->tn.op );
		}
		reclaim( p, RNULL, 0 );
		return;

	}

}

rcount()
{ /* count recursions */
	if( ++nrecur > NRECUR ){
		cerror( "expression causes compiler loop: try simplifying" );
		}

}

# ifndef BUG4
eprint( p, down, a, b )
	NODE *p;
	int *a, *b;
{
	int fieldsize, fieldoff;

	*a = *b = down+1;
	while( down >= 2 ){
		putchar('\t');
		down -= 2;
		}
	if( down-- ) print_str( "    " );


	printf( "0x%lx) %s", p, opst[p->in.op] );
	switch( p->in.op ) { /* special cases */

	case FLD:
		fieldsize = UPKFSZ(p->tn.rval);
		fieldoff = UPKFOFF(p->tn.rval);
		printf( " <%d:%d>", fieldoff, fieldsize );
		break;

	case REG:
		putchar(' '); print_str(rnames[p->tn.rval] );
		break;

	case FCON:
		if (p->fpn.type == FLOAT) {
			printf( " #%.7e", p->fpn.dval );
		} else {
			printf( " #%.15e", p->fpn.dval );
		}
		break;

	case ICON:
	case NAME:
	case OREG:
		putchar(' ');
		adrput( p );
		break;

	case STCALL:
	case UNARY STCALL:
	case STARG:
	case STASG:
		print_str_d( " size=", p->stn.stsize );
		print_str_d( " align=", p->stn.stalign );
		break;
		}

	print_str( ", " );
	tprint( p->in.type );
	print_str( ", " );
	if( p->in.rall == NOPREF ) print_str( "NOPREF" );
	else {
		if( p->in.rall & MUSTDO ) print_str( "MUSTDO " );
		else print_str( "PREF " );
		print_str(rnames[p->in.rall&~MUSTDO]);
		}
	SUPRINT( p->in.su );

}
# endif

# ifndef NOMAIN
NODE *
eread()
{

	/* call eread recursively to get subtrees, if any */

	register NODE *p;
	register i, c;
	register char *pc;
	register j;
	int k;

	i = rdin( 10 );

	p = talloc();

	p->in.op = i;

	i = optype(i);

	if( i == LTYPE ) p->tn.lval = rdin( CONBASE );
	if( i != BITYPE ) p->tn.rval = rdin( 10 );

	p->in.type = rdin(8 );
	p->in.rall = NOPREF;  /* register allocation information */

	switch (p->in.op){
	case STASG: 
	case STARG:
	case STCALL:
	case UNARY STCALL:
		p->stn.stsize = (rdin( CONBASE ) + (SZCHAR-1) )/SZCHAR;
		p->stn.stalign = rdin(10) / SZCHAR;
		if( getchar() != '\n' ) cerror( "illegal \n" );
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
		p->bn.reversed = 0;
		if( getchar() != '\n' ) cerror( "illegal \n" );
		break;
	case FCON:
		/* patch up fp value */
		k = p->tn.lval;
		j = p->tn.rval;
		*(int *)&(p->fpn.dval) = k;
		*((int *)&(p->fpn.dval)+1) = j;
		c = getchar();
		while ( c != '\n' )
			c = getchar();
		break;
	case REG:
		rbusy( p->tn.rval, p->in.type );  /* non usually, but sometimes justified */
		/* fall through */
	default:
#ifndef FLEXNAMES
		for( pc=p->in.name,j=0; ( c = getchar() ) != '\n'; ++j ){
			if( j < NCHNAM ) *pc++ = c;
			}
		if( j < NCHNAM ) *pc = '\0';
#else
		{ char buf[BUFSIZ];
		for( pc=buf,j=0; ( c = getchar() ) != '\n'; ++j ){
			if( j < BUFSIZ ) *pc++ = c;
			}
		if( j < BUFSIZ ) *pc = '\0';
		p->in.name = tstr(buf);
		}
#endif
	}

	/* now, recursively read descendents, if any */

	if( i != LTYPE ) p->in.left = eread();
	if( i == BITYPE ) p->in.right = eread();

	return( p );

}

CONSZ
rdin( base )
{
	register sign, c;
	CONSZ val;

	sign = 1;
	val = 0;

	while( (c=getchar()) > 0 ) {
		if( c == '-' ){
			if( val != 0 ) cerror( "illegal -");
			sign = -sign;
			continue;
			}
		if( c == '\t' ) break;
		if( c>='0' && c<='9' ) {
			val *= base;
			if( sign > 0 )
				val += c-'0';
			else
				val -= c-'0';
			continue;
			}
		cerror( "illegal character `%c' on intermediate file", c );
		break;
		}

	if( c <= 0 ) {
		cerror( "unexpected EOF");
		}
	return( val );
}
# endif

#ifndef FIELDOPS
	/* do this if there is no special hardware support for fields */

ffld( p, down, down1, down2 )
	NODE *p; int *down1, *down2;
{
	 /* look for fields that are not in an lvalue context, and rewrite them... */
	register NODE *shp;
	register s, o, v, ty;

	*down1 =  asgop( p->in.op );
	*down2 = 0;

	if( !down && p->in.op == FLD ){ /* rewrite the node */

		if( !rewfld(p) ) return;

		ty = p->in.type;
		v = p->tn.rval;
		s = UPKFSZ(v);
# ifdef RTOLBYTES
		o = UPKFOFF(v);  /* amount to shift */
# else
		o = tlen(p->in.left)*SZCHAR - s - UPKFOFF(v);  /* amount to shift */
#endif
		if (UPKFOFF(v)) {
			/*
			 * space to the left, must mask
			 */
			p->in.op = AND;
			p->in.right = talloc();
			p->in.right->in.op = ICON;
			p->in.right->in.rall = NOPREF;
			p->in.right->in.type = ty;
			p->in.right->tn.lval = bitmask(s);
			p->in.right->tn.rval = 0;
#ifndef FLEXNAMES
			p->in.right->in.name[0] = '\0';
#else
			p->in.right->in.name = "";
#endif
			/* now, if a shift is needed, do it */

			if( o != 0 ){
				/*
				 * space to the right, must shift
				 */
				shp = talloc();
				shp->in.op = RS;
				shp->in.rall = NOPREF;
				shp->in.type = ty;
				shp->in.left = p->in.left;
				shp->in.right = talloc();
				shp->in.right->in.op = ICON;
				shp->in.right->in.rall = NOPREF;
				shp->in.right->in.type = ty;
				shp->in.right->tn.rval = 0;
				shp->in.right->tn.lval = o;  /* amount to shift */
#ifndef FLEXNAMES
				shp->in.right->in.name[0] = '\0';
#else
				shp->in.right->in.name = "";
#endif
				p->in.left = shp;
			}
		} else if( o != 0 ) {
			/*
			 * no space to the left, but space to the right,
			 * must shift
			 */
			p->in.op = RS;
			p->in.rall = NOPREF;
			p->in.type = ty;
			p->in.right = talloc();
			p->in.right->in.op = ICON;
			p->in.right->in.rall = NOPREF;
			p->in.right->in.type = ty;
			p->in.right->tn.rval = 0;
			p->in.right->tn.lval = o;  /* amount to shift */
#ifndef FLEXNAMES
			p->in.right->in.name[0] = '\0';
#else
			p->in.right->in.name = "";
#endif
		}
	}
}
#endif

oreg2( p )
	register NODE *p;
{

	/* look for situations where we can turn * into OREG */

	NODE *q;
	register i;
	register r;
	register char *cp;
	register NODE *ql, *qr;
	CONSZ temp;

	switch(optype(p->in.op)) {
	case LTYPE:
		break;
	case UTYPE:
		oreg2(p->in.left);
		break;
	default:
		oreg2(p->in.left);
		oreg2(p->in.right);
		break;
	}

	if( p->in.op == UNARY MUL ){
		q = p->in.left;
		if( q->in.op == REG ){
			temp = q->tn.lval;
			r = q->tn.rval;
			cp = q->in.name;
			goto ormake;
			}

		if( q->in.op != PLUS && q->in.op != MINUS ) return;
		ql = q->in.left;
		qr = q->in.right;

#ifdef R2REGS

		/* look for doubly indexed expressions */

		if( q->in.op == PLUS) {
			if( (r=base(ql))>=0 && (i=offset(qr, tlen(p)))>=0) {
				makeor2(p, ql, r, i);
				return;
			} else if( (r=base(qr))>=0 && (i=offset(ql, tlen(p)))>=0) {
				makeor2(p, qr, r, i);
				return;
				}
			}


#endif

		if( (q->in.op==PLUS || q->in.op==MINUS) && qr->in.op == ICON &&
				ql->in.op==REG && szty(qr->in.type)==1) {
			temp = qr->tn.lval;
			if( q->in.op == MINUS ) temp = -temp;
			r = ql->tn.rval;
			temp += ql->tn.lval;
			cp = qr->in.name;
			if( *cp && ( q->in.op == MINUS || *ql->in.name ) ) return;
			if( !*cp ) cp = ql->in.name;

			ormake:
			if( notoff( p->in.type, r, temp, cp ) ) return;
			p->in.op = OREG;
			p->tn.rval = r;
			p->tn.lval = temp;
#ifndef FLEXNAMES
			for( i=0; i<NCHNAM; ++i )
				p->in.name[i] = *cp++;
#else
			p->in.name = cp;
#endif
			tfree(q);
			return;
			}
		}

}

canon(p)
	NODE *p;
{
	/* put p in canonical form */
	int oreg2(), sucomp();

#ifndef FIELDOPS
	int ffld();
	fwalk( p, ffld, 0 ); /* look for field operators */
# endif
	oreg2(p);  /* look for and create OREG nodes */
#ifdef MYCANON
	MYCANON(p);  /* your own canonicalization routine(s) */
#endif
	sucomp(p);  /* do the Sethi-Ullman computation */

}


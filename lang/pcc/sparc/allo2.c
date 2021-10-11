#ifndef lint
static	char sccsid[] = "@(#)allo2.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Historically, this file has been called ../mip/allo.c. However,
 * since it is not at all machine independent, it has been moved to
 * this directory. And since there is now an allo1.c, it has been
 * renamed as allo2.c.
 */

# include "cpass2.h"

NODE resc[3];

int busy[REGSZ];

int strtmpoff;		/* current structured temporary offset, relative */
			/* to the beginning of secondary temp space */

int maxstrtmpoff;	/* max over all strtmpoff's in the current procedure */

char strtmpname[20];	/* label equated to offset of the secondary temp */
			/* area.  The label is defined at the end of the */
			/* current procedure; see eobl2(), util2.c  */

#define SMALL_TEMP 4	/* size of "small temp", in words; temps no larger */
			/* than this are allocated in primary temp space, */
			/* i.e., within 4KB of %sp */

static int maxa, mina, maxb, minb, maxc, minc;

static int nexta, nextb, nextc;

#   define ANEXT(x) (x == maxa ? mina : x+1)
#   define BNEXT(x) (x == maxb ? minb : x+1)
#   define CNEXT(x) (x == maxc ? minc : x+1)

allo0()
{ /* free everything */

	register i;

	maxa = maxb = maxc = -1;
	mina = minb = minc = 0;

	REGLOOP(i){
		busy[i] = 0;
		if( rstatus[i] & STAREG ){
			/* tremendous cheat to avoid allocating RG1 */
			if ( i != RG1 ){
				if( maxa<0 ) mina = i;
				maxa = i;
			}
		}
		if( rstatus[i] & STBREG ){
			if( maxb<0 ) minb = i;
			maxb = i;
		}
		if( rstatus[i] & STCREG ){
			if( maxc<0 ) minc = i;
			maxc = i;
		}
	}
	nexta = mina;
	nextb = minb;
	nextc = minc;
}

# define TBUSY 01000

allo( p, q ) 
	NODE *p; struct optab *q; 
{

	register n, i, j;

	n = q->needs;
	i = 0;

	while( n & NACOUNT ){
		resc[i].in.op = REG;
		resc[i].tn.rval = freereg( p, n&NAMASK );
		resc[i].tn.lval = 0;
		resc[i].in.name = "";
		n -= NAREG;
		++i;
	}

	while( n & NBCOUNT ){
		resc[i].in.op = REG;
		resc[i].tn.rval = freereg( p, n&NBMASK );
		resc[i].tn.lval = 0;
		resc[i].in.name = "";
		n -= NBREG;
		++i;
	}

	while( n & NCCOUNT ){
		resc[i].in.op = REG;
		resc[i].tn.rval = freereg( p, n&NCMASK );
		resc[i].tn.lval = 0;
		resc[i].in.name = "";
		n -= NCREG;
		++i;
	}

	if( n & NTMASK ){
		switch(p->in.op) {
		case STCALL:
		case STARG:
		case UNARY STCALL:
		case STASG:
			/*
			 * this is a little wierd, since we can't guarantee
			 * that structured temps will fit in the primary
			 * temp area
			 */
			mktempnode( &resc[i],
			    (SZCHAR*p->stn.stsize + (SZINT-1))/SZINT );
			break;
		default:
			mktempnode( &resc[i], (n&NTMASK)/NTEMP );
			break;
		}
		++i;
	}

	/* turn off "temporarily busy" bit */

	ok:
	REGLOOP(j){
		busy[j] &= ~TBUSY;
	}

	for( j=0; j<i; ++j ) 
		if( resc[j].tn.rval < 0 ) return(0);

	return(1);
}

void
mktempnode( tp, n )
    register NODE *tp;
    int n;
{
    /*
     * make *tp an n-word tempoary OREG.
     * this is just a little extra packaging for freetemp()
     */
    tp->tn.op = OREG;
    tp->tn.rval = TMPREG;
    tp->tn.lval = BITOOR(freetemp( n ));
    tp->tn.name = tmpname;
    tp->tn.rall = NOPREF;
}

/*
 * Allocate and form the address of an n-word temporary area.
 * Note that the storage need not reside within the range of a
 * base+offset addressing mode, and therefore need not be allocated
 * from the same area as register spill temporaries.  Hence we may
 * use a different allocation function for large temps.
 *
 * Regardless of whether or not a temporary is allocated in the primary
 * temp area, the form of its addres is:
 *      (+ (REG TMPREG) (ICON Label+offset))
 * where Label is a label equated to the offset of the appropriate
 * temp area.  These labels are defined at end-of-function time [cf.,
 * eobl2(), util2.c].
 */
NODE *
mktempaddr( n , t)
    int n;
    TWORD t;
{
    register NODE *regnode;
    register NODE *iconnode;
    regnode = build_tn(REG, t, "", 0, TMPREG);
    if (n <= SMALL_TEMP) {
	/*
	 * allocate from the small temp area
	 */
	iconnode = build_tn(ICON, INT, tmpname, BITOOR(freetemp(n)), 0);
    } else {
	/*
	 * allocate from the large temp area
	 */
	iconnode = build_tn(ICON, INT, strtmpname, BITOOR(freestrtemp(n)), 0);
    }
    return build_in(PLUS, t, regnode, iconnode);
}
 
/*
 * Some observations on global allocation variables and the 
 * "portable" compiler structure.
 *   baseoff -- set upon block entry. contains max offset of
 *	variables allocated by front end. if temporaries and
 *	visible variables are allocated together, this is where
 *	temporary allocation starts.
 *   tmpoff -- set with baseoff at block entry. reset to baseoff
 *	at beginning of each expression. if temporaries and 
 *	visible variables are allocated together, this is the 
 *	current temporary allocation offset.
 *   maxoff -- set from baseoff at beginning of function. set to
 *	max( maxoff, baseoff ) at other block entries. may be 
 *	maintained by freetemp and used by eobl2 to calculate
 *	frame size.
 *   maxtemp -- set to 0 at function entry. may be maintained
 *	by freetemp for non-traditional temporary allocation
 *	schemes, like the one we have here.
 */
 
extern unsigned int offsz;

freetemp( k )
{ 
    /* allocate k integers worth of temp space
     * we also make the convention that, if the number of words is more than 1,
     * it must be aligned for storing doubles...  
     * temps are allocated as offsets from TMPREG, either growing towards 
     * smaller addresses if BACKTEMP is set, else toward large addresses.
     * We're using BACKTEMP if the temps, like the locals, are allocated
     * off of the frame pointer. Otherwise, we'll allocate off of sp,
     * ajacent to the parameter list. This is very non-traditional
     */
     
    int t;
#ifdef BACKTEMP
        tmpoff += k*SZINT;
        if( k>1 ) {
                SETOFF( tmpoff, ALDOUBLE );
        }
#else
        if( k>1 ){
                SETOFF( tmpoff, ALDOUBLE );
        }

        t = tmpoff;
        tmpoff += k*SZINT;
#endif
	if( tmpoff > maxoff ) maxoff = tmpoff;
	if( tmpoff >= offsz )
		cerror( "stack overflow" );
	if( tmpoff-baseoff > maxtemp ) maxtemp = tmpoff-baseoff;
#ifdef BACKTEMP
	return( -tmpoff );
#else
	return( t );
#endif
}

/*
 * allocate k integers worth of space in the secondary temp area.
 * Temps in this area are not constrained to reside within 4KB of
 * the stack pointer, so this should be used for things like large
 * structured args and function results.  Like freetemp, we assume:
 *
 * 1) if the number of words is more than 1, it must be aligned
 *    for storing doubles.
 * 2) offsets are allocated either growing towards smaller addresses
 *    if BACKTEMP is set, else toward large addresses.
 * 3) We're using BACKTEMP if the temps, like the locals, are allocated
 *    off of the frame pointer, using negative offsets. Otherwise, we'll
 *    allocate off of sp, using positive offsets.
 */

freestrtemp( k )
{ 
    int t;
#ifdef BACKTEMP
        strtmpoff += k*SZINT;
#endif
        if( k>1 ) {
                SETOFF( strtmpoff, ALDOUBLE );
        }
#ifndef BACKTEMP
        t = strtmpoff;
        strtmpoff += k*SZINT;
#endif
	if( strtmpoff > maxstrtmpoff ) maxstrtmpoff = strtmpoff;
#ifdef BACKTEMP
	return( -strtmpoff );
#else
	return( t );
#endif
}

freereg( p, n ) 
	NODE *p;
{
	/* allocate a register of type n */
	/* p gives the type, if floating */

	register j;

	/* not general; means that only one register (the result) OK for call */
	if( callop(p->in.op) ){
		j = callreg(p);
		if( usable( p, n, j ) ) return( j );
		/* have allocated callreg first */
	}
	j = p->in.rall & ~MUSTDO;
	if( j!=NOPREF && usable(p,n,j) ){ /* needed and not allocated */
		return( j );
	}
	if( n&NAMASK ){
		j = nexta;
		do {
			if ( (rstatus[j]&STAREG) && usable(p,n,j) ) {
				nexta = ANEXT(j);
				return(j);
			}
			j = ANEXT(j);
		} while (j != nexta);
		return(-1);
/*
 *		for( j=mina; j<=maxa; ++j ) if( rstatus[j]&STAREG ){
 *			if( usable(p,n,j) ){
 *				return( j );
 *			}
 *		}
 */
	}
	else if( n &NBMASK ){
		j = nextb;
		do {
			if ( (rstatus[j]&STBREG) && usable(p,n,j) ) {
				nextb = BNEXT(j);
				return(j);
			}
			j = BNEXT(j);
		} while (j != nextb);
		return(-1);
/*
 *		for( j=minb; j<=maxb; ++j ) if( rstatus[j]&STBREG ){
 *			if( usable(p,n,j) ){
 *				return(j);
 *			}
 *		}
 */
	}
	else if( n &NCMASK ){
		j = nextc;
		do {
			if ( (rstatus[j]&STCREG) && usable(p,n,j) ) {
				nextc = CNEXT(j);
				return(j);
			}
			j = CNEXT(j);
		} while (j != nextc);
		return(-1);
/* 
 *		for( j=minc; j<=maxc; ++j ) if( rstatus[j]&STCREG ){
 *			if( usable(p,n,j) ){
 *				return(j);
 *			}
 *		}
 */
	}

	return( -1 );
}

usable( p, n, r ) 
	NODE *p; 
{
	/* decide if register r is usable in tree p to satisfy need n */

	/* checks out, for the moment */
	/* if ( !istreg(r) ) cerror( "usable asked about nontemp register" ); */

	if ( busy[r] > 1 ) return(0);
	if ( isbreg(r) ){
		if ( n&(NAMASK|NCMASK) ) return(0);
	} else if  ( iscreg(r) ){
		if ( n&(NAMASK|NBMASK) ) return(0);
	} else {
		if ( n & (NBMASK|NCMASK) ) return(0);
	}
	if ( n&(NAPAIR|NAPAIRO|NBPAIR|NCPAIR) ) {
		/* even/odd register pair -- must be asked for explicitly */
		if ( r % 8 == 7 ) return (0);
		if ( (r & 01) && !(n&NAPAIRO) ) return(0);
		if ( !istreg(r+1) ) return( 0 );
		if ( busy[r+1] > 1 ) return( 0 );
		if ( ( busy[r] == 0 || shareit( p, r, n ) )
		  && ( busy[r+1] == 0  || shareit( p, r+1, n ) ) ){
			busy[r] |= TBUSY;
			busy[r+1] |= TBUSY;
			return(1);
		} else 
			return(0);
	}
	if ( busy[r] == 0 ) {
		busy[r] |= TBUSY;
		return(1);
	}

	/* busy[r] is 1: is there chance for sharing */
	return( shareit( p, r, n ) );

}

shareit( p, r, n ) 
	NODE *p; 
{
	/* can we make register r available by sharing from p
	   given that the need is n */
	if( (n&(NASL|NBSL|NCSL)) && ushare( p, 'L', r ) ) return(1);
	if( (n&(NASR|NBSR|NCSR)) && ushare( p, 'R', r ) ) return(1);
	return(0);
}

ushare( p, f, r ) 
	NODE *p; 
{
	/* can we find a register r to share on the left or right
		(as f=='L' or 'R', respectively) of p */
	p = getlr( p, f );
	if( p->in.op == UNARY MUL ) p = p->in.left;
	if( p->in.op == OREG ){
		if( R2TEST(p->tn.rval) ){
			return( r==R2UPK1(p->tn.rval) || r==R2UPK2(p->tn.rval) );
		}
		else 
			return( r == p->tn.rval );
	}
	if( p->in.op == REG ){
		return( r == p->tn.rval || 
			( szty(p->in.type) == 2 && r==p->tn.rval+1 ) );
	}
	return(0);
}

#ifdef VAX
recl2( p ) 
	register NODE *p; 
{
	register r = p->tn.rval;
#ifndef OLD
	int op = p->in.op;
	switch(optype(op))
	case LTYPE:
		break;
	case UTYPE:
		recl2(p->in.left);
		return;
	case BITYPE:
		recl2(p->in.left);
		recl2(p->in.right);
		return;
	}
	if (op == REG && r >= REGSZ)
		op = OREG;
	if( op == REG ) rfree( r, p->in.type );
	else if( op == OREG ) {
		if( R2TEST( r ) ) {
			if( R2UPK1( r ) != 100 ) rfree( R2UPK1( r ), PTR+INT );
			rfree( R2UPK2( r ), INT );
		} else {
			rfree( r, PTR+INT );
		}
	}
#else
	switch(optype(p->in.op))
	case LTYPE:
		break;
	case UTYPE:
		recl2(p->in.left);
		return;
	case BITYPE:
		recl2(p->in.left);
		recl2(p->in.right);
		return;
	}
	if( p->in.op == REG ) rfree( r, p->in.type );
	else if( p->in.op == OREG ) {
		if( R2TEST( r ) ) {
			if( R2UPK1( r ) != 100 ) rfree( R2UPK1( r ), PTR+INT );
			rfree( R2UPK2( r ), INT );
		} else {
			rfree( r, PTR+INT );
		}
	}
#endif
}
#else
recl2( p ) 
	register NODE *p; 
{
	register r = p->tn.rval;
	switch(optype(p->in.op)) {
	case LTYPE:
		break;
	case UTYPE:
		recl2(p->in.left);
		return;
	case BITYPE:
		recl2(p->in.left);
		recl2(p->in.right);
		return;
	}
	if( p->in.op == REG ) rfree( r, p->in.type );
	else if( p->in.op == OREG ) {
		if( R2TEST( r ) ) {
			rfree( R2UPK1( r ), PTR+INT );
			rfree( R2UPK2( r ), INT );
		} else {
			rfree( r, PTR+INT );
		}
	}
}
#endif


int rdebug = 0;

rfree( r, t ) TWORD t; {
	/* mark register r free, if it is legal to do so */
	/* t is the type */

# ifndef BUG3
	if( rdebug ){
		printf( "rfree( %s ), size %d\n", rnames[r], szty(t) );
	}
# endif
	if(picflag && r == BASEREG) return;

	if( istreg(r) ){
		if( --busy[r] < 0 ) cerror( "register overfreed");
		if( szty(t) == 2 ){
			if( iscreg(r) ) return; /* c-regs hold anything */
			if( istreg(r) != istreg(r+1) ) cerror( "illegal free" );
			if( --busy[r+1] < 0 ) cerror( "register overfreed" );
		}
	}
}

rbusy(r,t) 
	TWORD t; 
{
	/* mark register r busy */
	/* t is the type */

# ifndef BUG3
	if( rdebug ){
		printf( "rbusy( %s ), size %d\n", rnames[r], szty(t) );
	}
# endif

	if( istreg(r) ) ++busy[r];
	if( szty(t) == 2 ){
		if( istreg(r+1) ) ++busy[r+1];
		/* floating registers come in even/odd pairs */
		if( ( isbreg(r)  && (r&1) ) || (istreg(r)^istreg(r+1)) )
			cerror( "rbusy: illegal register pair" );
	}
}

# ifndef BUG3
rwprint( rw ){ /* print rewriting rule */
	register i, flag;
	static char * rwnames[] = {

		"RLEFT",
		"RRIGHT",
		"RESC1",
		"RESC2",
		"RESC3",
		0,
	};

	if( rw == RNULL ){
		print_str( "RNULL" );
		return;
	}

	if( rw == RNOP ){
		print_str( "RNOP" );
		return;
	}

	flag = 0;
	for( i=0; rwnames[i]; ++i ){
		if( rw & (1<<i) ){
			if( flag ) putchar('|');
			++flag;
			print_str( rwnames[i] );
		}
	}
}
# endif

reclaim( p, rw, cookie ) 
	NODE *p;
{
	register NODE **qq;
	register NODE *q;
	register i;
	NODE *recres[5];
	struct respref *r;

	/* get back stuff */

# ifndef BUG3
	if( rdebug ){
		printf( "reclaim( %o, ", p );
		rwprint( rw );
		print_str( ", " );
		prcook( cookie );
		print_str( " )\n" );
	}
# endif

	if( rw == RNOP || ( p->in.op==FREE && rw==RNULL ) ) return;  /* do nothing */

	recl2(p);

	if( callop(p->in.op) ){
		/* check that all scratch regs are free */
		callchk(p);  /* ordinarily, this is the same as allchk() */
	}

	if( rw == RNULL || (cookie&FOREFF) ){ /* totally clobber, leaving nothing */
		tfree(p);
		return;
	}

	/* handle condition codes specially */

	if( (cookie & FORCC) && (rw&(RESFCC|RESCC)) ) {
		/*
		 * result is CC register. Machines with
		 * coprocessors may have more than one (bletch)
		 */
		tfree(p);
		p->in.op = (rw&RESFCC ? FCCODES : CCODES);
		p->tn.lval = 0;
		p->tn.rval = 0;
		return;
	}

	/* locate results */

	qq = recres;

	if( rw&RLEFT) *qq++ = getlr( p, 'L' );;
	if( rw&RRIGHT ) *qq++ = getlr( p, 'R' );
	if( rw&RESC1 ) *qq++ = &resc[0];
	if( rw&RESC2 ) *qq++ = &resc[1];
	if( rw&RESC3 ) *qq++ = &resc[2];

	if( qq == recres ){
		cerror( "illegal reclaim");
	}

	*qq = NIL;

	/* now, select the best result, based on the cookie */

	for( r=respref; r->cform; ++r ){
		if( cookie & r->cform ){
			for( qq=recres; (q= *qq) != NIL; ++qq ){
				if( tshape( q, r->mform ) ) goto gotit;
			}
		}
	}

	/* we can't do it; die */
	cerror( "cannot reclaim");

	gotit:

	if( p->in.op == STARG ) p = p->in.left;  /* STARGs are still STARGS */

	q->in.type = p->in.type;  /* to make multi-register allocations work */
		/* maybe there is a better way! */
	q = tcopy(q);

	tfree(p);

	p->in.op = q->in.op;
	p->tn.lval = q->tn.lval;
	p->tn.rval = q->tn.rval;
	p->in.name = q->in.name;
	p->in.flags = q->in.flags;

	q->in.op = FREE;

	/* if the thing is in a register, adjust the type */

	switch( p->in.op ){

	case REG:
#ifdef VAX
		if( !rtyflg ){
			/* the C language requires intermediate results to change type */
			/* this is inefficient or impossible on some machines */
			/* the "T" command in match supresses this type changing */
			if( p->in.type == CHAR || p->in.type == SHORT ) p->in.type = INT;
			else if( p->in.type == UCHAR || p->in.type == USHORT ) p->in.type = UNSIGNED;
			else if( p->in.type == FLOAT ) p->in.type = DOUBLE;
		}
#endif
		if( ! (p->in.rall & MUSTDO ) ) return;  /* unless necessary, ignore it */
		i = p->in.rall & ~MUSTDO;
		if( i & NOPREF ) return;
		if( i != p->tn.rval ){
			if( busy[i] || ( szty(p->in.type)==2 && busy[i+1] ) ){
				/*
				 * check for overlapping move: from even-odd
				 * pair to an odd-even pair.  The former are
				 * required by the architecture; the latter
				 * arise because of calling conventions.
				 */
				if (i != p->tn.rval+1 && i != p->tn.rval-1) {
					cerror( "faulty register move" );
				}
			}
			rbusy( i, p->in.type );
			rfree( p->tn.rval, p->in.type );
			rmove( i, p->tn.rval, p->in.type );
			p->tn.rval = i;
		}

	case OREG:
#ifdef VAX
		if( p->in.op == REG || !R2TEST(p->tn.rval) ) {
			if( busy[p->tn.rval]>1 && istreg(p->tn.rval) ) cerror( "potential register overwrite");
		} else
			if( (R2UPK1(p->tn.rval) != 100 && busy[R2UPK1(p->tn.rval)]>1 && istreg(R2UPK1(p->tn.rval)) )
				|| (busy[R2UPK2(p->tn.rval)]>1 && istreg(R2UPK2(p->tn.rval)) ) )
			   cerror( "potential register overwrite");
#else
		if( R2TEST(p->tn.rval) ){
			int r1, r2;
			r1 = R2UPK1(p->tn.rval);
			r2 = R2UPK2(p->tn.rval);
			if( (busy[r1]>1 && istreg(r1)) || (busy[r2]>1 && istreg(r2)) ){
				cerror( "potential register overwrite" );
			}
		}
		else if( (busy[p->tn.rval]>1) && istreg(p->tn.rval) ) cerror( "potential register overwrite");
#endif
	}

}

NODE *
tcopy( p ) 
	register NODE *p; 
{
	/* make a fresh copy of p */

	register NODE *q;
	register r;

	ncopy( q=talloc(), p );
	r = p->tn.rval;
	if( p->in.op == REG ) rbusy( r, p->in.type );
	else if( p->in.op == OREG ) {
		if( R2TEST(r) ){
			if( R2UPK1(r) != 100 ) rbusy( R2UPK1(r), PTR+INT );
			rbusy( R2UPK2(r), INT );
		} else {
			rbusy( r, PTR+INT );
		}
	}

	switch( optype(q->in.op) ){

	case BITYPE:
		q->in.right = tcopy(p->in.right);
	case UTYPE:
		q->in.left = tcopy(p->in.left);
	}

	return(q);
}

allchk(){
	/* check to ensure that all register are free */

	register i;

	REGLOOP(i){
		if( istreg(i) && busy[i] ){
			if (mustbefree(i))
			    cerror( "register allocation error");
		}
	}

}


int
count_free_dregs(){
	register i, n = 0;
	
	REGLOOP(i){
		if( i != RG1 && (rstatus[i] & STAREG) && !busy[i] )
			n+= 1;
	}
	return n;
}

/*
 * the code to manage the rstatus stuff was previously in util2.c,
 * as it was considered machine dependent, while all the above stuff
 * was not. It logically belongs here.
 */
 
 int rstatus[] = {
	    0,					/*  g0       */
	    SAREG|STAREG,	SAREG,		/*  g1,  g2  */
	    SAREG,		SAREG,		/*  g3,  g4  */
	    SAREG,     		SAREG,		/*  g5,  g6  */
	    SAREG,				/*  g7       */

	    SAREG|STAREG,	SAREG|STAREG,	/*  o0,  o1  */
	    SAREG|STAREG,	SAREG|STAREG,	/*  o2,  o3  */
	    SAREG|STAREG,	SAREG|STAREG,	/*  o4,  o5  */
	    SAREG,      	SAREG|STAREG,	/*  o6,  o7  */
	    SAREG,		SAREG,		/*  l0,  l1  */
	    SAREG,		SAREG,		/*  l2,  l3  */
	    SAREG,		SAREG,		/*  l4,  l5  */
	    SAREG,		SAREG,		/*  l6,  l7  */
	    SAREG,		SAREG,		/*  i0,  i1  */
	    SAREG,		SAREG,		/*  i2,  i3  */
	    SAREG,		SAREG,		/*  i4,  i5  */
	    SAREG,      	SAREG,		/*  i6,  i7  */

	    SBREG|STBREG,	SBREG|STBREG,
	    SBREG|STBREG,	SBREG|STBREG,
	    SBREG|STBREG,	SBREG|STBREG,
	    SBREG|STBREG,	SBREG|STBREG,
	    SBREG|STBREG,	SBREG|STBREG,
	    SBREG|STBREG,	SBREG|STBREG,
	    SBREG|STBREG,	SBREG|STBREG,
	    SBREG|STBREG,	SBREG|STBREG,
	    SBREG|STBREG,	SBREG|STBREG,
	    SBREG|STBREG,	SBREG|STBREG,
	    SBREG|STBREG,	SBREG|STBREG,
	    SBREG|STBREG,	SBREG|STBREG,
	    SBREG|STBREG,	SBREG|STBREG,
	    SBREG|STBREG,	SBREG|STBREG,
	    SBREG|STBREG,	SBREG|STBREG,
	    SBREG|STBREG,	SBREG|STBREG,
	    SBREG,		SBREG,

    };

/*
 * Set up temporary registers.
 * Use any left over from register
 * variable allocation for scratch.
 */
int dregmap;
int fregmap;

setregs()
{
	register i;
	int navail, nfavail, ntosave;

	dregmap = maxtreg;
	fregmap = maxfreg;
	
	navail = 0;
	for( i=MIN_DVAR; i<=MAX_DVAR; i++ ){
		if (D_AVAIL( i, dregmap)){
#		    ifdef FORT
			markused(i);
#		    endif
		    rstatus[i] = SAREG|STAREG;
		    navail += 1;
		 } else {
		     rstatus[i] = SAREG;
		 }
	}
	nfavail = 0;
	for( i=MIN_FVAR; i<=MAX_FVAR; i++ ){
		if (F_AVAIL( i, fregmap)){
#		    ifdef FORT
			markused(i);
#		    endif
		    rstatus[i] = SBREG|STBREG;
		    nfavail += 1;
		 } else {
		     rstatus[i] = SBREG;
		 }
        }

	if (picflag) rstatus[BASEREG] = SAREG;

	fregs.d = navail + RO5 -RO0 + 1 + 1; /* count RO7 too */
	fregs.f = nfavail + MIN_FVAR-FR0;

	ntosave = MAX_FVAR - MIN_FVAR + 1 - nfavail;
	for ( i = MIN_GREG; i <= MAX_GREG; i++ )
		if (!D_AVAIL(i, dregmap))
			ntosave += 1;
	tmpoff = baseoff = ntosave * SZINT;
	if (baseoff>maxoff) maxoff = baseoff;
	allo0();	/* rstatus changed, so update max*, min* */

}

int
mustbefree( regno )
{
    return (RG0 <= regno && regno <= RO7 );
}


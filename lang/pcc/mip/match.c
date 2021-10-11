#ifndef lint
static	char sccsid[] = "@(#)match.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

# include "cpass2.h"

# ifdef WCARD1
# ifdef WCARD2
# define NOINDIRECT
# endif
# endif

extern vdebug;

int fldsz, fldshf;

int mamask[] = { /* masks for matching dope with shapes */
	SIMPFLG,	/* OPSIMP */
	SIMPFLG|ASGFLG,	/* ASG OPSIMP */
	COMMFLG,	/* OPCOMM */
	COMMFLG|ASGFLG,	/* ASG OPCOMM */
	MULFLG,		/* OPMUL */
	MULFLG|ASGFLG,	/* ASG OPMUL */
	DIVFLG,		/* OPDIV */
	DIVFLG|ASGFLG,	/* ASG OPDIV */
	UTYPE,		/* OPUNARY */
	TYFLG,		/* ASG OPUNARY is senseless */
	LTYPE,		/* OPLEAF */
	TYFLG,		/* ASG OPLEAF is senseless */
	0,		/* OPANY */
	ASGOPFLG|ASGFLG,/* ASG OPANY */
	LOGFLG,		/* OPLOG */
	TYFLG,		/* ASG OPLOG is senseless */
	FLOFLG,		/* OPFLOAT */
	FLOFLG|ASGFLG,	/* ASG OPFLOAT */
	SHFFLG,		/* OPSHFT */
	SHFFLG|ASGFLG,	/* ASG OPSHIFT */
	SPFLG,		/* OPLTYPE */
	TYFLG,		/* ASG OPLTYPE is senseless */
	INTRFLG,	/* OPINTR */
	TYFLG,		/* ASG OPINTR is senseless */
	LIBRFLG,	/* OPLIBR */
	TYFLG,		/* ASG OPLIBR is senseless */
};

int sdebug = 0;

tshape( p, shape ) 
	register NODE *p;
	register shape;
{
	/*
	 * return true if shape is appropriate for the node p
	 * side effect for SFLD is to set up fldsz,etc
	 */
	register o, mask;
	register lval;

	o = p->in.op;

# ifndef BUG3
	if ( sdebug ){
		printf( "tshape( %o, %o), op = %d\n", p, shape, o );
	}
# endif

	if ( shape & SPECIAL ){
		switch ( shape ){
		case SZERO:
			if ( o != ICON || p->in.name[0] ) return(0);
			return(p->tn.lval == 0);
		case SONE:
			if ( o != ICON || p->in.name[0] ) return(0);
			return(p->tn.lval == 1);
		case SMONE:
			if ( o != ICON || p->in.name[0] ) return(0);
			return(p->tn.lval == -1);
		case SCCON:
			if ( o != ICON || p->in.name[0] ) return(0);
			lval = p->tn.lval;
			return(lval >= -128 && lval <= 127);
		case SSOREG:	/* non-indexed OREG */
			return(o == OREG && !R2TEST(p->tn.rval));

		default:
# ifdef MULTILEVEL
			if ( shape & MULTILEVEL )
				return( mlmatch(p,shape,0) );
			else
# endif
			return (special( p, shape ));
		}
	}
	if ( shape & (SANY|INTEMP|SWADD) ) {
		if ( shape & SANY ) return(1);
		if ( (shape&INTEMP) && shtemp(p) ) return(1);
		if ( (shape&SWADD) && (o==NAME||o==OREG) ){
			if ( BYTEOFF(p->tn.lval) ) return(0);
		}
	}

# ifdef WCARD1
	if ( shape & WCARD1 )
		return( wcard1(p) & shape );
# endif

# ifdef WCARD2
	if ( shape & WCARD2 )
		return( wcard2(p) & shape );
# endif
	switch( o ){
	case NAME:
		return( shape&SNAME );
	case FCON:
	case ICON:
		return( shape & SCON );
	case FLD:
		if ( shape & SFLD ){
			if ( !flshape( p->in.left ) ) return(0);
			/* it is a FIELD shape; make side-effects */
			o = p->tn.rval;
			fldsz = UPKFSZ(o);
# ifdef RTOLBYTES
			fldshf = UPKFOFF(o);
# else
#	if TARGET == SPARC
			fldshf = SZCHAR*tlen(p->in.left) - fldsz - UPKFOFF(o);
#	endif
#	if TARGET == MC68000
			fldshf = SZINT - fldsz - UPKFOFF(o);
#	endif
# endif
			return(1);
		}
		return(0);
	case FCCODES:
	case CCODES:
		return( shape&SCC );
	case REG:
		/* distinctions:
		 * SAREG	any scalar register
		 * STAREG	any temporary scalar register
		 * SBREG	any lvalue (index) register
		 * STBREG	any temporary lvalue register
		 * SCREG	any floating register
		 * STCREG	any temporary floating register
		 */
		mask = rstatus[p->tn.rval];
		if ( (mask&(STAREG|STBREG|STCREG)) && busy[p->tn.rval]>1 ) {
			mask &= ~(STAREG|STBREG|STCREG);
		}
		return( shape & mask );
	case OREG:
		return( shape & SOREG );

# ifndef NOINDIRECT
	case UNARY MUL:
		/* return STARNM or STARREG or 0 */
		return( shumul(p->in.left) & shape );
# endif

	}

	return(0);
}

int tdebug = 0;

ttype( t, tword ) 
	register TWORD t;
	register tword;
{
	/* does the type t match tword */

	if ( tword & TANY ) return(1);
	if ( t == TVOID ) t=INT; /* void functions eased thru tables */
# ifndef BUG3
	if ( tdebug ){
		printf( "ttype( %o, %o )\n", t, tword );
	}
# endif
	if ( ISPTR(t) && (tword&TPTRTO) ) {
		do {
			t = DECREF(t);
		} while ( ISARY(t) );
		/* arrays that are left are usually only
		   in structure references... */
		return( ttype( t, tword&(~TPTRTO) ) );
	}
	if ( t != BTYPE(t) )
		return( tword & TPOINT );	/* TPOINT means not simple! */
	if ( tword & TPTRTO ) return(0);
	switch( t ){
	case CHAR:
		return( tword & TCHAR );
	case SHORT:
		return( tword & TSHORT );
	case STRTY:
	case UNIONTY:
		return( tword & TSTRUCT );
	case INT:
		return( tword & TINT );
	case UNSIGNED:
		return( tword & TUNSIGNED );
	case USHORT:
		return( tword & TUSHORT );
	case UCHAR:
		return( tword & TUCHAR );
	case ULONG:
		return( tword & TULONG );
	case LONG:
		return( tword & TLONG );
	case FLOAT:
		return( tword & TFLOAT );
	case DOUBLE:
		return( tword & TDOUBLE );
	}
	return(0);
}

#define MAXOPSET 2000

struct opvect {
	struct optab **first;
	struct optab **rewrite;
};

struct opvect opvect[DSIZE];
struct optab *opsets[MAXOPSET];

static struct optab **firstelement= opsets;
static struct optab **nextelement = opsets;

extern int opmatch(/* op, targetop */);	/* returns 1 if op matches targetop */

/*
 * setrew()
 *	Generates sets of templates to be examined by match(), one
 *	for each operator in 0..DSIZE.  The sets are represented as
 *	vectors of pointers into the code table.  The sets could be
 *	constructed by a separate program when the compiler is compiled,
 *	but doing it at startup enables us to deal more conveniently
 *	with user-selected processor options (e.g., -m68020 -m68881)
 */

setrew()
{
	register struct optab *q;
	register unsigned op;
	register struct optab **nextel = nextelement;
	register struct optab **firstel = firstelement;

	for( op = 0; op < DSIZE; ++op ) {
		if (dope[op]) {
			/*
			 * operator exists;
			 * scan table for matching templates
			 */
			register struct optab **firstrew;
			firstrew = (struct optab**)NULL;
			for (q = table; q->op != FREE; q++) {
				if (opmatch(op, q->op)) {
					if (q->needs == REWRITE) {
						if (!firstrew) {
							firstrew = nextel;
						}
					}
					*nextel++ = q;
				}
			}
			if (nextel-opsets >= MAXOPSET) {
				cerror("too many opsets (%d)",
					nextel-opsets);
			}
			opvect[op].first = firstel;
			opvect[op].rewrite = firstrew;
			*nextel++ = (struct optab*)NULL;
			firstel = nextel;
		} else {
			/* no operator */
			opvect[op].first = (struct optab**)NULL;
			opvect[op].rewrite = (struct optab**)NULL;
		} /* else */
	} /* for each op */
	nextelement = nextel;
	firstelement = firstel;
} /* setrew */

match( p, cookie ) 
	NODE *p; 
{
	/*
	 * called by: order, gencall
	 * look for match in table and generate code if found unless
	 * entry specified REWRITE.
	 * returns MDONE, MNOPE, or rewrite specification from table 
	 */

	register struct optab **opset;
	register struct optab *q;
	register op;
	register NODE *l, *r;
	register TWORD ltype, rtype;

	rcount();
	op = p->in.op;
	if (cookie == FORREW) {
		opset = opvect[op].rewrite;
	} else {
		opset = opvect[op].first;
		/*
		 * finish the special-case test for autoincrement trees
		 */
		if (op == UNARY MUL && !shumul(p->in.left)) {
			return(MNOPE);
		}
	}
	if (opset == (struct optab**)NULL) {
		cerror("no template for op '%s'", opst[op]);
	}
	switch(optype(op)) {
	case LTYPE:
		l = r = p;
		break;
	case UTYPE:
		l = p->in.left;
		r = p;
		break;
	case BITYPE:
		l = p->in.left;
		r = p->in.right;
		break;
	}
	ltype = l->in.type;
	rtype = r->in.type;
	for (q = *opset++; q != (struct optab *)NULL; q = *opset++) {

		/* see if goal matches */
		if ( !(q->visit & cookie ) ) continue;

		/* see if left child matches */
		if ( !tshape( l, q->lshape ) )
			continue;
		if ( !ttype( ltype, q->ltype ) ) 
			continue;

		/* see if right child matches */
		if ( !tshape( r, q->rshape ) )
			continue;
		if ( !ttype( rtype, q->rtype ) )
			continue;

		/*
		 * REWRITE means no code from this match but go ahead
		 * and rewrite node to help future match.
		 */
		if ( q->needs & REWRITE )
			return( q->rewrite );
		if ( !allo( p, q ) )
			continue;	/* if can't generate code, skip entry */

		/* resources are available */

		/* generate code */
		expand( p, cookie, q->cstring );
		reclaim( p, q->rewrite, cookie );

		return(MDONE);

	}

	return(MNOPE);
}

int rtyflg = 0;

expand( p, cookie, cp )
	NODE *p; 
	register char *cp;
{
	/* generate code by interpreting table entry */

	register char c;
	CONSZ val;

	rtyflg = 0;

	for( ; *cp; ++cp ){
		switch( *cp ){

		case '\\':
			cp++;
			/* FALL THROUGH */
		default:
			PUTCHAR( *cp );
			continue;  /* this is the usual case... */
		case 'T':
			/* rewrite register type is suppressed */
			rtyflg = 1;
			continue;

		case 'Z':  /* special machine dependent operations */
			switch( c = *++cp ) {
			case '1':
			case '2':
			case '3':
			case 'R':
			case 'L':	/* get down first */
				zzzcode( getlr( p, c ), *++cp, cookie );
				break;
			default:   /* normal zzzcode processing otherwise */
				zzzcode( p, c, cookie );
				break;
			}
			continue;

		case 'F':  /* this line deleted if FOREFF is active */
			if ( cookie & FOREFF )
				while( *++cp != '\n' )
					; /* VOID */
			continue;

#ifdef undef
		case 'R': /* this line deleted if descendent is already in a temp register */
			{
			NODE * t;

			t = getlr(p, *++cp );
			if ( istnode(t) )
				while( *++cp != '\n' )
					; /* VOID */
			continue;
			}
#endif sun
		case 'S':  /* field size */
			print_d(fldsz );
			continue;

		case 'H':  /* field shift */
			print_d( fldshf );
			continue;

		case 'M':  /* field mask */
		case 'N':  /* complement of field mask */
			val = lshift(bitmask(fldsz), fldshf);
			adrcon( *cp=='M' ? val : ~val );
			continue;

		case 'J':  /* struct size */
			print_d( p->stn.stsize );
			continue;

		case 'L':  /* output special label field */
			print_d( p->bn.label );
			continue;

		case 'O':  /* opcode string */
			hopcode( *++cp, p->in.op );
			continue;

		case 'B':  /* byte offset in word */
			val = getlr(p,*++cp)->tn.lval;
			val = BYTEOFF(val);
			printf( CONFMT, val );
			continue;

		case 'C': /* for constant value only */
			conput( getlr( p, *++cp ) );
			continue;

		case 'I': /* in instruction */
			insput( getlr( p, *++cp ) );
			continue;

		case 'A': /* address of */
			adrput( getlr( p, *++cp ) );
			continue;

		case 'U': /* for upper half of address, only */
			upput( getlr( p, *++cp ) );
			continue;

		}

	}

}

NODE *
getlr( p, c )
	NODE *p;
{

	/*
	 * return the pointer to the left or right side of p, or p itself,
	 * depending on the optype of p
	 */

	switch ( c ) {

	case '1':
	case '2':
	case '3':
		return ( &resc[c-'1'] );

	case 'L':
		return ( optype( p->in.op ) == LTYPE ? p : p->in.left );

	case 'R':
		return ( optype( p->in.op ) != BITYPE ? p : p->in.right );

	case '.':
		return ( p );

	}
	cerror( "bad getlr: %c", c );
	/* NOTREACHED */
}

# ifdef MULTILEVEL

union mltemplate{
	struct ml_head{
		int tag; /* identifies class of tree */
		int subtag; /* subclass of tree */
		union mltemplate * nexthead; /* linked by mlinit() */
		} mlhead;
	struct ml_node{
		int op; /* either an operator or op description */
		int nshape; /* shape of node */
		/* both op and nshape must match the node.
		 * where the work is to be done entirely by
		 * op, nshape can be SANY, visa versa, op can
		 * be OPANY.
		 */
		int ntype; /* type descriptor from mfile2 */
		} mlnode;
	};

# define MLSZ 30

extern union mltemplate mltree[];
int mlstack[MLSZ];
int *mlsp; /* pointing into mlstack */
NODE * ststack[MLSZ];
NODE **stp; /* pointing into ststack */

mlinit(){
	union mltemplate **lastlink;
	register union mltemplate *n;
	register mlop;

	lastlink = &(mltree[0].nexthead);
	n = &mltree[0];
	for( ; (n++)->mlhead.tag != 0;
		*lastlink = ++n, lastlink = &(n->mlhead.nexthead) ){
# ifndef BUG3
		if ( vdebug )print_str_d_nl("mlinit: ",(n-1)->mlhead.tag);
# endif
	/* wander thru a tree with a stack finding
	 * its structure so the next header can be located.
	 */
		mlsp = mlstack;

		for( ;; ++n ){
			if ( (mlop = n->mlnode.op) < OPSIMP ){
				switch( optype(mlop) ){

					default:
						cerror("(1)unknown opcode: %o",mlop);
					case BITYPE:
						goto binary;
					case UTYPE:
						break;
					case LTYPE:
						goto leaf;
					}
				}
			else{
				if ( mamask[mlop-OPSIMP] &
					(SIMPFLG|COMMFLG|MULFLG|DIVFLG|LOGFLG|FLOFLG|SHFFLG) ){
				binary:
					*mlsp++ = BITYPE;
					}
				else if ( ! (mamask[mlop-OPSIMP] & UTYPE) ){/* includes OPANY */

				leaf:
					if ( mlsp == mlstack )
						goto tree_end;
					else if ( *--mlsp != BITYPE )
						cerror("(1)bad multi-level tree descriptor around mltree[%d]",
						n-mltree);
					}
				}
			}
		tree_end: /* n points to final leaf */
		;
		}
# ifndef BUG3
		if ( vdebug > 3 ){
			print_str("mltree={\n");
			for( n= &(mltree[0]); n->mlhead.tag != 0; ++n)
				printf("%o: %d, %d, %o,\n",n,
				n->mlhead.tag,n->mlhead.subtag,n->mlhead.nexthead);
			print_str("	}\n");
			}
# endif
	}

mlmatch( subtree, target, subtarget ) NODE * subtree; int target,subtarget;{
	/*
	 * does subtree match a multi-level tree with
	 * tag "target"?  Return zero on failure,
	 * non-zero subtag on success (or MDONE if
	 * there is a zero subtag field).
	 */
	union mltemplate *head; /* current template header */
	register union mltemplate *n; /* node being matched */
	NODE * st; /* subtree being matched */
	register int mlop;

# ifndef BUG3
	if ( vdebug ) printf("mlmatch(%o,%d)\n",subtree,target);
# endif
	for( head = &(mltree[0]); head->mlhead.tag != 0;
		head=head->mlhead.nexthead){
# ifndef BUG3
		if ( vdebug > 1 )printf("mlmatch head(%o) tag(%d)\n",
			head->mlhead.tag);
# endif
		if ( head->mlhead.tag != target )continue;
		if ( subtarget && head->mlhead.subtag != subtarget)continue;
# ifndef BUG3
		if ( vdebug ) print_str_d_nl("mlmatch for ",target);
# endif

		/* potential for match */

		n = head + 1;
		st = subtree;
		stp = ststack;
		mlsp = mlstack;
		/* compare n->op, ->nshape, ->ntype to
		 * the subtree node st
		 */
		for( ;; ++n ){ /* for each node in multi-level template */
			/* opmatch */
			if ( n->op < OPSIMP ){
				if ( st->op != n->op )break;
				}
			else {
				register opmtemp;
				if ((opmtemp=mamask[n->op-OPSIMP])&SPFLG){
					if (st->op!=NAME && st->op!=ICON && st->op!=OREG && 
						! shltype(st->op,st)) break;
					}
				else if ((dope[st->op]&(opmtemp|ASGFLG))!=opmtemp) break;
				}
			/* check shape and type */

			if ( ! tshape( st, n->mlnode.nshape ) ) break;
			if ( ! ttype( st->type, n->mlnode.ntype ) ) break;

			/* that node matched, let's try another */
			/* must advance both st and n and halt at right time */

			if ( (mlop = n->mlnode.op) < OPSIMP ){
				switch( optype(mlop) ){

					default:
						cerror("(2)unknown opcode: %o",mlop);
					case BITYPE:
						goto binary;
					case UTYPE:
						st = st->left;
						break;
					case LTYPE:
						goto leaf;
					}
				}
			else{
				if ( mamask[mlop - OPSIMP] &
					(SIMPFLG|COMMFLG|MULFLG|DIVFLG|LOGFLG|FLOFLG|SHFFLG) ){
				binary:
					*mlsp++ = BITYPE;
					*stp++ = st;
					st = st->left;
					}
				else if ( ! (mamask[mlop-OPSIMP] & UTYPE) ){/* includes OPANY */

				leaf:
					if ( mlsp == mlstack )
						goto matched;
					else if ( *--mlsp != BITYPE )
						cerror("(2)bad multi-level tree descriptor around mltree[%d]",
						n-mltree);
					st = (*--stp)->right;
					}
				else /* UNARY */ st = st->left;
				}
			continue;

			matched:
			/* complete multi-level match successful */
# ifndef BUG3
			if ( vdebug ) print_str("mlmatch() success\n");
# endif
			if ( head->mlhead.subtag == 0 ) return( MDONE );
			else {
# ifndef BUG3
				if ( vdebug )print_str_d_nl("\treturns ",
					head->mlhead.subtag );
# endif
				return( head->mlhead.subtag );
				}
			}
		}
	return( 0 );
	}
# endif

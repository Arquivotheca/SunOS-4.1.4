#ifndef lint
static	char sccsid[] = "@(#)fort.c 1.1 94/10/31 SMI";
#endif

/*
 * binary file interface for /usr/lib/f1,
 * a pcc trees-based code generator, used by
 * pascal and modula-2; historically used by
 * f77, hence the name 'f1'
 */

# include "cpass2.h"
# include "f1defs.h"
# include "sw.h"

extern int chk_ovfl;

FILE * lrd;  /* for default reading routines */

char *
lnread()
{
	char buf[BUFSIZ];
	register char *cp = buf;
	register char *limit = &buf[BUFSIZ];

	for (;;) {
		if (fread(cp, sizeof (long), 1, lrd) !=  1)
			cerror("intermediate file read error");
		cp += sizeof (long);
		if (cp[-1] == 0)
			break;
		if (cp >= limit)
			cerror("lnread overran string buffer");
	}
	return (tstr(buf));
}

long lread(){
	static long x;
	if( fread( (char *) &x, 4, 1, lrd ) <= 0 ) cerror( "intermediate file read error" );
	return( x );
	}

lopen( s ) char *s; {
	/* if null, opens the standard input */
	if( *s ){
		lrd = fopen( s, "r" );
		if( lrd == NULL ) cerror( "cannot open intermediate file %s", s );
		}
	else  lrd = stdin;
	}

lcread( cp, n ) char *cp; {
	if( n > 0 ){
		if( fread( cp, 4, n, lrd ) != n )
			cerror( "intermediate file read error" );
		}
	}

lccopy( n ) register n; {
	register i;
	static char fbuf[32*4];
	if( n > 0 ){
		while ( n > 32){
			if ( fread( fbuf, 4, 32, lrd ) !=  32)
				cerror( "intermediate file read error" );
			if ( fwrite( fbuf, 4, 32, stdout ) != 32 ) 
				cerror( "output file error" );
			n -= 32;
			}
		if( fread( fbuf, 4, n, lrd ) != n )
			cerror( "intermediate file read error" );
		for( i=4*n; fbuf[i-1] == '\0' && i>0; --i ) { /* VOID */ }
		if( i ) {
			if( fwrite( fbuf, 1, i, stdout ) != i )
				cerror( "output file error" );
			}
		}
	}

/*
 * The code generator wants TVOID to be an explicitly
 * defined type; front ends other than C don't bother.
 */
static TWORD
pcc_type(t)
	TWORD t;
{
	return (t == UNDEF ? TVOID : t);
}

/*
 * stack for reading nodes in postfix form
 */

# define NSTACKSZ 250

NODE * fstack[NSTACKSZ];
NODE ** fsp;  /* points to next free position on the stack */

main( argc, argv ) char *argv[]; {
	int files;
	register long x;
	register NODE *p;
	register i,ncases;
	register struct sw *swtab;
	int align, sz;
	TWORD type;
	OFFSZ aoff;
	int flags;
	int unsafe_call;

	files = p2init( argc, argv );
	tinit();

		
	if( files ){
		while( files < argc && argv[files][0] == '-' ) {
			++files;
			}
		if( files > argc ) exit( nerrors );
		lopen( argv[files] );
		}
	else lopen( "" );

	fsp = fstack;

	unsafe_call = 0;
	for(;;){
		/* read nodes, and go to work... */
		x = lread();

		if( xdebug ) {
			fprintf( stderr, "op=%d, val = %d, rest = 0%o\n",
				FOP(x), FVAL(x), (int)FREST(x) );
		}
		switch( (int)FOP(x) ){  /* switch on opcode */

		case 0:
			fprintf( stderr, "null opcode ignored\n" );
			continue;
		case FTEXT:
			lccopy( FVAL(x) );
			putchar('\n');
			continue;

		case FLBRAC:
			flags = FVAL(x);
			ftnno = FREST(x);
			maxtreg = lread();
			maxfreg = lread();
			aoff = lread();
			TMP_BBEG;
			if (tmpoff%ALSTACK)
			    cerror("intermediate file-stack misalignment");
			/* enable/disable range checking */
			if( flags & FCHECK_FLAG ){
				chk_ovfl = 1;
			} else {
				chk_ovfl = 0;
			}
			if( flags & FENTRY_FLAG ){
				/* beginning of function */
				TMP_BFN;
				saveregs();
				bfcode2();
			} else {
				/* maxoff at end of ftn is max of autos and
				   temps over all blocks in the function */
				TMP_NFN;
			}
			setregs();
			continue;

		case FRBRAC:
			TMP_BEND;
			align = FREST(x);
			type = pcc_type(lread());
			sz = lread();
			eobl2(type, sz, align, !unsafe_call);
			unsafe_call = 0;
			continue;

		case FEOF:
#			if TARGET == MC68000
			    floatnote();
#			endif
			exit( nerrors );

		case FSWITCH:
			ncases = lread();
			swtab = (struct sw*)malloc(ncases*sizeof(struct sw));
			/* read n cases; case 0 is the default. */
			for (i = 0; i < ncases; i++) {
				swtab[i].sval = lread();
				swtab[i].slab = lread();
			}
			if (FVAL(x)) {
				/*unary*/
				p2switch((NODE*)*--fsp, swtab, ncases);
			} else {
				/* assume selector in FORCE reg */
				p2switch(NULL,swtab,ncases);
			}
			free(swtab);
			continue;

		case FCON:	/* floating point constant */
			p = talloc();
			p->in.op = FCON;
			p->fpn.type = pcc_type(FREST(x));
			lcread( (char*)&p->fpn.dval, 2 );
			goto bump;

		case ICON:
			p = talloc();
			p->in.op = ICON;
			p->in.type = pcc_type(FREST(x));
			p->tn.rval = 0;
			p->tn.lval = lread();
			if( FVAL(x) ){
#ifndef FLEXNAMES
				lcread( p->in.name, 2 );
#else
				p->in.name = lnread();
				/*
				 * KLUDGE: watch for setjmp, _setjmp here
				 */
				if (strcmp(p->in.name, "_setjmp") == 0
				    || strcmp(p->in.name, "__setjmp") == 0)
					unsafe_call = 1;
#endif
				}
#ifndef FLEXNAMES
			else p->in.name[0] = '\0';
#else
			else p->in.name = "";
#endif

		bump:
/*			p->in.su = 0; */
			p->in.rall = NOPREF;
			*fsp++ = p;
			if( fsp >= &fstack[NSTACKSZ] ) uerror( "expression depth exceeded" );
			continue;

		case NAME:
			p = talloc();
			p->in.op = NAME;
			p->in.type = pcc_type(FREST(x));
			p->tn.rval = 0;
			if( FVAL(x) ) p->tn.lval = lread();
			else p->tn.lval = 0;
#ifndef FLEXNAMES
			lcread( p->in.name, 2 );
#else
			p->in.name = lnread();
#endif
			goto bump;

		case OREG:
			p = talloc();
			p->in.op = OREG;
			p->in.type = pcc_type(FREST(x));
			p->tn.rval = FVAL(x);
			p->tn.lval = lread();
#ifndef FLEXNAMES
			lcread( p->in.name, 2 );
#else
			p->in.name = lnread();
#endif
			goto bump;

		case REG:
			p = talloc();
			p->in.op = REG;
			p->in.type = pcc_type(FREST(x));
			p->tn.rval = FVAL(x);
			rbusy( p->tn.rval, p->in.type );
			p->tn.lval = 0;
#ifndef FLEXNAMES
			p->in.name[0] = '\0';
#else
			p->in.name = "";
#endif
			goto bump;

		case FEXPR:
			lineno = FREST(x);
			if( FVAL(x) ) lcread( filename, FVAL(x) );
			if( fsp == fstack ) continue;  /* filename only */
			if( --fsp != fstack ) uerror( "expression poorly formed" );
			if( lflag ) lineid( lineno, filename );
			p = fstack[0];
			if( edebug ) fwalk( p, eprint, 0 );
# ifdef MYREADER
			MYREADER(p);
# endif

			nrecur = 0;
			delay( p );
			reclaim( p, RNULL, 0 );

			allchk();
			tcheck();
			TMP_EXPR;
			continue;

		case FLABEL:
			p2deflab(lread());
			continue;

		case GOTO:
			if( FVAL(x) ) {
				p2branch(lread());
				continue;
				}
			/* otherwise, treat as unary */
			goto def;

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
			p = talloc();
			p->bn.op = FOP(x);
			p->bn.type = pcc_type(FREST(x));
			p->bn.reversed = 0;
			p->in.right = *--fsp;
			p->in.left = *--fsp;
			goto bump;

		case STASG:
		case STARG:
		case STCALL:
		case UNARY STCALL:
			    /*
			     * size and alignment come from next long words
			     */
			p = talloc();
			p -> stn.stsize = lread();
			p -> stn.stalign = lread();
			goto defa;
		default:
		def:
			p = talloc();
		defa:
			p->in.op = FOP(x);
			p->in.type = pcc_type(FREST(x));

			switch( optype( p->in.op ) ){

			case BITYPE:
				p->in.right = *--fsp;
				p->in.left = *--fsp;
				goto bump;

			case UTYPE:
				p->in.left = *--fsp;
				p->tn.rval = 0;
				goto bump;

			case LTYPE:
				uerror( "illegal leaf node: %d", p->in.op );
				exit( 1 );
				}
			}
		}
	}

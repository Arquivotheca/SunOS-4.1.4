#ifndef lint
static	char sccsid[] = "@(#)pftn.c 1.1 94/10/31 SMI"; /* from S5R2 1.2 */
#endif

# include "cpass1.h"
# include "messages.h"

/*
 * Guy's changes to merge with System V lint:
 *
 * Here's the updated "pftn.c".  Again, the main changes were for the
 * message scheme.  Some minor "lint" cleanup was done (eliminating unused
 * variables, adding proper declarations of functions, adding casts for
 * function arguments).  Some "register" declarations were added (I think
 * these were inspired by 4.3BSD).
 * 
 * Explicit checks were added for "malloc" and "realloc" returning NULL;
 * the "malloc_with_care" trick didn't work when building either "lint" or
 * "cxref", I forget which.
 */

#ifdef LINT
#    define regvar_init()
#    define regvar_alloc(type) 1
#    define regvar_avail(type) 1
#else  !LINT
     extern void regvar_init();
     extern int regvar_alloc();
     extern int regvar_avail();
#endif !LINT

extern int errline,lineno;
extern int twopass;

struct instk {
	int in_sz;   /* size of array element */
	int in_x;    /* current index for structure member in structure initializations */
	int in_n;    /* number of initializations seen */
	int in_s;    /* sizoff */
	int in_d;    /* dimoff */
	TWORD in_t;    /* type */
	int in_id;   /* stab index */
	int in_fl;   /* flag which says if this level is controlled by {} */
	OFFSZ in_off;  /* offset of the beginning of this level */
	}
instack[100],
*pstk;

	/* defines used for getting things off of the initialization stack */




int ddebug = 0;
static char *curname; /* used for printing error messages */

static int fixclass();

/* (cautiously) handle static-extern conflicts */
static void redecl_err(/*sym, oldclass, newclass*/);

struct symtab * mknonuniq();

defid( q, class ) register NODE *q; {
	register struct symtab *p;
	int idp;
	register TWORD type;
	TWORD stp;
	register int scl;
	int dsym, ddef;
	int slev, temp;
 	int changed;

	if( q == NIL ) return;  /* an error was detected */

	idp = q->tn.rval;

	p = STP(idp);
	if (p == NULL) cerror( "tyreduce" );
	curname = p->sname;

# ifndef BUG1
	if( ddebug ){
		printf( "defid( %s (%d), ", p->sname, idp );
		tprint( q->in.type );
		printf( ", %s, (%d,%d) ), level %d\n", scnames(class), q->fn.cdim, q->fn.csiz, blevel );
		}
# endif

	fixtype( q, class );

	type = q->in.type;
	class = fixclass( class, type );

	stp = p->stype;
	slev = p->slevel;

# ifndef BUG1
	if( ddebug ){
		printf( "\tmodified to " );
		tprint( type );
		printf( ", %s\n", scnames(class) );
		printf( "\tprevious def'n: " );
		tprint( stp );
		printf( ", %s, (%d,%d) ), level %d\n", scnames(p->sclass), p->dimoff, p->sizoff, slev );
		}
# endif

	if( stp == FTN && p->sclass == SNULL ) {
		/* name encountered as function, not yet defined */
		goto enter;
	}
	/*
	 * check for names declared as arguments without also being
	 * specified in the formal parameter list.  Note that such
	 * names may already have definitions, e.g.,
	 *	extern int y;
	 *	...
	 *	foo(x)
	 *	    int y;
	 *	{
	 *	    ...(* use of y produces strange results! *)
	 *	}
	 */
	if( blevel==1 && stp!=FARG ) {
		switch( class ){

		default:
			/* "declared argument %.8s is missing" */
			/* "declared argument %s is missing" */
			if(!(class&FIELD)) UERROR( MESSAGE( 28 ), p->sname );
		case MOS:
		case STNAME:
		case MOU:
		case UNAME:
		case MOE:
		case ENAME:
		case TYPEDEF:
			;
		}
	}

	if( stp == UNDEF|| stp == FARG ){
		goto enter;
	}

	if( type != stp ) goto mismatch;
	/* test (and possibly adjust) dimensions */
	dsym = p->dimoff;
	ddef = q->fn.cdim;
 	changed = 0;
	for( temp=type; temp&TMASK; temp = DECREF(temp) ){
		if( ISARY(temp) ){
			if (dimtab[dsym] == 0) {
				dimtab[dsym] = dimtab[ddef];
 				changed = 1;
			}
			else if( dimtab[ddef]!=0 && dimtab[dsym] != dimtab[ddef] ){
				goto mismatch;
			}
			++dsym;
			++ddef;
		}
	}
 
 	if (changed) {
 		FIXDEF(p);
	}
 

	/* check that redeclarations are to the same structure */
	if( (temp==STRTY||temp==UNIONTY||temp==ENUMTY) && p->sizoff != q->fn.csiz
		 && class!=STNAME && class!=UNAME && class!=ENAME ){
		goto mismatch;
	}

	scl = ( p->sclass );

# ifndef BUG1
	if( ddebug ){
		printf( "\tprevious class: %s\n", scnames(scl) );
		}
# endif

	if( class&FIELD ){
		/* redefinition */
		if( !falloc( p, class&FLDSIZ, 1, NIL ) ) {
			/* successful allocation */
			psave( idp );
			return;
			}
		/* blew it: resume at end of switch... */
		}

	else switch( class ){

	case EXTERN:
		switch( scl ){
		case STATIC:
		case USTATIC:
			if ( slev == 0 ) {
				redecl_err( p, scl, class );
				return;
				}
			break;
		case EXTDEF:
		case EXTERN:
		case FORTRAN:
		case UFORTRAN:
			return;
			}
		break;

	case STATIC:
		if( scl==USTATIC || (scl==EXTERN && blevel==0) ){
			if (scl == EXTERN) {
				redecl_err( p, scl, class );
				}
			p->sclass = STATIC;
			if( ISFTN(type) ) curftn = idp;
			return;
			}
		break;

	case USTATIC:
		if( scl==STATIC || scl==USTATIC ) return;

		/* allow redeclaration of an extern
		   to be redeclared as static. i.e.:
			int foo();
			static int foo();
		*/       
		if (scl == EXTERN && slev==0) {
			p->sclass = USTATIC;
			if( ISFTN(type) )  {
				curftn = idp;
			}
			return;
		}

		break;

	case LABEL:
		if( scl == ULABEL ){
			p->sclass = LABEL;
			deflab( p->offset );
			return;
			}
		break;

	case TYPEDEF:
		if( scl == class ) return;
		break;

	case UFORTRAN:
		if( scl == UFORTRAN || scl == FORTRAN ) return;
		break;

	case FORTRAN:
		if( scl == UFORTRAN ){
			p->sclass = FORTRAN;
			if( ISFTN(type) ) curftn = idp;
			return;
			}
		break;

	case MOU:
	case MOS:
		if( scl == class ) {
			if( oalloc( p, &strucoff ) ) break;
			if( class == MOU ) strucoff = 0;
			psave( idp );
			return;
			}
		break;

	case MOE:
		if( scl == class && p->offset == strucoff ){
			strucoff++;
			psave( idp );
			return;
			}
		break;

	case EXTDEF:
		if( scl == EXTERN ) {
			p->sclass = EXTDEF;
			if( ISFTN(type) ) curftn = idp;
			return;
			}
		break;

	case STNAME:
	case UNAME:
	case ENAME:
		if( scl != class ) break;
		if( dimtab[p->sizoff] == 0 ) return;  /* previous entry just a mention */
		break;

	case ULABEL:
		if( scl == LABEL || scl == ULABEL ) return;
	case PARAM:
	case AUTO:
	case REGISTER:
		;  /* mismatch.. */

		}

	mismatch:
	/* allow nonunique structure/union member names */

	if( class==MOU || class==MOS || class & FIELD ){/* make a new entry */
		int * memp;
		register struct symtab *sp;
		p->sflags |= SNONUNIQ;  /* old entry is nonunique */
		/* determine if name has occurred in this structure/union */
		memp = &paramstk[paramno-1];
		sp = STP(*memp);
		while( sp != NULL && sp->sclass != STNAME
		    && sp->sclass != UNAME ) {
			if( sp->sflags & SNONUNIQ ){
				if (p->sname == sp->sname) {
					/* "redeclaration of %s" */
					UERROR(MESSAGE( 96 ),p->sname);
					break;
					}
				}
			memp--;
			sp = STP(*memp);
			}
		p = mknonuniq( (struct symtab *)idp ); /* update p and idp to new entry */
                idp = (int) p;
		goto enter;
		}
	if( blevel > slev && class != EXTERN && class != FORTRAN &&
		class != UFORTRAN && !( class == LABEL && slev >= 2 ) ){
		q->tn.rval = idp = hide( p );
		p = STP(idp);
		if (p == NULL) return;
		goto enter;
		}
	/* "redeclaration of %s" */
	UERROR( MESSAGE( 96 ), p->sname );
	if( class==EXTDEF && ISFTN(type) ) curftn = idp;
	return;

	enter:  /* make a new entry */

# ifndef BUG1
	if( ddebug ) printf( "\tnew entry made\n" );
# endif
	if( type == TVOID && class != TYPEDEF)
		/* "void type for %s" */
		UERROR(MESSAGE( 117 ),p->sname);
	p->stype = type;
	p->sclass = class;
	p->slevel = blevel;
	p->offset = NOOFFSET;
	p->suse = ((p->suse < 0) && (p->sclass == STATIC)) ? -lineno : lineno;
	p->sfile = ftitle;
	if( class == STNAME || class == UNAME || class == ENAME ) {
		p->sizoff = curdim;
		dstash( 0 );  /* size */
		dstash( -1 ); /* index to members of str or union */
		dstash( ALSTRUCT );  /* alignment */
		dstash( idp );
		}
	else {
		switch( BTYPE(type) ){
		case STRTY:
		case UNIONTY:
		case ENUMTY:
			p->sizoff = q->fn.csiz;
			break;
		default:
			p->sizoff = BTYPE(type);
			}
		}

	/* copy dimensions */

	p->dimoff = q->fn.cdim;

	/* allocate offsets */
	if( class&FIELD ){
		falloc( p, class&FLDSIZ, 0, NIL );  /* new entry */
		psave( idp );
		}
	else switch( class ){

	case AUTO:
		if ( tsize( p->stype, p->dimoff, p->sizoff ) == 0 ) {
			uerror("auto variable %s has size 0", p->sname);
			}
		oalloc( p, &autooff );
		break;
	case STATIC:
	case EXTDEF:
		p->offset = getlab();
		if( ISFTN(type) ) curftn = idp;
		break;
	case ULABEL:
	case LABEL:
		p->offset = getlab();
		p->slevel = 2;
		if( class == LABEL ){
			locctr( PROG );
			deflab( p->offset );
			}
		break;

	case EXTERN:
	case UFORTRAN:
	case FORTRAN:
		p->offset = getlab();
		p->slevel = 0;
		break;
	case MOU:
	case MOS:
		oalloc( p, &strucoff );
		if( class == MOU ) strucoff = 0;
		psave( idp );
		break;

	case MOE:
		p->offset = strucoff++;
		psave( idp );
		break;
	case REGISTER:
		p->offset = regvar_alloc(type);
		if( blevel == 1 ) p->sflags |= SSET;
		break;
		}

	/* user-supplied routine to fix up new definitions */

	FIXDEF(p);

# ifndef BUG1
	if( ddebug ) printf( "\tdimoff, sizoff, offset: %d, %d, %d\n", p->dimoff, p->sizoff, p->offset );
# endif

	}

/*
 * handle a redeclaration involving a change of scope
 * (static vs. extern conflict).  For variables, issue
 * a hard error; you might as well hear about it now,
 * because otherwise you'll get it in the assembler later.
 * For functions, we remain silent; since we don't distinguish
 * implicit declarations from explicit ones, warnings would
 * be mostly artifacts of one-pass semantic analysis.
 */
static void
redecl_err(p, oldclass, newclass)
    struct symtab *p;
    int oldclass;
    int newclass;
{
    if (oldclass == STATIC && newclass == EXTDEF
      || oldclass == EXTDEF && newclass == STATIC) {
	/* "redeclaration of %s" */
	UERROR(MESSAGE( 96 ),p->sname);
    }
}

psave( i )
{
    if( (paramno % PARAMSZ) == 0 ){
	if(paramstk == NULL) {
	    paramstk = (int*)malloc(PARAMSZ*sizeof(int));
	} else {
	    paramstk = (int*)realloc((char *)paramstk,
		(paramno+PARAMSZ)*sizeof(int));
	}
	if (paramstk == NULL)
		fatal( "not enough memory for parameter stack" );
    }
    paramstk[ paramno++ ] = i;
}

ftnend(){ /* end of function */
	if( retlab != NOLAB ){ /* inside a real function */
		efcode();
		}
	checkst(0);
	retstat = 0;
	tcheck();
	curclass = SNULL;
	brklab = contlab = retlab = NOLAB;
	flostat = 0;
	if( nerrors == 0 ){
		if( psavbc != & asavbc[0] ) cerror("bcsave error");
		if( paramno != 0 ) cerror("parameter reset error");
		if( swx != 0 ) cerror( "switch error");
		}
	psavbc = &asavbc[0];
	paramno = 0;
	autooff = AUTOINIT;
	regvar_init();
	reached = 1;
	swx = 0;
	reset_swtab();
	if (paramstk != NULL) free((char *)paramstk);
	paramstk = NULL;
	paramno = 0;
# if TARGET == I386
	if (!twopass)
# endif
		locctr(DATA);
	}

noargs(){
	register int i;
	register struct symtab *p;
	for (i=0; i<paramno; ++i){
		p = STP(paramstk[i]);
		if ( p == NULL) continue;
		/* "%s declared as parameter to non-function" */
		UERROR(MESSAGE( 138 ), p->sname);
		p->slevel = 1;
		p->suse   = -lineno; /* shut lint up */
		}
	if (paramno){
		paramno  = 0;
		clearst( blevel );
		checkst( blevel );
	    }
	}

dclargs(){
	register i;
	register struct symtab *p;
	register NODE *q;
	argoff = ARGINIT;
# ifndef BUG1
	if( ddebug > 2) printf("dclargs()\n");
# endif
	for( i=0; i<paramno; ++i ){
		p = STP(paramstk[i]);
		if (p == NULL) continue;
# ifndef BUG1
		if( ddebug > 2 ){
			printf("\t%s (0x%x) ", p->sname, p);
			tprint(p->stype);
			printf("\n");
			}
# endif
		if( p->stype == FARG ) {
			q = block(FREE,NIL,NIL,INT,0,INT);
			q->tn.rval = (int)p;
			defid( q, PARAM );
			}
		FIXARG(p); /* local arg hook, eg. for sym. debugger */
		oalloc( p, &argoff );  /* always set aside space, even for register arguments */
		}
	cendarg();
	bfcode( paramstk, paramno );
	paramno = 0;
	}

NODE *
rstruct( idn, soru ){ /* reference to a structure or union, with no definition */
	register struct symtab *p;
	register NODE *q;
	p = STP(idn);
	if (p == NULL) {
		fatal("null symbol in rstruct()");
		/* NOTREACHED */
	}
	switch( p->stype ){

	case UNDEF:
	def:
		q = block( FREE, NIL, NIL, 0, 0, 0 );
		q->tn.rval = idn;
		q->in.type = (soru&INSTRUCT) ? STRTY : ( (soru&INUNION) ? UNIONTY : ENUMTY );
		defid( q, (soru&INSTRUCT) ? STNAME : ( (soru&INUNION) ? UNAME : ENAME ) );
		break;

	case STRTY:
		if( soru & INSTRUCT ) break;
		goto def;

	case UNIONTY:
		if( soru & INUNION ) break;
		goto def;

	case ENUMTY:
		if( !(soru&(INUNION|INSTRUCT)) ) break;
		goto def;

		}
	stwart = instruct;
	return( mkty( p->stype, 0, p->sizoff ) );
	}

moedef( idn ){
	register NODE *q;

	q = block( FREE, NIL, NIL, MOETY, 0, 0 );
	q->tn.rval = idn;
	if( idn>=0 ) defid( q, MOE );
	}

bstruct( idn, soru ){ /* begining of structure or union declaration */
	register NODE *q;
	register struct symtab *p;

	psave( instruct );
	psave( curclass );
	psave( strucoff );
	strucoff = 0;
	instruct = soru;
	q = block( FREE, NIL, NIL, 0, 0, 0 );
	q->tn.rval = idn;
	if( instruct==INSTRUCT ){
		curclass = MOS;
		q->in.type = STRTY;
		if( idn >= 0 ) defid( q, STNAME );
		}
	else if( instruct == INUNION ) {
		curclass = MOU;
		q->in.type = UNIONTY;
		if( idn >= 0 ) defid( q, UNAME );
		}
	else { /* enum */
		curclass = MOE;
		q->in.type = ENUMTY;
		if( idn >= 0 ) defid( q, ENAME );
		}
	psave( idn = q->tn.rval );
	/* the "real" definition is where the members are seen */
	p = STP(idn);
	if (p != NULL) p->suse = lineno;
	return( paramno-4 );
	}

NODE *
dclstruct( oparam ){
	register struct symtab *p;
	register i, al, sa, sz, szindex;
	register TWORD temp;
	register high, low;

	/* paramstack contains:
		paramstack[ oparam ] = previous instruct
		paramstack[ oparam+1 ] = previous class
		paramstk[ oparam+2 ] = previous strucoff
		paramstk[ oparam+3 ] = structure name

		paramstk[ oparam+4, ... ]  = member stab indices

		*/


	p = STP(paramstk[oparam+3]);
	if (p == NULL) {
		szindex = curdim;
		dstash( 0 );  /* size */
		dstash( -1 );  /* index to member names */
		dstash( ALSTRUCT );  /* alignment */
		dstash( -lineno );	/* name of structure */
		}
	else {
		szindex = p->sizoff;
		}

# ifndef BUG1
	if( ddebug ){
		printf( "dclstruct( %s ), szindex = %d\n",
			p == NULL ? "??" : p->sname, szindex );
		}
# endif
	temp = (instruct&INSTRUCT)?STRTY:((instruct&INUNION)?UNIONTY:ENUMTY);
	stwart = instruct = paramstk[ oparam ];
	curclass = paramstk[ oparam+1 ];
	dimtab[ szindex+1 ] = curdim;
	al = ALSTRUCT;

	high = low = 0;

	for( i = oparam+4;  i< paramno; ++i ){
		p = STP(paramstk[i]);
		dstash( (int)p );
		if (p == NULL) continue;
		if( temp == ENUMTY ){
			if( p->offset < low ) low = p->offset;
			if( p->offset > high ) high = p->offset;
			p->sizoff = szindex;
			continue;
			}
		sa = talign( p->stype, p->sizoff );
		if( p->sclass & FIELD ){
			sz = p->sclass&FLDSIZ;
			}
		else {
			sz = tsize( p->stype, p->dimoff, p->sizoff );
			}
		if( sz == 0 ){
			/* "illegal zero sized structure member: %s" */
			WERROR( MESSAGE( 73 ), p->sname );
			}
		if( sz > strucoff ) strucoff = sz;  /* for use with unions */
		SETOFF( al, sa );
		/* set al, the alignment, to the lcm of the alignments of the members */
		}
	dstash( -1 );  /* endmarker */
	SETOFF( strucoff, al );

	if( temp == ENUMTY ){
		register TWORD ty;

# ifdef ENUMSIZE
		ty = ENUMSIZE(high,low);
# else
		if( (char)high == high && (char)low == low ) ty = ctype( CHAR );
		else if( (short)high == high && (short)low == low ) ty = ctype( SHORT );
		else ty = ctype(INT);
#endif
		strucoff = tsize( ty, 0, (int)ty );
		dimtab[ szindex+2 ] = al = talign( ty, (int)ty );
		}

	/* "zero sized structure" */
	if( strucoff == 0 ) UERROR( MESSAGE( 121 ) );
	dimtab[ szindex ] = strucoff;
	dimtab[ szindex+2 ] = al;
	dimtab[ szindex+3 ] = paramstk[ oparam+3 ];  /* name index */

	FIXSTRUCT( szindex, oparam ); /* local hook, eg. for sym debugger */
# ifndef BUG1
	if( ddebug>1 ){
		printf( "\tdimtab[%d,%d,%d] = %d,%d,%d\n", szindex,szindex+1,szindex+2,
				dimtab[szindex],dimtab[szindex+1],dimtab[szindex+2] );
		for( i = dimtab[szindex+1]; dimtab[i] > 0; ++i ){

			p = STP(dimtab[i]);
			printf( "\tmember %s(%d)\n",
				p == NULL ? "??" : p->sname, dimtab[i] );
			}
		}
# endif

	strucoff = paramstk[ oparam+2 ];
	paramno = oparam;

	return( mkty( temp, 0, szindex ) );
	}

yyaccpt(){
	ftnend();
	}

ftnarg( idn ) {
	register struct symtab *p;
	p = STP(idn);
	if (p == NULL) return;
	switch( p->stype ){

	case UNDEF:
		/* this parameter, entered at scan */
		break;
	case FARG:
		/* "redeclaration of formal parameter, %s" */
		UERROR(MESSAGE( 97 ), p->sname);
		/* fall thru */
	case FTN:
		/* the name of this function matches parm */
		/* fall thru */
	default:
		idn = hide(p);
		p = STP(idn);
		break;
	case TNULL:
		/* unused entry, fill it */
		;
		}
	p->stype = FARG;
	p->sclass = PARAM;
	psave( p );
	}

talign( ty, s) register unsigned ty; register s; {
	/* compute the alignment of an object with type ty, sizeoff index s */

	register i;
	if( s<0 && ty!=INT && ty!=CHAR && ty!=SHORT && ty!=UNSIGNED && ty!=UCHAR && ty!=USHORT 
#ifdef LONGFIELDS
		&& ty!=LONG && ty!=ULONG
#endif
					){
		return( fldal( ty ) );
		}

	for( i=0; i<=(SZINT-BTSHIFT-1); i+=TSHIFT ){
		switch( (ty>>i)&TMASK ){

		case FTN:
			cerror( "compiler takes alignment of function");
		case PTR:
			return( ALPOINT );
		case ARY:
			continue;
		case 0:
			break;
			}
		}

	switch( BTYPE(ty) ){

	case UNIONTY:
	case ENUMTY:
	case STRTY:
		return( (unsigned int) dimtab[ s+2 ] );
	case CHAR:
	case UCHAR:
		return( ALCHAR );
	case FLOAT:
		return( ALFLOAT );
	case DOUBLE:
		return( ALDOUBLE );
	case LONG:
	case ULONG:
		return( ALLONG );
	case SHORT:
	case USHORT:
		return( ALSHORT );
	default:
		return( ALINT );
		}
	}

OFFSZ
tsize( ty, d, s )  TWORD ty; {
	/* compute the size associated with type ty,
	    dimoff d, and sizoff s */
	/* BETTER NOT BE CALLED WHEN t, d, and s REFER TO A BIT FIELD... */

	int i;
	OFFSZ mult;

	mult = 1;

	for( i=0; i<=(SZINT-BTSHIFT-1); i+=TSHIFT ){
		switch( (ty>>i)&TMASK ){

		case FTN:
			/* "cannot take size of a function" */
			UERROR(MESSAGE( 134 ));
		case PTR:
			return( SZPOINT * mult );
		case ARY:
			mult *= (unsigned int) dimtab[ d++ ];
			continue;
		case 0:
			break;

			}
		}

	if( dimtab[s]==0 ) {
		if( ty == STRTY || ty == UNIONTY )
			/* "undefined structure or union" */
			UERROR( MESSAGE( 112 ));
		else if( ty == ENUMTY )
			/* "undefined enum" */
			UERROR( MESSAGE( 131 ));
		else
			/* "unknown size" */
			UERROR( MESSAGE( 114 ));
		return( SZINT );
		}
	return( (unsigned int) dimtab[ s ] * mult );
	}

inforce( n ) OFFSZ n; {  /* force inoff to have the value n */
	/* inoff is updated to have the value n */
	OFFSZ wb;
	register rest;
	/* rest is used to do a lot of conversion to ints... */

	if( inoff == n ) return;
	if( inoff > n ) {
		fatal( "initialization alignment error");
		}

	wb = inoff;
	SETOFF( wb, SZINT );

	/* wb now has the next higher word boundary */

	if( wb >= n ){ /* in the same word */
		rest = n - inoff;
		vfdzero( rest );
		return;
		}

	/* otherwise, extend inoff to be word aligned */

	rest = wb - inoff;
	vfdzero( rest );

	/* now, skip full words until near to n */

	rest = (n-inoff)/SZINT;
	zecode( rest );

	/* now, the remainder of the last word */

	rest = n-inoff;
	vfdzero( rest );
	if( inoff != n ) cerror( "inoff error");

	}

vfdalign( n ){ /* make inoff have the offset the next alignment of n */
	OFFSZ m;

	m = inoff;
	SETOFF( m, n );
	inforce( m );
	}


int idebug = 0;

int ibseen = 0;  /* the number of } constructions which have been filled */

int iclass;  /* storage class of thing being initialized */

int ilocctr = 0;  /* location counter for current initialization */

beginit(curid){
	/* beginning of initilization; set location ctr and set type */
	register struct symtab *p;

# ifndef BUG1
	if( idebug >= 3 ) printf( "beginit(), curid = %d\n", curid );
# endif

	p = STP(curid);
	if (p == NULL) return;

	iclass = p->sclass;
	if( curclass == EXTERN || curclass == FORTRAN ) iclass = EXTERN;
	switch( iclass ){

	case UNAME:
	case EXTERN:
		return;
	case AUTO:
	case REGISTER:
		break;
	case EXTDEF:
	case STATIC:
		ilocctr = ISARY(p->stype)?ADATA:DATA;
		locctr( ilocctr );
		defalign( talign( p->stype, p->sizoff ) );
		defnam( p );

		}

	inoff = 0;
	ibseen = 0;

	pstk = 0;

	instk( curid, p->stype, p->dimoff, p->sizoff, inoff );

	}

instk( id, t, d, s, off ) OFFSZ off; TWORD t; {
	/* make a new entry on the parameter stack to initialize id */

	register struct symtab *p;

	for(;;){
# ifndef BUG1
		if( idebug ) printf( "instk((%d, %o,%d,%d, %d)\n", id, t, d, s, off );
# endif

		/* save information on the stack */

		if( !pstk ) pstk = instack;
		else ++pstk;

		pstk->in_fl = 0;	/* { flag */
		pstk->in_id =  id ;
		pstk->in_t =  t ;
		pstk->in_d =  d ;
		pstk->in_s =  s ;
		pstk->in_n = 0;  /* number seen */
		pstk->in_x =  t==STRTY ?dimtab[s+1] : 0 ;
		pstk->in_off =  off;   /* offset at the beginning of this element */
		/* if t is an array, DECREF(t) can't be a field */
		/* INS_sz has size of array elements, and -size for fields */
		if( ISARY(t) ){
			pstk->in_sz = tsize( DECREF(t), d+1, s );
			}
		else if( (p=STP(id)) != NULL && p->sclass & FIELD ){
			pstk->in_sz = - ( p->sclass & FLDSIZ );
			}
		else {
			pstk->in_sz = 0;
			}

		if( (iclass==AUTO || iclass == REGISTER ) &&
			/* "no automatic aggregate initialization" */
			(ISARY(t) || t==STRTY) ) UERROR( MESSAGE( 79 ) );

		/* now, if this is not a scalar, put on another element */

		if( ISARY(t) ){
			t = DECREF(t);
			++d;
			continue;
			}
		else if( t == STRTY ){
			id = dimtab[pstk->in_x];
			p = STP(id);
			if( pstk->in_x == -1 || p == NULL
			    || p->sclass != MOS && !(p->sclass&FIELD) ) {
				uerror("initialization with bogus structure");
				return;
				}
			t = p->stype;
			d = p->dimoff;
			s = p->sizoff;
			off += p->offset;
			continue;
			}
		else return;
		}
	}

NODE *
getstr(){ /* decide if the string is external or an initializer, and get the contents accordingly */

	register l, temp;
	register NODE *p;

	if( (iclass==EXTDEF||iclass==STATIC) && (pstk->in_t == CHAR || pstk->in_t == UCHAR) &&
			pstk!=instack && ISARY( pstk[-1].in_t ) ){
		/* treat "abc" as { 'a', 'b', 'c', 0 } */

		strflg = 1;
		ilbrace();  /* simulate { */
		inforce( pstk->in_off );
		/* if the array is inflexible (not top level), pass in the size and
			be prepared to throw away unwanted initializers */
		lxgetstr((pstk-1)!=instack?dimtab[(pstk-1)->in_d]:0);  /* get the contents */
		irbrace();  /* simulate } */
		return( NIL );
		}
	else if ( lxsizeof ) {
		/* we are processing a sizeof expression, so don't
		   generate any string characters or fill bytes */
		dimtab[curdim] = lxstrlen()+1;
		p = buildtree( STRING, NIL, NIL );
		return(p);
		}
	else { /* make a label, and get the contents and stash them away */
		if( iclass != SNULL ){ /* initializing */
			/* fill out previous word, to permit pointer */
			vfdalign( ALPOINT );
			}
		temp = locctr( blevel==0?ISTRNG:STRNG ); /* set up location counter */
		deflab( l = getlab() );
		strflg = 0;
		lxgetstr(0); /* get the contents */
		locctr( blevel==0?ilocctr:temp );
		p = buildtree( STRING, NIL, NIL );
		p->tn.rval = -l;
		return(p);
		}
	}

putbyte( v ){ /* simulate byte v appearing in a list of integer values */
	register NODE *p;
	p = bcon(v);
	incode( p, SZCHAR );
	tfree( p );
	gotscal();
	}

endinit(){
	register TWORD t;
	register d, s, n, d1;
	struct symtab *p;

# ifndef BUG1
	if( idebug ) printf( "endinit(), inoff = %d\n", inoff );
# endif

	switch( iclass ){

	case EXTERN:
	case AUTO:
	case REGISTER:
		return;
		}

	pstk = instack;

	t = pstk->in_t;
	d = pstk->in_d;
	s = pstk->in_s;
	n = pstk->in_n;

	if( ISARY(t) ){
		d1 = dimtab[d];

		if ( pstk->in_sz == 0 ) {
			return;
			}
		vfdalign( pstk->in_sz );  /* fill out part of the last element, if needed */
		n = inoff/pstk->in_sz;  /* real number of initializers */
		if( d1 >= n ){
			/* once again, t is an array, so no fields */
			inforce( tsize( t, d, s ) );
			n = d1;
			}
		/* "too many initializers" */
		if( d1!=0 && d1!=n ) UERROR( MESSAGE( 108 ));
		/* "empty array declaration" */
		if( n==0 ) WERROR( MESSAGE( 35 ));
		dimtab[d] = n;
		if (d1==0) {
			p = STP(pstk->in_id);
			if (p != NULL) FIXDEF(p);
			}
		}

	else if( t == STRTY || t == UNIONTY ){
		/* clearly not fields either */
		inforce( tsize( t, d, s ) );
		}
	/* "bad scalar initialization" */
	else if( n > 1 ) UERROR( MESSAGE( 17 ));
	/* this will never be called with a field element... */
	else inforce( tsize(t,d,s) );

	paramno = 0;
	vfdalign( AL_INIT );
	inoff = 0;
	iclass = SNULL;

	}

doinit( p ) register NODE *p; {

	/* take care of generating a value for the initializer p */
	/* inoff has the current offset (last bit written)
		in the current word being generated */

	register sz, d, s;
	register TWORD t;

	/* note: size of an individual initializer is assumed to fit into an int */

	if( iclass <= SNULL ) goto leave;
	if( iclass == EXTERN || iclass == UNAME ){
		/* "cannot initialize extern or union" */
		UERROR( MESSAGE( 19 ) );
		iclass = -1;
		goto leave;
		}

	if( iclass == AUTO || iclass == REGISTER ){
		/* do the initialization and get out, without regard 
		    for filing out the variable with zeros, etc. */
		bccode();
		idname = pstk->in_id;
		p = buildtree( ASSIGN, buildtree( NAME, NIL, NIL ), p );
		ecomp(p);
		return;
		}

	if( p == NIL ) return;  /* for throwing away strings that have been turned into lists */

	if( ibseen ){
		/* "} expected" */
		UERROR( MESSAGE( 122 ));
		goto leave;
		}
	if( pstk == (struct instk *)0)
	    goto leave; /* we are having a syntax error -- pretend not to see */

# ifndef BUG1
	if( idebug > 1 ) printf( "doinit(%o)\n", p );
# endif

	t = pstk->in_t;  /* type required */
	d = pstk->in_d;
	s = pstk->in_s;
	if( pstk->in_sz < 0 ){  /* bit field */
		sz = -pstk->in_sz;
		}
	else {
		sz = tsize( t, d, s );
		}

	if (inoff > pstk->in_off) {
		/* "too many initializers" */
		UERROR( MESSAGE( 108 ) );
		goto leave;
		}

	inforce( pstk->in_off );

	p = buildtree( ASSIGN, clocal( block( NAME, NIL,NIL, t, d, s ) ), p );
	p->in.left->in.op = FREE;
	p->in.left = p->in.right;
	p->in.right = NIL;
	p->in.left = optim( p->in.left );
	if( p->in.left->in.op == UNARY AND ){
		p->in.left->in.op = FREE;
		p->in.left = p->in.left->in.left;
		}
	p->in.op = INIT;

	if( sz < SZINT ){ /* special case: bit fields, etc. */
		NODE *l;
		if( (l = p->in.left)->in.op != ICON )
			if( l->in.op == SCONV && l->in.left->tn.op == FCON){
				/*special case for floating init of fixed number*/
				/* do coersion here, as a favor */
				l = l->in.left;
				l->tn.lval = (long)(l->fpn.dval);
				l->tn.rval = 0;
				l->tn.type = INT;
				l->tn.op = ICON;
				incode( l, sz );
			} else {
				if (lineno!=errline)
					/* "illegal initialization" */
					UERROR( MESSAGE( 61 ) );
			}
		else incode( l, sz );
		}
	else {
		cinit( optim(p), sz );
		}

	gotscal();

	leave:
	tfree(p);
	}

gotscal(){
	register t, ix;
	register n, id;
	struct symtab *p;
	OFFSZ temp;

	for( ; pstk > instack; ) {

		if( pstk->in_fl ) ++ibseen;

		--pstk;
		
		t = pstk->in_t;

		if( t == STRTY ){
			ix = ++pstk->in_x;
			if( (id=dimtab[ix]) <= 0 ) continue;

			/* otherwise, put next element on the stack */

			p = STP(id);
			if (p != NULL)
				instk( id, p->stype, p->dimoff, p->sizoff,
					p->offset+pstk->in_off );
			return;
			}
		else if( ISARY(t) ){
			n = ++pstk->in_n;
			if( n >= dimtab[pstk->in_d] && pstk > instack ) continue;

			/* put the new element onto the stack */

			temp = pstk->in_sz;
			instk( pstk->in_id, (TWORD)DECREF(pstk->in_t), pstk->in_d+1, pstk->in_s,
				pstk->in_off+n*temp );
			return;
			}

		}
	}

ilbrace(){ /* process an initializer's left brace */
	register t;
	struct instk *temp;

	temp = pstk;

	for( ; pstk > instack; --pstk ){

		t = pstk->in_t;
		if( t != STRTY && !ISARY(t) ) continue; /* not an aggregate */
		if( pstk->in_fl ){ /* already associated with a { */
			/* "illegal {" */
			if( pstk->in_n ) UERROR( MESSAGE( 74 ));
			continue;
			}

		/* we have one ... */
		pstk->in_fl = 1;
		break;
		}

	/* cannot find one */
	/* ignore such right braces */

	pstk = temp;
	}

irbrace(){
	/* called when a '}' is seen */

# ifndef BUG1
	if( idebug ) printf( "irbrace(): paramno = %d on entry\n", paramno );
# endif

	if( ibseen ) {
		--ibseen;
		return;
		}

	for( ; pstk > instack; --pstk ){
		if( !pstk->in_fl ) continue;

		/* we have one now */

		pstk->in_fl = 0;  /* cancel { */
		gotscal();  /* take it away... */
		return;
		}

	/* these right braces match ignored left braces: throw out */

	}

TWORD
types( t1, t2, t3 ) TWORD t1, t2, t3; {
	/* return a basic type from basic types t1, t2, and t3 */

	TWORD t[3], noun, adj, unsg;
	register i;

	t[0] = t1;
	t[1] = t2;
	t[2] = t3;

	unsg = INT;  /* INT or UNSIGNED */
	noun = UNDEF;  /* INT, CHAR, FLOAT, or VOID */
	adj = INT;  /* INT, LONG, or SHORT */

	for( i=0; i<3; ++i ){
		switch( t[i] ){

		default:
		bad:
			/* "illegal type combination" */
			UERROR( MESSAGE( 70 ) );
			return( INT );

		case UNDEF:
			continue;

		case UNSIGNED:
			if( unsg != INT ) goto bad;
			unsg = UNSIGNED;
			continue;

		case LONG:
		case SHORT:
			if( adj != INT ) goto bad;
			adj = t[i];
			continue;

		case INT:
		case CHAR:
		case FLOAT:
			if( noun != UNDEF ) goto bad;
			noun = t[i];
			continue;
			}
		}

	/* now, construct final type */
	if( noun == UNDEF ) noun = INT;
	else if( noun == FLOAT ){
		if( unsg != INT || adj == SHORT ) goto bad;
		return( adj==LONG ? DOUBLE : FLOAT );
		}
	else if( noun == CHAR && adj != INT ) goto bad;

	/* now, noun is INT or CHAR */
	if( adj != INT ) noun = adj;
	if( unsg == UNSIGNED ) return( noun + (UNSIGNED-INT) );
	else return( noun );
	}

NODE *
tymerge( typ, idp ) NODE *typ, *idp; {
	/* merge type typ with identifier idp  */

	register unsigned t;
	register i;
	extern int eprint();

	if( typ->in.op != TYPE ) cerror( "tymerge: arg 1" );
	if(idp == NIL ) return( NIL );

# ifndef BUG1
	if( ddebug > 2 ) fwalk( idp, eprint, 0 );
# endif

	idp->in.type = typ->in.type;
	idp->fn.cdim = curdim;
	tyreduce( idp );
	idp->fn.csiz = typ->fn.csiz;

	for( t=typ->in.type, i=typ->fn.cdim; t&TMASK; t = DECREF(t) ){
		if( ISARY(t) ) dstash( dimtab[i++] );
		}

	/* now idp is a single node: fix up type */

	idp->in.type = ctype( idp->in.type );

	if( (t = BTYPE(idp->in.type)) != STRTY && t != UNIONTY && t != ENUMTY ){
		idp->fn.csiz = t;  /* in case ctype has rewritten things */
		}

	return( idp );
	}

tyreduce( p ) register NODE *p; {

	/* build a type, and stash away dimensions, from a parse tree of the declaration */
	/* the type is build top down, the dimensions bottom up */
	register o, temp;
	register unsigned t;

	o = p->in.op;
	p->in.op = FREE;

	if( o == NAME ) return;

	t = INCREF( p->in.type );
	if( o == UNARY CALL ) t += (FTN-PTR);
	else if( o == LB ){
		t += (ARY-PTR);
		temp = p->in.right->tn.lval;
		p->in.right->in.op = FREE;
		if( ( temp == 0 ) & ( p->in.left->tn.op == LB ) )
			/* "null dimension" */
			UERROR( MESSAGE( 85 ) );
		}

	p->in.left->in.type = t;
	tyreduce( p->in.left );

	if( o == LB ) dstash( temp );

	p->tn.rval = p->in.left->tn.rval;
	p->in.type = p->in.left->in.type;

	}

fixtype( p, class ) register NODE *p; {
	register unsigned t, type;
	register mod1, mod2;
	/* fix up the types, and check for legality */

	if( (type = p->in.type) == UNDEF || type == TERROR ) return;
	if( mod2 = (type&TMASK) ){
		t = DECREF(type);
		while( mod1=mod2, mod2 = (t&TMASK) ){
			if( mod1 == ARY && mod2 == FTN ){
				/* "array of functions is illegal" */
				UERROR( MESSAGE( 14 ) );
				type = 0;
				}
			else if( mod1 == FTN && ( mod2 == ARY || mod2 == FTN ) ){
				/* "function returns illegal type" */
				UERROR( MESSAGE( 47 ) );
				type = 0;
				}
			t = DECREF(t);
			}
#ifdef FLOATMATH
		if( (FLOATMATH<=1) && t == FLOAT && mod1 == FTN ) {
			/* there are no FLOAT functions */
			MODTYPE(type, DOUBLE);
			}
#endif
		}

	/* detect function arguments, watching out for structure declarations */
	/* for example, beware of f(x) struct { int a[10]; } *x; { ... } */
	/* the danger is that "a" will be converted to a pointer */

	if( class==SNULL && blevel==1 && !(instruct&(INSTRUCT|INUNION)) ) class = PARAM;
	if( class == PARAM || ( class==REGISTER && blevel==1 ) ){
		if( type == FLOAT ){
#ifdef FLOATMATH
		if (FLOATMATH<=1)
#endif
		    type = DOUBLE;
		}else if( ISARY(type) ){
			++p->fn.cdim;
			type += (PTR-ARY);
			}
		else if( ISFTN(type) ){
			/* "a function is declared as an argument" */
			WERROR( MESSAGE( 11 ) );
			type = INCREF(type);
			}

		}

	if( instruct && ISFTN(type) ){
		/* "function illegal in structure or union"  */
		UERROR( MESSAGE( 46 ) );
		type = INCREF(type);
		}
	p->in.type = type;
	}

uclass( class ) register class; {
	/* give undefined version of class */
	if( class == SNULL ) return( EXTERN );
	else if( class == STATIC ) return( USTATIC );
	else if( class == FORTRAN ) return( UFORTRAN );
	else return( class );
	}

static int
fixclass( class, type ) TWORD type; {

	/* first, fix null class */

	if( class == SNULL ){
		if( instruct&INSTRUCT ) class = MOS;
		else if( instruct&INUNION ) class = MOU;
		else if( blevel == 0 ) class = EXTDEF;
		else if( blevel == 1 ) class = PARAM;
		else class = AUTO;

		}

	/* now, do general checking */

	if( ISFTN( type ) ){
		switch( class ) {
		default:
			/* "function %s has illegal storage class" */
			UERROR( MESSAGE( 45 ), curname );
		case AUTO:
			class = EXTERN;
		case EXTERN:
		case EXTDEF:
		case FORTRAN:
		case TYPEDEF:
		case STATIC:
		case UFORTRAN:
		case USTATIC:
			;
			}
		}

	if( class&FIELD ){
		/* "illegal use of field" */
		if( !(instruct&INSTRUCT) ) UERROR( MESSAGE( 72 ) );
		return( class );
		}

	switch( class ){

	case MOU:
		/* "illegal class for %s" */
		if( !(instruct&INUNION) ) UERROR( MESSAGE( 52 ), curname );
		return( class );

	case MOS:
		/* "illegal class for %s" */
		if( !(instruct&INSTRUCT) ) UERROR( MESSAGE( 52 ), curname );
		return( class );

	case MOE:
		/* "illegal class for %s" */
		if( instruct & (INSTRUCT|INUNION) ) UERROR( MESSAGE( 52 ), curname );
		return( class );

	case REGISTER:
		if( blevel == 0 ) {
			/* "illegal register declaration %s" */
			UERROR( MESSAGE( 68 ), curname );
			return( AUTO );
		}
		if( regvar_avail( type ) )
			return( class );
		if( blevel == 1 ) return( PARAM );
		return( AUTO );

	case AUTO:
	case LABEL:
	case ULABEL:
		/* "illegal class for %s" */
		if( blevel < 2 )
			UERROR( MESSAGE( 52 ), curname );
		return( class );

	case PARAM:
		/* "illegal class for %s" */
		if( blevel != 1 )
			UERROR( MESSAGE( 52 ), curname );
		return( class );

	case UFORTRAN:
	case FORTRAN:
# ifdef NOFORTRAN
			NOFORTRAN;    /* a condition which can regulate the FORTRAN usage */
# endif
		/* "fortran declaration must apply to function" */
		if( !ISFTN(type) ) UERROR( MESSAGE( 40 ) );
		else {
			type = DECREF(type);
			if( ISFTN(type) || ISARY(type) || ISPTR(type) ) {
				/* "fortran function has wrong type" */
				UERROR( MESSAGE( 41 ) );
				}
			}
	case STNAME:
	case UNAME:
	case ENAME:
		return( class );
	case EXTERN:
	case STATIC:
	case EXTDEF:
	case TYPEDEF:
	case USTATIC:
		if( blevel == 1 ){
			/* "illegal class for %s" */
			UERROR( MESSAGE( 52 ), curname );
			return PARAM;
			}
		return( class );

	default:
		cerror( "illegal class: %d", class );
		/* NOTREACHED */

		}
	}

struct symtab *
mknonuniq(idp) register struct symtab *idp; {/* locate a symbol table entry for */
	/* an occurrence of a nonunique structure member name */
	/* or field */
	register struct symtab * sp;

        sp = (struct symtab *) malloc (sizeof ( struct symtab ));
        if (sp == NULL)
                fatal( "not enough memory for symbol table" );
        sp->hashVal = idp->hashVal;
        sp->stype = TNULL;
	sp->sflags = SNONUNIQ | SMOS;
        sp->next = stab[idp->hashVal];
	sp->symrefs = NULL;
	sp->svarno = 0;
        stab[idp->hashVal] = sp;
	sp->sname = idp->sname;
#ifndef BUG1
	if ( ddebug )
	{
		printf( "\tnonunique entry for %s from 0x%x to 0x%x\n",
			idp->sname, idp, sp );
	}
#endif
	return ( sp );
	}

lookup( name, s ) /* look up name: must agree with s w.r.t. STAG, SMOS and SHIDDEN */
register char *name;
{ 

	register int i;
	register struct symtab *sp;

	/* compute initial hash index */
# ifndef BUG1
	if( ddebug > 2 ){
		printf( "lookup( %s, %d ), stwart=%d, instruct=%d\n",
			name, s, stwart, instruct );
		}
# endif

	i = (int) name;
	i = i%SYMTSZ;
	sp = stab[i];

	if ( sp == NULL ) {
        	sp = (struct symtab *) malloc ( sizeof ( struct symtab ));
		if (sp == NULL)
			fatal( "not enough memory for symbol table" );
                stab[i] = sp;
		sp->sflags = s;  /* set STAG, SMOS if needed, turn off all others */
		sp->sname = name;
		sp->stype = UNDEF;
		sp->sclass = SNULL;
                sp->hashVal = i;
                sp->next = NULL;
		sp->symrefs = NULL;
		sp->svarno = 0;
		return( (int)sp );
		}
        for ( ; sp != NULL; sp = sp->next ) {
		if( (sp->sflags & (STAG|SMOS|SHIDDEN)) != s ) continue;
		if ( sp->sname == name )
			return ( (int)sp );
		}

	sp = (struct symtab *) malloc ( sizeof ( struct symtab ));
	if (sp == NULL)
		fatal( "not enough memory for symbol table" );
        sp->next = stab[i];
        stab[i] = sp;
	sp->sflags = s;  /* set STAG, SMOS if needed, turn off all others */
	sp->sname = name;
	sp->stype = UNDEF;
	sp->sclass = SNULL;
        sp->hashVal = i;
	sp->symrefs = NULL;
	sp->svarno = 0;
	return( (int)sp );
	}

#ifndef checkst
/* if not debugging, make checkst a macro */
checkst(lev){
	register int s, i, j;
	register struct symtab *p, *q;

	for( i=0; i<SYMTSZ; ++i ){
            for ( p=stab[i]; p != NULL; p = p->next ) {
		j = lookup( p->sname, p->sflags&(SMOS|STAG) );
		q = STP(j);
		if( q != p ){
			if( q->stype == UNDEF ||
			    q->slevel <= p->slevel ){
				cerror( "check error: %s", q->sname );
				}
			}
		else if( p->slevel > lev )
			cerror( "%s check at level %d", p->sname, lev );
		}
	    }
	}
#endif

clearst( lev )
{
    /* clear entries of internal scope  from the symbol table */
    register struct symtab *p,**q;
    register int temp, i;

    temp = lineno;
    aobeg();
    for(i=0; i<SYMTSZ; i++) {
	q = &stab[i];
	p = *q;
	while (p != NULL) {
	    lineno = p->suse;
	    if( lineno < 0 ) lineno = - lineno;
	    if( p->slevel>lev ){
		/* must clobber */
		if(p->stype == UNDEF || (p->sclass == ULABEL && lev<2)){
		    lineno = temp;
		    /* "%s undefined" */
		    UERROR( MESSAGE( 4 ), p->sname );
		} else {
		    aocode(p);
		}
# ifndef BUG1
		if (ddebug)
		    printf("removing %s from [ %d], flags %o level %d\n",
			p->sname,p,p->sflags,p->slevel);
# endif
		if( p->sflags & SHIDES )
		    unhide(p);
		/* delete from hash chain */
		p = p->next;
		(*q)->next = NULL;
		free((char *)*q);
		*q = p;
	    } else {
		/* advance to next element of chain */
		q = &p->next;
		p = *q;
	    }
	}
    }
    lineno = temp;
    aoend();
}

hide( p ) register struct symtab *p; {
	register struct symtab *q;
        q = (struct symtab*)malloc(sizeof(struct symtab));
	if (q == NULL)
		fatal( "not enough memory for symbol table" );
	*q = *p;
	p->sflags |= SHIDDEN;
	q->symrefs = NULL;
	q->sflags = (p->sflags&(SMOS|STAG)) | SHIDES;
        q->next = stab[p->hashVal];
        stab[p->hashVal] = q;
	/* "%.8s redefinition hides earlier one" */
	/* "%s redefinition hides earlier one" */
	if( hflag ) WERROR( MESSAGE( 2 ), p->sname );
	/* "declaration of %.8s hides parameter" */
	/* "declaration of %s hides parameter" */
	if (blevel==2 && p->slevel==1) WERROR( MESSAGE( 139 ), p->sname );
# ifndef BUG1
	if( ddebug ) printf( "\t%d hidden in %d\n", p, q );
# endif
	return( idname = (int)q );
	}

unhide( p ) register struct symtab *p; {
	register struct symtab *q;
	register s;

	s = p->sflags & (SMOS|STAG);

	for ( q=p->next; q!= NULL; q = q->next ) {

		if( (q->sflags&(SMOS|STAG)) == s ){
			if (p->sname == q->sname) {
				q->sflags &= ~SHIDDEN;
# ifndef BUG1
				if( ddebug ) printf( "unhide uncovered %d from %d\n", q, p);
# endif
				return;
				}
			}

		}
	cerror( "unhide fails" );
	}

static
prflags(x)
{
	if (x & SMOS) printf(" MOS");
	if (x & SHIDDEN) printf(" HIDDEN");
	if (x & SHIDES) printf(" HIDES");
	if (x & SSET) printf(" SET");
	if (x & SREF) printf(" REF");
	if (x & SNONUNIQ) printf(" NONUNIQ");
	if (x & STAG) printf(" TAG");
	if (x & SPRFORWARD) printf(" PRFORWARD");
	if (x & SADDRESS) printf(" ADDRESS");
	printf("\n");
}

prsym(s)
	register struct symtab *s;
{
	printf("s->sname = '%s'\n", s->sname);
	printf("s->stype = "); tprint(s->stype); printf("\n");
	printf("s->sclass = %s\n", scnames(s->sclass));
	printf("s->slevel = 0x%x\n", s->slevel);
	printf("s->sflags = "); prflags(s->sflags);
	printf("s->offset = 0x%x\n", s->offset);
	printf("s->dimoff = 0x%x\n", s->dimoff);
	printf("s->sizoff = 0x%x\n", s->sizoff);
	printf("s->suse = 0x%x\n", s->suse);
	printf("s->hashVal = 0x%x\n", s->hashVal);
	printf("s->next = 0x%x\n", s->next);
	printf("s->symrefs = 0x%x\n", s->symrefs);
	printf("s->svarno = 0x%x\n", s->svarno);
}

prpstk()
{
	struct instk *p;
	struct symtab *s;
	int i;

	for( i = 0; instack+i <= pstk; i++) {
		printf("instack[%2d].in_sz  = %d\n", i, instack[i].in_sz);
		printf("instack[%2d].in_x   = %d\n", i, instack[i].in_x);
		printf("instack[%2d].in_n   = %d\n", i, instack[i].in_n);
		printf("instack[%2d].in_s   = %d\n", i, instack[i].in_s);
		printf("instack[%2d].in_d   = %d\n", i, instack[i].in_d);
		printf("instack[%2d].in_t   = ", i);
		tprint(instack[i].in_t); printf("\n");
		s = STP(instack[i].in_id);
		if (s == NULL) {
			printf("instack[%2d].in_id  = (0x%x)\n",
				i, s);
		} else {
			printf("instack[%2d].in_id  = \"%s\" (0x%x)\n",
				i, s->sname, s);
		}
		printf("instack[%2d].in_fl  = %d\n", i, instack[i].in_fl);
		printf("instack[%2d].in_off = %d\n", i, instack[i].in_off);
		printf("\n");
	}
}

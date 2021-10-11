#ifndef lint
static	char sccsid[] = "@(#)common.c 1.1 94/10/31 SMI";
#endif

/*
 * Guy's changes from System V lint (12 Apr 88):
 *
 * Updated "common.c".  "lint" cleanups, fixing so that fatal errors in
 * "lint" print "lint error" rather than "compiler error", explicit checks
 * for failed "malloc"s added, #ifdef FLEXNAMES removed.
 */

#ifdef PASS1
#include "cpass1.h"
#endif

#ifdef PASS2
#include "cpass2.h"
#endif PASS2

#ifdef FORT
#undef BUFSTDERR
#endif

#ifndef ONEPASS
#undef BUFSTDERR
#endif

# ifndef EXIT
# define EXIT exit
# endif

int nerrors = 0;  /* number of errors */
int errline = -1; /* where last error took place */

unsigned int offsz;

unsigned caloff(){
	register i;
	unsigned int temp;
	unsigned int off;
	temp = 1;
	i = 0;
	do {
		temp <<= 1;
		++i;
		} while( temp > 0 );
	off = 1 << (i-1);
	return (off);
	}

int lshift(x,sz) {
	if (sz >= SZINT) return 0;
	return x << sz;
	}

int bitmask(sz) {
	return lshift(1,sz)-1;
	}

	/* VARARGS1 */
uerror( s, a, b, c, d, e, f ) char *s; { /* nonfatal error message */
	/* the routine where is different for pass 1 and pass 2;
	/*  it tells where the error took place */

	fflush( stdout );
	++nerrors;
	errline = lineno;
	where('u');
	fprintf( stderr, s, a, b, c, d, e, f );
	fprintf( stderr, "\n" );
#ifdef BUFSTDERR
	fflush(stderr);
#endif
	if( nerrors > 30 ) fatal( "too many errors");
	}

	/* VARARGS1 */
cerror( s, a, b, c, d, e, f ) char *s; { /* compiler error: die */
	fflush( stdout );
	where('c');
	if( nerrors && nerrors <= 30 ){ /* give the compiler the benefit of the doubt */
		fprintf( stderr, "cannot recover from earlier errors: goodbye!\n" );
		}
	else {
#ifdef LINT
		fprintf( stderr, "lint error: " );
#else
		fprintf( stderr, "compiler error: " );
#endif
		fprintf( stderr, s, a, b, c, d, e, f );
		fprintf( stderr, "\n" );
		}
#ifdef BUFSTDERR
	fflush(stderr);
#endif
	EXIT(1);
	}

/*VARARGS1*/
fatal( s, a, b, c, d, e, f ) char *s; { /* non-compiler but fatal error: die */
	fflush( stdout );
	where('f');
	fprintf( stderr, "fatal error: " );
	fprintf( stderr, s, a, b, c, d, e, f );
	fprintf( stderr, "\n" );
#ifdef BUFSTDERR
	fflush(stderr);
#endif
	EXIT(1);
	}
int Wflag = 0; /* Non-zero means do not print warnings */

	/* VARARGS1 */
werror( s, a, b, c, d, e, f ) char *s; {  /* warning */
	if(Wflag) return;
	fflush( stdout );
	where('w');
	fprintf( stderr, "warning: " );
	fprintf( stderr, s, a, b, c, d, e, f );
	fprintf( stderr, "\n" );
#ifdef BUFSTDERR
	fflush(stderr);
#endif
	}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifdef	TESTALLOC
/*
 * this enables us to test 
 * tree allocation without having
 * to construct bizarre test programs
 */
#undef	TREESZ
#define TREESZ	16
#endif	TESTALLOC

#define	MAXTSEG	64				/* max # of tree segments */
static	NODE	treespace[TREESZ];		/* initial tree space */
static	NODE	*treeseg[MAXTSEG] = {treespace};/* table of seg pointers */
static	NODE	**activeseg = &treeseg[0];	/* ptr to active seg slot */
static	NODE	*node = treespace;		/* ptr to active segment */
static	NODE	*nextfree = treespace;		/* ptr to next free node */
static	int	nsegs = 1;			/* # of allocated segments */
static	int	recycling = 0;			/* =1 if using old nodes */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static NODE *
tsegalloc()
{
	NODE *newseg;
	if ( activeseg == &treeseg[nsegs-1] ) {
		/*
		 * no unused segments; allocate a new one
		 */
		if (nsegs == MAXTSEG) {
			cerror("out of tree space; try simplifying");
			/*NOTREACHED*/
		}
		newseg = (NODE *)malloc(TREESZ*sizeof(NODE));
		if (newseg == NIL) {
			fatal("no memory for expression trees");
			/*NOTREACHED*/
		}
		*++activeseg = newseg;
		nsegs++;
	} else {
		/*
		 * segment already allocated; use it
		 */
		newseg = *++activeseg;
		if (newseg == NIL) {
			cerror("tree space allocation");
			/*NOTREACHED*/
		}
	}
	return newseg;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

tinit()
{
	/* initialize tree space allocation */
	activeseg = &treeseg[0];	/* ptr to active seg slot */
	node = *activeseg;		/* ptr to active segment */
	nextfree = node;		/* ptr to next free node */
	recycling = 0;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define TNEXT(p) (p == &node[TREESZ]? node : p+1)
 
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

NODE *
pcc_talloc()
{
	register NODE *p, *q;
	static NODE *lastfrag;

	if ( nextfree < &node[TREESZ] )
		return nextfree++;
	if ( !recycling ) {
		recycling = 1;
		lastfrag = node;
	}
	q = lastfrag;
	for( p = TNEXT(q); p != q; p = TNEXT(p) ) {
		if (p->tn.op == FREE) {
			return lastfrag = p;
		}
	}
	/*
	 * current tree space segment is full;
	 * get a new one.
	 */
	nextfree = node = tsegalloc();
	recycling = 0;
	return nextfree++;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

pcc_tcheck()
{
	/*
	 * ensure that all nodes have been freed.  Note that in the
	 * C compiler, some nodes remain active until the end of the
	 * current function, so this should not be called until then.
	 * In the backend-only configuration (fort) this is called
	 * on a statement-by-statement basis.
	 */

	register NODE *p;
	register NODE *limit;
	NODE **seg;

	if( !nerrors ) {
		/*
		 * all segments below the top one must be scanned
		 * from beginning to end.  The top one need only
		 * be scanned up to the high-water mark (nextfree)
		 */
		for( seg = &treeseg[0]; seg < activeseg; seg++ ) {
			limit = *seg + TREESZ;
			for (p = *seg; p < limit; p++ ) {
				if( p->in.op != FREE ) {
					fprintf( stderr, "op: %s, val: %ld\n",
					    pcc_opname(p->in.op), p->tn.lval );
					cerror( "wasted space: %#x", p );
				}
			}
		}	
		limit = nextfree;
		for( p=node; p < limit; ++p ) {
			if( p->in.op != FREE ) {
				fprintf( stderr, "op: %s, val: %ld\n",
				    pcc_opname(p->in.op), p->tn.lval );
				cerror( "wasted space: %#x", p );
			}
		}
	}
	tinit();
	freetstr();
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void
pcc_tfree(p)
	register NODE *p;
{
	if (p == NIL)
		return;
again:
	switch(optype(p->in.op)) {
	case UTYPE:
		pcc_tfree(p->in.left);
		/*fall through*/
	case LTYPE:
		p->in.op = FREE;
		return;
	case BITYPE:
		pcc_tfree(p->in.left);
		p->in.op = FREE;
		p = p->in.right;
		goto again;
	}
}

/*
 * free a single node
 */
void
pcc_freenode(p)
	NODE *p;
{
	p->in.op = FREE;
}

/*
 * make a copy of a tree.  Note that this should
 * not be confused with the tcopy routine in pass 2,
 * which also updates the status of registers used in
 * the tree.
 */

NODE *
pcc_tcopy(p)
	register NODE *p;
{
	register NODE *q = talloc();
	*q = *p;
	switch(optype(p->in.op)) {
	case BITYPE:
		q->in.right = pcc_tcopy(p->in.right);
		/*FALL THROUGH*/
	case UTYPE:
		q->in.left = pcc_tcopy(p->in.left);
		break;
	case LTYPE:
		break;
	}
	return(q);
}

fwalk( t, f, down ) register NODE *t; int (*f)(); {

	int down1, down2;

	more:
	down1 = down2 = 0;

	(*f)( t, down, &down1, &down2 );

	switch( optype( t->in.op ) ){

	case BITYPE:
		fwalk( t->in.left, f, down1 );
		t = t->in.right;
		down = down2;
		goto more;

	case UTYPE:
		t = t->in.left;
		down = down1;
		goto more;

		}
	}

walkf( t, f )
	register NODE *t;
	int (*f)();
{
	switch(optype(t->in.op)) {
	case LTYPE:
		break;
	case UTYPE:
		walkf(t->in.left, f);
		break;
	default:
		walkf(t->in.left, f);
		walkf(t->in.right, f);
		break;
	}
	(*f)( t );
}

#if TARGET == I386 && !defined(LINT)
#include "dope.h"
#else
int dope[ DSIZE ];
char *opst[DSIZE];

struct dopest { int dopeop; char opst[8]; int dopeval; } indope[] = {

	NAME, "NAME", LTYPE,
	STRING, "STRING", LTYPE,
	REG, "REG", LTYPE,
	OREG, "OREG", LTYPE,
	ICON, "ICON", LTYPE,
	FCON, "FCON", LTYPE,
	FCCODES, "FCCODES", LTYPE,
	CCODES, "CCODES", LTYPE,
	UNARY MINUS, "U-", UTYPE,
	UNARY MUL, "U*", UTYPE,
	UNARY AND, "U&", UTYPE,
	UNARY CALL, "UCALL", UTYPE|CALLFLG,
	UNARY FORTCALL, "UFCALL", UTYPE|CALLFLG,
	NOT, "!", UTYPE|LOGFLG,
	COMPL, "~", UTYPE,
	FORCE, "FORCE", UTYPE,
	INIT, "INIT", UTYPE,
	SCONV, "SCONV", UTYPE,
	PCONV, "PCONV", UTYPE,
	PLUS, "+", BITYPE|FLOFLG|COMMFLG,
	ASG PLUS, "+=", BITYPE|ASGFLG|ASGOPFLG|FLOFLG,
	MINUS, "-", BITYPE|FLOFLG,
	ASG MINUS, "-=", BITYPE|FLOFLG|ASGFLG|ASGOPFLG,
	MUL, "*", BITYPE|FLOFLG|MULFLG|COMMFLG,
	ASG MUL, "*=", BITYPE|FLOFLG|MULFLG|ASGFLG|ASGOPFLG,
	AND, "&", BITYPE|SIMPFLG|COMMFLG,
	ASG AND, "&=", BITYPE|SIMPFLG|ASGFLG|ASGOPFLG,
	QUEST, "?", BITYPE,
	COLON, ":", BITYPE,
	ANDAND, "&&", BITYPE|LOGFLG,
	OROR, "||", BITYPE|LOGFLG,
	CM, ",", BITYPE,
	COMOP, ",OP", BITYPE,
	ASSIGN, "=", BITYPE|ASGFLG,
	DIV, "/", BITYPE|FLOFLG|MULFLG|DIVFLG,
	ASG DIV, "/=", BITYPE|FLOFLG|MULFLG|DIVFLG|ASGFLG|ASGOPFLG,
	MOD, "%", BITYPE|DIVFLG,
	ASG MOD, "%=", BITYPE|DIVFLG|ASGFLG|ASGOPFLG,
	LS, "<<", BITYPE|SHFFLG,
	ASG LS, "<<=", BITYPE|SHFFLG|ASGFLG|ASGOPFLG,
	RS, ">>", BITYPE|SHFFLG,
	ASG RS, ">>=", BITYPE|SHFFLG|ASGFLG|ASGOPFLG,
	OR, "|", BITYPE|COMMFLG|SIMPFLG,
	ASG OR, "|=", BITYPE|SIMPFLG|ASGFLG|ASGOPFLG,
#if TARGET==SPARC || TARGET==VAX
	ER, "^", BITYPE|COMMFLG|SIMPFLG,
	ASG ER, "^=", BITYPE|SIMPFLG|ASGFLG|ASGOPFLG,
#else
	ER, "^", BITYPE|COMMFLG,
	ASG ER, "^=", BITYPE|ASGFLG|ASGOPFLG,
#endif
	INCR, "++", BITYPE|ASGFLG,
	DECR, "--", BITYPE|ASGFLG,
	STREF, "->", BITYPE,
	CALL, "CALL", BITYPE|CALLFLG,
	FORTCALL, "FCALL", BITYPE|CALLFLG,
	EQ, "==", BITYPE|LOGFLG,
	NE, "!=", BITYPE|LOGFLG,
	LE, "<=", BITYPE|LOGFLG,
	LT, "<", BITYPE|LOGFLG,
	GE, ">=", BITYPE|LOGFLG,
	GT, ">", BITYPE|LOGFLG,
	UGT, "UGT", BITYPE|LOGFLG,
	UGE, "UGE", BITYPE|LOGFLG,
	ULT, "ULT", BITYPE|LOGFLG,
	ULE, "ULE", BITYPE|LOGFLG,
	TYPE, "TYPE", LTYPE,
	LB, "[", BITYPE,
	CBRANCH, "CBRANCH", BITYPE,
	FLD, "FLD", UTYPE,
	PMCONV, "PMCONV", BITYPE,
	PVCONV, "PVCONV", BITYPE,
	RETURN, "RETURN", BITYPE|ASGFLG|ASGOPFLG,
	CAST, "CAST", BITYPE|ASGFLG|ASGOPFLG,
	GOTO, "GOTO", UTYPE,
	STASG, "STASG", BITYPE|ASGFLG,
	STARG, "STARG", UTYPE,
	STCALL, "STCALL", BITYPE|CALLFLG,
	UNARY STCALL, "USTCALL", UTYPE|CALLFLG,
	CHK,  "CHK",  BITYPE,
	-1,	"",	0
};

mkdope(){
	register struct dopest *q;

	for( q = indope; q->dopeop >= 0; ++q ){
		dope[q->dopeop] = q->dopeval;
		opst[q->dopeop] = q->opst;
	}
#ifndef LINT
	moredope(); /* machine dependent */
#endif !LINT
}
#endif TARGET == I386

/*
 * ascii name of operator
 */
char *
pcc_opname(o)
{
    char *s;
    static char noname[30];

    if (o >= 0 && o < DSIZE) {
	s = opst[o];
    } else {
	s = NULL;
    }
    if (s == NULL) {
	s = noname;
	sprintf(s, "op#%d", o);
    }
    return(s);
}

tprint( t )  TWORD t; { /* output a nice description of the type of t */

	static char * tnames[] = {
		"undef",
		"farg",
		"char",
		"short",
		"int",
		"long",
		"float",
		"double",
		"strty",
		"unionty",
		"enumty",
		"moety",
		"uchar",
		"ushort",
		"unsigned",
		"ulong",
		"void",
		"TERROR",
		};

	for(;; t = DECREF(t) ){

		if( ISPTR(t) ) printf( "PTR " );
		else if( ISFTN(t) ) printf( "FTN " );
		else if( ISARY(t) ) printf( "ARY " );
		else {
			if ((unsigned)t > TVOID) {
				printf( "t=0x%x ", t );
				}
			else {
				printf( "%s ", tnames[t]);
				}
			return;
			}
		}
	}

#define TSTRSZ		2048

struct stringbuf {
	struct stringbuf *next;
	char *nextch;
	char *buf;
	char *limit;
};

static struct stringbuf *tsp = NULL;

char *
tstr(cp)
	register char *cp;
{
	register int len;
	register char *buf;
	register struct stringbuf *sp;

	len = strlen(cp)+1;
	for (sp = tsp; sp != NULL; sp = sp->next) {
		if (sp->nextch + len < sp->limit)
			break;
	}
	if (sp == NULL) {
		unsigned bufsiz = (len > TSTRSZ ? len : TSTRSZ);
		sp = (struct stringbuf *)malloc(sizeof(struct stringbuf));
		if (sp == NULL)
			fatal("no memory for temporary strings");
		buf = malloc(bufsiz);
		if (buf == NULL)
			fatal("no memory for temporary strings");
		sp->buf = buf;
		sp->nextch = buf;
		sp->limit = buf + bufsiz;
		sp->next = tsp;
		tsp = sp;
	}
	buf = sp->nextch;
	sp->nextch += len;
	strcpy(buf, cp);
	return (buf);
}

/*
 * free temp string buffers, saving the last one for next time.
 * This is an adjustment to the fact that tcheck() is called
 * once per statement in the standalone code generators, while
 * in the one-pass compiler it is called once at function end.
 */
void
freetstr()
{
	struct stringbuf *sp, *next;

	if (tsp != NULL) {
		/* save one buffer for next time */
		sp = tsp->next;
		tsp->next = NULL;
		tsp->nextch = tsp->buf;
		/* free the rest */
		while (sp != NULL) {
			next = sp->next;
			free(sp->buf);
			free(sp);
			sp = next;
		}
	}
}


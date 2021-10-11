#ifndef lint
static	char sccsid[] = "@(#)code.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 *  code.c - front end IR/assembly code generators
 *
 *	Routines in this file generate pseudo-ops and other detritus
 *	to the IR or assembler stream. (but no machine instructions!)
 *	This is also where one finds most of the functions that switch
 *	behavior depending on whether one-pass or two-pass compilation
 *	is in effect. (the other place is local.c) This switching can
 *	easily be made at compiler generation time if desired.
 */

# include "cpass1.h"
# include "sw.h"
# include <sys/types.h>
# include <a.out.h>
# include <stab.h>
# include <ctype.h>
# include <varargs.h>
# include <signal.h>

/* state info for intermediate/assembly code generation */

char *passname = "ccom";
int twopass = 0;	/* are we generating an IR file? */
int optimize = 1;	/* optimization level */
extern _doprnt();	/* from stdio */
int lastloc = { -1 };	/* current csect { PROG, DATA, ADATA, ... } */
FILE *ir_output = NULL;	/* IR output file */
int innerlist = 0;	/* mixed source/assembly listing */

/* from ir_misc */

extern void ir_init(/*procname,procno,proctype,procglobal,stsize,stalign,
			source_file, source_line*/);
extern void ir_bbeg(/*autosize, regmask, fregmask*/);

/* from ir_wf */

extern void ir_write_file(/*irfile, opt_level*/);

/* from ir_map */

extern void ir_pass(/*s*/);
extern void ir_branch(/*n*/);
extern void ir_labeldef(/*lab*/);
extern void ir_exit(/*ftype,fsize,falign,retlab*/);
extern void ir_mkalias(/*sym*/);
extern void ir_map_aliases(/*sym*/);

/* state info for generating dbx and IR data */

int gdebug;		/* is source debugging enabled? (implies !optimize) */
int safe_opt_level;	/* safe optimization level, set from source info */
char NULLNAME[8];
int labelno;
int fdefflag;		/* are we within a function definition ? */

static TWORD ftype;		/* type of function result */
static OFFSZ fsize = 0;		/* size of function result */
static int   falign = 0;	/* alignment of result */
static struct symtab *fvar;	/* hidden var for function result -- for IR only */

static char *builtin_va_alist_name = NULL; /* hashed name of va_alist arg */

#define SZVARARGS (SZINT * (MAXINREG - RI0 + 1))  /* size of va_alist */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * interpret filename arguments from the command line
 */

void
fileargs(name)
    char *name;
{
    char buf[BUFSIZ];
    static int fdef = 0;

    switch(fdef++) {
    case 0:
	/* 1st filename is associated with stdin */
	if (name[0] == '\0') {
	    break;
	}
	if (freopen(name, "r", stdin) == NULL) {
	    fprintf(stderr, "ccom:can't open %s\n", name);
	    exit(1);
	}
	sprintf(buf, "%c%s%c", '"', name, '"');
	ftitle = hash(buf);
	break;
    case 1:
	if (name[0] == '\0') {
	    break;
	}
	if (twopass) {
	    /*
	     * 2nd argument is the name of a random-access binary file;
	     * stdout remains available for debugging output
	     */
	    if ((ir_output = fopen(name, "w")) == NULL) {
		fprintf(stderr, "%s:can't open %s\n", passname, name);
		exit(1);
	    }
	} else {
	    /* 2nd argument is associated with stdout */
	    if (freopen(name, "w", stdout) == NULL) {
		fprintf(stderr, "%s:can't open %s\n", passname, name);
		exit(1);
	    }
	}
	break;
    default:
	fprintf(stderr,"%s: name %s ignored\n", passname, name);
	break;
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * putprintf handles formatted text writes to one of two destinations.
 *	In one-pass compilation mode, output goes directly to stdout.
 *	In two-pass mode, it goes
 *	    -- to the IR data structure, if we are within a function;
 *	    -- to stdout, otherwise (assume this is data, not code)
 *	Note that we are assuming all the <varargs.h> stuff works
 *	as advertised when moving to a new host...
 */

/* VARARGS */
putprintf(va_alist)
    va_dcl
{
    va_list argp;
    char *fmt;
    char strbuf[BUFSIZ];

    va_start(argp);
    fmt = va_arg(argp, char*);
    if (twopass && fdefflag && lastloc == PROG) {
	/* print into a string buffer and build an IR_PASS triple */
	(void)vsprintf(strbuf, fmt, argp);
	ir_pass(strbuf);
    } else {
	/* print to stdout, as in normal printf */
	(void)vfprintf(stdout, fmt, argp);
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

branch( n )
{
	/* output a branch to label n */
	if( twopass ) {
		ir_branch(n);
	} else {
		/* generate branch directly */
		p2branch(n == retlab ? EXITLAB : n);
	}
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

defalign(n) 
{
	/* cause the alignment to become a multiple of n */
#ifndef VCOUNT
	if( lastloc != PROG && n > ALCHAR ) {
		switch (n){
		case ALSHORT:
		case ALINT:
		case ALDOUBLE:
			putprintf("	.align	%d\n", n/SZCHAR);
			break;
		default:
			cerror("bogus alignment\n");
		}
	}
#endif
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

locctr( l )
{
	register temp;
	extern int xdebug;
	/* l is PROG, ADATA, DATA, STRNG, ISTRNG, or STAB */

	if( l == lastloc ) return(l);
	temp = lastloc;
	lastloc = l;
#ifndef VCOUNT
	switch( l ){

	case PROG:
		putprintf( "	.seg	\"text\"\n" );
		psline(lineno);
		break;

	case DATA:
#ifdef SMALL_DATA
		if (!xdebug){
		    putprintf( "	.seg	\"sdata\"\n" );
		    break;
		}
#endif SMALL_DATA
		/* else FALL THROUGH */
	case ADATA:
		putprintf( "	.seg	\"data\"\n" );
		break;

	case BSS:
		putprintf( "	.seg	\"bss\"\n" );
		break;

	/* The following levels of .data statements will be problems if
	   there is ever any relocation required. */
	case STRNG:
		putprintf( "	.seg	\"data1\"\n" );
		break;

	case ISTRNG:
		putprintf( "	.seg	\"data1\"\n" );
		break;

	case STAB:
	default:
		cerror( "illegal location counter" );
		}
#endif

	return( temp );
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

deflab( n )
{
	/* emit something to define the current position as label n */
	if (twopass && fdefflag) {
		if (lastloc == PROG) {
			/* text label: generate IR_LABELDEF operator */
			ir_labeldef(n);
		} else {
			/* data label: print directly to data file */
			putprintf("L%d:\n", n);
		}
	} else {
		p2deflab(n);
	}
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

getlab()
{
	static int crslab = 10;
	/* return a number usable for a label */
	return( ++crslab );
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

efcode()
{
    register i;
    register struct symtab *p;

    if (twopass) {
	/* end of block; write data to IR file here */
	if (ir_output == NULL) {
	    ir_output = fopen("ir.out", "w");
	    if (ir_output == NULL) {
		cerror("cannot open ir.out");
	    }
	}
	for (i = 0; i < SYMTSZ; i++) {
	    p = stab[i];
	    while (p != NULL) {
		switch(p->sclass) {
		case EXTERN:
		case EXTDEF:
		case STATIC:
		    ir_map_aliases(p);
		    break;
		}
		p = p->next;
	    }
	}  
	ir_exit(ftype, fsize, falign, retlab);
	ir_write_file(ir_output, safe_opt_level);
    } else {
	p2bend(ftype, fsize, falign, safe_opt_level);
    }
    free(fvar);	/* leftover copy of symbol table entry */
    fvar = NULL;
    fdefflag = 0;
    safe_opt_level = optimize;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef VCOUNT
/*
 * generate intermediate code to force shortening of an
 * incoming argument reg to its declared type. This is necessary
 * because of the backend's convention that small integral values
 * in registers are always extended to their 32-bit representation.
 */
static void
launder_param( regno, type )
    int regno;
    TWORD type;
{
    NODE *r,*l;

    switch (type){
    case CHAR:
    case UCHAR:
    case SHORT:
    case USHORT:
	r = block(REG, NIL, regno, INT, 0, INT);
	r = block(SCONV, r, NIL, type, 0, (int)type);
	l = block(REG, NIL, regno, type, 0, (int)type);
	l = block(ASSIGN, l, r, type, 0, (int)type);
	ecomp(l);
	return;
    }
}

/*
 * generate intermediate code to load a value parameter
 * from the stack into a register
 */
static void
load_param(offset, regno, size, type)
    OFFSZ offset;
    int regno;
    int size;
    TWORD type;
{
    NODE *l,*r;
    TWORD ptrtype;

    while (size > 0) {
	type = (size>=SZINT)? INT : type;
	ptrtype = INCREF(type);
	r = block(REG, NIL, AUTOREG, ptrtype, 0, INT);
	r = block(PLUS, r, bcon(offset/SZCHAR), ptrtype, 0, INT);
	r = block(UNARY MUL, r, NIL, type, 0, (int)type);
	l = block(REG, NIL, regno, type, 0, (int)type);
	l = block(ASSIGN, l, r, type, 0, (int)type);
	ecomp(l);
	regno++;
	offset += SZINT;
	size -= SZINT;
    }
}

/*
 * generate intermediate code to store words (or part
 * thereof) from incoming argument registers to a stack
 * location
 */
static void
store_param(offset, regno, size)
    OFFSZ offset;
    int regno;
    int size;
{
    NODE *l,*r;
    TWORD ptrtype;
    TWORD type;

    while (size > 0 && regno <= MAXINREG) {
	type = (size>=SZINT)? INT : (size>=SZSHORT)?SHORT : CHAR;
	ptrtype = INCREF(type);
	l = block(REG, NIL, AUTOREG, ptrtype, 0, INT);
	l = block(PLUS, l, bcon(offset/SZCHAR), ptrtype, 0, INT);
	l = block(UNARY MUL, l, NIL, type, 0, (int)type);
	r = block(REG, NIL, regno, type, 0, (int)type);
	r = block(ASSIGN, l, r, type, 0, (int)type);
	ecomp(r);
	regno++;
	offset += SZINT;
	size -= SZINT;
    }
}

/*
 * This is where we actually bind parameters to registers. Because
 * of the "portable" compiler structure, we didn't previously have
 * enough information to bind parameters to the right registers.
 * Now we do. Iterate through the parameter list, calculating addresses.
 * For register variables, calculate the register number. For non-
 * register variables, calculate the register number AND generate 
 * instructions to move the variable to memory, if necessary.
 */
static void
bind_params(a,n)
    int a[];		/* parameter list, from bfcode[] */
    int n;		/* # of parameters */
{
    register i;
    register struct symtab *p;
    TWORD    ttype;
    int	true_class;
    int do_store;
    int argreg;
    int varsize;
    OFFSZ off;
    int strparam;

    off = ARGINIT;
    blevel = 2;

    for( i=0; i<n; ++i ){
	p = STP(a[i]);
	if (p == NULL)
	    break;
	true_class = p->sclass;
	ttype = p->stype;
	strparam = 0;
	do_store = 1;
	p->sclass = PARAM;
	p->offset = NOOFFSET;
	if ((ttype==STRTY) || (ttype==UNIONTY)){
	    p->stype = INCREF(ttype);
	    strparam = 1;
	    do_store = 0;
	}
	oalloc( p, &off );
	p->stype = ttype;
	argreg = RI0 + (p->offset-ARGINIT)/SZINT;
	if (p->sname == builtin_va_alist_name){
	    /*
	     * va_alist must be the last parameter
	     */
	    if ( i != n-1 ) {
		uerror("there are parameters after va_alist");
		break;
	    }
	}
	if (ir_optimizing()) {
	    /*
	     * if we are optimizing, then we won't know the reg
	     * assignments until after the optimization pass, and
	     * we won't know which parameters need to be made
	     * addressable until the end of the procedure.  So
	     * we put off all of this code until later.  Reg args
	     * are initialized by iropt; stores are generated by
	     * cgrdr -- including the VARARGS case.
	     */
	    continue;
	}
	if( true_class == REGISTER ){
	    do_store = 0;
	}
	if (do_store) {
	    /* must move out of the register */
	    if (p->sname == builtin_va_alist_name){
		/*
		 * move all the remaining parameter
		 * registers out to the stack
		 */
		varsize = SZVARARGS;
	    } else {
		varsize = (ISPTR(p->stype))? SZPOINT: tsize( p->stype, p->dimoff, p->sizoff );
	    }
	    store_param(p->offset, argreg, varsize);
	} else {
	    if ( argreg <= MAXINREG  && !ISFLOATING(ttype)){
		/* can use the register we're already in */
		if(gdebug) /* describe it as a param before it becomes a reg */
			outstab(p);
		p->sclass = REGISTER;
		p->offset = argreg;
		SETD( regvar, argreg );
		launder_param( argreg, p->stype );
	    } else if ( regvar_avail( ttype ) ){
		if(gdebug) 
			outstab(p);
		varsize = strparam? SZPOINT : tsize( ttype, p->dimoff, p->sizoff );
		if (ISFLOATING(ttype))
		    store_param(p->offset, argreg, varsize);
		p->sclass = REGISTER;
		argreg = regvar_alloc(p->stype);
		load_param(p->offset, argreg, varsize, p->stype);
		p->offset = argreg;
		markused(argreg);
	    } else {
		/* out of registers -- tough cookies */
	    }
	}
	if(gdebug) /* now we know which register or offset will be used */
		outstab(p);
    }
    blevel = 1;
}

#endif

/*
 * Make up an automatic var to hold the result of the current
 * function. We only do this with optimization on, under the
 * assumption that Iropt will modify uses of this var to use
 * a register instead.  The reason for all this is to avoid
 * making temp registers live across basic blocks, which would
 * happen if we translated return statements using the FORCE
 * op directly.
 *
 * Note that because all local symbols are returned the heap
 * by clearst(), we have a dangling reference problem in efcode(),
 * which occurs after clearst().  So we make a copy of the hidden
 * var now and use it later.  The copy must also be in the heap,
 * because STP() wants it there.
 *
 * 1e+308 on the vomit meter.
 */
static struct symtab *
mkfvar(s)
	struct symtab *s;
{
	NODE *p;
	char name[30];
	int temp;
	TWORD t;
	struct symtab *copy;

	temp = blevel;
	blevel = 2;
	t = DECREF(s->stype);
	if (t == STRTY || t == UNIONTY) {
		t = INCREF(t);
	}
	p = block(NAME, NIL, NIL, t, s->dimoff, s->sizoff);
	sprintf(name,"#ftn.%d", ftnno);
	p->tn.rval = lookup(hash(name),0);
	defid(p, AUTO);
	s = STP(p->tn.rval);
	p->in.op = FREE;
	blevel = temp;
	/* save copy of s (see note above) */
	copy = (struct symtab*)malloc(sizeof(struct symtab));
	*copy = *s;
	return copy;
}

/*
 * fake up a NAME node referring to the function result var
 */
NODE *
pcc_fvar()
{
    NODE *p;

    if (fvar == NULL) {
	cerror("pcc_fvar(): no result for this function");
    }
    p = block(NAME, NIL, fvar, fvar->stype, fvar->dimoff, fvar->sizoff);
    return optim(p);
}

bfcode( a, n ) int a[]; 
{
	/*
	 * code for the beginning of a function; a is an array of
	 * indices in stab for the arguments; n is the number
	 */
	register struct symtab *p;
	int fglobal;

	p = STP(curftn);
	if (p == NULL) return;
#ifndef VCOUNT
	fglobal = (p->sclass == EXTDEF);
	ftype = DECREF(p->stype);	/* strip off FTN */
	if (ftype == TVOID) {
	    fsize = 0;
	    falign = 0;
	} else {
	    fsize = tsize(ftype, p->dimoff, p->sizoff)/SZCHAR;
	    falign = talign(ftype, p->sizoff)/SZCHAR;
	}
	fdefflag = 1;
	ftnno = getlab();
	retlab = getlab();
	safe_opt_level = optimize;
	if (twopass) {
		/* initialize IR header for this procedure */
		ir_init(exname(p->sname), ftnno, ftype, fglobal, fsize, falign,
			ftitle, lineno);
		if (ftype != TVOID) {
			/* allocate hidden var for function result */
			fvar = mkfvar(p);
		}
		locctr(PROG);
		defalign(ALINT);
	} else {
		/* define entry point directly */
		locctr( PROG );
		defalign(ALINT);
		putprintf( "	.proc %#o\n", ftype ); /* function type, for c2 */
		defnam( p );
		saveregs();
	}

	/* routine prolog */
	bccode();
	/* deal with incoming parameters */
	bind_params(a,n);

	/* sync allocation info with back end */
	bccode();
	if (gdebug) {
#ifdef STABDOT
		pstabdot(N_SLINE, lineno);
#else
		pstab(NULLNAME, N_SLINE);
		putprintf("0,%d,LL%d\n", lineno, labelno);
		putprintf("LL%d:\n", labelno++);
#endif
	}
#endif
	fdefflag = 1;
	if (twopass) {
		int i;
		for( i=0; i<n; ++i ){
			p = STP(a[i]);
			ir_argument(p);
		}
	}
#ifdef ONEPASS
	bfcode2();
#endif
}

/*
 * Called just before the first executable statement in a block.
 * This passes auto storage and reg allocation information to
 * the back end.
 */
bccode()
{
	SETOFF( autooff, SZINT );
	/* set aside store area offset */
#ifndef VCOUNT
	if (twopass) {
		ir_bbeg( autooff, regvar, floatvar );
	} else {
		p2bbeg( autooff, regvar, floatvar );
	}
#endif
}

ejobcode( flag )
{
	/* called just before final exit */
	/* flag is 1 if errors, 0 if none */
}

#if VCOUNT
static int nregisterableparams, nregisterablelocals;
static int nparams, nlocals;
static int proccalls;
#endif
aobeg()
{
	/* called before removing automatics from stab */
}

aocode(p) struct symtab *p; 
{
	/* called when automatic p removed from stab */
#   ifndef VCOUNT
	if (twopass) {
	    /*
	     * compute static aliasing information on IR
	     * leaf nodes that refer to this symbol
	     */
	    switch(p->sclass) {
	    case AUTO:
	    case EXTERN:
	    case EXTDEF:
	    case STATIC:
	    case PARAM:
		ir_map_aliases(p);
		break;
	    }
	}
#   endif

#if VCOUNT
	/* modified cisreg */
	switch (p->stype){
	case TVOID:  case MOETY: case TERROR:
	    return;
	case STRTY: case UNIONTY:
	    /* struct/union parameters are really pointers */
	    if (p->slevel != 1 ) return;
	case DOUBLE:
	case FLOAT:
	case INT:
	case UNSIGNED:
	case SHORT:
	case USHORT:
	case CHAR:
	case UCHAR:	
		break;
	default:
		if( !ISPTR(p->stype) ) return;
	}
	if (!(p->sflags&(SMOS|STAG)) ){
	    if (p->sflags&SADDRESS){
		if (p->slevel==1)
		    nparams++;
		else
		    nlocals++;
	    } else {
		if (p->slevel==1)
		    nregisterableparams++;
		else
		    nregisterablelocals++;
	    }
	}
#endif

}

aoend()
{
	/* called after removing all automatics from stab */
#if VCOUNT
	if (blevel != 0 ) return;
	/* only print sum for outer block */
	printf( "%s:%d:%d:%d:%d:%d:%d\n",  STP(curftn)->sname,
	    nregisterableparams, nregisterablelocals,
	    nregisterableparams+ nregisterablelocals,
	    nparams, nlocals, proccalls);
	nregisterableparams= nregisterablelocals =0;
	nparams= nlocals =0;
	proccalls = 0;
#endif
}

#if VCOUNT
amper_of(n) int n;
{
    struct symtab *p;
    if (n > 0 && n != NONAME ){
	p = STP(n);
	p->sflags |= SADDRESS;
    }
}

does_call()
{
	proccalls = 1;
}

#endif

defnam( p ) register struct symtab *p; 
{
	/* define the current location as the name p->sname */

#ifndef VCOUNT
	if( p->sclass == EXTDEF ){
		putprintf( "	.global	%s\n", exname( p->sname ) );
	}
	if( p->sclass == STATIC && p->slevel>1 ) {
		deflab( p->offset );
	} else {
		putprintf( "%s:\n", exname( p->sname ) );
	}
#endif

}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * initstr(), emitstr(), resetstr() are subroutines of strcode()
 * which handles string constants (below)
 */

static char strbuf[80];

static char*
emits(bp)
	register char *bp;
{
	/* emit a string */
	*bp++ = '"';
	*bp++ = '\0';
	putprintf("%s\n", strbuf);
	bp = strbuf;
	return bp;
}

static char*
inits()
{
	/* init a string for output */
	register char *bp;
	sprintf(strbuf, "	.ascii	\"");
	bp = strbuf;
	while(*++bp);
	return bp;
}

static char*
resets(bp)
	register char *bp;
{
	/* emit the string and re-initialize */
	(void) emits(bp);
	return inits();
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * strcode(s, slen): put out a string constant as
 *	a series of .ascii directives in the assembler
 *	stream.  slen is the length of the string
 *	minus the trailing null.
 */
strcode( s, slen )
	register char *s;
	register slen;
{
	register int lastoctal;
	register char *bp;	/* ptr to output buffer */
	register char c;
	register i;

#	define putchars(bp,t) { lastoctal = 0; *(bp)++ = (t); }

	lastoctal = 0;
	bp = inits();
	for (i = 0; i < slen; i++) {
		c = *s++;
		if (bp > strbuf+64) {
			bp = resets(bp);	
		}
		switch(c) {
		case '\\':
		case '"':
			/* must escape */
			putchars(bp,'\\');
			putchars(bp,c);
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (lastoctal) {
				/* flush string to avoid confusion with
				   octal digits */
				bp = resets(bp);
			}
			putchars(bp,c);
			break;
		default:
			if (isprint(c) && isascii(c)) {
				putchars(bp,c);
			} else {
				/* nonprinting -- put out in octal */
				lastoctal++;
				sprintf(bp, "\\%03o", (unsigned char)c);
				while(*++bp);
			}
			break;
		}
	} 
	/* terminate with explicit "\0" escape */
	putchars(bp,'\\');
	putchars(bp,'0');
	bp = emits(bp);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

zecode( n )
{
	/* n integer words of zeros */
	OFFSZ temp;
	register i;

	if( n <= 0 ) return;
#ifndef VCOUNT
	putprintf( "	.skip	%d\n", (SZINT/SZCHAR)*n );
#endif
	temp = n;
	inoff += temp*SZINT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static inwd;	/* current bit offsed in word */
static word;	/* word being built from fields */

incode( p, sz ) register NODE *p; 
{

	/* generate initialization code for assigning a constant c
		to a field of width sz */
	/* we assume that the proper alignment has been obtained */
	/* inoff is updated to have the proper final value */
	/* we also assume sz  < SZINT */

	if((sz+inwd) > SZINT) cerror("incode: field > int");
	word |= (p->tn.lval & ((1 << sz) -1)) << (SZINT - sz - inwd);
	inwd += sz;
	inoff += sz;
	while (inwd >= 16) {
#ifndef VCOUNT
		putprintf( "	.half	%ld\n", (word>>16)&0xFFFFL );
		word <<= 16;
#endif
		inwd -= 16;
	}
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * Emit the initialization of p into a space of
 * size sz.  Assume the proper alignment has been obtained.
 * On exit, inoff is updated to have the proper final value.
 */
cinit( p, sz ) NODE *p; 
{
	/*
	 * as a favor (?) to people who want to write 
	 *	int i = 9600/134.5;
	 * we will, under the proper circumstances, do
	 * a coersion here.
	 */
	NODE *l;
	long *lval;
	float fval;
	struct symtab *s;

	switch (p->in.type) {
	case INT:
	case UNSIGNED:
		l = p->in.left;
		if (l->in.op != SCONV || l->in.left->tn.op != FCON) break;
		l->in.op = FREE;
		l = l->in.left;
		l->tn.lval = (long)(l->fpn.dval);
		l->tn.rval = NONAME;
		l->tn.op = ICON;
		l->tn.type = INT;
		p->in.left = l;
		break;
	}
	p = p->in.left;
	if (p->in.op != ICON && p->in.op != FCON) {
		uerror("illegal initialization");
	}
	switch( p->in.type ) {
	case CHAR:
	case UCHAR:
		putprintf("	.byte	0x%x\n", p->tn.lval);
		break;
	case SHORT:
	case USHORT:
		putprintf("	.half	0x%x\n", p->tn.lval);
		break;
	case FLOAT:
		fval = (float)p->fpn.dval;
		lval = (long*)&fval;
		putprintf("	.word	0x%x\n", lval[0]);
		break;
	case DOUBLE:
		lval = (long*)&(p->fpn.dval);
		putprintf("	.word	0x%x,0x%x\n", lval[0], lval[1]);
		break;
	case INT:
	case UNSIGNED:
	case LONG:
	case ULONG:
	default:
		if (p->tn.rval == NONAME) {
			putprintf("	.word	0x%x\n", p->tn.lval);
		} else if (p->tn.rval < 0) {
			putprintf("	.word	L%d+0x%x\n", -p->tn.rval,
				p->tn.lval);
		} else {
			s = STP(p->tn.rval);
			if (s == NULL) {
				cerror("garbage name in cinit()");
			}
			if ( s->sclass == STATIC && s->slevel >0){
				putprintf("	.word	L%d+0x%x\n",
					s->offset, p->tn.lval);
			} else {
				putprintf("	.word	%s+0x%x\n",
					exname(s->sname), p->tn.lval);
			}
			if (twopass && fdefflag) {
				/* watch out -- taking address of a symbol */
				ir_mkalias(s);
			}
		}
		break;
	}
	inoff += sz;
}


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

vfdzero( n )
{ /* define n bits of zeros in a vfd */

	if( n <= 0 ) return;

	inwd += n;
	inoff += n;
#ifndef VCOUNT
	while (inwd >= 16) {
		putprintf( "	.half	%ld\n", (word>>16)&0xFFFFL );
		word <<= 16;
		inwd -= 16;
	}
#endif
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * on machines where floating-point constants make no sense,
 * turn them into names.
 */

prtdcon( p )
	NODE *p;
{
	/*
	 * if your backend won't hack FCONs, output a labeled,
	 * properly aligned constant containing the floating point
	 * value, and rewrite p as a NAME with rval = -labelno
	 */
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

fldal( t ) unsigned t; 
{ /* return the alignment of field of type t */
	uerror( "illegal field type" );
	return( ALINT );
}

fldty( p ) struct symtab *p; 
{ /* fix up type of field p */
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

where(c)
{ /* print location of error  */
	/* c is either 'u', 'c', or 'w' */
	/* GCOS version */
	fprintf( stderr, "%s, line %d: ", ftitle, lineno );
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

listline()
{
    static FILE *curfile = NULL;
    static char *curtitle = NULL;
    static int curline = 0;
    char *line, buf[BUFSIZ-20];
    int len;

    if (curtitle != ftitle) {
	/* filename is null, or has changed */
	if (curfile != NULL) {
	    (void)fclose(curfile);
	}
	curtitle = ftitle;
	len = strlen(curtitle);
	if (len <= 2) return;
	(void)bcopy(ftitle+1, buf, len-2);
	buf[len-2] = '\0';
	curfile = fopen(buf,"r");
	curline = 0;
    }
    if (curfile == NULL) {
	return;
    }
    if (curline > lineno) {
	curline = 0;
	(void)fseek(curfile, 0, 0);
    }
    while (curline < lineno) {
	line = fgets(buf, sizeof(buf), curfile);
	if (line == NULL) {
	    return;
	}
	curline++;
    }
    buf[sizeof(buf)-2] = '\n';
    buf[sizeof(buf)-1] = '\0';
    putprintf("!%6d	%s", lineno, line);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

builtin_init(){
    /*
     * initialize builtin_va_alist_name to the (hashed) pointer to the string
     * "__builtin_va_alist". Parameters are compared to this as they are
     * laundered in the procedure prolog.
     */
    builtin_va_alist_name = hash( BUILTIN_VA_ALIST );
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void
handler( sig )
{
	cerror( "got signal %d", sig );
	/*NOTREACHED*/
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

main( argc, argv ) char *argv[]; 
{
	int v;
	extern _fltused;
#ifdef BUFSTDERR
	char errbuf[BUFSIZ];
	setbuf(stderr, errbuf);
#endif
	/*
	 * Some syntax errors make it impossible to keep the semantic
	 * stack(s) ballanced.  This small kluge allows us to quit
	 * without dumping core.
	 */
	signal( SIGQUIT, handler );
	signal( SIGFPE, handler );
	signal( SIGBUS, handler );
	signal( SIGSEGV, handler );
	builtin_init();
	v = mainp1( argc, argv );
	if (fclose(stdout) == EOF) {
		/* we should really check all writes; failing that, ... */
		fatal("cannot close output file");
		/*NOTREACHED*/
	}
	exit( v );
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* called from the scanner */
asmout()
{
    /*
     * dump the saved asm string directly into the assembly language stream 
     */
    extern int stringsize;
    extern char *stringval;
#ifndef VCOUNT
#ifndef LINT
    putprintf("!#ASMOUT#\n");
    putprintf("%.*s\n", stringsize, stringval);
    putprintf("!#ENDASM#\n");
    if (ir_optimizing()) {
	werror("asm statement disables optimization");
    }
    safe_opt_level = 0;
#endif
#endif
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * generate code for a switch expression.  We must do two
 * tasks normally done by ecomp(): free the expression, in
 * case of an error, and reset pass 1 temp allocation, in
 * all cases.
 */
genswitch(p, swtab, n)
	NODE *p;
	struct sw swtab[];
{
	if( nerrors ) {
		tfree(p);
	} else {
		if (twopass) {
			/* generate IR_SWITCH operator */
			ir_switch(p, swtab, n);
		} else {
			p2tree(p);
			p2switch(p, swtab, n);	/* generate code directly */
		}
	}
	p1tmpfree();
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

ecode( p )
	NODE *p;
{
	/* walk the tree and write out the nodes.. */

	if( nerrors ) return;
	if (twopass) {
		/* map trees to IR */
		(void) ir_compile(p);
	} else {
		/* do it directly */
		p2tree(p);
		p2compile(p);
	}
}

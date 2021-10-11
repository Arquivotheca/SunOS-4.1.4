#ifndef lint
static	char sccsid[] = "@(#)util2.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#include "cpass2.h"
#include "ctype.h"

/* 
 *	garbage ripped out of local2.68, which was huge
 */
# ifdef FORT
int ftlab1, ftlab2;
# endif
/* a lot of the machine dependent parts of the second pass */
#ifdef FORT
#   define fltfun 0
#else
    extern int fltfun;
#endif

int madecall = 0; /* if a procedure call was made from this procedure */
int maxargs = 0;
int maxalloc, allocoff;
static char endtmpname[20];
extern char strtmpname[];
extern NODE *rw_asgop();

int storall;

# define BITMASK(n) ((1L<<(n))-1)

/* output forms for floating point constants */
typedef enum { F_hexfloat, F_immed, F_loworder, F_highorder } Floatform;

void adrput();
static void fcon();
static void dumpfcons();

void
print_d(value)
	register int value;
{	
	char buffer[8]; 
	register char *p= buffer+8; 
	register int i= 0;

	if (value == 0) {
		putchar('0'); 
		return;
	}
	if (value < 0) {
		putchar('-'); 
		value= -value;
	}
	if (value <= 32767) { 
		register short v= value;
		while (v != 0) {
			*--p= (v % 10) + '0';
			v /= 10; 
			i++; 
		}
	} else {
		while (value != 0) {
			*--p= (value % 10) + '0';
			value /= 10; 
			i++; 
		}
	}
	for(; i>0; i--)
		putchar(*p++);
}

void
print_x(value)
	register unsigned value;
{	
	char buffer[8]; 
	register char *p= buffer+8; 
	register int i= 0;

	if (value == 0) {
		putchar('0'); 
		return;
	}
	if ((int)value < 0) {
		value = -value;
		putchar('-'); 
	}
	while (value != 0) {
		*--p = "0123456789abcdef"[value & 0xf];
		value >>= 4;
		i++; 
	}
	putchar('0'); 
	putchar('x');
	fwrite(p, 1, i, stdout);
}

void
print_str(s)
	char *s;
{
	fputs(s, stdout);
}

void
print_str_nl(s)
	char *s;
{
	puts(s);
}

void
print_str_d(s, d)
	char *s;
	int d;
{
	fputs(s, stdout);
	print_d(d);
}

void
print_str_d_nl(s, d)
	char *s;
	int d;
{
	fputs(s, stdout);
	print_d(d);
	putchar('\n');
}

void
print_str_str(s1, s2)
	char *s1, *s2;
{
	fputs(s1, stdout);
	fputs(s2, stdout);
}

void
print_str_str_nl(s1, s2)
	char *s1, *s2;
{
	fputs(s1, stdout);
	puts(s2);
}

void print_label(l)
	int l;
{
	putchar('L');
	print_d(l);
	putchar(':');
	putchar('\n');
}

where(c)
{
	fprintf( stderr, "%s, line %d: ", filename, lineno );
}

lineid( l, fn )
	char *fn; 
{
	/* identify line l and file fn */
	printf( "!	line %d, file %s\n", l, fn );
}

epr(p)
	NODE *p;
{
	extern fwalk();
	fwalk(p, eprint, 0);
}

int	usedregs; /* Flag word for registers used in current routine */
int	usedfpregs;	/* Flag word for floating point registers */
int	force_used=0;	/* if 0, function returns nothing */
int toff, maxtoff;

cntbits(i)
	register int i;
{
	register int j,ans;

	for (ans=0, j=0; i!=0 && j<SZINT; j++) { 
		if (i&1) ans++; 
		i >>= 1 ; 
	}
	return(ans);
}

bfcode2(){
	/* called from reader whenever we notice the ftnno changed */
	/* just make up the tmp-offset name  and alloca-offset name*/
	sprintf(tmpname, "LP%d", ftnno );
	sprintf(endtmpname, "LT%d", ftnno );
	sprintf(strtmpname, "LST%d", ftnno );

}

eobl2( functype, funcsize, funcalign, opt_level )
	TWORD functype;
	OFFSZ funcsize;
	int funcalign;
	int opt_level;
{
	OFFSZ spoff;	/* fp-relative offset of sp (stack-frame size) */
	OFFSZ tmpoff;	/* sp-relative offset of tmp area	*/
	OFFSZ stackargs; /* number of bytes for stack-bound arguments */
#define ARGSSAVED   (MAXINREG-RI0+1)*(SZINT/SZCHAR)
#define ARGSAVESIZE (ARGSSAVED+(ARGINIT-SAVESIZE)/SZCHAR)
#	ifndef FORT
	    extern int ftlab1, ftlab2;
#	endif

	maxoff /= SZCHAR;	/* max size of temp area */
	maxalloc /= SZCHAR;	/* max size of allocated variable (local)area */
	maxstrtmpoff /= SZCHAR; /* max size of secondary temp area */
	/*
	 * the offset of the temporary area is:
	 * 1) the size of the register window save area plus
	 * 2) the size of the arg save area, if any, plus
	 * 3) the size of the stack-bound args, if any
	 */
	tmpoff = SAVESIZE/SZCHAR;  /* the size of the register window */
	if ( madecall ){
	    stackargs = maxargs - ARGSSAVED;
	    tmpoff += ARGSAVESIZE; /* the size of the arg save area, if any */
	    if (stackargs > 0 ){ 
	        tmpoff += stackargs; /* the size of the stack-bound args, if any */
	    }	  
	    madecall = 0;
	}
	SETOFF( tmpoff, ALSTACK/SZCHAR );
	if (maxoff+tmpoff >= TMPMAX) 
		uerror( "stack overflow: too many temporary variables");
	/*
	 * The frame size is:
	 * 1) the size of the locals plus
	 * 2) the size of the temps plus
	 * 3) the size of the stack-bound args, if any, plus
	 * 4) the size of the arg save area, if any, plus
	 * 5) the size of the register window save area
	 * note that the latter three items are already computed in tmpoff.
	 */
	spoff = tmpoff + maxoff + maxalloc + maxstrtmpoff;
	SETOFF( spoff, ALSTACK/SZCHAR );
	/*
	 * set masks for saving/restoring registers
	 */
	/* usedfpregs &= FREGVARMASK; */
	/*
	 * generate epilogue
	 */
	if (functype == STRTY || functype == UNIONTY) {
	    p2strftn(funcsize, funcalign);
	} else {
	    printf( "LE%d:\n",ftnno);
	    if (!ISFLOATING( functype ) && functype != TVOID && force_used){
		print_str("	mov	%o0,%i0\n");
	    }
	    printf("	ret\n	restore\n");
	}
	if (opt_level == 0) {
		/* turn off (part of) peephole optimizer for this routine */
		printf( "	.optim	\"-O~Q~R~S\"\n" );
	}
	printf( "       LF%d = %ld\n", ftnno, -spoff );
	printf( "	LP%d = %ld\n", ftnno, tmpoff );
	printf( "	LST%d = %ld\n", ftnno, maxoff+tmpoff );
	printf( "	LT%d = %ld\n", ftnno, maxoff+tmpoff+maxstrtmpoff );
	maxargs = 0;
	usedregs = 0;
	usedfpregs = 0;
	force_used = 0;
	dumpfcons();
#undef PARAMSAVESIZE
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*
 * build an interior tree node
 */
NODE *
build_in( op, type, left, right )
	int op;		/* operator     */
	TWORD type;	/* result type  */
	NODE * left;	/* left subtree */
	NODE * right;	/* right subtree*/
{
	register NODE *q = talloc();
	q->in.op = op;
	q->in.type = type;
	q->in.rall = NOPREF;
	q->in.name = "";
	q->in.left = left;
	q->in.right = right;
	return q;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*
 * build a terminal tree node
 */
NODE *
build_tn( op, type, name, lval, rval )
	int op;		/* operator     */
	TWORD type;	/* result type  */
	char * name;	/* symbolic part of constant */
	CONSZ  lval;	/* left part of value */
	int    rval;	/* right part of value*/
{
	register NODE *q = talloc();
	q->tn.op = op;
	q->tn.type = type;
	q->tn.rall = NOPREF;
	q->tn.name = name;
	q->tn.lval = lval;
	q->tn.rval = rval;
	return q;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

struct hoptab { int opmask; char * opstring; } ioptab[] = {

	ASG PLUS, "add",
	    PLUS, "add",
	ASG MINUS, "sub",
	    MINUS, "sub",
	ASG OR,	"or",
	    OR,	"or",
	ASG AND, "and",
	    AND, "and",
	ASG ER,	"xor",
	    ER,	"xor",
	ASG LS,	"sl",
	    LS,	"sl",
	ASG RS,	"sr",
	    RS,	"sr",
	ASG MUL, "mul",
	    MUL, "mul",
	ASG DIV, "div",
	    DIV, "div",
	-1, ""    };

hopcode( f, o )
{
	/* output the appropriate string from the above table */

	register struct hoptab *q;

	for( q = ioptab;  q->opmask>=0; ++q ){
		if( q->opmask == o ){
			if( f == 'F') putchar('f');
			printf(q->opstring);
			return;
			}
		}
	cerror( "no hoptab for %s", opst[o] );
}

char *
rnames[] = {  /* keyed to register number tokens */
	"%g0",  "%g1",  "%g2",  "%g3",  "%g4",  "%g5",  "%g6",  "%g7",
	"%o0",  "%o1",  "%o2",  "%o3",  "%o4",  "%o5",  "%sp",  "%o7",
	"%l0",  "%l1",  "%l2",  "%l3",  "%l4",  "%l5",  "%l6",  "%l7",
	"%i0",  "%i1",  "%i2",  "%i3",  "%i4",  "%i5",  "%fp",  "%i7",
	"%f0",  "%f1",  "%f2",  "%f3",  "%f4",  "%f5",  "%f6",  "%f7",
	"%f8",  "%f9",  "%f10", "%f11", "%f12", "%f13", "%f14", "%f15",
	"%f16", "%f17", "%f18", "%f19", "%f20", "%f21", "%f22", "%f23",
	"%f24", "%f25", "%f26", "%f27", "%f28", "%f29", "%f30", "%f31",
};

tlen(p) NODE *p;
{
	switch(p->in.type) {
		case CHAR:
		case UCHAR:
			return(SZCHAR/SZCHAR);

		case SHORT:
		case USHORT:
			return(SZSHORT/SZCHAR);

		case DOUBLE:
			return(SZDOUBLE/SZCHAR);

		default:
			return(SZINT/SZCHAR);
		}
}


rmove( rt, rs, t )
	register int rt,rs;
{
	int fsrce, fdest;
	int tmpoff;
	char  *move;
	NODE tmporeg;
	static char MOVFMT[]="\t%s\t%s,%s\n";

	fsrce = isbreg(rs);
	fdest = isbreg(rt);
	if (fsrce == fdest ){
	    move = (fsrce)?"fmovs":"mov";
	    if ( szty(t) == 2 ){
		/*
		 * move two regs; beware of overlaps
		 */
		if (rs+1 == rt) {
		    printf(MOVFMT, move, rnames[rs+1], rnames[rt+1]);
		    printf(MOVFMT, move, rnames[rs], rnames[rt]);
		} else {
		    printf(MOVFMT, move, rnames[rs], rnames[rt]);
		    printf(MOVFMT, move, rnames[rs+1], rnames[rt+1]);
		}
		markused(rs+1);
		markused(rt+1);
	    } else {
		/*
		 * move one reg
		 */
		printf(MOVFMT, move, rnames[rs], rnames[rt]);
	    }
	} else {
	    mktempnode( &tmporeg, 1);
	    print_str_str("	st	", rnames[rs]);
	    print_str(",");
	    oregput( &tmporeg );
	    print_str("\n	ld	");
	    oregput( &tmporeg );
	    print_str_str_nl(",", rnames[rt] );
	    if ( t==DOUBLE ){
	    	print_str_str("	st	", rnames[rs+1]);
		print_str(",");
	    	oregput( &tmporeg );
	    	print_str("\n	ld	");
	    	oregput( &tmporeg );
	    	print_str_str_nl(",", rnames[rt+1] );
		markused(rs+1);
		markused(rt+1);
	    } 
	}
	markused(rs);
	markused(rt);
}

struct respref
respref[] = {
	/* if he asks for...	he'll really accept */
	INTAREG,		INTAREG,
	INAREG|INTAREG,		INAREG|INTAREG,
	INTBREG,		INTBREG,
	INBREG|INTBREG,		INBREG|INTBREG,
	INAREG|INTAREG|INBREG|INTBREG,	SZERO,
	INAREG|INTAREG|INBREG|INTBREG,	SOREG|STARREG|STARNM|SNAME|SCON,
	INTEMP,			INTEMP,
	FORARG,			FORARG,
	INAREG|INTAREG|INBREG|INTBREG,
				INAREG|INTAREG|INBREG|INTBREG,
	0,	0 };

setsto( p, c )
	NODE *p;
	/* cookie  c; */
{
	register NODE *q;
	int regno;
	int r = NOPREF;
	switch(p->in.op){
	case STARG:
	    /* you cannot store one of these */
	    p = p->in.left;
	    break;
	}
	switch (p->in.type){
	case FLOAT:
	case DOUBLE:
	case STRTY:
		break;
	default:
		if (c == INTEMP ){
			if ( stotree != NIL  &&  stocook == INAREG ){
			    /* try to conserve register we've already allocated */
			    c = INAREG;
			    r = storall;
			}else if ( (regno = grab_temp_reg( p->in.type )) >= 0 ){
			    c = INAREG;
			    r = MUSTDO | regno;
			}
		}
	}
	stotree = p;
	stocook = c;
	storall = r;
}

szty(t)
	TWORD t;
{
	/* size, in A-or-D registers, needed to hold thing of type t */
	return(t==DOUBLE ? 2 : 1);
}

/*
 * Rewrite a suitably aligned and sized FLD node in an
 * rvalue context, as SCONV over a small memory operand.
 * Called from ffld().
 *
 * The type of the FLD operand may have been converted to UCHAR
 * or USHORT if feasible, with the offset adjusted accordingly,
 * by a subcase of optim2().
 *
 * Optimization of a FLD in an lvalue context is done
 * by special case matching in special() and table.c.
 */
rewfld( p ) NODE *p; 
{
	TWORD t;
	OFFSZ v;
	NODE *q;
	int fsize, fshf;

	/*
	 * Bit fields have been "optimized" to allow packing them in
	 * rather tightly with non-field elements. Here we'll weaken
	 * the underlying type, to allow for shorter, and less-well-
	 * aligned accesses.
	 */
	q = p->in.left;
	/* the type of p is unsigned. the type of q determines the access */
	t = q->in.type;
	v = p->tn.rval; /* as in tshape */
	fsize = UPKFSZ(v);
	fshf = UPKFOFF(v);/* different from tshape */
	/*
	 * look for synchronized fields: those with a 0 shift count
	 * and a field size the same as the access type. These can be
	 * rewritten as an SCONV over a cheap memory operand, dispensing
	 * with the FLD operator entirely.
	 */
	if ( fshf == 0 && fsize == tlen(q)*SZCHAR ){
		p->in.op = SCONV;
		p->in.type = UNSIGNED;
		p->in.right = NULL;
		return(0);
	}
	return(1);
}

callreg(p) NODE *p; 
{
	return(ISFLOATING(p->in.type)? FR0 : RO0);
}

shltype( o, p )
	register o;
	NODE *p; 
{
	return( o== REG || o == NAME || o == ICON || o == OREG || o == FCON
		|| ( o==UNARY MUL && shumul(p->in.left)) );
}

flshape( p ) register NODE *p; 
{
	/* tshape( p, MEMADDR ); */
	return (tshape( p, SOREG|STARNM ) );
}

shtemp( p ) register NODE *p; 
{
	if( p->in.op == UNARY MUL ) p = p->in.left;
	if( p->in.op == REG )
		return( !istreg( p->tn.rval ) );
	if( p->in.op == OREG && !R2TEST(p->tn.rval) )
		return( !istreg( p->tn.rval ) );
	return( p->in.op == NAME || p->in.op == ICON );
}

spsz( t, v ) TWORD t; CONSZ v; 
{

	/* is v the size to increment something of type t */

	if( !ISPTR(t) ) return( 0 );
	t = DECREF(t);

	if( ISPTR(t) ) return( v == (SZPOINT/SZCHAR) );

	switch( t ){

	case UCHAR:
	case CHAR:
		return( v == 1 );

	case SHORT:
	case USHORT:
		return( v == (SZSHORT/SZCHAR) );

	case INT:
	case UNSIGNED:
	case FLOAT:
		return( v == (SZINT/SZCHAR) );

	case DOUBLE:
		return( v == (SZDOUBLE/SZCHAR) );
		}

	return( 0 );
}


shumul( p ) register NODE *p; 
{
	register o;
	extern int xdebug;

	if (xdebug) {
	     printf("\nshumul:op=%d, ", p->in.op);
	     switch (optype(p->in.op)){
	    case BITYPE:
			printf( " rop=%d, rname=%s, rlval=%D", 
			    p->in.right->in.op, p->in.right->in.name,  
			    p->in.right->tn.lval);
			/* fall through */
	    default:
			printf( "lop=%d, plty=%d ", 
			    p->in.left->in.op, p->in.left->in.type );
	    case LTYPE: ; /* do nothing */
	    }
	    putchar('\n');
	}


	o = p->in.op;
	switch (o){
	case REG:	return STARNM;
	}


	return( 0 );
}

adrcon( val ) CONSZ val; 
{
	printf( CONFMT, val );
}

conput( p ) register NODE *p; 
{
	switch( p->in.op ){
	case INIT:
		p = p->in.left;
		fcon( p, F_immed );
		return;
	case FCON:
		fcon( p, F_highorder );
		return;

	case ICON:
	case NAME:
		acon( p );
		return;

	case REG:
		print_str(rnames[p->tn.rval] );
		markused(p->tn.rval);
		return;

	default:
		cerror( "illegal conput" );
	}
}

oregput(p)
	register NODE *p;
{
	int base = p->tn.rval;
	int val;

	base = p->tn.rval;
	if (R2TEST(base)){
		val = R2UPK2(base);
		base = R2UPK1(base);
		markused(base);
		markused(val);
		printf("[%s+%s]", rnames[base], rnames[val]);
	} else {
		markused(base);
		putchar( '[' );
		print_str( rnames[base] );
		if( p->tn.lval != 0 || p->in.name[0] != '\0' ) { 
			putchar('+'); 
			acon( p ); 
		}
		putchar(']'); 
	} /* if */
} /* oregput */

/*
 * output the address of the second word in the
 * pair pointed to by p (for DOUBLEs)
 */
void
upput( p ) register NODE *p; 
{
	CONSZ save;
	int r;

	if( p->in.op == FLD ){
		p = p->in.left;
	}

	save = p->tn.lval;
	switch( p->in.op ){

	case NAME:
		p->tn.lval += SZINT/SZCHAR;
		acon( p );
		break;

	case FCON:
		fcon( p, F_loworder );
		break;

	case ICON:
		/* addressable value of the constant */
		p->tn.lval &= BITMASK(SZINT);
		acon( p );
		break;

	case REG:
		r = p->tn.rval+1;
		print_str(rnames[r] );
		markused(r);
		break;

	case OREG:
		p->tn.lval += SZINT/SZCHAR;
		oregput(p);
		break;

	case UNARY MUL:
		/*
		 * if the lower address is   [g1],
		 * then the upper address is [g1+4]
		 */
		putchar('[');
		print_str(rnames[r=p->tn.rval]);
		print_str("+4]");
		markused(r);
		break;

	default:
		cerror( "illegal upper address" );
		break;

		}
	p->tn.lval = save;
}

void
adrput( p ) register NODE *p; 
{
	/* the 68k code saves lval and restores after the switch */
	register int r;
	/* output an address, with offsets, from p */

	if( p->in.op == FLD ){
	    p = p->in.left;
	}
	switch( p->in.op ){

	case NAME:
	case ICON:
		putchar('[');
		acon( p );
		putchar(']');
		return;

	case FCON:
		fcon( p, F_highorder );
		return;

	case REG:
		r = p->tn.rval;
		print_str(rnames[r] );
		markused(r);
		return;

	case OREG:
		oregput(p);
		break;

	case UNARY MUL:
		/* STARNM or STARREG found */
		if( tshape(p, STARNM) ) {
			putchar('[');
			adrput( p->in.left);
			putchar(']');
		} else {
			cerror("illegal adrput");
		}
		return;

	default:
		cerror( "illegal address" );
		return;

	}

}

/* print out a constant */
acon( p ) register NODE *p; 
{ 
	if( p->in.name[0] == '\0' ){
		print_x(p->tn.lval);
	} else {
		print_str(p->in.name );
		if( p->tn.lval != 0 ){
			putchar('+');
			print_x(p->tn.lval );
		}
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * fcon(p, form):  output a floating point constant in one of several forms
 */

int fgetlab();

#ifdef DOTFLOAT
static char floatfmt[] = "0f%.7e";
static char doublefmt[] = "0f%.15e";
#endif DOTFLOAT

static void
fcon( p, form )
	register NODE *p;
	Floatform form;
{
	float x;
	long *lp;

	switch(form) {
	case F_hexfloat:
		/* put out single precision representation in hex */
		x = p->fpn.dval;
		lp = (long*)&x;
		printf("0x%x", *lp);
		return;
	case F_highorder:
		/*
		 * put out a reference to the highorder word of a
		 * double constant from the pool
		 */
		printf("L%d", fgetlab(p));
		return;
	case F_loworder:
		/*
		 * put out a reference to the loworder word of a
		 * double constant from the pool
		 */
		printf("L%d+4", fgetlab(p));
		return;
	case F_immed:
		/*
		 * put out the constant in e-floating point format;
		 * for INIT nodes and coprocessor instructions.
		 */
#ifdef DOTFLOAT
		if (p->in.type == FLOAT) {
			printf(floatfmt, p->fpn.dval);
		} else {
			printf(doublefmt, p->fpn.dval);
		}
#else  !DOTFLOAT
		if (p->in.type == FLOAT) {
			float x = p->fpn.dval;
			lp = (long*)&x;
			printf("0x%x", lp[0]);
		} else {
			lp = (long*)&p->fpn.dval;
			printf("0x%x,0x%x", lp[0], lp[1]);
		}
#endif !DOTFLOAT
		return;
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * floating point constant pool -- a data structure that
 * associates floating point constants with labels
 */

struct floatcon {
	int conlab;
	TWORD contype;
	double conval;
	struct floatcon *left, *right;
};

static struct floatcon *fconpool;	/* the search tree */
static struct floatcon *dconpool;	/* the double-precision search tree */
static struct floatcon *newfloatcon();	/* floatcon storage allocator */
static int fconlookup();		/* search routine */

/*
 * search for, and create if necessary,
 * a label in the floating point constant pool
 */

int
fgetlab(p)
	NODE *p;
{
	if ( p->in.type == FLOAT )
	    return(fconlookup(&fconpool, p));
	else
	    return(fconlookup(&dconpool, p));
}

/*
 * double-word integral comparison of two doubles.
 * We do this in order to distinguish 0.0 from -0.0.
 */
static int
dbl_equal(a,b)
	int *a, *b;
{
	return (((a)[0] == (b)[0]) && ((a)[1] == (b)[1]));
}

static int
fconlookup(fpp, p)
	struct floatcon **fpp;
	NODE *p;
{
	struct floatcon *fp = *fpp;
	if (fp == NULL) {
		*fpp = fp = newfloatcon();
		fp->conval = p->fpn.dval;
		fp->contype = p->fpn.type;
		fp->conlab = getlab();
		fp->left = NULL;
		fp->right = NULL;
		return(fp->conlab);
	} else if (dbl_equal((int*)&fp->conval, (int*)&p->fpn.dval)) {
		return(fp->conlab);
	} else if (fp->conval < p->fpn.dval) {
		return(fconlookup(&fp->left, p));
	} else {
		return(fconlookup(&fp->right,p));
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * storage allocation for the floating point constant pool
 */

#define FCONSEGSIZ 256
struct fconseg {
	struct fconseg  *nextseg;
	struct floatcon *nextcon;
	struct floatcon storage[FCONSEGSIZ];
};

static struct fconseg *curseg = NULL;

/*
 * allocate storage for a
 * labeled floating point constant
 */
static struct floatcon *
newfloatcon()
{
	struct fconseg *newseg;

	if (curseg == NULL
	    || curseg->nextcon == curseg->storage+FCONSEGSIZ) {
		newseg = (struct fconseg*)malloc(sizeof(struct fconseg));
		newseg->nextseg = curseg;
		curseg = newseg;
		curseg->nextcon = curseg->storage;
	}
	return(curseg->nextcon++);
}

/*
 * emit the floating point constant
 * pool to the output stream
 */
static void
dumpfcons()
{
	register struct floatcon *fp,*nextfree;
	struct fconseg *segp, *temp;
	long *lp;

	segp = curseg;
	if (segp != NULL)
		puts("	.seg	\"data\"");
	while(segp != NULL) {
		nextfree = segp->nextcon;
		for (fp = segp->storage; fp < nextfree; fp++) {
#ifdef DOTFLOAT
			if (fp->contype == FLOAT) {
				printf("	.align	%d\n", ALFLOAT/SZCHAR);
				printf("L%d:	.single	", fp->conlab);
				printf(floatfmt, fp->conval);
			} else {
				printf("	.align	%d\n", ALDOUBLE/SZCHAR);
				printf("L%d:	.double	", fp->conlab);
				printf(doublefmt, fp->conval);
			}
#else !DOTFLOAT
			if (fp->contype == FLOAT) {
				float x = fp->conval;
				lp = (long*)&x;
				printf("	.align	%d\n", ALFLOAT/SZCHAR);
				printf("L%d:	.word	0x%lx",
					fp->conlab, lp[0]);
			} else {
				lp = (long*)&fp->conval;
				printf("	.align	%d\n", ALDOUBLE/SZCHAR);
				printf("L%d:	.word	0x%lx,0x%lx",
					fp->conlab, lp[0], lp[1]);
			}
#endif !DOTFLOAT
			putchar('\n');
		}
		temp = segp->nextseg;
		free(segp);
		segp = temp;
	}
	curseg = NULL;
	fconpool = NULL;
	dconpool = NULL;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

nextcook( p, cookie ) 
	NODE *p; 
{
	/* we have failed to match p with cookie; try another */
	if ( cookie == FORREW ) return( 0 );  /* hopeless! */
	if ( !(cookie&OPEREG( p->in.type )  ))
		return( OPEREG( p->in.type ) );
	return ( FORREW );
}

lastchance( p, cook ) NODE *p; 
{
	if ((cook == SCC || cook & (INBREG|INTBREG)) && !tshape(p,OPEREG(p->in.type)))
		return OPEREG(p->in.type);
	/* forget it! */
	return(0);
}


NODE * addroreg(l)
/*
 * OREG was built in clocal()
 * for an auto or formal parameter
 * now its address is being taken
 * local code must unwind it
 * back to PLUS/MINUS REG ICON
 * according to local conventions
 */
{
	cerror("address of OREG taken");
}




/* return 1 if node is a SCONV from short to int */
isconv( p, t1, t2 )
    register NODE *p;
    TWORD t1, t2;
{
    register v;
    if ( p->in.op==SCONV && (p->in.type==INT || p->in.type==UNSIGNED) && 
	((v=p->in.left->in.type)==t1 || v == t2))
	      return( 1 );
	return( 0 );
}

static int
ispow2( i ) int i;
{
    return ( i & (i-1)) == 0;
}

void
makecall ( p )
    register NODE *p;
{
    char * name;
    struct hardop { int opname, isasg; char * subname };
    static struct hardop hardops[] = {
	MUL,	0,	".mul",
	ASG MUL,1,	".mul",
	DIV,	0,	".div",
	ASG DIV,1,	".div",
	MOD,	0,	".rem",
	ASG MOD,1,	".rem",
	-1
    };
    static struct hardop uops[] = {
	MUL,	0,	".umul",
	ASG MUL,1,	".umul",
	DIV,	0,	".udiv",
	ASG DIV,1,	".udiv",
	MOD,	0,	".urem",
	ASG MOD,1,	".urem",
	-1
    };
    struct fortrannode { int opname; char * singlename, *doublename; } *fptr;
    static struct fortrannode fortrannode[] = {
    	FCOS,	"_Fcos",	"_cos",
    	FSIN,	"_Fsin",	"_sin",
    	FTAN,	"_Ftan",	"_tan",
    	FACOS,	"_Facos",	"_acos",
    	FASIN,	"_Fasin",	"_asin",
    	FATAN,	"_Fatan",	"_atan",
    	FCOSH,	"_Fcosh",	"_cosh",
    	FSINH,	"_Fsinh",	"_sinh",
    	FTANH,	"_Ftanh",	"_tanh",
    	FEXP,	"_Fexp",	"_exp",
    	FLOGN,	"_Flog",	"_log",
    	FLOG10,	"_Flog10",	"_log10",
    	FLOG2,	"_Flog2",	"_log2",
#ifndef FSQRT_OP
    	FSQRT,	"_Fsqrt",	"_sqrt",
#endif
    	F10TOX,	"_Fexp10",	"_exp10",
    	F2TOX,	"_Fexp2",	"_exp2",
    	FAINT,	"_Faint",	"_aint",
    	FANINT,	"_Fanint",	"_anint",
    	FNINT,	"_Fnint",	"_nint",
    	/* BAD 
	    FABS,	"_Fabss_",	"_fabs",
	    FSQR,	"_Fsqrs_",	"_fsqr",
	*/
    	0,	0,		0
    };
    register struct hardop * h;
    register op;
    register NODE *q;
    TWORD t;
    op = p->in.op;
    t = p->in.type;
    if (op == SCONV){
	if (p->in.left->tn.type == DOUBLE)
		name = "__dtou";
	else
		name = "__ftou";
    } else if (dope[op] & LIBRFLG){
    	/* fortran builtin */
    	for ( fptr= &fortrannode[0]; fptr->opname != 0; fptr++)
    		if ( fptr->opname == op ) break;
    	if (fptr->opname == 0 ){
    		fprintf( stderr, "HEY!");
    		epr(p);
    		cerror("couldn't find fortran name");
    	}
	/* ONLY UNARY TYPES HERE!!! */
    	name = (p->in.left->in.type == FLOAT)?fptr->singlename : fptr->doublename;
    } else {
	if (ISUNSIGNED(p->in.left->tn.type)) {
		h = uops;
		t = UNSIGNED;
	} else {
		h = hardops;
		t = INT;
	}
    	while (h->opname!= op && h->opname != -1)
	    h++;
    	if (h->opname == -1)
	    cerror("op not in hard table\n");
    	if (h->isasg){
	    p = rw_asgop( p );
    	}
    	name = h->subname;
    }
    /*
     * If the type of the routine (t) matches the type of the
     * original expression, we rewrite the original expression
     * in place as a call.  Otherwise, we need to include a
     * conversion to get the type right.
     */
    if (p->in.type == t) {
	/*
	 * type matches, rewrite as CALL
	 */
	if (optype(op) == BITYPE ){
	    /* build comma op for args to function */
	    p->in.right = build_in( CM, INT, p->in.left, p->in.right );
	} else {
	    p->in.right = p->in.left;
	}
	p->in.op = CALL;
	/* put function name in left node of call */
	p->in.left =  build_tn( ICON, INCREF( FTN + t ), name, 0, 0);
    } else {
	/*
	 * type doesn't match, rewrite as SCONV over CALL
	 */
	q = build_in( CALL, t, NULL, NULL );
	if (optype(op) == BITYPE ){
	    /* build comma op for args to function */
	    q->in.right = build_in( CM, INT, p->in.left, p->in.right );
	} else {
	    q->in.right = p->in.left;
	}
	/* put function name in left node of call */
	q->in.left =  build_tn( ICON, INCREF( FTN + t ), name, 0, 0);
	p->in.op = SCONV;
	p->in.left = q;
	p->in.right = NULL;
    }
}

static 
zappost(p) NODE *p; {
    /* look for ++ and -- operators and remove them */

    register o, ty;
    register NODE *q;
    o = p->in.op;
    ty = optype( o );

    switch( o ){
    case INCR:
    case DECR:
	q = p->in.left;
	p->in.right->in.op = FREE;  /* zap constant */
	ncopy( p, q );
	q->in.op = FREE;
	return;
    }

    if( ty == BITYPE ) zappost( p->in.right );
    if( ty != LTYPE ) zappost( p->in.left );
}

static 
fixpre(p) NODE *p; {
    register o, ty;
    o = p->in.op;
    ty = optype( o );

    switch( o ){
    case ASG PLUS:
	p->in.op = PLUS;
	break;
    case ASG MINUS:
	p->in.op = MINUS;
	break;
    }

    if( ty == BITYPE ) fixpre( p->in.right );
    if( ty != LTYPE ) fixpre( p->in.left );
}

fixasgops( p )
    register NODE *p;
{
    NODE *q;
    q = build_in( NOASG (p->in.op), p->in.type, tcopy(p->in.left), p->in.right);
    p->in.op = ASSIGN;
    p->in.right = q;
    zappost(q->in.left); /* remove post-INCR(DECR) from new node */
    fixpre(q->in.left);	/* change pre-INCR(DECR) to +/-	*/
}

static void
fix_builtin( p, n )
    register NODE *p;
    int n;
{
# define ALIGNMENT (ALSTACK/SZCHAR)
    /*
     * n keyed to table in hardops:
     * 0 -- __builtin_alloca
     */
    register NODE * x, *y, *size;
    int val;

    switch (n){
    case 0: /* __builtin_alloca */
    	    /*
    	     * have call to __builtin_alloca with 1 parameter:
    	     *    (CALL (ICON __builtin_alloca) (size) )
    	     * change to three parameters:
    	     *    (CALL (ICON __builtin_alloca) 
    	     *          (, (, (& (+ (size) (ICON z)) (ICON ~z))
    	     *                (ICON tmpname))
    	     *		   (ICON endtmpname))
    	     * where z is ALIGNMENT-1, a.k.a. 7
    	     * if size is a constant, we can do the rounding here
    	     * this permits the routine to determine if there are any
    	     * temps that may need savings (fat chance) and to do the
    	     * save. If we are very tricky, someday, we'll be able to
    	     * copy only as many temps as are USED in this expression...
    	     */
    	     
    	    size = p->in.right;
    	    if (size->in.op == ICON && size->in.name[0] == '\0'){
    	        /* if size is not unsigned, make it so */
    	        size->tn.type = UNSIGNED;
    	        /* do rounding here */
    	        size->tn.lval = (size->tn.lval + (ALIGNMENT-1) ) & ~(ALIGNMENT-1);
    	    } else {
    	        /* if size is not unsigned, make it so */
    	        if (size->in.type != UNSIGNED){
    	    	    size = build_in(SCONV, UNSIGNED, size, 0);
    	    	}
    	    	/* round up to 0 mod ALIGNMENT */
    	    	size = build_in( AND, UNSIGNED,
    	    		     build_in( PLUS, UNSIGNED,
    	    			   size,
    	    			   build_tn( ICON, UNSIGNED, "", ALIGNMENT-1, 0)
    	    		     ),
    	    		     build_tn( ICON, UNSIGNED, "", ~(ALIGNMENT-1), 0)
    	    		);
    	    }
	    p->in.right = build_in( CM, INT, 
	    		      build_in( CM, INT, 
	    		          size,
	    		          build_tn( ICON, UNSIGNED, tmpname, 0, 0)
	    		      ),
	    		      build_tn( ICON, UNSIGNED, endtmpname, 0, 0 )
	    		  );
    	    break;
    }
}

static void
hardops( p )
    register NODE *p;
{
    register NODE *val;
    register int i;
    static char *builtin_names[] = {
	"___builtin_alloca",
    };
again:
    switch (p->in.op ){
    case MUL:
    case ASG MUL:
	if (!ISFLOATING(p->in.type)){
	    val = p->in.right;
	    if ( chk_ovfl || val->tn.op != ICON || !can_conmul( val->tn.lval)){
		makecall( p );
	    }
	}
	break;
    case DIV:
    case ASG DIV:
    case MOD:
    case ASG MOD:
	if (!ISFLOATING(p->in.type)){
	    val = p->in.right;
	    if ( val->tn.op != ICON
	      || val->tn.lval <= 0 
	      || !ispow2(val->tn.lval)){
		makecall( p );
	    }
	}
	break;
    case CALL:
    case STCALL:
	val = p->in.left;
	if (val->tn.op == ICON && val->tn.lval == 0){
	    for (i = 0 ; i < sizeof builtin_names / sizeof builtin_names[0]; i++)
		if (strcmp( val->in.name, builtin_names[i]) == 0 ){
		    fix_builtin( p, i );
		    break;
		}
	}
	break;
    case SCONV:
	/* convert from float/double to unsigned */
	if (ISUNSIGNED(p->in.type) && ISFLOATING(p->in.left->tn.type)) {
	    if (p->in.type == UNSIGNED) {
		makecall( p );
	    } else {
		/*
		 * small unsigned int; convert to INT, then
		 * truncate the result (the root SCONV {uchar|ushort} op 
		 * takes care of this)
		 */
		p->in.left = build_in(SCONV, INT, p->in.left, NULL);
	    }
	}
	break;
    default:
	if (dope[p->in.op] & LIBRFLG){
	    /* FORTRAN builtin */
	    makecall( p );
	}
	break;
    }
    switch (optype(p->in.op)){
    case BITYPE:
	hardops( p->in.right );
	/* fall through */
    case UTYPE:
	/* tail recursion */
	p = p->in.left;
	goto again;
    }
}

extern int dregmap;
extern int fregmap;

myreader(p) register NODE *p; 
{
#ifdef FORT
	void unoptim2();
	unoptim2(p);
#endif FORT
	if (picflag) {
		picode(p);
	}
	optim2(p);
	if (malign_flag)
		malign_ld_st(p);
	hardops(p);
	canon( p );		/* expands r-vals for fields */
	toff = 0;  /* stack offset swindle */

	dregmap = maxtreg;
	fregmap = maxfreg;

}


special( p, shape ) register NODE *p; 
{
	int fwidth, foffset;

	/* special shape matching routine */

	switch (shape){

	case SPEC_PVT:
	    /* right subtree of x + y*z ; an idiom from matrix computations */
	    if (p->in.op != MUL) return 0;
	    if (tshape(p->in.left,SCON|SAREG|STAREG|SBREG|SOREG|SNAME|STARREG)){
		return tshape(p->in.right,
		    SCON|SAREG|STAREG|SBREG|SOREG|SNAME|STARREG);
	    }
	    return 0;

	case SNONPOW2:
	    /*
	     * constant, NOT a power of 2.  This is a kludge to enable
	     * us to match a template for ASG MOD on the 68020.  The
	     * semantics of the DIVSLL instruction make it impossible
	     * to return a remainder in the lhs of <reg> %= <ea>, unless
	     * an extra instruction is generated. Returning the result
	     * in RESC1 makes the extra instruction unnecessary.
	     */
	    return( p->in.op == ICON && (p->tn.lval & p->tn.lval-1) );

	case SSOURCE:
	    /*
	     * something that looks like a SunRise source operand: 
	     * either a cpu register or a small constant
	     */
	    if (p->tn.op == REG && p->tn.rval <= RI7) return 1;
	    shape = SICON;
	    break;
	case SSNAME:
	    /*
	     * name of a small object -- we can access it directly
	     */
	    return (p->tn.op == NAME && p->tn.rval != 0 );
	case SDOUBLE:
	    /*
	     * name of a double aligned thing -- we can use ldd/std instructions.
	     */
	    if (dalign_flag)
		return (p->tn.op == NAME && p->tn.lval%(ALDOUBLE/SZCHAR) == 0 );

	    return 0;

	case SDOUBLEO:
	    /*
	     * pointer to a double aligned thing -- we can use ldd/std
	     * instructions.
	     */
	    if (dalign_flag)
	    	return ( p->in.op == OREG && p->tn.lval%(ALDOUBLE/SZCHAR) == 0);

	    return ( p->in.op == OREG 
		&& (p->tn.rval == AUTOREG || p->tn.rval == SPREG)
		&& p->tn.lval%(ALDOUBLE/SZCHAR) == 0 );

	case SXCON:
	    /*
	     * name of a small object -- we can access it directly
	     */
	    return (p->tn.op == ICON && p->tn.rval != 0 );
	case SFCON:
	    /*
	     * an FCON node: why this has to be a special I don't know
	     */
	    return( p->tn.op == FCON );
	case SFLD8_16:
	    /*
	     * bit field with {byte|short} size and alignment
	     */
	    if (p->in.op == FLD && flshape(p->in.left)) {
		fwidth = UPKFSZ(p->tn.rval);
		foffset = UPKFOFF(p->tn.rval);
		if ( fwidth == SZCHAR && foffset == 0
		  && p->in.left->in.type == UCHAR ) {
		    return 1;
		}
		if ( fwidth == SZSHORT && foffset == 0
		  && p->in.left->in.type == USHORT ) {
		    return 1;
		}
	    }
	    return 0;
	case SCOMPLR:
	    /* COMPL over AREG */
	    if (p->in.op == COMPL && p->in.left->in.op == REG
	      && isareg(p->in.left->tn.rval))
		return 1;
	    return 0;
	case SAREGPAIR:
	    /*
	     * Even-odd AREG pair.  Note that odd-even pairs may
	     * occur in input trees, even though allo() guarantees
	     * even-odd pairs when the NAPAIR attribute is specified.
	     */
	    if (p->in.op == REG && isareg(p->tn.rval) && !(p->tn.rval&1))
		return 1;
	    return 0;
	}

	switch (p->tn.op){
	case NAME:
	case ICON:
		break;
	default:
		return 0;
	}
	/* hack: tmpname + any offset is guaranteed to be small */
	if(p->in.name != 0 && p->in.name != tmpname && p->in.name[0] != '\0' ) return (0);
	switch( shape ) {
	case SICON:
		return( p->tn.op == ICON && IN_IMMEDIATE_RANGE( p->tn.lval) );
	case SINAME:
		return( p->tn.op == NAME && IN_IMMEDIATE_RANGE( p->tn.lval) );
	case SONES:
		return( p->tn.op == ICON && (p->tn.lval == -1 || p->tn.lval == (1<<fldsz)-1 ));
	default:
		cerror( "bad special shape" );
	}

	return( 0 );
}


#ifdef FORT
void
unoptim2( p ) register NODE *p;
{
    /* FORTRAN thinks we can do double operations on float operands */
    NODE * double_conv();
    switch( optype(p->in.op)){
    case BITYPE:
	unoptim2(p->in.left);
	unoptim2(p->in.right);
	if (p->in.type == DOUBLE){
	    switch (p->in.op) {
	    case QUEST:
	    case CM:
	    case COMOP:
	    case CALL:
		    return;
	    }
	    if (p->in.right->in.type != DOUBLE)
		p->in.right = double_conv(p->in.right);
	    if (p->in.left->in.type != DOUBLE)
		p->in.left = double_conv(p->in.left);
	    return;
	}
	if ( logop(p->in.op) ){
	    if (p->in.left->in.type==DOUBLE && p->in.right->in.type != DOUBLE)
		p->in.right = double_conv(p->in.right);
	    if (p->in.right->in.type==DOUBLE && p->in.left->in.type != DOUBLE)
		p->in.left = double_conv(p->in.left);
	}
	return;
    case UTYPE:
	unoptim2(p->in.left);
	if (p->in.type == DOUBLE ){
	    switch( p->in.op ){
	    case UNARY MUL:
	    case UNARY CALL:
	    case SCONV:
		return;
	    }
	    if (p->in.left->in.type != DOUBLE)
		p->in.left = double_conv(p->in.left);
	}
	return;
    case LTYPE: return;
    }
}
#endif FORT


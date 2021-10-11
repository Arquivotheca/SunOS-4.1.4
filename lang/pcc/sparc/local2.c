#ifndef lint
static	char sccsid[] = "@(#)local2.c 1.1 94/10/31 Copyr 1989 Sun Micro";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include "cpass2.h"
#include "ctype.h"
#include "pcc_sw.h"

#include <sun4/trap.h>

extern int chk_ovfl;

/* 
 * local2.c
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
# define BITMASK(n) ((1L<<(n))-1)


void stmove();
void incraddr();

zzzcode( p, c, cookie ) NODE *p; 
{

	register m,temp;
	NODE *s;
	static TWORD save_type;
	static NODE *save_source;
	extern int nparams;

	switch( c ){

	case 'a': /* number of outgoing arguments in registers this call */
		print_d(nparams);
		return;

	case 'c':
		if (cookie&FORCC)
			print_str("cc");
		return;

	case 'C': /* check overflow */
		if (chk_ovfl)
			print_str("cc");
		return;

	case 'D':
		/* rewrite DOUBLE double-OREG as single-OREG */
		reduce_oreg2( p );
		return;
	
	case 'f': /* complicated floating-point operation */
		floatcode( p, cookie );
		return;

	case 'F': /* oh boy, bit fields again... */
		assign_field( p, cookie );
		return;

	case 'B':
		if (p->in.op == FLD)
			p = p->in.left;
		m = p->in.type;
		switch(m){
		case UCHAR:
		case USHORT:
		    c = 'u';
		    break;
		case CHAR:
		case SHORT:
		    c = 's';
		    break;
		default:
		    return;
		}
		putchar(c);
		goto suffix;

	case 'b':
		if (p->in.op == FLD)
			p = p->in.left;
		m = p->in.type;		/* fall into suffix: */

	suffix:	
		switch(m) {
		case CHAR:
		case UCHAR:
			c = 'b';
			break;
		case SHORT:
		case USHORT:
			c = 'h';
			break;
		default:
			c = ' ';
			return;
		}
		putchar(c);
		return;

	case '.':
		/*
		 * print type character for the root node
		 */
		switch(p->in.type) {
		case CHAR:
			c = 'b';
			break;
		case SHORT:
			c = 'w';
			break;
		case FLOAT:
			c = 's';
			break;
		case DOUBLE:
			c = 'd';
			break;
		case UCHAR:
		case USHORT:
			/*
			 * the 68881 sign-extends integer operands
			 */
			cerror("compiler botched unsigned operand");
			/*NOTREACHED*/
		default:
			c = 'l';
			break;
		}
		putchar(c);
		return;
		
	case 'N':  /* logical ops, turned into 0-1 */
		/* use register given by RESC1 */
		branch(m=getlab());
		/* FALL THROUGH */

	make_boolean:
		deflab( p->bn.label );
		temp = getlr( p, '1' )->tn.rval;
		printf( "	mov	0,%s\n", rnames[temp]);
		markused(temp);
		deflab( m );
		return;

	case 'H':
		cbgen(p->in.op, p->in.left->in.type, p->bn.label,
			p->bn.reversed, FCCODES);
		return;

	case 'I':
		cbgen(p->in.op, p->in.left->in.type, p->bn.label,
			p->bn.reversed, CCODES);
		return;

	case '~':
		/* complimented CR */
		p->in.right->tn.lval = ~p->in.right->tn.lval;
		conput( getlr( p, 'R' ) );
		p->in.right->tn.lval = ~p->in.right->tn.lval;
		return;

	case 'M':
		/* negated CR */
		p->in.right->tn.lval = -p->in.right->tn.lval;
	case 'O':
		conput( getlr( p, 'R' ) );
		p->in.right->tn.lval = -p->in.right->tn.lval;
		return;

	case 'K': /* clipping of assignment to small register objects */
		klipit(save_type, save_source, p);
		return;

	case 'k': /* collect source for clipping operation */
		save_source = p;
		return;

	case 't': /* collect type for clipping operation */
		if (p->in.op == FLD) {
			p = p->in.left;
		}
		save_type = p->in.type;
		return;

	case 'T':
		/* Truncate longs for type conversions:
		    INT|UNSIGNED -> CHAR|UCHAR|SHORT|USHORT
		   increment offset to second word */

		m = p->in.type;
		p = p->in.left;
		switch( p->in.op ){
		case NAME:
		case OREG:
			if (p->in.type==SHORT || p->in.type==USHORT)
			  p->tn.lval += (m==CHAR || m==UCHAR) ? 1 : 0;
			else p->tn.lval += (m==CHAR || m==UCHAR) ? 3 : 2;
			return;
		case REG:
			return;
		default:
			cerror( "Illegal ZT type conversion" );
			return;

			}


	case 'W':	/* structure size */
		if( p->in.op == STASG )
			print_d(p->stn.stsize);
		else	cerror( "Not a structure" );
		return;

	case 'S':  /* structure assignment, with lots of registers */
		stmove( p, cookie, 1 );
		return;

	case 's':  /* structure assignment, without as many registers */
		stmove( p, cookie, 0 );
		return;

	case 'm': /* multiplication by a constant */
		conmul( p, cookie );
		return;

	case 'd': /* division by a constant */
		condiv( p, cookie );
		return;

	case 'r': /* remainder by a constant */
		conrem( p, cookie );
		return;

	case 'V': /* bounds test */
		bound_test( p, cookie );
		return;

	case 'v': /* generate trap instruction if check integer overflow */
		/* Is there a way to detect unsigned overflow? */
		if (chk_ovfl && (save_type == INT || save_type == LONG))
			printf("	tvs	%d\n", ST_RANGE_CHECK);
		return;

	case 'u': /* unary op over REG */
		if (optype(p->in.op) != UTYPE) {
		    cerror("unary op expected, got %s", pcc_opname(p->in.op));
		}
		p = p->in.left;
		if (p->in.op != REG) {
		    cerror("REG expected, got %s", pcc_opname(p->in.op));
		}
		adrput(p);
		return;

	case 'n': /* bit operator with negation */
		/*
		 * This has to be done here, instead of using the OI
		 * macro, because xor with negation is "xnor" instead
		 * of "xorn", and so is different from "andn" and "orn".
		 * Consistency is the hobgoblin of small minds, I guess.
		 */
		switch(p->in.op) {
		case AND:
		case ASG AND:
			printf("andn");
			break;
		case OR:
		case ASG OR:
			printf("orn");
			break;
		case ER:
		case ASG ER:
			printf("xnor");
			break;
		}
		return;

	case 'X':
		/* unshifted field mask */
		printf("0x%x", (1<<fldsz)-1);
		return;

	default:
		cerror( "illegal zzzcode" );
	}
}

static
klipit( dest_type, sourcep, destp)
    TWORD dest_type;
    NODE *sourcep, *destp;
{
    char *sname = rnames[sourcep->tn.rval];
    char *dname = rnames[destp->tn.rval];
    int kount;
    
    switch(dest_type){
    case UCHAR:
	printf("	and	%s,0xff,%s\n", sname, dname);
	return;
    case USHORT:
	printf("	sll	%s,16,%s\n	srl	%s,16,%s\n",
		sname, dname, dname, dname );
	return;
    case CHAR: kount = 24; break;
    case SHORT: kount = 16; break;
    default:
	if (sname != dname )
	    printf("	mov	%s,%s\n", sname, dname );
	return;
    }
    printf("	sll	%s,%d,%s\n	sra	%s,%d,%s\n",
	        sname, kount,dname,     dname, kount,dname );
}

void
incraddr( p , v)
	register NODE *p; 
	long v;
{
	/* bump the address p by the value v. */
	int r;
	register NODE *lp;

	if ( v == 0 ) return; /* no effect */
	if( p->in.op == FLD ){
		p = p->in.left;
	}
	switch( p->in.op ){
	case UNARY MUL:
		/* if its a REG, make this an OREG */
		lp = p->in.left;
		if (lp->tn.op == REG){
		    r = lp->tn.rval;
		    lp->tn.op = FREE;
		    p->tn.op = OREG;
		    p->tn.rval = r;
		    p->tn.lval = v;
		    p->tn.name = "";
		 } else if ((lp->in.op == PLUS || lp->in.op == MINUS)
		 && lp->in.right->in.op == ICON){
		     /* already in right configuration */
		     if (lp->in.op == PLUS) {
			 lp->in.right->tn.lval += v;
		     } else {
			 lp->in.right->tn.lval -= v;
		     }
		 } else {
		     /* must insert a + & an ICON node */
		     p->in.left = build_in( PLUS, lp->in.type,
		     		      lp,
		     		      build_tn( ICON, INT, "", v, 0)
		     		  );
		 }
		 break;
	case NAME:
	case ICON:
	case OREG:
		p->tn.lval += v;
		break;
	default:
		cerror( "illegal incraddr address" );
		break;

	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
reduce_oreg2( p )
	NODE * p;
{
	int base, index, target;
	base  = R2UPK1( p->tn.rval);
	index = R2UPK2( p->tn.rval);
	target = resc[0].tn.rval;
	/*
	 * we wish to emit an add instruction
	 * and turn this into a 1-reg OREG.
	 */
	printf("	add	%s,%s,%s\n",
		rnames[base], rnames[index], rnames[target]);
	p->tn.rval = target;
	
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/* everything you never wanted to know about condition codes */

char *
ccodes[] =	{ "e", "ne", "le", "l", "ge", "g", "leu", "lu", "geu", "gu" };

char *
fnegccodes[] =	{ "ne", "e", "ug", "uge", "ul", "ule" };

/* logical relations when compared in reverse order (cmp R,L) */

#ifdef FORT
short revrel[] = { EQ, NE, GE, GT, LE, LT, UGE, UGT, ULE, ULT };
#else
extern short revrel[];
#endif

/* negated logical relations -- integer comparisons only */

int negrel[] =	{ NE, EQ, GT, GE, LT, LE, UGT, UGE, ULT, ULE } ;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

insput( p ) register NODE *p; 
{
	printf( ccodes[p->in.op-EQ] );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * Generate conditional branches from a relational operator.
 * The exact form of the branch depends on:
 * 1. whether or not the compared operands have floating point types
 * 2. whether or not the coprocessor condition code is used
 * 3. whether the operator has been negated.
 */

cbgen( o, type, label, reversed, mode )
	int o;		/* relational op {EQ, NE, LT, LE, ...} */
	TWORD type;	/* operand type - determines opcode */
	int label;	/* destination label */
	int reversed;	/* =1 if op is to be negated */
	int mode;	/* if FCCODES, use coprocessor condition codes */
{
	char *prefix;
	char *opname;
	char **cond;

	cond = ccodes;
	if (reversed) {
		/* treat !(a < b) as (a >= b) */
		if (mode == CCODES )
		    o = negrel[o-EQ];
		else
		    cond = fnegccodes;
	}
	if (mode == CCODES )
	    prefix = "b";
	else
	    prefix = "fb";
	opname = cond[o-EQ];
	printf("	%s%s	L%d\n	nop\n", prefix, opname, label);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

p2branch(lab)
	int lab;
{
	if (lab == EXITLAB) {
		/* branch to exit */
		printf("	b	LE%d\n	nop\n", ftnno);
	} else {
		/* normal label */
		printf("	b	L%d\n	nop\n", lab);
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

p2getlab()
{
	static int crslab = 2000000;
	return( crslab++ );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

p2deflab( lab )
{
	if (lab == EXITLAB) {
		/* define exit label */
		printf("LE%d:\n", ftnno);
	} else {
		/* define normal label */
		printf("L%d:\n", lab);
	}
}



/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * generate code to return a structure-valued function result.
 * Address of source is in RO0 (use the dark side of the FORCE)
 */
p2strftn(size, align)
	OFFSZ size;
	int align;
{
	/* copy output (in o0) to caller */
	deflab( EXITLAB );
	genimmediate( size, RO1 );
	printf("	call	.stret%d\n", align);
	printf("	nop\n" );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * saveregs() is called at entry points to generate routine
 * prologues.
 */
void
saveregs()
{
	register i;
	extern int proflg;
	extern int bnzeroflg;

	/* routine prolog */
	printf("	!#PROLOGUE# 0\n");
	printf("	sethi	%%hi(LF%d),%%g1\n",ftnno);
	printf("	add	%%g1,%%lo(LF%d),%%g1\n",ftnno);
	printf("	save	%%sp,%%g1,%%sp\n" );
	if (picflag){
		printf("1:\n");
		printf("	call    2f\n");
		printf("        sethi	%%hi(__GLOBAL_OFFSET_TABLE_ - (1b-.)), %%l7\n");
		printf("2:\n");
		printf("	or	%%l7, %%lo(__GLOBAL_OFFSET_TABLE_ - (1b-.)), %%l7\n");
		printf("	add	%%l7, %%o7, %%l7\n");
	}
	/* floating point regs are saved at call sites */
	printf("	!#PROLOGUE# 1\n");
	if ( proflg ) {	/* profile code */
		i = getlab();
		printf("	set	L%d,%s\n", i, rnames[RO0]);
		printf("	call	mcount\n	nop\n");
		printf("	.reserve	L%d,%d,\"bss\",%d\n", i,
			SZINT/SZCHAR, SZINT/SZCHAR);
	}
	if ( bnzeroflg ) {
		/*
		 * initialize activation records to a regular
		 * but nonzero bit pattern; for troubleshooting
		 * uninitialized variable problems that lint can't
		 * detect.
		 */
		printf("	mov	%%sp,%s\n", rnames[RO0]);
		printf("	sub	%%fp,%%sp,%s\n", rnames[RO1]);
		printf("	call	bnzero\n");
		printf("	nop\n");
	}
}

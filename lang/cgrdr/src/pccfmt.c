#ifndef lint
static	char sccsid[] = "@(#)pccfmt.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

# define	FORT
#if TARGET == I386
extern int chk_ovfl;  
#include "mfile2.h"
#else
#include "cpass2.h"
#endif
# include "pcc_sw.h"

/*
 * This file implements the pcc side of the iropt -> pcc interface.
 * Note that integer constant folding is now done here.  We assume:
 * 1. reassociation of expressions is forbidden
 * 2. Front ends that use unsigned ints accept C's rules of
 *    unsigned arithmetic (in particular, an unsigned int divided
 *    by a signed int is unsigned, and uses unsigned division)
 * 3. host machine integral types and target machine integral types
 *    are equal in size
 */

#define nncon(p) ((p)->in.op == ICON && (p)->in.name[0] == '\0')
#define ftitle ffilename
extern char *ffilename;

/* export */

NODE	*pccfmt_icon(/*offset, string, ctype*/);
NODE	*pccfmt_fcon(/*dval, ctype*/);
NODE	*pccfmt_name(/*offset,string,ctype*/);
NODE	*pccfmt_oreg(/*regno,offset,string,ctype*/);
NODE	*pccfmt_reg(/*regno,ctype*/);
void	 pccfmt_stmt(/*file_name, line_number*/);
void	 pccfmt_doexpr(/*file_name, line_number,p*/);
NODE	*pccfmt_labelref(/*labelno, string, ctype*/);
void	 pccfmt_labeldef(/*labelno,string*/);
int	 pccfmt_goto(/*labelno*/);
NODE	*pccfmt_indirgoto(/*lp,ctype*/);
static	 int urelop(/*opno, lp, rp*/);
NODE	*pccfmt_binop(/*opno, lp, rp, ctype*/);
NODE	*pccfmt_unop(/*opno, lp, ctype*/);
int	 pccfmt_pass(/*string*/);
void	 pccfmt_lbrac(/*procno, regmask, frame_size, fregmask*/);
void	 pccfmt_rbrac(/*t, size, align, opt_level*/);
int	 pccfmt_eof(/**/);
NODE	*pccfmt_st_op(/*opno, size, align, lp, rp, ctype*/);
NODE	*pccfmt_tmp(/*nwords, ctype*/);
NODE	*pccfmt_addroftmp(/*tmp*/);
void	 pccfmt_switch(/*selector, swtab, ncases*/);
NODE	*pccfmt_fld(/*lp, bfoffset, bfwidth, ctype*/);
void	 pccfmt_chk_ovflo(/*ovflo*/);

#if TARGET == I386
NODE	*pccfmt_special_node();
#endif

#if TARGET == I386
/* register use information.  Imported from rcc cg.
extern int regused, regvar, maxarg;
 */
extern int regvar;
#endif

NODE *
pccfmt_icon(offset, string, ctype)
	long offset;
	char *string;
	TWORD ctype;
{
	NODE *p;

	p = talloc();
	p->in.op = ICON;
	p->in.type = ctype;
	p->tn.rval = 0;
	p->tn.lval = offset;
	if(string[0] != '\0') {
		p->in.name = tstr(string);
	} else {
		p->in.name = "";
	}
#if TARGET != I386
	p->in.rall = NOPREF;
	p->in.flags = 0;
#endif
	return p;
}

NODE *
pccfmt_fcon(dval, ctype)
	double dval;
	TWORD ctype;
{
	NODE *p;
	p = talloc();
	p->in.op = FCON;
	p->in.type = ctype;
#if TARGET != I386
	p->in.rall = NOPREF;
#endif
	p->fpn.dval = dval;
	return p;
}

NODE *
pccfmt_name(offset,string,ctype)
	long offset;
	char *string;
	TWORD ctype;
{
	NODE *p;

	p = talloc();
	p->in.op = NAME;
	p->in.type = ctype;
	p->tn.rval = 0;
	p->tn.lval = offset;
	p->in.name = tstr(string);
#if TARGET != I386
	p->in.rall = NOPREF; 
	p->in.flags = 0;
#endif
	return p; 
}

NODE *
pccfmt_oreg(regno,offset,string,ctype)
	int regno;
	long offset;
	char *string;
	TWORD ctype;
{
	NODE *p, *q;

	/* don't make assumptions about reachability here */
	p = pccfmt_reg(regno, INCREF(ctype));
	q = pccfmt_icon(offset, string, INT);
	p = pccfmt_binop(PLUS, p, q, INCREF(ctype));
	p = pccfmt_unop(UNARY MUL, p, ctype);
	return p; 
}

NODE *
pccfmt_reg(regno,ctype)
	int regno;
	TWORD ctype;
{
	NODE *p;

	p = talloc();
	p->in.op = REG;
	p->in.type = ctype;
	p->tn.rval = regno;;
	rbusy( p->tn.rval, p->in.type );
	p->tn.lval = 0;
	p->in.name = tstr("");
#if TARGET != I386
	p->in.rall = NOPREF; 
	p->in.flags = 0;
#endif
	return p; 
}

void
pccfmt_stmt(file_name, line_number)
	char *file_name;
	int line_number;
{
	filename = file_name;
#if TARGET != I386
	lineno = line_number;
#endif
}

void
pccfmt_doexpr(file_name, line_number,p)
	char *file_name;
	int line_number;
	NODE *p;
{
#if TARGET == I386
	NODE * condit();
	extern int eprint();
	void convert_types();
#endif

	filename = file_name;
#if TARGET == I386
 	if( lflag ) lineid( line_number, file_name );
#	ifdef MYREADER
	    MYREADER(p);
#	endif
	convert_types(p);
	/* eliminate the conditionals */
	if( p && odebug>2 ) eprint(p);
	p = condit( p, CEFF, -1, -1 );
	if( p ) {
#		ifdef	MYP2OPTIM
		    NODE * MYP2OPTIM();
		    p = MYP2OPTIM(p);	/* perform local magic on trees */
#		endif  MYP2OPTIM
		/* expression does something */
		/* generate the code */
		if( odebug>2 ) eprint(p);
		codgen( p );
	}
	else if( odebug>1 ) PUTS( "null effect\n" );
#else /* TARGET != I386 */
	lineno = line_number;
	if( lflag ) lineid( lineno, filename );
	if( edebug ) fwalk( p, eprint, 0 );
	nrecur = 0;
#ifdef MYREADER
	MYREADER(p);
#endif
	delay( p );
#endif /* TARGET != I386 */
	reclaim( p, RNULL, 0 );
	TMP_EXPR;
	allchk();
	tcheck();
}

NODE *
pccfmt_labelref(labelno, string, ctype)
	long labelno;
	char *string;
	TWORD ctype;
{
	char label[20];
	extern char *sprintf();

	if (string == NULL || string[0] == '\0') {
		sprintf(label, LABFMT, labelno);
		return pccfmt_icon(0L, label, ctype);
	} else {
		return pccfmt_icon(0L, string, ctype);
	}
}

void
pccfmt_labeldef(labelno,string)
	long labelno;
	char *string;
{
	if (string != NULL && string[0] != '\0') {
		fputs(string,stdout);
	} else {
		printf(LABFMT, labelno);
	}
	printf(":\n");
}

pccfmt_goto(labelno)
	long labelno;
{
	branch( labelno );
}

NODE *
pccfmt_indirgoto(lp,ctype)
	NODE *lp;
	TWORD ctype;
{
	NODE *p;

	p = talloc();
	p->in.op = GOTO;
	p->in.type = ctype;
	p->in.left = lp;
	p->tn.rval = 0;
#	if TARGET != I386
	p->in.rall = NOPREF; 
	p->in.flags = 0;
#	endif
	return(p);
}

static int
urelop(opno, lp, rp)
	int opno;
	NODE *lp, *rp;
{
	if (ISUNSIGNED(lp->in.type) || ISPTR(lp->in.type)
	    || ISUNSIGNED(rp->in.type) || ISPTR(rp->in.type)) {
		switch(opno) {
		case LT: return ULT;
		case LE: return ULE;
		case GT: return UGT;
		case GE: return UGE;
		}
	}
	return opno;
}

NODE *
pccfmt_binop(opno, lp, rp, ctype)
	int opno;
	NODE *lp, *rp;
	TWORD ctype;
{
	NODE *p;
	CONSZ val;

	/*
	 * map relational operators on unsigned data onto
	 * unsigned relational operators.
	 */
	if (logop(opno)) {
		opno = urelop(opno, lp, rp);
	}
	/*
	 * check for offset + symbol; want symbol + offset
	 */
	if (opno == PLUS && lp->in.op == ICON && rp->in.op == ICON
	    && rp->tn.name[0] != '\0') {
		/* want the symbol on the left, the nncon on the right */
		p = lp;
		lp = rp;
		rp = p;
	}
	/*
	 * check for ((base + offset1) [+-] offset2)
	 */
	if (lp->in.op == PLUS && ISPTR(lp->in.type)
	    && (opno == PLUS || opno == MINUS)
	    && nncon(lp->in.right) && nncon(rp)) {
		if (opno == PLUS) {
			lp->in.right->tn.lval += rp->tn.lval;
		} else {
			lp->in.right->tn.lval -= rp->tn.lval;
		}
		rp->in.op = FREE;
		return(lp);
	}
	/*
	 * check for both operands constant;
	 * evaluate at compile time if feasible.
	 */
	if ( nncon(rp) && lp->in.op == ICON
	    && (lp->tn.name[0] == '\0' || opno == PLUS || opno == MINUS) ) {
		switch (opno) {
		/*
		 * relational ops ( ==, !=, <=, <, >=, > )
		 */
		case EQ:
			val = (lp->tn.lval == rp->tn.lval);
			break;
		case NE:
			val = (lp->tn.lval != rp->tn.lval);
			break;
		case LE:
			val = (lp->tn.lval <= rp->tn.lval);
			break;
		case LT:
			val = (lp->tn.lval < rp->tn.lval);
			break;
		case GE:
			val = (lp->tn.lval >= rp->tn.lval);
			break;
		case GT:
			val = (lp->tn.lval > rp->tn.lval);
			break;
		case ULE:
			val = ((unsigned)lp->tn.lval <= (unsigned)rp->tn.lval);
			break;
		case ULT:
			val = ((unsigned)lp->tn.lval < (unsigned)rp->tn.lval);
			break;
		case UGE:
			val = ((unsigned)lp->tn.lval >= (unsigned)rp->tn.lval);
			break;
		case UGT:
			val = ((unsigned)lp->tn.lval > (unsigned)rp->tn.lval);
			break;
		/*
		 *  arithmetic/logical ops (+,-,*,div,mod,and,or,xor)
		 */
		case PLUS:
			/* this case also handles symbol+offset */
			val = (lp->tn.lval + rp->tn.lval);
			break;
		case MINUS:
			/* this case also handles symbol-offset */
			val = (lp->tn.lval - rp->tn.lval);
			break;
		case MUL:
			if (ISUNSIGNED(lp->tn.type) || ISUNSIGNED(rp->tn.type)){
				val = (unsigned)lp->tn.lval * (unsigned)rp->tn.lval;
			} else {
				val = (lp->tn.lval * rp->tn.lval);
			}
			break;
		case DIV:
			if (rp->tn.lval == 0) {
				werror("division by 0");
				goto nonconst;
			}
			if (ISUNSIGNED(lp->tn.type) || ISUNSIGNED(rp->tn.type)){
				val = (unsigned)lp->tn.lval / (unsigned)rp->tn.lval;
			} else {
				val = (lp->tn.lval / rp->tn.lval);
			}
			break;
		case MOD:
			if (rp->tn.lval == 0) {
				werror("division by 0");
				goto nonconst;
			}
			if (ISUNSIGNED(lp->tn.type) || ISUNSIGNED(rp->tn.type)){
				val = (unsigned)lp->tn.lval % (unsigned)rp->tn.lval;
			} else {
				val = (lp->tn.lval % rp->tn.lval);
			}
			break;
		case AND:
			val = (lp->tn.lval & rp->tn.lval);
			break;
		case OR:
			val = (lp->tn.lval | rp->tn.lval);
			break;
		case ER:
			val = (lp->tn.lval ^ rp->tn.lval);
			break;
		case LS:
			val = (lp->tn.lval << rp->tn.lval);
			break;
		case RS:
			if (ISUNSIGNED(lp->tn.type)) {
				val = ( (unsigned)(lp->tn.lval)
					>> rp->tn.lval );
			} else {
				val = (lp->tn.lval >> rp->tn.lval);
			}
			break;
		default:
			goto nonconst;
		}
		lp->tn.type = ctype;
		lp->tn.lval = val;
		rp->in.op = FREE;
		return(lp);
	} /* if nncon */

nonconst:
	p = talloc();
	switch(opno) {
	case LT:
	case LE:
	case GT:
	case GE:
	case EQ:
	case NE:
	case ULT:
	case ULE:
	case UGT:
	case UGE:
		p->bn.op = opno;
		p->bn.type = ctype;
		p->bn.reversed = 0;
		p->in.left = lp;
		p->in.right = rp;
#		if TARGET != I386
		p->in.rall = NOPREF; 
#		endif
		break;

	default:
		p->in.op = opno;
		p->in.type = ctype;
		p->in.left = lp;
		p->in.right = rp;
#		if TARGET != I386
		p->in.rall = NOPREF; 
		p->in.flags = 0;
#		endif
		break;

	}
	return p; 
}

NODE *
pccfmt_unop(opno, lp, ctype)
	int opno;
	NODE *lp;
	TWORD ctype;
{
	NODE *p;

	if (nncon(lp)) {
		switch(opno) {
		case UNARY MUL:
			/* u-page hack */
			lp->tn.op = NAME;
			break;
		case UNARY MINUS:
			lp->tn.lval = -(lp->tn.lval);
			break;
		case NOT:
			lp->tn.lval = !(lp->tn.lval);
			break;
		case COMPL:
			lp->tn.lval = ~(lp->tn.lval);
			break;
		case SCONV:
			switch(ctype) {
			case CHAR:
				lp->tn.lval = (char)lp->tn.lval;
				break;
			case SHORT:
				lp->tn.lval = (short)lp->tn.lval;
				break;
			case INT:
				lp->tn.lval = (int)lp->tn.lval;
				break;
			case LONG:
				lp->tn.lval = (long)lp->tn.lval;
				break;
			case UCHAR:
				lp->tn.lval = (unsigned char)lp->tn.lval;
				break;
			case USHORT:
				lp->tn.lval = (unsigned short)lp->tn.lval;
				break;
			case UNSIGNED:
				lp->tn.lval = (unsigned)lp->tn.lval;
				break;
			case ULONG:
				lp->tn.lval = (unsigned long)lp->tn.lval;
				break;
			default:
				if (!ISPTR(ctype))
					goto nonconst;
				lp->tn.lval = (unsigned long)lp->tn.lval;
				break;
			}
			break;
#if (TARGET != I386)
		case PCONV:
			if (ctype == DOUBLE)
				goto nonconst;
			break;
#endif (TARGET != I386)
		default:
			goto nonconst;
		}
		lp->tn.type = ctype;
		return lp;
	}
nonconst:

#if (TARGET == I386)
	if( opno == PCC_UNARY_MUL && lp->in.op == ICON ) {
	    /*
	     * the 386 code generator won't deal with
	     * an unfolded indirection-over-constant,
	     * so fold it here.
	     */
	    lp->in.op = NAME;
	    lp->in.type = ctype;
	    return lp;
	}
        if( opno == PCC_PCONV ) {
            lp->in.type = ctype;
            return lp;
        } 
#endif
	p = talloc();
	p->in.op = opno;
	p->in.type = ctype;
	p->in.left = lp;
	p->tn.rval = 0;
#	if TARGET != I386
	p->in.rall = NOPREF; 
	p->in.flags = 0;
#	endif
	return p; 
}


#if (TARGET == I386)
is_tap_node(p)
NODE *p;
{
	switch(p->in.op) {
	  case PCC_VAUTO:
	  case PCC_VPARAM:
	  case PCC_TEMP:
		return 1;
	  default:
		break; 
	}
	return 0;     
}
#endif (TARGET == I386)


pccfmt_pass(string)
	char *string;
{
	puts(string);
}

void
pccfmt_lbrac(procno, regmask, frame_size, fregmask)
	int procno, regmask;
	OFFSZ frame_size;
	int fregmask;
{
	long newftnno;
	OFFSZ aoff;

	newftnno = procno;
#if TARGET == I386
	regused = regvar = ~regmask;
	maxarg = 0;
#else
	maxtreg = regmask;
	maxfreg = fregmask;
#endif
	aoff = frame_size;
	SETOFF(aoff, ALSTACK);
	TMP_BBEG;
	if( ftnno != newftnno ){
		/* beginning of function */
		TMP_BFN;
		ftnno = newftnno;
		bfcode2();
	} else {
		TMP_NFN;
		/* maxoff at end of ftn is max of autos and temps
		   over all blocks in the function */
	}
	saveregs();
#if TARGET != I386	
	setregs();
#endif
}

void
pccfmt_rbrac(t, size, align, opt_level)
	TWORD t;
	OFFSZ size;
	int align;
	int opt_level;
{
	TMP_BEND;
	eobl2(t, size, align, opt_level);
}

pccfmt_eof()
{
	return( nerrors );
}


NODE *
pccfmt_st_op(opno, size, align, lp, rp, ctype)
	int opno, size, align;
	NODE *lp, *rp;
	TWORD ctype;
{
	NODE *p;

	p = talloc();
#if TARGET == I386
#define SZCHAR 8
	p->stn.stsize = size*SZCHAR;
	p->stn.stalign = align*SZCHAR;
#else
	p->stn.stsize = size;
	p->stn.stalign = align;
#endif
	p->in.op = opno;
	p->in.type = ctype;

	switch(optype(opno)) {

		case BITYPE:
			p->in.left = lp;
			p->in.right = rp;
#			if TARGET != I386
			p->in.rall = NOPREF; 
#			endif
			break;

		case UTYPE:
			p->in.left = lp;
			p->tn.rval = 0;
#			if TARGET != I386
			p->in.rall = NOPREF; 
#			endif
			break;

		default:
			uerror( "illegal leaf node: %d", p->in.op );
			exit(1);
	}
	return p; 
}

NODE*
pccfmt_tmp(nwords, ctype)
	unsigned nwords;
	TWORD ctype;
{
#if TARGET == I386
	register t;
	NODE *pccfmt_TAP_node();

	t = freetemp(nwords);
	return pccfmt_TAP_node(PCC_VAUTO, BITOOR(t), ctype);
#else
	NODE *p;

	p = talloc();
	p->in.op = OREG;
	p->in.type = ctype;
	p->tn.rval = TMPREG;
	p->tn.lval = freetemp(nwords);
	p->tn.lval = BITOOR(p->tn.lval);
#	if TARGET == SPARC
	    p->in.name = tmpname;
#	else
	    p->in.name = "";
#	endif
	p->in.rall = NOPREF; 
	p->in.flags = 0;
	return p; 
#endif
}

NODE *
pccfmt_addroftmp(tmp)
	NODE *tmp;
{
	NODE *p, *lp, *rp;

#	if TARGET == I386
	lp = pccfmt_TAP_node(PCC_VAUTO, tmp->tn.lval, tmp->in.type);
	p = pccfmt_unop(UNARY AND, lp, lp ->in.type);
#	endif

#	if TARGET != I386
	lp = pccfmt_reg(TMPREG, tmp->in.type | PTR);
	rp = pccfmt_icon(tmp->tn.lval, "", INT);
#	if TARGET == SPARC
	    rp->tn.name = tmpname;
#	endif
	p  = pccfmt_binop(PLUS, lp, rp, tmp->in.type | PTR);
#	endif

	return p;
}

void
pccfmt_switch(selector, swtab, ncases)
	NODE *selector;
	struct sw swtab[];
	int ncases;
{
#if TARGET == I386
	convert_types(selector);
#endif
	p2switch(selector, swtab, ncases); /* boy, that was easy. */
}

NODE *
pccfmt_fld(lp, bfoffset, bfwidth, ctype)
	NODE *lp;
	int bfoffset, bfwidth;
	TWORD ctype;
{
	NODE *p;

	p = talloc();
	p->in.op = FLD;
	p->in.type = ctype;
	p->tn.rval = PKFIELD(bfwidth, bfoffset);
	p->in.left = lp;
	p->in.name = "";
#	if TARGET != I386
	p->in.rall = NOPREF; 
	p->in.flags = 0;
#	endif
	return p; 
}

#if TARGET == I386
/*	pccfmt_TAP_node
 *
 *	Generates VAUTO, VPARAM, and VTEMP nodes.
 *
 */
NODE *
pccfmt_TAP_node( op, offset, ctype )
	int op;			/* should check for VAUTO..VPARAM at some pt. */
	int offset;
	TWORD ctype;
{
	NODE *p;

	p = talloc();
	p -> in.op = op;
	p -> in.type = ctype;
	p -> tn.lval = offset;
	p -> tn.rval = 0;
	return p;
}

/*	pccfmt_special_node
 *
 *	Generates RNODE and SNODE nodes for pcc to use
 *
 */
NODE*
pccfmt_special_node( opno, ctype )
	int opno;
	TWORD ctype;
{
	NODE *p;

	p = talloc();
	p -> in.op = opno;
	p -> in.type = ctype;
	return p;
}

void
convert_types( p )
	NODE *p;
{	PCC_TWORD ttype();
	p ->in.type = ttype(p->in.type);
	switch (optype(p->in.op)){
		case BITYPE:
			convert_types(p->in.right);
		case UTYPE:
			convert_types(p->in.left);
		case LTYPE:
		default:
			break;
	}
	return;
}

void 
pccfmt_fixargs( p )
	NODE *p;
{	int off;
	NODE * insertargnodes();

	off = 0;
	p -> in.right = insertargnodes( p->in.right, &off);
	return;
}
static
NODE *
insertargnodes( p, off )
	NODE *p;
	int *off;
{
	/*	walk a call tree and fix the arguments . This is a FORTRAN special
	 *	which assumes call by reference except for real and double that
	 *	are passed to intrinsics.
	 */
    register PCC_TWORD t;
	NODE *arg;
	int size, align;

	align = ALINT;			/*	align on integer boundaries */
    t = p->in.type;
    if (p->in.op == CM) {	/* commas separate arguments */
		p->in.left = insertargnodes( p->in.left, off );
		p->in.right = insertargnodes( p->in.right, off );
		return p;
    }
	if (p->in.op == PCC_STARG)
		return p;

    if ( t == PCC_STRTY || t == PCC_UNIONTY ){
		arg = pccfmt_st_op(PCC_STARG, sizeof(int), ALINT,
				(p->in.op == (UNARY AND)?p:pccfmt_unop(UNARY AND, p, PCC_INT)), 
				(PCC_NODE*)NULL, PCC_STRTY | PCC_PTR);
		arg->in.op = PCC_FUNARG;	/*	?? */

    } else if(p->tn.op == PCC_REG && p->tn.rval== AUTOREG) {
                /* Special case for passing static links (%ebp) for Pascal
                   WIN 12/7/87 */
                tfree(p);
                arg = pccfmt_special_node(PCC_FUNARG, t);
                arg->in.right = NULL;
                size = sizeof(int);
                p = pccfmt_TAP_node(VAUTO, 0, t);
                p = pccfmt_unop(PCC_UNARY_AND, p,t);
                arg->in.left  = p;
    } else {
		arg = pccfmt_special_node(PCC_FUNARG, t);	
		arg->in.right = NULL;
		if (t == PCC_DOUBLE) 
			size = sizeof(double);		/* not portable and wrong for
							 * small types, too.  */
		else size = sizeof(int);
		arg ->in.left = p;
	}

    SETOFF( *off, align );
    arg->tn.rval = *off;

    *off += size;

    return arg;
}
#endif 


void
pccfmt_chk_ovflo(ovflo)
{
	chk_ovfl = ovflo;
}

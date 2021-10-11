#ifndef lint
static	char sccsid[] = "@(#)pcc.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include    "cg_ir.h"
#if TARGET == I386
#include    "ops.h"
#endif
#include    "pcc_ops.h"
#include    "pcc_f1defs.h"
#include    "pcc_node.h"
#include    "pcc_sw.h"
#include    <stdio.h>
#include    <ctype.h>

/*
 * this file implements the iropt side of the iropt-> pcc interface
 * in general, functions with names that start with pcc_... map an 
 * iropt construct to a pcc one - for simplicity this convention is 
 * extended to functions that don't return values
 */

/* advance x to a multiple of y */
# define SETOFF(x,y)   if( (x)%(y) != 0 ) (x) = ( ((x)/(y) + 1) * (y))

typedef enum { Text, Data, Bss } CSECT;

static void  set_locctr(/*csect*/);
static void  int_data_stmt(/*size,value*/);
static void  float_data_stmt(/*size,value*/);
static void  string_data_stmt(/*string*/);
static void  bss_seg();

extern PCC_NODE *pccfmt_icon(), *pccfmt_name(), *pccfmt_oreg(), *pccfmt_reg();
extern PCC_NODE *pccfmt_binop(), *pccfmt_unop(), *pccfmt_indirgoto();
extern PCC_NODE *pccfmt_st_op(), *pccfmt_tmp(), *pccfmt_addroftmp();
extern PCC_NODE *pccfmt_fcon();
extern PCC_NODE *pccfmt_fld();
extern PCC_NODE *pccfmt_labelref();
#if TARGET == I386
extern PCC_NODE *pccfmt_special_node();
extern PCC_NODE *pccfmt_TAP_node();
extern void pccfmt_fixargs();
#endif
extern void	pccfmt_stmt();
extern void	pccfmt_doexpr();
extern void	pccfmt_labeldef();
extern void	pccfmt_switch();
extern void	pccfmt_lbrac();
extern void	pccfmt_rbrac();
extern void	pccfmt_chk_ovflo();

/* from pcc/mip/common.c */
extern void	pcc_tfree(/*p*/);

static PCC_NODE *pcc_addrof(), *pcc_assign(), *pcc_binval(), *pcc_call();
static PCC_NODE *pcc_cx_param(), *pcc_cx_binval(), *pcc_cx_unval();
static PCC_NODE *pcc_cbranch(), *pcc_err();
static PCC_NODE *pcc_fval(), *pcc_goto(), *pcc_indirgoto(), *pcc_label(); 
static PCC_NODE *pcc_repeat(), *pcc_switch();
static PCC_NODE *pcc_unval();
static PCC_NODE *pcc_leaf();
static PCC_NODE *pcc_intr_call();
static PCC_NODE *pcc_pass();
static PCC_NODE *pcc_stmt();
static PCC_NODE *pcc_foreff();
static void	pcc_flags();

static PCC_TWORD pcc_type();

#define BAD_PCCOP 0

static struct map_op {
	int pcc_op;
	PCC_NODE *(*map_ftn)();
	char * implement_complex;
} op_map_tab[] = {
	/*IR_LEAFOP*/ { BAD_PCCOP, pcc_err , "" },
	/*IR_ENTRYDEF*/ { BAD_PCCOP, pcc_err, "" },
	/*IR_EXITUSE*/ { BAD_PCCOP, pcc_err, "" },
	/*IR_IMPLICITDEF*/ { BAD_PCCOP, pcc_err, "" },
	/*IR_IMPLICITUSE*/ { BAD_PCCOP, pcc_err, "" },
	/*IR_PLUS*/ {	PCC_PLUS, pcc_binval, "add" },
	/*IR_MINUS*/ {	PCC_MINUS, pcc_binval, "minus" },
	/*IR_MULT*/ {	PCC_MUL, pcc_binval, "mult" },
	/*IR_DIV*/ {	PCC_DIV, pcc_binval, "div" },
	/*IR_REMAINDER*/ { PCC_MOD, pcc_binval, "" },
	/*IR_AND*/ { PCC_AND, pcc_binval, "" },
	/*IR_OR*/ { PCC_OR, pcc_binval, "" },
	/*IR_XOR*/ { PCC_ER, pcc_binval, "" },
	/*IR_NOT*/ { PCC_NOT, pcc_unval, "" },
	/*IR_LSHIFT*/ { PCC_LS, pcc_binval, "" },
	/*IR_RSHIFT*/ { PCC_RS, pcc_binval, "" },
	/*IR_SCALL*/ {	PCC_CALL, pcc_call, "" },
	/*IR_FCALL*/ {	PCC_CALL, pcc_call, "" },
	/*IR_EQ*/ { PCC_EQ, pcc_binval, "eq" },
	/*IR_NE*/ { PCC_NE, pcc_binval, "ne" },
	/*IR_LE*/ { PCC_LE, pcc_binval, "" },
	/*IR_LT*/ { PCC_LT, pcc_binval, "" },
	/*IR_GE*/ { PCC_GE, pcc_binval, "" },
	/*IR_GT*/ { PCC_GT, pcc_binval, "" },
	/*IR_CONV*/ { PCC_SCONV, pcc_unval, "" },
	/*IR_COMPL*/ { PCC_COMPL, pcc_unval, "" },
	/*IR_NEG*/ { PCC_UNARY_MINUS, pcc_unval, "neg" },
	/*IR_ADDROF*/ { BAD_PCCOP, pcc_addrof, "" },
	/*IR_IFETCH*/ { PCC_UNARY_MUL, pcc_unval, "" },
	/*IR_ISTORE*/ { PCC_ASSIGN, pcc_assign, "" },
	/*IR_GOTO*/ {	PCC_GOTO, pcc_goto, "" },
	/*IR_CBRANCH*/ { PCC_CBRANCH, pcc_cbranch, "" },
	/*IR_SWITCH*/ { PCC_FSWITCH, pcc_switch, "" },
	/*IR_REPEAT*/ { BAD_PCCOP, pcc_repeat, "" },
	/*IR_ASSIGN*/ { PCC_ASSIGN, pcc_assign, "" },
	/*IR_PASS*/ {	PCC_FTEXT, pcc_pass, "" },
	/*IR_STMT*/ {	PCC_FEXPR, pcc_stmt,  "" },
	/*IR_LABELDEF*/ {	PCC_FLABEL, pcc_label, "" },
	/*IR_INDIRGOTO*/ {	PCC_GOTO, pcc_indirgoto, "" },
	/*IR_FVAL*/ {	PCC_FORCE, pcc_fval, "" },
	/*IR_LABELREF*/ {	BAD_PCCOP, pcc_err, "" },
	/*IR_PARAM*/ {	BAD_PCCOP, pcc_err, "" },
	/*IR_PCONV*/{   PCC_PCONV, pcc_unval, "" },
	/*IR_FOREFF*/ { BAD_PCCOP, pcc_foreff, "" }
};

static BOOLEAN is_c_special_op(), is_same_tree();
static BOOLEAN in_main_source = IR_TRUE;
static BOOLEAN set_main_source();
static int log2();
static void endata_const();
static OFFSZ addr_offset(/*ap*/);

/*
 * source statement coordinates, for error reporting
 * and statement level test coverage profiling (tcov)
 */
char *source_file;
int source_lineno;

/*
 * global state information for tcov profiling
 */
int stmtprofflag = 0;		/* =1 -> statement profiling is enabled */
FILE *dotd_fp = NULL;		/* output file for statement profiling */
char *dotd_filename = NULL;	/* name of statement profiling output file */
				/* ... compiled into object code! (bletch) */
static long bb_struct_label = 0;/* label number for bblink struct */
static long bb_array_label = 0;	/* label number for bb counters array */
char *dotd_basename = NULL;	/* 'basename' of dotd_filename */
int  dotd_baselen;		/* strlen(dotd_basename) */
int bbnum = 0;			/* basic block number, for stmt profiling */
static BOOLEAN bb_new_block= IR_FALSE;	/* TRUE => new basic block */
static void bb_init_code();	/* tcov code for function entry */
static void bb_incr_code();	/* tcov code for basic block entry */

/*
 * hack: 1-block lookahead for fall-through detection
 */
static BLOCK *curblock = NULL;

/*
 * main routine of IR=>PCC mapping
 */
PCC_NODE *
pcc_iropt_node(np)
	register IR_NODE *np;
{
	PCC_NODE *(*map_ftn)();

	if(np->operand.tag == ISTRIPLE) {
		map_ftn = op_map_tab[(int) ((TRIPLE*)np)->op ].map_ftn;
		pcc_flags((TRIPLE*)np);
		return (*map_ftn)(np);
	} else {
		return pcc_leaf((LEAF*) np);
	}
}
#if TARGET == I386
/*	TAP added to return the special rcc node types given an ir
 *	segment class for a leaf. TAP stands for TEMP AUTO PARAM
 *		RS 7/6/87
 */
int
TAP( class )
	SEGCLASS class;
{
	switch (class){
		case ARG_SEG:
			return PCC_VPARAM;
		case AUTO_SEG:
			return PCC_VAUTO;
		case TEMP_SEG:
			return PCC_TEMP;
		default:
			return 0;
	}
}
#endif
/*
 * offset part of an address.
 *
 * For AUTO_SEG segments, this is the sum of the segment offset and
 * the operand offset; the two are maintained separately to simplify
 * reallocation of storage to automatic struct and union variables.
 *
 * For operands in other segments, the operand offset is returned
 * as is.
 */
static OFFSZ
addr_offset(ap)
	ADDRESS *ap;
{
	if (ap->seg->descr.class == AUTO_SEG) {
		/* relocatable AUTO segment */
		return (OFFSZ)(ap->seg->offset + ap->offset);
	}
	return (OFFSZ)ap->offset;
}

/*
**  pcc doesn't support addrof: if the address is a compile time
**  constant generate an icon else (locations with register bases)
**  do explicit arithmetic.  rcc supports U& of VAUTO, VPARAM, and
**  TEMP, so for TARGET==I386 we do something different (RS) 7/6/87
*/
static PCC_NODE *
pcc_addrof(np)
	IR_NODE *np;
{
	register LEAF *leafp;
	PCC_TWORD tword;
	char name[BUFSIZ];
	ADDRESS *ap;
	PCC_NODE *p, *lp, *rp;
	TRIPLE *tp;
	TYPE t;

	if(np->operand.tag == ISTRIPLE) {
		tp = (TRIPLE*) np;
		if(tp->left->operand.tag == ISTRIPLE) {
			quita("pcc_addrof: addrof expr op >%s<",
				op_descr[ORD(tp->op)].name);
		} else {
			leafp = (LEAF*) tp->left;
		}
	} else {
		leafp = (LEAF*) np;
	}
	t = leafp->type;
	t.tword = PCC_INCREF(t.tword);
	tword = pcc_type(t);
	switch(leafp->class) {

	case CONST_LEAF:
		if(IR_ISPTRFTN(leafp->type.tword)) {
			p = pccfmt_icon(0L, leafp->val.const.c.cp, tword);
		} else {
			if(leafp->visited ==  IR_FALSE) {
				endata_const(leafp);
			}
#if TARGET == I386
			sprintf(name,".L%dD%d",hdr.procno,leafp->leafno);
#else
			sprintf(name,"L%dD%d",hdr.procno,leafp->leafno);
#endif
			p = pccfmt_icon(0L, name, tword);
		}
		break;

	case VAR_LEAF:
		if(leafp->val.addr.seg->descr.content == REG_SEG ) {
			quit("pcc_addrof: address of variable in register segment");
		}
		ap = &leafp->val.addr;
		if(ap->seg->base == NOBASEREG) {
			if(ap->labelno != 0) {
				sprintf(name,"v.%d",ap->labelno);
				p = pccfmt_icon(ap->offset,name,tword);
			} else {
				p = pccfmt_icon(ap->offset,ap->seg->name,tword);
			}
		} else {
#if TARGET == I386
			/*	if this is a T,A, or P, then create a node and
			 *	apply a U& to it. (RS) 7/6/87
			 */
			int n_op = TAP(ap->seg->descr.class);
			if (n_op){
				p = pccfmt_TAP_node(n_op, addr_offset(ap) , tword);
				p = pccfmt_unop(PCC_UNARY_AND, p, p->in.type); 
				return p;
			}
#endif
			lp = pccfmt_reg(ap->seg->base,tword);
			rp = pccfmt_icon(addr_offset(ap), "", pcc_type(inttype));
			p = pccfmt_binop(PCC_PLUS, lp, rp, tword);
		}
		break;

	default:
		quit("pcc_addrof: address of address constant");
	}
	return p;
}

static PCC_NODE *
pcc_assign(tp)
	TRIPLE *tp;
{
	PCC_NODE *p, *lp, *rp;
	PCC_TWORD ctype;
	TYPE t;
	TRIPLE *right;
	int op;

	t = tp->right->operand.type;
	if( tp->op == IR_ISTORE ) {
		/* lhs is PTR (...) */
		ctype = pcc_type(t);
		lp = pcc_iropt_node(tp->left);
		if(IR_ISCOMPLEX(t.tword) || t.tword == PCC_STRTY
		    || t.tword == PCC_UNIONTY ) {
			/*
			 * map complexes as a structure assignment
			 * this now requires lvalues on both sides
			 */
			if(tp->right->operand.tag == ISLEAF) {
				rp = pcc_addrof(tp->right);
			} else if( tp->right->operand.tag == ISTRIPLE && 
				tp->right->triple.op == IR_IFETCH) {
				rp = pcc_iropt_node(tp->right->triple.left);
			} else {
				rp = pcc_iropt_node(tp->right);
			}
			p = pccfmt_st_op(PCC_STASG, t.size, t.align, lp, rp,
				PCC_STRTY|PCC_PTR );
		} else {
			lp = pccfmt_unop(PCC_UNARY_MUL, lp, ctype );
			if (tp->type.tword == IR_BITFIELD) {
				lp = pccfmt_fld(lp, tp->type.align,
					tp->type.size, ctype);
			}
			rp = pcc_iropt_node(tp->right);
			p = pccfmt_binop(PCC_ASSIGN, lp, rp, ctype);
		}
	} else { /* op == IR_ASSIGN */
		if(IR_ISCOMPLEX(t.tword) || t.tword == PCC_STRTY
		    || t.tword == PCC_UNIONTY) {
			lp = pcc_addrof(tp->left);
			if(tp->right->operand.tag == ISLEAF) {
				rp = pcc_addrof(tp->right);
			} else if( tp->right->operand.tag == ISTRIPLE
			    && tp->right->triple.op == IR_IFETCH) {
				rp = pcc_iropt_node(tp->right->triple.left);
			} else {
				rp = pcc_iropt_node(tp->right);
			}
			p = pccfmt_st_op(PCC_STASG, t.size, t.align, lp, rp,
				PCC_STRTY | PCC_PTR );
		} else {
			ctype = pcc_type(t);
			lp = pcc_iropt_node(tp->left);
			right = (TRIPLE*) tp->right;
			if(right->tag == ISTRIPLE && is_c_special_op(right)
			    && is_same_tree(tp->op, tp->left, right)) {
				/* turn it into an ASG tree */
				op = PCC_ASG op_map_tab[(int) right->op].pcc_op;
				right = (TRIPLE*) right->right;
			} else {
				op = PCC_ASSIGN;
			}
			rp = pcc_iropt_node((IR_NODE*)right);
			p = pccfmt_binop(op, lp, rp, ctype);
		}
	}
	pccfmt_doexpr(source_file, source_lineno,p);
	return (PCC_NODE*) NULL;
}

static PCC_NODE *
pcc_binval(tp)
	TRIPLE *tp;
{
	PCC_TWORD ltword, rtword;
	PCC_NODE *p, *lp, *rp;
	int i;

	ltword = tp->left->operand.type.tword;
	rtword = tp->right->operand.type.tword;
	if( IR_ISCOMPLEX(ltword) || IR_ISCOMPLEX(rtword) ) {
		if(ltword == rtword) {
			return pcc_cx_binval(tp);
		}
		/* operators of a complex op must be of same type */
		quit("pcc_binval: missing IR_CONV in complex op");
	}
	if(tp->op == IR_MULT) { /* try to turn it into a shift */
		if( IR_ISINT(tp->type.tword) ) {
			if( tp->left->operand.tag == ISLEAF
			    && ( (LEAF *)tp->left)->class == CONST_LEAF ) {
				IR_NODE *node;
				node = tp->left;
				tp->left = tp->right;
				tp->right = node;
			}
			if( tp->right->operand.tag == ISLEAF &&
				( (LEAF *)tp->right)->class == CONST_LEAF ) {
				i = log2( ((LEAF *)tp->right)->val.const.c.i );
				if( i < 0 ) { /* not power of 2 */
					goto contin;
				}
				if( i == 0 ) {
					/* multiply by 1 */
					p = pcc_iropt_node( tp->left );
					return p;
				}
				/* else */
				tp->op = IR_LSHIFT;
				tp->right = (IR_NODE*)ileaf((long)i);
			}
		}
	}
#if TARGET==I386
          /* For pascal, convert oreg type constructs off FP  into a TAP node.  
             WIN 11/16/87 */
        if(tp->op == IR_PLUS || tp->op == IR_MINUS ) { /* convert OREG trees into TAP */ 
           if( (tp->left->operand.tag == ISLEAF) &&
               (tp->right->operand.tag== ISLEAF)) {
                 LEAF    *lfp,*rfp;
                 ADDRESS *ap;          
                 int     newop,offst;

                 lfp = (LEAF *)tp->left;
                 rfp = (LEAF *)tp->right;
                 if(lfp->class != VAR_LEAF || rfp->class != CONST_LEAF) goto contin;
                 ap = &lfp->val.addr;
                 if (ap->seg->descr.class == DREG_SEG &&
                     ap->offset == AUTOREG) {
                   if(tp->op == IR_MINUS) {
                       newop = VPARAM;
                       offst = -rfp->val.addr.offset;
		     }
                   else {
                       newop = VAUTO;
                       offst = rfp->val.addr.offset;
		     }
                   p = pccfmt_TAP_node(newop, offst , pcc_type(tp->type));
   		   p = pccfmt_unop(PCC_UNARY_AND, p, p->in.type); 
                   return p;
                 }
	   }
	}
#endif

contin:
	lp = pcc_iropt_node(tp->left);
	rp = pcc_iropt_node(tp->right);
	p =pccfmt_binop(op_map_tab[(int)tp->op].pcc_op,
		lp, rp, pcc_type(tp->type));
	return p;
}

/*
 * pass a complex parameter by reference
 */
static PCC_NODE *
pcc_cx_param(np)
	IR_NODE *np;
{
	if(np->operand.tag == ISLEAF) {
		return pcc_addrof(np);
	}
	if( np->operand.tag == ISTRIPLE && np->triple.op == IR_IFETCH) {
		return pcc_iropt_node(np->triple.left);
	}
	/*
	 * a complex-valued expression is always translated into
	 * (libcall(&tmp,...), &tmp), so we assume here that the
	 * result will always be an lvalue
	 */
	return pcc_iropt_node(np);
}

static PCC_NODE *
pcc_cx_binval(tp)
	TRIPLE *tp;
{
	PCC_NODE *p, *lp, *rp, *tmp;
	char *opname;
	TYPE t;
	PCC_TWORD btype;
	char  name[BUFSIZ];

	t = tp->left->operand.type;
	opname = op_map_tab[(int) tp->op ].implement_complex;
	if( opname == NULL || opname[0] == '\0' ) {
		quita("pcc_cx_binval: bad op '%s'", op_descr[ORD(tp->op)].name);
	}
	/*
	 * if result is complex, translate as
	 *	(libcall(&tmp, lhs, rhs), &tmp);
	 * else translate as
	 *	libcall(lhs,rhs)
	 */
# if TARGET != I386
	sprintf(name , "__F%c_%s", (t.tword == IR_COMPLEX ? 'c' : 'z'), opname);
#else
        sprintf(name , "_F%c_%s", (t.tword == IR_COMPLEX ? 'c' : 'z'), opname);
#endif
	if (!IR_ISCOMPLEX(tp->type.tword)) {
		/* libcall(lhs, rhs) */
		btype = pcc_type(tp->type);
		lp = pcc_cx_param(tp->left);
		rp = pcc_cx_param(tp->right);
		rp = pccfmt_binop(PCC_LISTOP, lp, rp, PCC_INT);
		lp = pccfmt_icon(0L, name, PCC_INCREF(PCC_FTN|btype) );
		p  = pccfmt_binop(PCC_CALL, lp, rp, btype );
#if TARGET == I386
			pccfmt_fixargs(p);
#endif
	} else {
		/* (&tmp,lhs) */
		btype = pcc_type(t);
		tmp = pccfmt_tmp(t.size/sizeof(int), btype);
		lp = pccfmt_addroftmp(tmp);
		rp = pcc_cx_param(tp->left);
		lp = pccfmt_binop(PCC_LISTOP, lp, rp, PCC_INT);

		/* (&tmp,lhs,rhs) */
		rp = pcc_cx_param(tp->right);
		rp = pccfmt_binop(PCC_LISTOP, lp, rp, PCC_INT);

		/* libcall(&tmp,lhs,rhs) */
		lp = pccfmt_icon(0L, name, PCC_INCREF(PCC_FTN|PCC_INT));
		lp  = pccfmt_binop(PCC_CALL, lp, rp, PCC_INT );
#if TARGET == I386
			pccfmt_fixargs(lp);
#endif
		/* (libcall(&tmp,lhs,rhs), &tmp) */
		rp = pccfmt_addroftmp(tmp);
		p = pccfmt_binop(PCC_COMOP, lp, rp, PCC_INCREF(btype));
		pcc_tfree(tmp);
	}
	return p;
}

static PCC_NODE *
pcc_call(tp) 
	TRIPLE *tp;
{
	LEAF *func;
	TRIPLE *args;
	IR_NODE *arg;
	PCC_TWORD tylist;
	TRIPLE *param;
	PCC_NODE *lp, *rp, *p, *left;
	BOOLEAN rewrite_lib_call();
	TYPE t;

	func= (LEAF*) tp->left;
	if(func->class == CONST_LEAF) {
		if(func->func_descr == INTR_FUNC) {
			return pcc_intr_call(tp);
		}
		if(func->func_descr == SUPPORT_FUNC) {
			/* try to replace the call by a simpler tree */
			if(rewrite_lib_call(tp, &p) == IR_TRUE) {
				return p;
			}
		}
		t = func->type;
		if (PCC_ISFTN(t.tword)) {
			/* should be PTR FTN, not FTN */
			t.tword = PCC_INCREF(t.tword);
		}
		lp = pccfmt_icon(0L, func->val.const.c.cp, pcc_type(t));
	} else {
		lp = pcc_iropt_node((IR_NODE*)func);
	}

	tylist = pcc_type(inttype);
	args = (TRIPLE*) tp->right;
	left = (PCC_NODE*) NULL;
	if(args) {
		TFOR(param,args->tnext) {
			arg = param->left;
			t = arg->operand.type;
			if (t.tword == PCC_STRTY || IR_ISCOMPLEX(t.tword)
			    || t.tword == PCC_UNIONTY) {
				/* structured argument */
				if (arg->operand.tag == ISLEAF) {
					rp = pcc_addrof(arg);
				} else if (arg->operand.tag == ISTRIPLE
				    && arg->triple.op == IR_IFETCH) {
					rp = pcc_iropt_node(arg->triple.left);
				} else {
					quit("pcc_call: illegal structured argument");
				}
				rp = pccfmt_st_op(PCC_STARG, t.size, t.align,
					rp, (PCC_NODE*)NULL, PCC_STRTY);
			} else {
				rp = pcc_iropt_node(arg);
			}
			if(left) {
				left = pccfmt_binop(PCC_LISTOP, left, rp, tylist);
			} else {
				left = rp;
			}
		} 
	}
	t = tp->type;
	if (tp->is_stcall) {
		/*
		 * structured result. Note that the result type
		 * is represented as PTR STRTY, but size and alignment
		 * are those of the structure type.  Nauseating, but
		 * where else are you going to put this information?
		 */
		if(args) {
			p = pccfmt_st_op(PCC_STCALL, t.size, t.align,
				lp, left, PCC_INCREF(PCC_STRTY));
#if TARGET == I386
			pccfmt_fixargs(p);
#endif
		} else {
			p = pccfmt_st_op(PCC_UNARY_STCALL, t.size, t.align,
				lp, (PCC_NODE*)NULL, PCC_INCREF(PCC_STRTY));
		}
	} else {
		/* normal result */
		if(args) {
			p = pccfmt_binop(PCC_CALL, lp, left, pcc_type(t));
#if TARGET == I386
			pccfmt_fixargs(p);
#endif
		} else {
			p = pccfmt_unop(PCC_UNARY_CALL, lp, pcc_type(t));
		}
	}
	if(tp->op == IR_SCALL) {
		pccfmt_doexpr(source_file, source_lineno,p);
		p = (PCC_NODE*) NULL;
	}
	return p;
}

/*
 * this is a hack to deal with an unconditional branch from
 * the end of a basic block to the beginning of the next
 * basic block; it requires that we have a pointer to
 * the current block, which is set at the beginning of the
 * main loop in map_to_pcc().
 */
static void
pcc_branch(tp, labelno)
	TRIPLE *tp;
	long labelno;
{
	if (curblock!= NULL && curblock->next != NULL
	    && tp == curblock->last_triple
	    && curblock->next->labelno == labelno) {
		/* branching to next block */
		return;
	}
	pccfmt_goto(labelno);
}

static PCC_NODE *
pcc_cbranch(tp)
	TRIPLE *tp;
{
	TRIPLE *false_lab, *true_lab;
	PCC_NODE *p, *lp, *rp;

	lp = pcc_iropt_node(tp->left);
	true_lab = (TRIPLE*) tp->right; /* guess and ... */
	false_lab = true_lab->tnext;
	if(false_lab->right->leaf.val.const.c.i) { /* reverse if necessary */
		lp = pccfmt_unop(PCC_NOT, lp, PCC_INT);
	}
	rp = pccfmt_icon(false_lab->left->leaf.val.const.c.i, "", PCC_INT );
	p = pccfmt_binop(PCC_CBRANCH, lp, rp, PCC_INT);
	pccfmt_doexpr(source_file, source_lineno, p);
	pcc_branch(tp, true_lab->left->leaf.val.const.c.i);
	return (PCC_NODE*) NULL;
}

static PCC_NODE *
pcc_err(tp) 
	TRIPLE *tp;
{
	quita("pcc_err: >%s<", op_descr[ORD(tp->op)].name);
}

static PCC_NODE *
pcc_fval(tp)
	TRIPLE *tp;
{
	PCC_NODE *p, *lp;
	TYPE t;

	t = tp->left->operand.type;
	if(IR_ISCOMPLEX(t.tword) || t.tword == PCC_STRTY
	    || t.tword == PCC_UNIONTY ) {
		/*
		 * structured return value.  We get an rvalue,
		 * pcc wants an lvalue.
		 */
		if(tp->left->operand.tag == ISLEAF) {
			lp = pcc_addrof(tp->left);
		} else if( tp->left->operand.tag == ISTRIPLE && 
			tp->left->triple.op == IR_IFETCH) {
			lp = pcc_iropt_node(tp->left->triple.left);
		} else {
			quita("strange structured return value");
		}
	} else {
		/*
		 * normal return value.  What you see is
		 * what you get.
		 */
		lp = pcc_iropt_node(tp->left);
	}
#if TARGET == I386
	/*	rcc requires function returns to be of the form
	 *	ASSIGN RNODE expr, where expr is the function value
	 *	expression.   ?? about structures, but will leave
	 *	until G. Pollice resolves (RS) 7/2/87.
	 */
	p = pccfmt_binop(PCC_ASSIGN,
		pccfmt_special_node(PCC_RNODE, lp->in.type), 
		lp, lp->in.type);
#else
	p = pccfmt_unop(PCC_FORCE, lp, lp->in.type);
#endif
	pccfmt_doexpr(source_file, source_lineno,p);
	return (PCC_NODE*) NULL;
}


static PCC_NODE *
pcc_goto(tp)
	TRIPLE *tp;
{
	TRIPLE *labelref;
	long labelno;

	labelref = (TRIPLE*) tp->right;
	if(labelref->op != IR_LABELREF) {
		quita("pcc_goto: >%s<", op_descr[ORD(labelref->op)].name);
	}
	labelno = labelref->left->leaf.val.const.c.i;
	if (labelno == EXIT_LABELNO) {
		pccfmt_goto((long)PCC_EXITLAB);
	} else {
		pcc_branch(tp, labelno);
	}
	return (PCC_NODE*) NULL;
}

static PCC_NODE *
pcc_indirgoto(tp)
	TRIPLE *tp;
{
	PCC_NODE *p, *lp;

	lp = pcc_iropt_node(tp->left);
	p = pccfmt_indirgoto(lp, pcc_type(tp->left->operand.type));
	pccfmt_doexpr(source_file, source_lineno,p);
	return (PCC_NODE*) NULL;
}

static PCC_NODE *
pcc_label(tp)  
	TRIPLE *tp;
{
	LEAF *leafp;

	if((leafp = (LEAF*)tp->left) && leafp->val.const.c.i) {
		pccfmt_labeldef(leafp->val.const.c.i,"");
	}
	if (stmtprofflag && in_main_source) {
		bb_new_block = IR_TRUE;
	}
	return (PCC_NODE*) NULL;
}

static PCC_NODE *
pcc_pass(tp)
	TRIPLE *tp;
{
	pccfmt_pass(tp->left->leaf.val.const.c.cp);
	return (PCC_NODE*) NULL;
}

static PCC_NODE *
pcc_stmt(tp)
	TRIPLE *tp;
{
	source_file = tp->left->leaf.val.const.c.cp;
	source_lineno = tp->right->leaf.val.const.c.i;
	pccfmt_stmt(source_file, source_lineno);
	if (stmtprofflag && set_main_source(source_file)) {
		if(bb_new_block) {
			bb_incr_code();
			fprintf (dotd_fp, "%d 0\n", source_lineno);
			bb_new_block = IR_FALSE;
		}
	}
	return (PCC_NODE*) NULL;
}

static PCC_NODE *
pcc_foreff(tp)
	TRIPLE *tp;
{
	PCC_NODE *lp;

	/* evaluate left operand, discard result */
	lp = pcc_iropt_node(tp->left);
	pccfmt_doexpr(source_file, source_lineno,lp);
	return (PCC_NODE*) NULL;
}

static PCC_NODE *
pcc_repeat(tp)
	TRIPLE *tp;
{
	TYPE t;
	PCC_TWORD ctype;
	TRIPLE *loop_lab, *end_lab;
	PCC_NODE *p,*q;
#	if TARGET == MC68000
	    ADDRESS *ap;
	    char buff[BUFSIZ];
	    int regno;
#	endif

	t = tp->left->leaf.type;
	ctype = pcc_type(t);
	end_lab = (TRIPLE*) tp->right;
	loop_lab = end_lab->tnext;

#	if TARGET == MC68000
	    ap = &(tp->left->leaf.val.addr);
	    if( ap->seg->descr.class == DREG_SEG ) {
		    /* use a dbra */
		    regno = ap->offset;
		    sprintf(buff,"	dbra	d%d,L%d", regno,
			    loop_lab->left->leaf.val.const.c.i);
		    pccfmt_pass(buff);
		    if(t.tword == PCC_INT || t.tword == PCC_LONG ) {
			    /* test if there are any more iterations to do :
			    ** clear low word and subtract 1, and
			    ** if result is not negative, continue
			    */
			    sprintf(buff,"	clrw	d%d",regno); 
			    pccfmt_pass(buff);
			    sprintf(buff,"	subql	#0x1,d%d",regno); 
			    pccfmt_pass(buff);
			    sprintf(buff,"	jpl	L%d",
				    loop_lab->left->leaf.val.const.c.i);
			    pccfmt_pass(buff);
		    }
		    pcc_branch(tp, end_lab->left->leaf.val.const.c.i);
		    return (PCC_NODE *) (NULL);
	    }
#	endif
	/*
	 * rewrite as:
	 *	(CBRANCH, ((x -= 1) >= 0), loop);
	 *	(GOTO, endloop);
	 */
	p = pccfmt_binop(PCC_ASG_MINUS, 
		pcc_leaf((LEAF*)tp->left) , pcc_leaf(ileaf(1L)), ctype);
	p = pccfmt_binop((PCC_ISUNSIGNED(ctype) ? PCC_ULT : PCC_LT),
		p, pcc_leaf(ileaf(0L)), PCC_INT);
	q = pccfmt_icon(loop_lab->left->leaf.val.const.c.i, "", PCC_INT);
	p = pccfmt_binop(PCC_CBRANCH, p, q, PCC_INT);
	pccfmt_doexpr(source_file, source_lineno,p);
	pcc_branch(tp, end_lab->left->leaf.val.const.c.i);
	return (PCC_NODE *) (NULL);
}


static PCC_NODE *
pcc_switch(tp) 
	TRIPLE *tp;
{
	register i;
	register unsigned nlab;
	register TRIPLE *tp2;
	register struct pcc_sw *swtab;
	PCC_NODE *sel;		/* selector code tree */

	nlab = 0;
	TFOR(tp2,(TRIPLE*)tp->right) {
		nlab++;
	}
	swtab = (struct pcc_sw*)malloc(nlab*sizeof(struct pcc_sw));
	/* fill swtab from list based at tp->right */
	i = 0;
	TFOR(tp2,(TRIPLE*)tp->right) {
		swtab[i].slab = tp2->left->leaf.val.const.c.i;
		swtab[i].sval = tp2->right->leaf.val.const.c.i;
		i++;
	}
	sel = pcc_iropt_node(tp->left);
#if TARGET == I386
	/*	rcc switch expects a slightly different form of tree, namely
	 *	ASSIGN SNODE expr rather than expr.  (RS) 7/5/87
	 */
	sel = pccfmt_binop(PCC_ASSIGN,
		pccfmt_special_node(PCC_SNODE, PCC_INT),
		pccfmt_unop(SCONV, sel, PCC_INT),
		PCC_INT);
#else
	if (sel->in.type != PCC_INT) {
		sel = pccfmt_unop(PCC_SCONV, sel, PCC_INT);
	}
#endif
	pccfmt_switch( sel, swtab, (int)nlab );
	free((char*)swtab);
	return (PCC_NODE*) NULL;
}

static PCC_NODE *
pcc_unval(tp) 
	TRIPLE *tp;
{
	PCC_NODE *lp, *p;
	PCC_TWORD tword, ltword;

	tword = tp->type.tword;
	ltword = tp->left->operand.type.tword;
	/* check for: an operation on a complex value */
	/* or a conversion which yields a complex */
	if (IR_ISCOMPLEX(tword) || IR_ISCOMPLEX(ltword)) {
		/* check for conversion which does not change the type */
		if (tp->op == IR_CONV
		    && same_irtype(tp->type, tp->left->operand.type)) {
			p = pcc_iropt_node(tp->left);
			lp = p->in.left;
			pcc_freenode(p);
			return lp;
		} else {
			/* nontrivial conversion; must translate */
			return pcc_cx_unval(tp);
		}
	}
	lp = pcc_iropt_node(tp->left);
	p = pccfmt_unop(op_map_tab[(int)tp->op].pcc_op, 
		lp, pcc_type(tp->type));
	if (tword == IR_BITFIELD) {
		p = pccfmt_fld(p, tp->type.align, tp->type.size,
			pcc_type(tp->type));
	}
	return p;
}

static PCC_NODE *
pcc_reg(ap, tword)
	ADDRESS *ap;
	PCC_TWORD tword;
{
#	if TARGET == MC68000
	    /* 3 flavors of regs on the 68000 */
	    if( ap->seg == &seg_tab[DREG_SEGNO] ) {
		    return pccfmt_reg((int)ap->offset, tword);
	    }
	    if( ap->seg == &seg_tab[AREG_SEGNO] ) {
		    return pccfmt_reg((int)(AREGOFFSET+ap->offset), tword);
	    }
	    return pccfmt_reg((int)(FREGOFFSET+ap->offset), tword);
#	endif
#	if TARGET == SPARC
	    /* 2 flavors on sparc */
	    if( ap->seg == &seg_tab[DREG_SEGNO] ) {
		    return pccfmt_reg((int)ap->offset, tword);
	    }
	    return pccfmt_reg((int)(FREGOFFSET+ap->offset), tword);
#	endif
#	if TARGET == I386
	    /* so many kinds of registers on the 386, we don't
	       even attempt to classify them. */
	    return pccfmt_reg((int)ap->offset, tword);
#	endif
}

static PCC_NODE *
pcc_fcon(data, t)
	char *data;	/* unaligned ptr to binary data */
	PCC_TWORD t;
{
	union {
		long lval[2];
		float fval;
		double dval;
	} u;

	/* l->const->isbinary == IR_TRUE */
	/* t is PCC_FLOAT or PCC_DOUBLE */
	if (t == PCC_FLOAT) {
		u.lval[0] = l_align(data);
		return pccfmt_fcon((double)u.fval, t);
	} else {
		u.lval[0] = l_align(data);
		u.lval[1] = l_align(data+sizeof(long));
		return pccfmt_fcon(u.dval, t);
	}
}


static PCC_NODE *
pcc_leaf(l)
	LEAF *l;
{
	char buff[BUFSIZ];
	PCC_TWORD pcctword;
	register ADDRESS * ap;
	PCC_NODE *p, *lp, *rp;

	pcctword = pcc_type(l->type);
	ap= &l->val.addr;
	
	switch(l->class) {
	case VAR_LEAF:
		if(ap->seg->base == NOBASEREG) {
			if(ap->seg->descr.content == REG_SEG) {
				p = pcc_reg(ap, pcctword);
			} else if(ap->labelno != 0) {
				sprintf(buff,"v.%d",ap->labelno);
				p = pccfmt_name(ap->offset, buff, pcctword);
			} else {
				p = pccfmt_name(ap->offset, ap->seg->name,
					pcctword);
			}
		} else {
#if TARGET == I386
			/*	create a T, A, or P node when called for */
			int n_op = TAP(ap->seg->descr.class);
			if (n_op)
				p = pccfmt_TAP_node(n_op, addr_offset(ap),
					pcctword);
			else
				p = pccfmt_oreg(ap->seg->base, ap->offset, "",
					pcctword);
#else
			p = pccfmt_oreg(ap->seg->base,
				addr_offset(ap), "", pcctword);
#endif
		}
		if (l->type.tword == IR_BITFIELD) {
			p = pccfmt_fld(p, l->type.align, l->type.size,
				pcctword);
		}
		break;

	case CONST_LEAF:
		if(IR_ISINT(l->type.tword)) {
			p = pccfmt_icon(l->val.const.c.i,"",pcctword);
		} else if(IR_ISCHAR(l->type.tword)
		    && l->type.size <= 1) {
			if (pcctword == PCC_UCHAR) {
				unsigned char ucval = l->val.const.c.cp[0];
				p = pccfmt_icon((long)ucval, "", pcctword);
			} else {
				char cval = l->val.const.c.cp[0];
				p = pccfmt_icon((long)cval, "", pcctword);
			}
		} else if(l->type.tword == IR_LABELNO) {
#if TARGET == I386
			sprintf(buff,".L%d",l->val.const.c.i);
#else
			sprintf(buff,"L%d",l->val.const.c.i);
#endif
			p = pccfmt_icon(0L,buff,PCC_INT);
#if TARGET != I386
		/* rcc can't handle FCON's */
		} else if(PCC_ISFLOATING(pcctword)
		    && l->val.const.isbinary == IR_TRUE) {
			p = pcc_fcon(l->val.const.c.bytep[0], pcctword);
#endif
		} else if(IR_ISPTRFTN(l->type.tword)) {
			p = pccfmt_icon(0L, l->val.const.c.cp, pcctword);
		} else {
			if(l->visited ==  IR_FALSE) {
				endata_const(l);
			}
#if TARGET == I386
			sprintf(buff,".L%dD%d",hdr.procno,l->leafno);
#else
			sprintf(buff,"L%dD%d",hdr.procno,l->leafno);
#endif
			p = pccfmt_name(0L, buff, pcctword);
		}
		break;

	case ADDR_CONST_LEAF:
		if (l->type.tword == IR_STRING) {
			/* IR_STRING is mapped to PCC_CHAR */
			pcctword = PCC_INCREF(pcctword);
		}
		if(ap->seg->base == NOBASEREG) {
			if(ap->labelno != 0) {
				sprintf(buff,"v.%d",ap->labelno);
				p = pccfmt_icon(ap->offset, buff, pcctword);
			} else {
				sprintf(buff,"%s",ap->seg->name);
				p = pccfmt_icon(ap->offset, buff, pcctword);
			}
		} else {
#if TARGET == I386
                        /* WIN 11/23/87 */
			p  = pccfmt_TAP_node(VAUTO, addr_offset(ap), pcctword);
			p = pccfmt_unop(PCC_UNARY_AND, p, pcctword);

#else
			lp = pccfmt_reg(ap->seg->base, pcctword);
			rp = pccfmt_icon(addr_offset(ap), "", PCC_INT);
			p = pccfmt_binop(PCC_PLUS, lp, rp, pcctword);
#endif
		}
		break;

	default:
		quita("bad leaf >%d< in pcc_leaf",(int) l->class);
	}
	return p;
}

static PCC_TWORD
pcc_type(t)
	TYPE t;
{
	register PCC_TWORD btype;

	if(PCC_ISARY(t.tword)) {
		t.tword = PCC_DECREF(t.tword);
		t.tword = PCC_INCREF(t.tword);
	}
	btype = PCC_BTYPE(t.tword);
	if( !PCC_STDTYPE(btype) ) switch (btype) {
	case IR_DCOMPLEX:
	case IR_COMPLEX:
		btype= PCC_STRTY;
		break;

	case IR_LABELNO:
	case IR_BOOL:
		btype= PCC_INT;
		break;

	case IR_STRING:
		btype = PCC_CHAR;
		break;

	case IR_BITFIELD:
		btype = PCC_UNSIGNED;
		break;

	default:
		quita("pcc_type: unknown ir type %d", t.tword&PCC_BTMASK);
		break;
	}
	PCC_MODTYPE(t.tword,btype);
	return t.tword;
}

/*
 * Set/clear code generator modes on a per-triple basis.
 */
static void
pcc_flags(tp)
	TRIPLE *tp;
{
	if (tp->chk_ovflo) {
		pccfmt_chk_ovflo(tp->chk_ovflo? 1 : 0);
	}
}

# if TARGET == SPARC
/*
 * walk through the leaf table and generate code to store
 * all incoming parameters that need to be made addressable.
 * A parameter in the ARG_SEG, within the offset range
 *	ARGINIT..ARGINIT+INREGS*SZINT
 * needs to be stored from a register in the range %i0..%i5
 * if its leaf table entry has the value must_store_arg == TRUE
 * (meaning that the argument is assumed to be addressable by code
 * in the procedure body)
 */

# define INREGS 6	/* %i0..%i5 */
# define STORE_LBOUND   (ARGINIT/SZCHAR)
# define STORE_UBOUND	((ARGINIT+(INREGS*SZINT))/SZCHAR)
# define ADD_MEMBER(mask,r)  ((mask) |= (1<<(r)))
# define IS_MEMBER(mask,r)   ((mask) & (1<<(r)))

static void
store_params()
{
    LEAF *leafp;
    register OFFSZ offset;
    int size;
    int regno, argreg;
    PCC_TWORD tword;
    PCC_NODE *p, *q;
    unsigned storemask;

    storemask = 0;
    argreg = seg_tab[ARG_SEGNO].base;
    for(leafp= leaf_tab; leafp; leafp = leafp->next_leaf) {
	/* this is very bloody */
	if ((leafp->class == VAR_LEAF || leafp->class == ADDR_CONST_LEAF)
	  && leafp->val.addr.seg == &seg_tab[ARG_SEGNO]
	  && (offset=leafp->val.addr.offset) >= STORE_LBOUND
	  && offset < STORE_UBOUND ) {
	    /*
	     * parameter - may be in register or word-aligned memory
	     */
	    regno = RI0 + ((offset*SZCHAR)-ARGINIT)/SZINT;
	    if (leafp->must_store_arg == IR_TRUE) {
		/*
		 * memory-bound parameter -- must store from i-reg
		 */
		if (leafp->pass1_id != NULL
		  && strcmp(leafp->pass1_id, BUILTIN_VA_ALIST) == 0) {
		    /*
		     * VARARGS -- store all of regno..MAXINREG
		     */
		    while (regno <= MAXINREG) {
			ADD_MEMBER(storemask, regno);
			regno++;
		    }
		} else {
		    /*
		     * normal parameter -- just store one
		     */
		    size = leafp->type.size;
		    while (size > 0) {
			ADD_MEMBER(storemask, regno);
			size -= (SZINT/SZCHAR);
			regno++;
		    }
		}
	    } else {
		/*
		 * register parameter -- don't need to store,
		 * but must widen.  Sparc code generator assumes
		 * that small ints will be widened when they are
		 * loaded into registers.
		 */
		tword = pcc_type(leafp->type);
		switch(tword) {
		case PCC_CHAR:
		case PCC_UCHAR:
		case PCC_SHORT:
		case PCC_USHORT:
		    /*
		     * treat the register's current contents as an INT,
		     * and generate a *truncating* conversion to force
		     * sign extension or zero extension.
		     */
		    p = pccfmt_reg(regno, tword);
		    q = pccfmt_reg(regno, PCC_INT);
		    q = pccfmt_unop(PCC_SCONV, q, tword);
		    p = pccfmt_binop(PCC_ASSIGN, p, q, tword);
		    pccfmt_doexpr(source_file, source_lineno, p);
		    break;
		} /* switch */
	    } /* if reg param */
	} /* if arg leaf */
    } /* for each leaf */
    /*
     * generate code to store parameter registers
     */
    offset = STORE_LBOUND; 
    for (regno = RI0; regno < RI0+INREGS; regno++) {
	if (IS_MEMBER(storemask, regno)) {
	    p = pccfmt_oreg(argreg, offset, "" , PCC_INT);
	    q = pccfmt_reg(regno, PCC_INT);
	    p = pccfmt_binop(PCC_ASSIGN, p, q, PCC_INT);
	    pccfmt_doexpr(source_file, source_lineno, p);
	}
	offset += (SZINT/SZCHAR);
    }
}
# endif

/*
 * generate pcc tree for:
 *	if (!_bb_struct.initflag) {
 *	    _bb_init_func(&_bb_struct);
 *	}
 * This occurs at function entry, for statement profiling.
 * It is disabled for functions declared within #included
 * source files.
 */
static void
bb_init_code()
{
	PCC_TWORD t;
	register PCC_NODE *p, *q;
	long lab;

	if (bb_struct_label == 0) {
		bb_struct_label = p2getlab();
		bb_array_label = p2getlab();
	}
	/*
	 * if (!_bb_struct.initflag)
	 *	(note that initflag is the first field)
	 */
	p = pccfmt_labelref(bb_struct_label, "", PCC_INCREF(PCC_PTR|PCC_STRTY));
	p = pccfmt_unop(PCC_UNARY_MUL, p, PCC_INT);
	p = pccfmt_unop(PCC_NOT, p, PCC_INT);
	lab = p2getlab();
	p = pccfmt_binop(PCC_CBRANCH,
		p, pccfmt_icon(lab, "", PCC_INT), PCC_INT);
	pccfmt_doexpr(source_file, source_lineno, p);
	/*
	 *	_bb_init_func(&_bb_struct);
	 */
	t = PCC_INCREF(PCC_FTN|PCC_TVOID); /* PTR FTN VOID */
	p = pccfmt_labelref(bb_struct_label, "", (PCC_PTR|PCC_STRTY));
#if TARGET == I386
	q = pccfmt_icon(0L, "__bb_init_func", t);
#else
	q = pccfmt_icon(0L, "___bb_init_func", t);
#endif
	p = pccfmt_binop(PCC_CALL, q, p, PCC_TVOID);
#if TARGET == I386
			pccfmt_fixargs(p);
#endif
	pccfmt_doexpr(source_file, source_lineno, p);
	pccfmt_labeldef(lab, "");
}

/*
 * generate pcc tree for:
 *	bb_struct.counters[<basic block #>]++;
 * This occurs at the first statement of a basic block, for
 * statement profiling.  It is disabled for functions declared
 * within #included source files.
 */
static void
bb_incr_code()
{
	PCC_NODE *p;

	p = pccfmt_labelref(bb_array_label, "", (PCC_PTR|PCC_INT));
	p = pccfmt_binop(PCC_PLUS,
		p, pccfmt_icon((long)bbnum*4, "", PCC_INT), PCC_INT);
	p = pccfmt_unop(PCC_UNARY_MUL, p, PCC_INT);
	p = pccfmt_binop(PCC_ASG_PLUS,
		p, pccfmt_icon(1L, "", PCC_INT), PCC_INT);
	pccfmt_doexpr(source_file, source_lineno, p);
	bbnum++;
}

/*
 * auto storage allocation is defined by the
 * largest extent of any AUTO_SEG segment.
 */
static long
auto_size()
{
	SEGMENT *seg;
	long maxextent, extent;

	maxextent = 0;
	for (seg = seg_tab; seg != NULL; seg = seg->next_seg) {
		if (seg->descr.class == AUTO_SEG) {
#ifdef BACKAUTO
			extent = -seg->offset;
#else
			extent = seg->offset + seg->len;
#endif
			if (extent > maxextent) {
				maxextent = extent;
			}
		}
	}
	return maxextent;
}

map_to_pcc()
{
	register BLOCK *bp;
	char name[BUFSIZ];
	register TRIPLE *tp;
	long frame_size;
	TYPE t;
	int nentries;

	bss_seg(&seg_tab[BSS_SMALL_SEGNO]);
	bss_seg(&seg_tab[BSS_LARGE_SEGNO]);
	/*
	 * the directive
	 *	|#PROC# 0<octal number>
	 * describes the type of the current function to c2.  This
	 * is used to determine which registers are live on exit.
	 * The register set is derived by the peephole optimizer,
	 * which, as a consequence, must know about the PCC type
	 * system.
	 *
	 * Note that because we associate a function with a unique type,
	 * we have problems with FORTRAN multiple entry points when the
	 * entry point types are different.  If we have multiple entry
	 * points, then we represent the type as UNDEF, and expect the
	 * code generators and peephole optimizers to consider all of
	 * the function result registers live on exit.
	 *
	 * See also:
	 *	pcc/{sparc,m68k,i386}: eobl2()
	 *	as/sparc: opt_regsets.c
	 *	c2/m68k:  main.c
	 * Future bug:
	 *	multiple entry points + types requiring the
	 *	code generator's structure return protocol
	 *	[this combination not used at present]
	 *
	 * 1e+308 on the vomit meter.
	 */
	nentries = 0;
	t = hdr.proc_type;
	if (t.tword == PCC_UNDEF) {
		t.tword = PCC_TVOID;
	}
	for(bp=entry_block ; bp ; bp = bp->next) {
		if(bp->is_ext_entry) {
			nentries++;
			if (nentries > 1) {
				t.tword = PCC_UNDEF;
				break;
			}
		}
	}
	set_locctr(Text);
#	if TARGET == MC68000
	    sprintf(name, "\t.proc\n|#PROC# %#o", pcc_type(t));
#	endif
#       if TARGET == I386
	    sprintf(name, "/#PROC# %#o", pcc_type(t));
#	endif
#	if TARGET == SPARC
	    sprintf(name, "\t.proc\t%#o", pcc_type(t));
#	endif

#if TARGET != I386
	pccfmt_pass(name);
#endif
	for(bp=entry_block ; bp ; bp = bp->next) {
		if(bp->labelno <= 0) { /*unreachable code */
			continue;
		}
		if(bp->entryname && *(bp->entryname) != '\0') {
			if (bp->entry_is_global) {
#			    if TARGET == MC68000 || TARGET == I386
				sprintf(name,"\t.globl\t%s\n",bp->entryname);
#			    endif
#			    if TARGET == SPARC
				sprintf(name,"\t.global\t%s\n",bp->entryname);
#			    endif
			    pccfmt_pass(name);
			}
			pccfmt_labeldef(0L, bp->entryname);
		}
		if(bp->is_ext_entry) { /* put out an lbrac */
			frame_size = auto_size()*SZCHAR;
			SETOFF(frame_size, ALSTACK);
#			if TARGET == MC68000 || TARGET == I386
			    pccfmt_lbrac(hdr.procno, hdr.regmask, frame_size,
				0);
#			endif
#			if TARGET == SPARC
			    pccfmt_lbrac(hdr.procno, hdr.regmask, frame_size,
				hdr.fregmask);
			    store_params();
#			endif
			source_file = hdr.source_file;
			source_lineno = hdr.source_lineno;
			if (stmtprofflag && set_main_source(source_file)) {
				bb_init_code();
				bb_new_block = IR_TRUE;
			}
		}
		if(bp->last_triple) {
			curblock = bp;
			TFOR(tp,bp->last_triple->tnext) {
				/*
				 * IR_STMT and IR_PASS are not ROOT_OPs,
				 * because iropt expects a ROOT_OP to have
				 * real operands on the branches.  So we
				 * have to special case them here.
				 */
				if (tp->op == IR_STMT) {
					pcc_stmt(tp);
				} else if (tp->op == IR_PASS) {
					pcc_pass(tp);
				} else if( ISOP(tp->op,ROOT_OP) ) {
					(void)pcc_iropt_node((IR_NODE*)tp);
				}
			}
		}
	}
	pccfmt_rbrac(t.tword, (long)t.size, t.align, hdr.opt_level);
	bb_new_block = IR_FALSE;
}

/*
 * set_main_source(source) returns TRUE if we are in the main
 *	main source file (as opposed to an included file).
 *	We assume the main source file has the same 'basename'
 *	as the .d filename passed in with the -A<filename> option.
 *	Yes, this a kludge.
 */
static BOOLEAN
set_main_source(source)
	register char *source;
{
	extern char *rindex();		/* rindex(name,'/') => basename */
	extern char *index();		/* index(basename,'.') => suffix */
	char *source_basename;		/* basename of source file name */
	char *suffix;			/* filename suffix */
	register int len;		/* length of basename */

	in_main_source = IR_FALSE;
	if (source == NULL) {
		return IR_FALSE;
	}
	len = strlen(source);
	if (len >= 2 && source[0] == '"' && source[len-1] == '"') {
		/* strip quotes from source, if there are any */
		len -= 2;
		source++;
	}
	if (len > 0) {
		/* have non-trivial source filename */
		source_basename = rindex(source, '/');
		if (source_basename == NULL) {
			source_basename = source;
		} else {
			source_basename++;
		}
		suffix = index(source_basename, '.');
		if (suffix == NULL) {
			/* no '.' suffix, look at everything after the '/' */
			len -= (source_basename - source);
		} else {
			/* look at substring between '/' and '.' */
			len = suffix - source_basename;
		}
		if (strncmp(source_basename, dotd_basename, len) == 0) {
			/* basename(source)matches basename(dotd file) */
			in_main_source = IR_TRUE;
		}
	}
	return in_main_source;
}


/*
 * return yes, if the p->op is one of the C language binary ops that
 * can turn into the IR_ASSIGN and op, such as +, -, | etc.
 */
static BOOLEAN
is_c_special_op( p )
	TRIPLE *p;
{ 
	switch(p->op) {
	case IR_PLUS:
	case IR_MINUS:
	case IR_MULT:
	case IR_REMAINDER:
	case IR_DIV:
	case IR_AND:
	case IR_OR:
	case IR_XOR:
	case IR_LSHIFT:
	case IR_RSHIFT:
		if( IR_ISCOMPLEX(p->type.tword) ) {
			return IR_FALSE;
		}
		return( IR_TRUE );
	}
	/* else */
	return( IR_FALSE );
}

/* return log base 2 of n if n a power of 2; otherwise -1 */
static int
log2(n) long n;
{
	int k;

	/* trick based on binary representation */

	if(n<=0 || (n & (n-1))!=0)
		return(-1);

	for(k = 0 ;  n >>= 1  ; ++k) {}

	return(k);
}

static BOOLEAN
is_same_tree( op, left, right )
	register IR_OP op;
	register IR_NODE *left;
	register TRIPLE *right;
{
	register IR_NODE *operand1, *operand2;
	
	operand1 = right->left;
	operand2 = right->right;
	
	if( op == IR_ASSIGN && left->operand.tag == ISLEAF ) {
		if( left == operand1 ) {
			return IR_TRUE;
		}
		if(ISOP(right->op, COMMUTE_OP) && left == operand2) {
			right->right = operand1;
			right->left = operand2;
			return IR_TRUE;
		}
		return IR_FALSE;
	}

	if( op == IR_ISTORE ) {
		if( operand1->operand.tag == ISTRIPLE
		    && ((TRIPLE *)operand1)->op == IR_IFETCH
		    && left == ((TRIPLE *)operand1)->left ) {
			return IR_TRUE;
		}
		if(ISOP(right->op, COMMUTE_OP)
		    && operand2->operand.tag == ISTRIPLE
		    && ((TRIPLE *)operand2)->op == IR_IFETCH
		    && left == ((TRIPLE *)operand2)->left ) {
			right->right = operand1;
			right->left = operand2;
			return IR_TRUE;
		}
	}
	return IR_FALSE;
}

static PCC_NODE *
pcc_cx_unval(tp)
	TRIPLE *tp;
{
	TYPE from_type, to_type;
	char from_char, to_char;
	PCC_TWORD pcc_fntype, pcc_totype;
	char *opname;
	PCC_NODE *p, *lp, *rp, *tmp;
	char  name[BUFSIZ];

	to_type = tp->type;
	from_type = tp->left->operand.type;
	/*
	 * construct the name of the called library routine
	 */
	if(tp->op == IR_CONV) {
		switch(to_type.tword) {
		case PCC_SHORT:
		case PCC_LONG:
		case PCC_INT:
			to_char = 'i';
			break;
		case PCC_FLOAT:
			to_char = 'f';
			break;
		case PCC_DOUBLE:
			to_char = 'd';
			break;
		case IR_COMPLEX:
			to_char = 'c';
			break;
		case IR_DCOMPLEX:
			to_char = 'z';
			break;
		default :
			quit("pcc_cx_unval: complex conversion, bad to type");
			break;
		}
		switch(from_type.tword) {
		case PCC_SHORT:
		case PCC_LONG:
		case PCC_INT:
			from_char = 'i';
			break;
		case PCC_FLOAT:
			from_char = 'f';
			break;
		case PCC_DOUBLE:
			from_char = 'd';
			break;
		case IR_COMPLEX:
			from_char = 'c';
			break;
		case IR_DCOMPLEX:
			from_char = 'z';
			break;
		default:
			quit("pcc_cx_unval: complex conversion, bad from type");
			break;
		}
# if TARGET != I386
		sprintf(name, "__F%c_conv_%c", from_char, to_char);
# else
		sprintf(name, "_F%c_conv_%c", from_char, to_char);
# endif
	} else {
		opname = op_map_tab[(int) tp->op ].implement_complex;
		if( opname == NULL || opname[0] == '\0' ) {
			quita("pcc_cx_unval: bad op '%s'",
				op_descr[ORD(tp->op)].name);
		}
#if TARGET == I386
		sprintf(name , "_F%c_%s",
#else
		sprintf(name , "__F%c_%s",
#endif TARGET == I386
			(to_type.tword == IR_COMPLEX ? 'c' : 'z'), opname);
	}
	/*
	 * if result is complex, translate as
	 *	(libcall(&tmp,lhs),&tmp)
	 * else translate as
	 *	(libcall(lhs))
	 */
	pcc_fntype = pcc_type(to_type)|PCC_FTN;
	lp = pccfmt_icon(0L, name, PCC_INCREF(pcc_fntype));
	if (IR_ISCOMPLEX(from_type.tword)) {
		rp = pcc_cx_param(tp->left);
	} else {
		rp = pcc_iropt_node(tp->left);
	}
	pcc_totype = pcc_type(to_type);
	if (IR_ISCOMPLEX(to_type.tword)) {
		/* (libcall(&tmp,lhs),&tmp) */
		tmp = pccfmt_tmp(to_type.size/sizeof(int), pcc_totype);
		rp = pccfmt_binop(PCC_LISTOP, pccfmt_addroftmp(tmp),
			rp, PCC_INCREF(pcc_totype));
		lp  = pccfmt_binop(PCC_CALL, lp, rp, PCC_TVOID );
#if TARGET == I386
			pccfmt_fixargs(lp);
#endif
		p = pccfmt_binop(PCC_COMOP, lp, 
			pccfmt_addroftmp(tmp), PCC_INCREF(pcc_totype));
		pcc_tfree(tmp);
	} else {
		/* libcall(lhs) */
		p = pccfmt_binop(PCC_CALL, lp, rp, pcc_totype);
#if TARGET == I386
			pccfmt_fixargs(p);
#endif
	}
	return p;
}

static PCC_NODE *
pcc_intr_call(tp)
	TRIPLE *tp;
{
	char *fname;
	TRIPLE *param;
	PCC_NODE *lp, *p;	/* for building unary operator trees */
	PCC_NODE *value, *lbound, *rbound;	/* for building CHK trees*/
	int pcc_op;
	PCC_TWORD tword;

	param = (TRIPLE*) tp->right;
	fname = tp->left->leaf.val.const.c.cp;
	tword = pcc_type(tp->type);
#if TARGET == I386
        /*      no leading underscore */
        pcc_op = is_intrinsic(fname);
#else
        /*      give is_instrinsic cp+1 to strip the underscore */
	pcc_op = is_intrinsic(fname+1);
#endif
	switch(pcc_op) {
	case -1:
		quita("pcc_intr_call: >%s<", fname);
		/*NOTREACHED*/
	case PCC_CHK:
		/*
		 * pascal/modula-2 range check operator
		 */
		value = pcc_iropt_node(param->left);
		param = param->tnext;
		lbound = pcc_iropt_node(param->left);
		param = param->tnext;
		rbound = pcc_iropt_node(param->left);
		p = pccfmt_binop(PCC_LISTOP, lbound, rbound, tword);
		p = pccfmt_binop(PCC_CHK, value, p, tword);
		break;
	default:
		lp = pcc_iropt_node(param->left);
		p = pccfmt_unop(pcc_op, lp, tword);
		break;
	}
	return p;
}

/*
 * the following routines are an attempt to localize the
 * details of the pseudo-ops of the target assemblers.
 */

static void
set_locctr(c)
	CSECT c;
{
	char *pseudo;
	char line[BUFSIZ];
#	if TARGET == MC68000 || TARGET == I386
	    switch(c) {
	    case Text:	pseudo = ".text"; break;
	    case Data:	pseudo = ".data"; break;
	    case Bss:	pseudo = ".bss"; break;
	    }
#	endif
#	if TARGET == SPARC
	    switch(c) {
	    case Text:	pseudo = ".seg	\"text\""; break;
	    case Data:	pseudo = ".seg	\"data\""; break;
	    case Bss:	pseudo = ".seg	\"bss\""; break;
	    }
#	endif
	sprintf(line,"	%s", pseudo);
	pccfmt_pass(line);
}

/*
 * name of a storage allocation pseudo-op
 */
static char*
storage_pseudo_op(size)
{
	switch(size) {
#	if TARGET == MC68000
	    case 1: return ".byte";
	    case 2: return ".word";
	    case 4: return ".long";
	    default: quita("unexpected size (%d) in storage_pseudo_op", size);
#	endif
#	if TARGET == SPARC
	    case 1: return ".byte";
	    case 2: return ".half";
	    case 4: return ".word";
	    default: quita("unexpected size (%d) in storage_pseudo_op", size);
#	endif
#	if TARGET == I386
	    case 1: return ".byte";
	    case 2: return ".value";
	    case 4: return ".long";
	    default: quita("unexpected size (%d) in storage_pseudo_op", size);
#	endif
	/*NOTREACHED*/
	}
}

/*
 * set alignment.
 */
static void
set_align(align)
{
	char pseudo[BUFSIZ];

	switch(align) {
	case 1:
		return;
	case 2:
	case 4:
	case 8:
		sprintf(pseudo,"	.align	%d", align);
		break;
	default:
		quita("illegal alignment: 0x%x\n", align);
	}
	pccfmt_pass(pseudo);
}

/*
 * integer data statement, value is binary
 */
static void
int_data_stmt(size,value)
	int size;
	long value;
{
	char line[BUFSIZ];
	sprintf(line,"	%s	0x%lx", storage_pseudo_op(size), value);
	pccfmt_pass(line);
}

/*
 * floating point data statement, value is encoded as ascii string
 */
static void
float_data_stmt(size, value)
	int size;
	char *value;
{
	char line[BUFSIZ];
	char *pseudo;
#if TARGET == I386
	char *format;

	pseudo = (size == SZFLOAT/SZCHAR ? ".float" : ".double");
	if (*value == '+')	/* assembler hates +xxx */
		value++;
	if (*value == '.')
		format = "	%s	0%s";	/* assembler requires leading 0 for fractions
								*/
	else if (*value == '-' && value[1] == '.'){
		format = "	%s	-0%s";
		value++;
	} else
		format = "	%s	%s";
	sprintf(line, format, pseudo, value);
#else
	pseudo = (size == SZFLOAT/SZCHAR ? ".single" : ".double");
	sprintf(line, "	%s	0R%s", pseudo, value);
#endif
	pccfmt_pass(line);
}


/*
 * string_data_stmt(string): put out a string constant as
 *	a series of .byte directives in the assembler stream.
 */
static void
string_data_stmt(string)
	char *string;
{
	char strbuf[80];
	char c;

	while( (c = *string++) != '\0' ) {
		sprintf(strbuf, "\t.byte\t0x%x", (unsigned char)c);
		pccfmt_pass(strbuf);
	}
	/* null-terminate the string */
	sprintf(strbuf, "\t.byte\t0x%x", 0);
	pccfmt_pass(strbuf);
}


/*
**  add a constant to data segment so it's possible to refer to its
**  address by label
*/
static void
endata_const(leafp)
LEAF *leafp;
{
	char line[BUFSIZ];
	struct constant *const;

	set_locctr(Data);
	if (leafp->type.tword == PCC_DOUBLE
	  || leafp->type.tword == IR_COMPLEX
	  || leafp->type.tword == IR_DCOMPLEX) {
	    set_align(ALDOUBLE/SZCHAR);
	} else {
	    set_align(leafp->type.align);
	}
#if TARGET == I386
	sprintf(line, ".L%dD%d:", hdr.procno, leafp->leafno);
# else
	sprintf(line, "L%dD%d:", hdr.procno, leafp->leafno);
# endif
	pccfmt_pass(line);
	const = &leafp->val.const;
	switch(PCC_BTYPE(leafp->type.tword)) {

	case PCC_CHAR:
	case PCC_UCHAR:
	case IR_STRING:
		string_data_stmt(const->c.cp);
		break;

	case PCC_LONG:
	case PCC_ULONG:
	case PCC_INT:
	case PCC_UNSIGNED:
	case PCC_SHORT:
	case PCC_USHORT:
	case IR_BOOL:
		int_data_stmt(leafp->type.size, const->c.i);
		break;

	case PCC_FLOAT:	/* use bit represenatation */
		if(const->isbinary == IR_TRUE) {
			int_data_stmt((int)leafp->type.size,
				l_align(const->c.bytep[0]));
		} else {
			float_data_stmt(leafp->type.size, const->c.fp[0]);
		}
		break;

	case PCC_DOUBLE:
		if(const->isbinary == IR_TRUE) {
			int_data_stmt((int)(leafp->type.size/2),
				l_align(const->c.bytep[0]));
			int_data_stmt((int)(leafp->type.size/2),
				l_align(const->c.bytep[0]+4));
		} else {
			float_data_stmt((int)leafp->type.size,
				const->c.fp[0]);
		}
		break;

	case IR_COMPLEX:	
		if(const->isbinary == IR_TRUE) {
			int_data_stmt((int)(leafp->type.size/2),
				l_align(const->c.bytep[0]));
			int_data_stmt((int)(leafp->type.size/2),
				l_align(const->c.bytep[1]));
		} else {
			float_data_stmt((int)(leafp->type.size/2),
				const->c.fp[0]);
			float_data_stmt((int)(leafp->type.size/2),
				const->c.fp[1]);
		}
		break;

	case IR_DCOMPLEX:	
		if(const->isbinary == IR_TRUE) {
			int_data_stmt((int)(leafp->type.size/4),
				l_align(const->c.bytep[0]));
			int_data_stmt((int)(leafp->type.size/4),
				l_align(const->c.bytep[0]+4));
			int_data_stmt((int)(leafp->type.size/4),
				l_align(const->c.bytep[1]));
			int_data_stmt((int)(leafp->type.size/4),
				l_align(const->c.bytep[1]+4));
		} else {
			float_data_stmt((int)(leafp->type.size/2),
				const->c.fp[0]);
			float_data_stmt((int)(leafp->type.size/2),
				const->c.fp[1]);
		}
		break;

	default:
		quita("endata_const: unknown type (0x%x)",
			PCC_BTYPE(leafp->type.tword) );
	}
	set_locctr(Text);
	leafp->visited = IR_TRUE;
}

/*
 * allocate storage for a BSS segment.
 * Assume worst case alignment.
 */
static void
bss_seg(segp)
	SEGMENT *segp;
{
	char line[BUFSIZ];

	if (segp->len > 0) {
		set_locctr(Bss);
#		if TARGET == MC68000
		    set_align(SZLONG/SZCHAR);
		    sprintf(line,"%s:\t.skip\t%d", segp->name, segp->len);
#		endif
#		if TARGET == SPARC
		    set_align(SZDOUBLE/SZCHAR);
		    sprintf(line,"\t.reserve\t%s,%d,\"bss\"", segp->name,
			segp->len);
#		endif
#		if TARGET == I386
		    set_align(SZLONG/SZCHAR);
		    sprintf(line,"%s:\t.set\t.,.+%d", segp->name, segp->len);
#		endif
		pccfmt_pass(line);
	}
}

static void
label_data_stmt(labelnum, size)
	long labelnum;
	int size;
{
	char line[BUFSIZ];
	char label[100];

	sprintf(label, LABFMT, labelnum);
	sprintf(line, "\t%s\t%s", storage_pseudo_op(size), label);
	pccfmt_pass(line);
}

/*
 * At end-of-file when compiling with the -a option,
 * define an initialized bblink structure.  Must match:
 *
 *	struct bblink {
 *	    int		initflag;
 *	    char	*filename;
 *	    unsigned	*counters;
 *	    int		ncntrs;
 *	    struct	bblink	*next;
 *	} bb_struct = {
 *	    0,
 *	    <.d filename>,
 *	    <&array of basic block counters>
 *	    <number of basic blocks>
 *	    NULL,
 *	};
 *
 * This is really sleazy, but the old, nice way allocated
 * storage dynamically, which didn't work with programs that
 * use sbrk(), notably /usr/lib/iropt.
 *
 * 1e+308 on the vomit meter.
 */

define_bb_struct()
{
	char line[BUFSIZ];
	char label[100];
	long dotd_filename_label;

	set_locctr(Data);
	set_align(SZINT/SZCHAR);
	pccfmt_labeldef(bb_struct_label, "");
	/*
	 * int initflag = 0;
	 */
	int_data_stmt(SZINT/SZCHAR, 0L);
	/*
	 * char	*filename = <dotd file name>;
	 */
	dotd_filename_label = p2getlab();
	label_data_stmt(dotd_filename_label, SZPOINT/SZCHAR);
	/*
	 * unsigned *counters = bb_array;
	 */
	label_data_stmt(bb_array_label, SZPOINT/SZCHAR);
	/*
	 * int ncntrs = <number of basic blocks>
	 */
	int_data_stmt(SZINT/SZCHAR, (long)bbnum);
	/*
	 * struct bblink *next = NULL;
	 */
	int_data_stmt(SZINT/SZCHAR, 0L);
	/*
	 * generate the .d filename string characters
	 */
	pccfmt_labeldef(dotd_filename_label, "");
	string_data_stmt(dotd_filename);
	/*
	 * allocate static storage for basic block counters
	 */
	set_locctr(Bss);
	set_align(SZINT/SZCHAR);
	sprintf(label, LABFMT, bb_array_label);
#	if TARGET == MC68000
	    sprintf(line, "\t.lcomm\t%s,%d", label, bbnum*(SZINT/SZCHAR));
#	endif
#	if TARGET == SPARC
	    sprintf(line, "%s:\t.skip\t%d", label, bbnum*(SZINT/SZCHAR));
#	endif
#	if TARGET == I386
	    sprintf(line, "%s:\t.set\t.,.+%d", label, bbnum*(SZINT/SZCHAR));
#	endif
	pccfmt_pass(line);
}

stmtprof_eof()
{
	if (bb_struct_label != 0) {
		define_bb_struct();
	}
	fclose(dotd_fp);	/* avoid chicken-and-egg problem */
}

#ifndef lint
static	char sccsid[] = "@(#)rewrite.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include    "cg_ir.h"
#include    "pcc_ops.h"
#include    "pcc_types.h"
#include    "pcc_node.h"
#include    <stdio.h>

#define ARG1 arg_v[0]
#define ARG2 arg_v[1]
#define ARG3 arg_v[2]
#define ARG4 arg_v[3]
extern int source_lineno;
extern char *source_file;
extern void pccfmt_doexpr();
LOCAL BOOLEAN rewrite_string_op();

BOOLEAN
rewrite_lib_call(tp, q)
TRIPLE *tp;
PCC_NODE **q;
{
register LEAF *func;
register char *fname;


	func= (LEAF*) tp->left;
	fname = func->val.const.c.cp;
	*q = (PCC_NODE*) NULL;

#if (TARGET == I386) 
	if(strncmp(fname, "s_copy",6) == 0) {
		return rewrite_string_op(tp, PCC_ASSIGN, q);
	}

	if(strncmp(fname, "s_cmp",5) == 0) {
		return rewrite_string_op(tp, PCC_MINUS, q);
	}
#else
	if(strncmp(fname, "_s_copy",7) == 0) {
		return rewrite_string_op(tp, PCC_ASSIGN, q);
	}

	if(strncmp(fname, "_s_cmp",6) == 0) {
		return rewrite_string_op(tp, PCC_MINUS, q);
	}
#endif (TARGET == I386) 
	return IR_FALSE;
}


LOCAL BOOLEAN
rewrite_string_op(tp,op, q)
TRIPLE *tp;
int op;
PCC_NODE **q;
{
IR_NODE *(arg_v[4]), **arg_vp = arg_v;
TRIPLE *args, *param;
PCC_NODE *p, *rp, *lp, *pcc_iropt_node(), *pccfmt_unop(), *pccfmt_binop();

	args = (TRIPLE*) tp->right;
	TFOR(param,  args->tnext) {
		*arg_vp++ = param->left;
	}
	if(	ARG4->operand.tag == ISLEAF && 
		ARG3->operand.tag == ISLEAF && 
		((LEAF*)ARG4)->class == CONST_LEAF &&
		((LEAF*)ARG3)->class == CONST_LEAF &&
		((LEAF*)ARG4)->val.const.c.i == 1  &&
		((LEAF*)ARG3)->val.const.c.i == 1
	) {
		lp = pcc_iropt_node(ARG1);
		lp =  pccfmt_unop(PCC_UNARY_MUL, lp, PCC_CHAR );
		rp = pcc_iropt_node(ARG2);
		rp =  pccfmt_unop(PCC_UNARY_MUL, rp, PCC_CHAR );
		p = pccfmt_binop(op, lp, rp, PCC_CHAR);
		if(tp->op == IR_SCALL) {
			pccfmt_doexpr(source_file, source_lineno,p);
		} else {
			*q = p;
		}
		return IR_TRUE;
	} else {
		return IR_FALSE;
	}
}

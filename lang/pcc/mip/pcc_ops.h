/* @(#)pcc_ops.h 1.1 94/10/31 SMI */

/*
 * Operator encodings of the Portable C compiler
 * (pcc trees) intermediate code.  Operators marked
 * to the back end.
 */

# ifndef _pcc_ops_
# define _pcc_ops_

# ifndef YACC
  /*
   * these operators are actually defined by
   * %term and %left directives in cgram.y.
   * Infinity on the vomit meter.
   */
# define PCC_ERROR	1
# define PCC_NAME	2
# define PCC_STRING	3
# define PCC_ICON	4
# define PCC_FCON	5
# define PCC_PLUS	6
# define PCC_MINUS	8	/* also unary == PCC_NEG */
# define PCC_MUL	11	/* also unary == PCC_INDIRECT */
# define PCC_AND	14	/* also unary == PCC_ADDROF */
# define PCC_OR		17
# define PCC_ER		19
# define PCC_QUEST	21
# define PCC_COLON	22
# define PCC_ANDAND	23
# define PCC_OROR	24
				/* 25-36 used as yacc tokens */
# define PCC_GOTO	37
				/* 38-55 used as yacc tokens */
# define PCC_CM		56
				/* 57 used as yacc token */
# define PCC_ASSIGN	58
# endif YACC

# define PCC_COMOP	59
# define PCC_DIV	60
# define PCC_MOD	62
# define PCC_LS		64
# define PCC_RS		66
				/* 68-69 defined in mip.h */
# define PCC_CALL	70	/* also unary */
				/* UNUSED		71 */
# define PCC_FORTCALL	73	/* also unary */
				/* UNUSED		74 */
# define PCC_NOT	76
# define PCC_COMPL	77
# define PCC_INCR	78
# define PCC_DECR	79
# define PCC_EQ		80
# define PCC_NE		81
# define PCC_LE		82
# define PCC_LT		83
# define PCC_GE		84
# define PCC_GT		85
# define PCC_ULE	86
# define PCC_ULT	87
# define PCC_UGE	88
# define PCC_UGT	89
				/* UNUSED		90 */
				/* UNUSED		91 */
				/* UNUSED		92 */
				/* UNUSED		93 */
# define PCC_REG	94
# define PCC_OREG	95
				/* 96-97 defined in mip.h */
# define PCC_STASG	98
# define PCC_STARG	99
# define PCC_STCALL	100	/* also unary */
				/* UNUSED		101 */

/* unary ops */

# define PCC_UNARY_MUL		(PCC_UNARY PCC_MUL)
# define PCC_UNARY_MINUS	(PCC_UNARY PCC_MINUS)
# define PCC_UNARY_AND		(PCC_UNARY PCC_AND)
# define PCC_UNARY_CALL		(PCC_UNARY PCC_CALL)
# define PCC_UNARY_STCALL	(PCC_UNARY PCC_STCALL)
# define PCC_UNARY_FORTCALL	(PCC_UNARY PCC_FORTCALL)

/* assign ops */

# define PCC_ASG_PLUS	(PCC_ASG PCC_PLUS)
# define PCC_ASG_MINUS	(PCC_ASG PCC_MINUS)
# define PCC_ASG_MUL	(PCC_ASG PCC_MUL)
# define PCC_ASG_DIV	(PCC_ASG PCC_DIV)
# define PCC_ASG_MOD	(PCC_ASG PCC_MOD)
# define PCC_ASG_AND	(PCC_ASG PCC_AND)
# define PCC_ASG_OR	(PCC_ASG PCC_OR)
# define PCC_ASG_ER	(PCC_ASG PCC_ER)
# define PCC_ASG_LS	(PCC_ASG PCC_LS)
# define PCC_ASG_RS	(PCC_ASG PCC_RS)

/* lists */

# define PCC_LISTOP	PCC_CM

/*
 * conversion operators
 */

# define PCC_FLD	103
# define PCC_SCONV	104
# define PCC_PCONV	105

/*
 * special node operators
 */

				/* 106-107 defined in mip.h */
# define PCC_FORCE	108	/* leave result in function result register */
# define PCC_CBRANCH	109
# define PCC_INIT	110
				/* 111-113 defined in mip.h */
# define PCC_CHK	114

/*
 *  floating point intrinsics
 */

# define PCC_FABS	115	/*  y = abs(x) */
# define PCC_FCOS	116	/*  y = cos(x) */
# define PCC_FSIN	117	/*  y = sin(x) */
# define PCC_FTAN	118	/*  y = tan(x) */
# define PCC_FACOS	119	/*  y = arccos(x) */
# define PCC_FASIN	120	/*  y = arcsin(x) */
# define PCC_FATAN	121	/*  y = arctan(x) */
# define PCC_FCOSH	122	/*  y = cosh(x) */
# define PCC_FSINH	123	/*  y = sinh(x) */
# define PCC_FTANH	124	/*  y = tanh(x) */
# define PCC_FEXP	125	/*  y = e**x */
# define PCC_F10TOX	126	/*  y =	10**x */
# define PCC_F2TOX	127	/*  y = 2**x */
# define PCC_FLOGN	128	/*  y = log_base_e(x) */
# define PCC_FLOG10	129	/*  y = log_base_10(x) */
# define PCC_FLOG2	130	/*  y = log_base_2(x) */
# define PCC_FSQR	131	/*  y = sqr(x) */
# define PCC_FSQRT	132	/*  y = sqrt(x) */
# define PCC_FAINT	133	/*  y = aint(x) */
# define PCC_FANINT	134	/*  y = anint(x) */
# define PCC_FNINT	135	/*  n = nint(x) (biased round towards 0) */
				/*  UNUSED 136-147 */
				/*  148-149 defined in mip.h */
# define PCC_LAST	149
# define PCC_DSIZE	(PCC_LAST+1)

/*
 *  prefix unary operator modifier
 */

# define PCC_ASG	1+
# define PCC_UNARY	2+
# define PCC_NOASG	(-1)+
# define PCC_NOUNARY	(-2)+

# define PCC_EXITLAB	(-1)	/* EXITLAB means branch to procedure exit */

/*
 * operator types (arity)
 */

# define PCC_LTYPE	002
# define PCC_UTYPE	004
# define PCC_BITYPE	010
# define PCC_TYFLG	016

/*
 * other operator attributes
 */

# define PCC_ASGFLG	01
# define PCC_LOGFLG	020
# define PCC_SIMPFLG	040
# define PCC_COMMFLG	0100
# define PCC_DIVFLG	0200
# define PCC_FLOFLG	0400
# define PCC_LTYFLG	01000	/* not used in pcc_dope[] */
# define PCC_CALLFLG	02000
# define PCC_MULFLG	04000
# define PCC_SHFFLG	010000
# define PCC_ASGOPFLG	020000

# define PCC_SPFLG	040000
# define PCC_INTRFLG	0100000
# define PCC_LIBRFLG	0200000

# define pcc_optype(o)	  (pcc_dope[o]&PCC_TYFLG)
# define pcc_asgop(o)	  (pcc_dope[o]&PCC_ASGFLG)
# define pcc_logop(o)	  (pcc_dope[o]&PCC_LOGFLG)
# define pcc_callop(o)	  (pcc_dope[o]&PCC_CALLFLG)
# define pcc_simpleop(o)  (pcc_dope[o]&PCC_SIMPFLG)
# define pcc_commuteop(o) (pcc_dope[o]&PCC_COMMFLG)
# define pcc_divop(o)	  (pcc_dope[o]&PCC_DIVFLG)
# define pcc_fltop(o)	  (pcc_dope[o]&PCC_FLOFLG)
# define pcc_mulop(o)	  (pcc_dope[o]&PCC_MULFLG)
# define pcc_shiftop(o)	  (pcc_dope[o]&PCC_SHFFLG)
# define pcc_asgcompop(o) (pcc_dope[o]&PCC_ASGOPFLG)
# define pcc_intrinop(o)  (pcc_dope[o]&PCC_INTRFLG)
# define pcc_librop(o)    (pcc_dope[o]&PCC_LIBRFLG)

extern	int pcc_dope[];
extern	char *pcc_opname();

# endif _pcc_ops_


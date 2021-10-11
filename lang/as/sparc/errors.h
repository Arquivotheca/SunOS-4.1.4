/*	@(#)errors.h	1.1	10/31/94	*/
extern long int error_count;

#define ERR_SYNTAX	 1
#define ERR_INVCHAR	 2
#define ERR_UNK_OPC	 3
#define ERR_INVDIGIT	 4
#define ERR_REDEF_RES	 5
#define ERR_UNTERM_STR	 6
#define ERR_LPAREN	 7
#define ERR_RPAREN	 8
#define ERR_REDEFN	 9
#define ERR_ABSREQD	10
#define ERR_VAL_ASI	11
#define ERR_VAL_13	12
#define ERR_VAL_22	13
#define ERR_ADDRSYNTAX	14
#define ERR_DIVZERO	15
#define ERR_INV_REG	16
#define WARN_DELAY_SLOT	17
#define ERR_SETHI_LO	18
#define ERR_QSTRING	19
#define ERR_ALIGN	20
#define ERR_OP_COUNT	21
#define ERR_INV_OP	22
#define ERR_VAL_NOFIT	23
#define ERR_ALIGN_BDY	24
#define ERR_NOT_WDISP	25
#define ERR_DISP2BIG	26
#define ERR_GBL_LOC	27
#define ERR_LOC_NDEF	28
#define ERR_SYM_NDEF	29
#define ERR_CMP_EXP_REG	30
#define ERR_NO_INPUT	31
#define ERR_CANT_OPEN_I	32
#define ERR_UNK_SYM	33
#define ERR_UNK_OPT	34
#define ERR_ILL_LOCAL	35
#define ERR_NEED_CPP	36
#define ERR_PCT_HILO	37
#define ERR_AD_OPERAND	38
#define WARN_QUES_RELOC	39
#define ERR_REDEFN_OPT	40
#define ERR_CANT_OPEN_O	41
#define ERR_FPNUM_INV	42
#define ERR_GBL_DEF	43
#define ERR_VAL_INVALID	44
#define ERR_FEATURE_NYS	45
#define ERR_FPI_BFCC	46
#define ERR_CLR		47
#define ERR_ODDREG	48
#define ERR_R15_ONLY	49
#define ERR_EXPR_SYNTAX	50
#define WARN_CTI_EOSEG	51
#define ERR_REG_ALIGN	52
#define ERR_RETT_NO_JMP	53
#define ERR_OUT_REG_CNT	54
#define ERR_CANTCALC_EX	55
#define ERR_CANTCALC_DS	56
#define WARN_SYN_UNSUPP	57
#define ERR_ARITH_HILO	58
#define ERR_WRITE_ERROR	59
#define ERR_OPT_COMBO	60
#define WARN_STACK_ALIG	61	
#define ERR_OPT_HAND	62
#define ERR_JMP_ARGC	63

#ifdef SUN3OBJ
# define ERR_SEGNAME	64
#endif /*SUN3OBJ*/

#ifdef CHIPFABS
# define ERR_VAL_23	65
#else /*CHIPFABS*/
# define ERR_unused00	65	/* unused; available */
#endif /*CHIPFABS*/
#define WARN_LDFSR_BFCC 66
#define ERR_BRY_DBL_REG 67
#define WARN_STABS_OPT  68
#define ERR_VAL_9	69
#define WARN_STABS_R    70

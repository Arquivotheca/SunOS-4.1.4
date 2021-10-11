/* @(#)pcc_f1defs.h 1.1 94/10/31 SMI */

/*
 * definitions for the binary file interface to /usr/lib/f1
 * (backend based on pcc trees, currently used by pascal and
 * modula-2. formerly used by f77, hence the name 'f1')
 */

# ifndef _pcc_f1defs_
# define _pcc_f1defs_

/*
 * the following operators are interpreted in mip/fort.c
 * for the /usr/lib/f1 interface. They should NOT occur
 * in pcc expression trees.
 */

# define PCC_FTEXT	200	/* pass string to assembler */
# define PCC_FEXPR	201	/* delimits an expression */
# define PCC_FSWITCH	202	/* case/switch statement table */
# define PCC_FLBRAC	203	/* pass reg/auto allocation info to backend */
# define PCC_FRBRAC	204	/* end of function */
# define PCC_FEOF	205	/* end of file */
# define PCC_FARIF	206	/* arithmetic if (reserved, not supported) */
# define PCC_FLABEL	207	/* label definition */

/*
 *  pack operator, val, rest into binary format used by /usr/lib/f1
 */
# define PCC_FPACK(fop,val,rest) \
	( ( ( (rest) & 0177777 ) << 16 ) \
	| ( ( (val) & 0377 ) << 8 ) \
	| ( (fop) & 0377 ) )

/*
 * unpack fields of a code record from f1 input
 */

# define PCC_FOP(x) (int)((x)&0377)
# define PCC_FVAL(x) (int)(((x)>>8)&0377)
# define PCC_FREST(x) (((x)>>16)&0177777)

/*
 * code generator options
 */

# define PCC_FENTRY_FLAG 0x1	/* FLBRAC operator marks an entry point */
# define PCC_FCHECK_FLAG 0x2	/* generate overflow and range checks */

# endif _pcc_f1defs_

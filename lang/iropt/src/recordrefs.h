/* @(#)recordrefs.h 1.1 94/10/31 SMI	 */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __recordrefs__
#define __recordrefs__

/*
 * SITE -- a point in the program where a reference
 * to a variable or expression occurs
 */

/* typedef struct site SITE; */

struct site {
	BLOCK *bp;
	TRIPLE *tp;
} /* SITE */;

/*
 * VAR_REF -- one is built for every def/use of a variable
 */

typedef enum {
	VAR_DEF,	/* block-local var def */
	VAR_AVAIL_DEF,	/* downward-exposed var def */
	VAR_USE1,	/* block-local var use, operand 1 */
	VAR_USE2, 	/* block-local var use, operand 2 */
	VAR_EXP_USE1,	/* upward-exposed var use, operand 1 */
	VAR_EXP_USE2,	/* upward-exposed var use, operand 2 */
} VAR_REFERENCE_TYPE;

/* typedef struct var_ref VAR_REF; */

struct var_ref {
	VAR_REFERENCE_TYPE reftype:16;
	unsigned short defno;	   /* index in data flow bit vectors */
	struct var_ref *next_vref; /* references to a given leaf are linked */
	int refno;
	SITE site;  	/* site where the deed occurs*/
	LEAF *leafp;    /* leaf used or defined */
	struct var_ref *next;	/* link for traversing all var refs */
} /* VAR_REF */;

/*
 * EXPR_REF -- one is built for every def/use of a value
 */

typedef enum {
	EXPR_GEN,	/* block-local expr def */
	EXPR_AVAIL_GEN,	/* downward-exposed expr def */
	EXPR_USE1,	/* block local expr use, operand 1 */
	EXPR_USE2,	/* block local expr use, operand 2 */
	EXPR_EXP_USE1,	/* upward-exposed var use, operand 1 */
	EXPR_EXP_USE2,	/* upward-exposed var use, operand 2 */
	EXPR_KILL,	/* expression killed in this block */
} EXPR_REFERENCE_TYPE;

/* typedef struct expr_ref EXPR_REF; */

struct expr_ref {
	EXPR_REFERENCE_TYPE reftype:16;
	unsigned short defno;
	struct expr_ref *next_eref; /* references to a given expr are linked */
	int refno;
	SITE site;
} /* EXPR_REF */;

/*	masses of expr kill records are created for large routines - to
** obtain reasonble performance expr kill references are kept in compressed
** structs  - the reftype and next_eref fields are common to expr_ref
** and expr_kill ref. Expr kill records do not appear on d/u chains so
** the space used to store them is reclaimed after build_expr_sets
*/

/* typedef struct expr_kill_ref EXPR_KILLREF; */

struct expr_kill_ref {
	EXPR_REFERENCE_TYPE reftype:16;
	unsigned short blockno;
	struct expr_ref *next_eref; /* references to a given expr are linked */
# ifdef DEBUG
	int refno;
# endif
} /* EXPR_KILLREF */;

/*
 * implied limit on growth of data structures:
 * In the m68k implementations of bit_{set,reset,test}, an index is
 * computed using the mc68010 'mulu' instructions.  Hence, defno's
 * and blockno's are limited to 0..65535.
 */

#define MAX_VARDEFS 65535
#define MAX_EXPRDEFS 65535
#define MAX_NBLOCKS  MAX_EXPRDEFS	/* because of expr_killref hack */
#define NULLINDEX 65535

/*
 * counters for VAR_REF, EXPR_REF, EXPR_KILLREF records
 */
extern int nvardefs, nexprdefs;		/* #defs of variables and expressions */
extern int navailvardefs, navailexprdefs; /* n of these available on bb exit */
extern int nvarrefs, nexprrefs;		/* #uses+defs of vars and exprs */
extern int nexprkillrefs;

/* chain of all var_refs for a procedure */
extern VAR_REF *var_ref_head;

/* examine each triple and build the corresponding variable and/or expr
   reference entries distinguishing definitions, uses and killed exprs */
extern void	record_refs(/*BOOLEAN do_exprs*/);

/* free storage occupied by expr_killref records */
extern void	free_expr_killref_space();

#endif

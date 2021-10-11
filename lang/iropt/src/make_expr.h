/*	@(#)make_expr.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __make_expr__
#define __make_expr__

/*
 * expressions
 */

/* typedef struct expr EXPR; */

struct expr {
	IR_OP op:8;
	BOOLEAN recompute:1;
	TYPE type;
	struct expr *left, *right;
	int exprno;
	LEAF *save_temp;  /* tmp that saves the value */
	LIST *depends_on; /*list of leaves that this expression depends on*/
	struct expr_ref *references; /* description of sites which reference this expression */
	struct expr_ref *ref_tail;
	struct expr *next_expr;
} /* EXPR */;

#define		EXPR_HASH_SIZE 256
extern int	nexprs;
extern EXPR	*expr_hash_tab[EXPR_HASH_SIZE];

extern void	dump_exprs();
extern void	free_exprs();
extern void	free_depexpr_lists();
extern void	entable_exprs();
extern EXPR	*find_expr(/*np*/);
extern void	clean_save_temp();

#endif

/*	@(#)expr_df.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __expr_df__
#define __expr_df__

extern SET_PTR expr_gen, expr_kill, expr_in;
extern SET_PTR exprdef_gen, exprdef_kill, exprdef_in;
extern SET_PTR exprdef_mask, all_exprdef_mask;
extern int exprdef_set_wordsize;
extern int expr_set_wordsize;
extern int navailexprdefs;

extern void build_expr_sets(/*setbit_flag*/);
extern void compute_avail_expr();
extern void compute_exprdef_in();
extern void build_exprdef_mask();
extern void expr_reachdefs();

#endif

/*	@(#)var_df.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __var_df__
#define __var_df__

extern SET_PTR	save_reachdefs;	 /* out[] sets from reaching defs */
extern int	var_avail_defno; /* counter for available var definitions */

extern void	compute_df(/*do_vars,do_exprs,do_global,do_livedefs*/);
extern void	dump_cooked(/*point*/);
extern void	build_var_sets(/*setbit_flag*/);

#endif

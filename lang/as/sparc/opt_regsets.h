/*	@(#)opt_regsets.h 1.1 94/10/31	Copyr 1986 SMI */

/* some well-known sets */
extern register_set empty_regset, zero_regset, out_regset, window_regset, float_regset, cc_regset;

/* routines to manipulate sets */
extern register_set regset_add( );
extern register_set regset_sub( );
extern register_set regset_intersect( );
extern register_set regset_incl( );
extern register_set regset_excl( );
extern register_set regset_mkset( );
extern register_set regset_mkset_multiple( );
extern Bool regset_equal();
extern Bool regset_subset();
extern Bool regset_in();
extern Bool regset_empty();
extern void regset_make_leaf_exit();
extern void regset_procedure_exit_regs_read();
extern void regset_setnode( );
extern void regset_propagate( );
extern void regset_recompute( );
extern int regset_select();
extern register_set following_liveset( );
/* routines to keep track of nodes for me */
extern Node * popnode();
extern void pushnode( );

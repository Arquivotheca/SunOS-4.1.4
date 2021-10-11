/*	@(#)debug.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __debug__
#define __debug__

extern void	mem_stats(/*phase*/);
extern void	print_var_ref(/*rp*/);
extern void	dump_var_refs();
extern void	dump_expr_refs();
extern void	dump_byte_tab(/*header,tab,nrows,ncols*/);
extern void	print_type(/*type*/);
extern void	print_address(/*ap*/);
extern void	print_const_leaf(/*lp*/);
extern void	print_leaf(/*lp*/);
extern void	print_operand(/*np*/);
extern void	print_triple(/*tp, indent*/);
extern void	print_block(/*p, detailed*/);
extern void	print_segment(/*sp*/);
extern void	dump_segments();
extern void	dump_leaves(); 
extern void	dump_triples(); 
extern void	dump_blocks(); 
extern void	show_list(/*start*/);
extern void	mapc(/*list, fun*/);
extern void	tpr(/*tp*/);
extern void	lpr(/*leafp*/);
extern void	bpr(/*blockp*/);
extern void	npr(/*np*/);
extern void	tnpr(/*n*/);
extern void	lnpr(/*n*/);
extern void	bnpr(/*n*/);
extern void	lvpr(/*T*/);
extern void	lvnpr(/*Tn*/);
extern int	nitems(/*listp*/);
extern void	dump_set(/*set*/);
extern void	dump_row(/*set,i*/);

#endif

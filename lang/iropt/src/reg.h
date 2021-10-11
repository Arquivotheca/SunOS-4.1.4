/*    @(#)reg.h 1.1 94/10/31 SMI      */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef __reg__
#define __reg__

/*
 * Header file for register allocation.
 */
#include "cost.h"

#if TARGET == MC68000
#define MAX_DREG 6			       /* save d0, d1 for CG */
#define MAX_AREG (picflag == IR_TRUE ? 3 : 4)  /* save a0, a1, a6, a7 for CG */
#define MAX_FREG 6			       /* save f0, f1 for CG */
#define MAX_FPAREG 12			       /* save fp0, fp1, fp2, fp3 for CG */
#define FIRST_DREG 7
#define FIRST_AREG (picflag == IR_TRUE ? 4 : 5)
#define FIRST_FREG 7
#define FIRST_FPAREG 4

#define ARGOFFSET 8
#define INIT_CONT 20	/* cycle time to save and restore a register on */
			/* procedure entry and exit */
#define FINIT_CONT 60
#define FPA_BASE_WEIGHT 2  /* cycles/fpa-access saved if base reg used */
#define SET_REGMASK(fpa,b,f81,a,d) \
	(((FIRST_FPAREG+MAX_FPAREG-fpa) << 16 ) | \
	(( b )<< 12 ) | \
	((FIRST_FREG-MAX_FREG+f81)<<8) | \
	((FIRST_AREG-MAX_AREG+a)<<4) | \
	(FIRST_DREG-MAX_DREG+d))

#endif

#if TARGET == SPARC
#define TOTAL_DREG 31		/* total number of general registers */ 
#define MAX_ARG_REG 6		/* words that passed in register */
#define FIRST_ARG_REG 24	/* i0 */

#ifdef USE_GREGS
#   define MAX_DREG (picflag ? 17 : 18)		/* i5~i0, (l7)~l0, g5~g2 */
#else
#   define MAX_DREG (picflag ? 13 : 14)		/* i5~i0, (l7)~l0 */
#endif

#define MAX_FREG 24		/* reserve 0..7 for CG */
#define FIRST_DREG 29		/* i5 */
#define FIRST_FREG 31		/* count down from f31 */

#define ARGOFFSET 68
#define INIT_CONT 0
#define COST_SAVE_FP 12		/* 12 cycles to save and restore each pair of */
				/* floating point register at CALL */
#define AVAIL_INT_REG (picflag ? 0x3f7f003c : 0x3fff003c)
			 	/* reserve i7,i6,(l7),o7~o0,g7,g6,g1,g0 */
#define AVAIL_F_REG 0xffffff00		/* reserve f0~f7 for CG */

#ifdef USE_GREGS
#   define SCRATCH_REGMASK 0x00000000	/* cg reserves o7-o0 */
#else
#   define SCRATCH_REGMASK 0x0000003c	/* cg reserves o7-o0, g5-g2 */
#endif

#endif

#if TARGET == I386
/* MAX_FREG, FIRST_DREG, FIRST_AREG, FIRST_FREG, FINIT_CONT,
FLD_ST_WT AVAIL_REG SCRATCH_REGMASK modified to accomodate weitek.
must be in sync with ../../pcc/i386/machdep.h ..KMC*/
#define MAX_DREG 1	    /* ebx (!) */
#define MAX_AREG 2	    /* esi, edi */
#define MAX_FREG (weitek?11:0) /* 387 removed with new stack model st(2) - st(7) */
#define FIRST_DREG 8	    /* ebx (any color you want,as long as it's black) */
#define FIRST_AREG 9	    /* esi..edi */
#define FIRST_FREG (weitek?23:3)    /* wf30 or st(0) (count down from here) */
#define ARGOFFSET 8
#define INIT_CONT 6	/* time lost saving (pushl) and restoring (movl) ... */
			/* ... a 386 register on procedure entry and exit */
#define FINIT_CONT (weitek? 36: 96)
	/* time to save and restore floating pointreg */
	/* For 387: save (fld;fstpl) and restore (fldl;fstp) */
#define FLD_ST_WT (weitek? 6: 11)	
	/* benefit of loading a float reg from reg vs mem */
#define AVAIL_REG (weitek? 0x00ffe7f7: 0x070f)   
	/* regs available to iropt and cg (1 = available): all but esp, ebp */
#define SCRATCH_REGMASK (weitek? 0x00f7: 0x000f)
	/* regs reserved for cg (1 = reserved): eax,edx,ecx,st(0) */

#define AREG_SEGNO DREG_SEGNO	/*KLUDGE*/
#endif

/*
 * WEB -- as the name implies, a tangled mess of d/u chains.
 * Used in register allocation to handle different lifespans
 * of the same variable as distinct objects.
 */

/* typedef struct web WEB; */

struct web {
	short web_no;
	BOOLEAN import:1; /* used before defined */
	struct web *next_web;
	struct web *prev_web;
	LEAF *leaf;
	LIST *define_at, *use;
	COST d_cont, a_cont, f_cont;
	struct web *same_reg;
	struct sort *sort_d, *sort_a, *sort_f;
	SET interference;
} /* WEB */;

typedef enum { A_TREE=0, D_TREE, F_TREE } TREE;

typedef struct sort {
	COST weight;
	WEB *web;
	struct sort *left, *right;
	int regno;
	LIST *lp;
} SORT;

/* global vars */
extern int	regmask;
extern int	fregmask;
extern int	nwebs;
extern int	n_dsort;
extern int	n_asort;
extern int	n_fsort;
extern int	alloc_web;
extern WEB	*web_free;
extern WEB	*web_head;
extern WEB	**defs_to_webs;
extern LIST	*sort_d_list;
extern LIST	*sort_a_list;
extern LIST	*sort_f_list;

#if TARGET == MC68000
extern int	fpa_base_reg;
extern COST	fpa_base_weight;
#endif

#if TARGET == SPARC
extern COST	save_f_reg;	
#endif

#if TARGET == I386
extern BOOLEAN weitek;		/*KMC*/
#endif

extern void	compute_fpsave_cost();
extern void	reg_init();
extern void	reg_cleanup();
extern BOOLEAN	can_in_reg(/* leafp */);
extern void	new_web(/*leafp, rp*/);
extern void	delete_web(/*web*/);
extern void	free_webs();
extern void	merge_same_var(/*leafp, visited*/);
extern void	dump_webs(/*show_conflict*/);
extern void	weigh_web(/* web, vp */);
extern void	sort_webs();
extern void	merge_sorts(/* lp, pass */);
extern void	reorder_sorts(/* lp, lpp */);
extern BLOCK	*find_df_block(/* web */);

#endif

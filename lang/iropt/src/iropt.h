/*	@(#)iropt.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "ir_common.h"
#include "ir_types.h"
#include "align.h"

/* 	mnemonics for debugflags	 */

#    define DDEBUG	       0 /* -d option used                          */ 
#    define B_IV_DEBUG         1 /* print debug info. before IV             */
#    define A_IV_DEBUG         2 /* print debug info. after IV              */
#    define B_LOOP_DEBUG       3 /* print debug info. before loop           */
#    define A_LOOP_DEBUG       4 /* print debug info. after loop            */
#    define B_LOCALCSE_DEBUG   5 /* print debug info. before local cse      */
#    define B_GLOBALCSE_DEBUG  6 /* print debug info. before global cse     */
#    define A_GLOBALCSE_DEBUG  7 /* print debug info. after global cse      */
#    define B_LOCALPPG_DEBUG   8 /* print debug info. before local ppg      */
#    define B_GLOBALPPG_DEBUG  9 /* print debug info. before global ppg     */
#    define A_GLOBALPPG_DEBUG 10 /* print debug info. after global ppg      */
#    define B_REGALLOC_DEBUG  11 /* print debug info. before reg allocation */
#    define A_REGALLOC_DEBUG  12 /* print debug info. after register alloc  */
#    define B_UNROLL_DEBUG    13 /* print debug info. before loop unrolling */
#    define A_UNROLL_DEBUG    14 /* print debug info. after loop unrolling  */
#    define ALIAS_DEBUG	      15 /* print debug info. in aliaser module     */
#    define PAGE_DEBUG	      16 /* print debug info. in paging routines    */
#    define SHOWRAW_DEBUG     17 /* print IR before control flow analysis */
#    define SHOWCOOKED_DEBUG  18 /* print IR after control flow analysis */
#    define SHOWDF_DEBUG      19 /* print data flow information */
#    define SHOWLIVE_DEBUG    20 /* print live variables information */
#    define SHOWDENSITY_DEBUG 21 /* print density of sets */
#    define B_TAIL_DEBUG      22 /* print debug info. before tail recursion */
#    define A_TAIL_DEBUG      23 /* print debug info. after tail recursion */

#   define SHOWRAW debugflag[SHOWRAW_DEBUG]
#   define SHOWCOOKED debugflag[SHOWCOOKED_DEBUG]
#   define SHOWDF debugflag[SHOWDF_DEBUG]
#   define SHOWLIVE debugflag[SHOWLIVE_DEBUG]
#   define SHOWDENSITY debugflag[SHOWDENSITY_DEBUG]

#define MAXDEBUGFLAG A_TAIL_DEBUG

#    define IV_PHASE		0
#    define LOOP_PHASE		1
#    define CSE_PHASE		2
#    define COPY_PPG_PHASE	3
#    define REG_ALLOC_PHASE	4
#    define UNROLL_PHASE	5
#    define TAIL_REC_PHASE	6
#    define MAXOPTIMFLAG TAIL_REC_PHASE

#    define DO_IV	 optimflag[IV_PHASE]
#    define DO_LOOP	 optimflag[LOOP_PHASE]
#    define DO_CSE	 optimflag[CSE_PHASE]
#    define DO_COPY_PPG	 optimflag[COPY_PPG_PHASE]
#    define DO_REG_ALLOC optimflag[REG_ALLOC_PHASE]
#    define DO_UNROLL	 optimflag[UNROLL_PHASE]
#    define DO_TAIL_REC  optimflag[TAIL_REC_PHASE]
#    define MAXOPTIMFLAG TAIL_REC_PHASE

/*
 * iropt has the following levels of optimization:
 *
 *	optim_level = 0:  Do nothing.
 *	optim_level = 1:  Do nothing.
 *	optim_level = 2:  Do partial optimization (local vars only)
 *	optim_level = 3:  Do full optimization (local and global vars)
 *	optim_level = 4:  Same as level 3, but try to track pointer values
 *
 * If no level is specified, the default level is 3.  Note that this
 * does not necessarily coincide with the default level for a particular
 * language; for example, f77 -O means f77 -O3, but cc -O means -O2.
 */

#    define PARTIAL_OPT_LEVEL 2
#    define MAX_OPT_LEVEL     4
#    define partial_opt ((BOOLEAN)(optim_level==PARTIAL_OPT_LEVEL))
#    define DO_ALIASING(hdr) \
	((hdr).lang != FORTRAN && (hdr).opt_level == MAX_OPT_LEVEL)

/*
 * EXT_VAR(leaf) identifies a leaf whose references may
 * be assumed to have nonlocal side effects.   This is
 * used primarily at the -O2 level.
 */
#define EXT_VAR(leaf) \
	((&(leaf)->val)->addr.seg->descr.external == EXTSTG_SEG || \
	 (&(leaf)->val)->addr.seg->descr.class == HEAP_SEG || \
	 (leaf)->overlap != NULL || \
	 (leaf)->no_reg == IR_TRUE)

/*
 * pcc base types
 */
# define UNDEF		PCC_UNDEF
# define FARG		PCC_FARG
# define CHAR		PCC_CHAR
# define SHORT		PCC_SHORT
# define INT		PCC_INT
# define LONG		PCC_LONG
# define FLOAT		PCC_FLOAT
# define DOUBLE		PCC_DOUBLE
# define STRTY		PCC_STRTY
# define UNIONTY	PCC_UNIONTY
# define ENUMTY		PCC_ENUMTY
# define MOETY		PCC_MOETY
# define UCHAR		PCC_UCHAR
# define USHORT		PCC_USHORT
# define UNSIGNED	PCC_UNSIGNED
# define ULONG		PCC_ULONG
# define TVOID		PCC_TVOID

/*		
 * extended ir base types
 */
# define BOOL		IR_BOOL
# define EXTENDEDF	IR_EXTENDEDF
# define COMPLEX	IR_COMPLEX
# define DCOMPLEX	IR_DCOMPLEX
# define STRING		IR_STRING
# define LABELNO	IR_LABELNO
# define BITFIELD	IR_BITFIELD
# define NTYPES		IR_LASTTYPE+1

/*
 * pcc type macros
 */
# define MODTYPE(x,y)	PCC_MODTYPE(x,y)
# define BTYPE(x)	PCC_BTYPE(x)
# define ISUNSIGNED(x)	PCC_ISUNSIGNED(x)
# define UNSIGNABLE(x)	PCC_UNSIGNABLE(x)
# define ISPTR(x)	PCC_ISPTR(x)
# define ISFTN(x)	PCC_ISFTN(x)
# define ISARY(x)	PCC_ISARY(x)
# define INCREF(x)	PCC_INCREF(x)
# define DECREF(x)	PCC_DECREF(x)

/*
 * ir type macros
 */
# define ISCHAR(x)	IR_ISCHAR(x)
# define ISINT(x)	IR_ISINT(x)
# define ISBOOL(x)	IR_ISBOOL(x)
# define ISREAL(x)	IR_ISREAL(x)
# define ISCOMPLEX(x)	IR_ISCOMPLEX(x)
# define ISPTRFTN(x)	IR_ISPTRFTN(x)

/*
 * iropt-specific type macros
 */
# define ISFLOAT(x)	((x) == FLOAT)
# define ISDOUBLE(x)	((x) == DOUBLE)
# define ISEXTENDEDF(x)	((x) == EXTENDEDF)

/*
 * miscellaneous iropt macros
 */
#define M(x) 	(1<<((int)(x)))
#define ONEOF(x,y) (M(x) & (y) )
#define	MSKUSE	(M(VAR_USE1)|M(VAR_USE2)|M(VAR_EXP_USE1)|M(VAR_EXP_USE2))
#define ISUSE(z) ONEOF(z,MSKUSE)
#define	MSKEUSE	(M(EXPR_USE1)|M(EXPR_USE2)|M(EXPR_EXP_USE1)|M(EXPR_EXP_USE2))
#define ISEUSE(z) ONEOF(z,MSKEUSE)
#define ISCONST(leafp) (((LEAF*)leafp)->class == ADDR_CONST_LEAF || \
			((LEAF*)leafp)->class == CONST_LEAF  )

#define LOCAL 	static
#define	ORD(x)	((int) x)
#define	BPW	32

/*
 * Target-specific integer constant subranges.
 *
 * MIN_SMALL_INT..MAX_SMALL_INT is the set of constants that can
 *	be used cheaply in immediate-mode instruction operands.
 *
 * MIN_SMALL_OFFSET..MAX_SMALL_OFFSET is the set of constants that
 *	are supported directly by the target machine's base+offset
 *	addressing.
 *
 * These are used in check_recompute() (make_expr.c), among other places.
 */

#if TARGET == SPARC
#define MAX_SMALL_INT	4095
#define MIN_SMALL_INT	-4096
#define MAX_SMALL_OFFSET MAX_SMALL_INT
#define MIN_SMALL_OFFSET MIN_SMALL_INT
#endif

#if TARGET == MC68000
#define MAX_SMALL_INT	8
#define MIN_SMALL_INT	-8
#define MAX_SMALL_OFFSET 32767
#define MIN_SMALL_OFFSET -32768
#endif

#if TARGET == I386
#    define IS_SMALL_INT(leafp) \
	(   (((LEAF*)leafp)->class == CONST_LEAF) && \
	    (ISINT(((LEAF*)leafp)->type.tword)) )

#    define IS_SMALL_OFFSET(leafp) \
	(   (((LEAF*)leafp)->class == CONST_LEAF) && \
	    (ISINT(((LEAF*)leafp)->type.tword)) )

#else  /* TARGET != I386 */
#    define IS_SMALL_INT(leafp) \
	(   (((LEAF*)leafp)->class == CONST_LEAF) && \
	    (ISINT(((LEAF*)leafp)->type.tword)) && \
	    (((LEAF*)leafp)->val.const.c.i < MAX_SMALL_INT ) && \
	    (((LEAF*)leafp)->val.const.c.i >= MIN_SMALL_INT ) )

#    define IS_SMALL_OFFSET(leafp) \
	(   (((LEAF*)leafp)->class == CONST_LEAF) && \
	    (ISINT(((LEAF*)leafp)->type.tword)) && \
	    (((LEAF*)leafp)->val.const.c.i < MAX_SMALL_OFFSET ) && \
	    (((LEAF*)leafp)->val.const.c.i >= MIN_SMALL_OFFSET ) )
#endif  /* TARGET != I386 */

/*
 * Iropt internal data structure definitions.  Most structures
 * are only hung in effigy here; the internals are defined in
 * the indicated header files.
 */

/*
 * label records (internals in cf.h)
 */
typedef struct labelrec LABELREC;

/*
 * sets (internals in bit_util.h)
 */
typedef unsigned long *SET;
typedef unsigned long *BIT_INDEX;
typedef struct set_description *SET_PTR; 

/*
 * LIST data types -- in theory, this is what the data field
 * of a LIST node points at.  In practice, the contents of
 * this union are not used.  Instead, the "datap" field of
 * a list node is cast to some other pointer type, using LCAST.
 */
typedef union list_u {
	int meaningless;
} LDATA;

/*
 * expressions  (internals in make_expr.h)
 */

typedef struct expr EXPR;

/*
 * expr and var references (internals in recordrefs.h)
 */

typedef struct site SITE;
typedef struct var_ref VAR_REF;
typedef struct expr_ref EXPR_REF;
typedef struct expr_kill_ref EXPR_KILLREF;

/*
 * register allocation (internals in reg.h)
 */

typedef struct web WEB;	/* internals of struct web in reg.h */

/* pointers to main IR structures */

extern HEADER	hdr;
extern BLOCK	*entry_block, *exit_block;
extern BLOCK	*first_block_dfo;
extern BLOCK	*last_block;
extern SEGMENT	*seg_tab, *last_seg;
extern LEAF	*leaf_tab, *leaf_top;
extern TRIPLE	*triple_tab, *triple_top;
extern LIST	*ext_entries;
extern LIST	*labelno_consts;
extern LIST	*indirgoto_targets;

# define LEAF_HASH_SIZE 256
extern LIST	*leaf_hash_tab[LEAF_HASH_SIZE];

/* IR structure counters */

extern int	nseg;
extern int	nblocks;
extern int	nleaves;
extern int	ntriples;
extern int	npointers;
extern int	nelvars;

/* modes and flags */

extern BOOLEAN	optimflag[MAXOPTIMFLAG+1];
extern BOOLEAN	debugflag[MAXDEBUGFLAG+1];
extern char	*debug_nametab[MAXDEBUGFLAG+2];
extern char	*optim_nametab[MAXDEBUGFLAG+2];
extern BOOLEAN	use68881;
extern BOOLEAN	use68020;
extern BOOLEAN	usefpa;
extern BOOLEAN	picflag;
extern BOOLEAN	multientries;
extern BOOLEAN	unknown_control_flow;
extern BOOLEAN	regs_inconsistent;
extern int	optim_level;
extern char	*ir_file;	
extern char	*heap_start;

/* functions */

extern int	main(/*argc,argv*/);	
extern BOOLEAN	optimizing();
extern void	proc_init();

/* from libc */
extern char	*sprintf(/*s,f,...*/);

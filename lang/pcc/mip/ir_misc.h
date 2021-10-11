/*	@(#)ir_misc.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include "machdep.h"
#include "ir_common.h"
#include "ir_types.h"

/* local aliases */

# define TRUE IR_TRUE
# define FALSE IR_FALSE
# define LABEXIT -1
# define LABFALLTHRU -2

typedef union list_u {	/* union of items stored on LIST structures */
	IR_NODE ir_node;
} LDATA;

typedef struct chain {
	struct chain *nextp;
	char * datap;
} CHAIN;

/* leaf and segment hash tables */

# define LEAF_HASH_SIZE 307
# define SEG_HASH_SIZE 307

extern LIST *leaf_hash_tab[LEAF_HASH_SIZE];
extern CHAIN *seg_hash_tab[SEG_HASH_SIZE];

/* optimization levels */

#define PARTIAL_OPT_LEVEL 2 /* cse, loop opts, etc., but avoid globals */
#define FULL_OPT_LEVEL    3 /* same as 2, but include globals and indirects; */
			    /* use conservative aliasing info from front end */
#define IROPT_ALIAS_LEVEL 4 /* same as 3, but try to compute stronger */
			    /* aliasing info in iropt */
#define DEFAULT_OPT_LEVEL FULL_OPT_LEVEL

extern int optimize;	/* optimization level */


extern int nsegs;	/* #segments for this routine */
extern int nleaves;	/* #leaf operands for this routine */
extern int ntriples;	/* #triples for this routine */
extern int nblocks;	/* #blocks for this routine (always 2 in front end) */
extern int nlists;	/* #list nodes for this routine */
extern int npointers;	/* #trackable pointers (simple vars only) */
extern int naliases;	/* #names that can be aliased by pointers */
extern int ir_debug;	/* flag to control IR debugging output */

extern HEADER hdr;
extern STRING_BUFF *first_string_buff, *string_buff;
extern LEAF	*first_leaf, *last_leaf;
extern BLOCK	*first_block, *current_block;
extern SEGMENT	*first_seg, *last_seg;

extern struct opdescr_st op_descr[];

/*
 * estimates of the relative cost of a leaf node reference.
 * This is only a wild guess, because we don't know which
 * things will go in registers.
 */
#define COST_REG 0
#define COST_AUTO 1
#define COST_SMALLCONST 1
#define COST_LARGECONST 2
#define COST_EXTSYM 2
#define COST_EXPR 3

/*
 * COST_SIMPLE is the point below which it is cheaper
 * to recompute than to cache, assuming we get a register,
 * and assuming no future references...
 */
#if TARGET == MC68000 || TARGET == I386
#    define COST_SIMPLE COST_EXTSYM
#endif

#if TARGET == SPARC
#    define COST_SIMPLE COST_AUTO
#endif

/*	@(#)cg_ir.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "machdep.h"
#include "ir_common.h"
#include "ir_types.h"

/* from libc */
extern char *malloc();
extern long lseek();
extern char *sprintf();

extern HEADER hdr;

#define LOCAL 	static
#define	ORD(x)	((int) x)
#define	BPW		32

extern BLOCK *entry_block;
extern SEGMENT *seg_tab;
extern TRIPLE *triple_tab;
extern LEAF* leaf_tab;
extern int nseg, nblocks, nleaves,ntriples, nlists, 
		strtabsize, npointers, nelvars;
extern char * base_type_names[] ;

extern LIST *leaf_hash_tab[];
# define LEAF_HASH_SIZE 256

typedef union list_u {
	struct leaf leaf;
	struct triple triple;
	struct block block;
} LDATA;

extern LIST *new_list();
extern LEAF *leaf_lookup();
extern BOOLEAN same_irtype();
extern LEAF *ileaf();
extern long l_align();
extern TYPE inttype;
extern char *ckalloc();
extern void quit(), quita();

# define NTYPES IR_LASTTYPE+1

#define ISCONST(leafp) \
    (   ((LEAF*)(leafp))->class == CONST_LEAF \
	((LEAF*)(leafp))->class == ADDR_CONST_LEAF )

extern struct opdescr_st op_descr[];

# if TARGET == MC68000
#    define AREGOFFSET  A0
#    define STACK_REG   A6
#    define BSS_REG     A5
#    define FREGOFFSET  FP0
#    define MAXDISPL    32768
#    define HIGH_DREG   D7
#    define N_DREGS     6
#    define ARGFPOFFSET 8
# endif
# if TARGET == SPARC
#    define GREGOFFSET  RG0		/* G0 */
#    define OREGOFFSET  RO0		/* O0 */
#    define LREGOFFSET  RL0		/* L0 */
#    define IREGOFFSET  RI0		/* I0 */
#    define FREGOFFSET  FR0 		/* F0 */
#    define STACK_REG   RI6		/* I6 */
#    define ARGFPOFFSET 68		/* first parameter offset */
# endif

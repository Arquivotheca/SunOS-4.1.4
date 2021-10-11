/*	@(#)scheduling.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

typedef struct list_node {
	Node *nodep;
	struct list_node *next;
} LIST_NODE;

typedef struct resource {
/*
 * use_np == NULL && used == FALSE : no use since begining of bb.
 * use_np != NULL && used == TRUE  : def_np must NOT be NULL and
 *				     the definition is used by use_np.
 * use_np != NULL && used == FALSE : there are used BEFORE the def_np
 * 				     and there is NO use after the def_np yes.
 * use_np == NULL && used == TRUE  : BUG
 */

	LIST_NODE *def_np, *use_np;
	Bool used;
} RESOURCE;

typedef struct indirct_pointer {
	unsigned int no_alias;
	RESOURCE resources;
} INDIRECT_POINTER;

/* Structure to keep track of a currently executing fpu_node and the cycles
 * till it completes execution.  Will only bew used in the array fpu_status
 * in opt_resequence.c to keep track of the last three fpu_nodes
 */
typedef struct fpu_node {
	Node *fnode;
	int cycles;
} FPU_NODE;

#define MAX_FPU_NODES 3                /* Max number of instructions in the fpu */
#define MAX_RESOURCES _NB_ALL_RES
#define MAX_GP	RN_GP(31)+1

#define IU_LOAD_CYCLES 2	       /* two cycle load */
#define IU_STORE_CYCLES 3	       /* tree cycle store */
#define IU_ARITH_CYCLES 1	       /* one cycle arithmatic */
#define FPU_MULS_CYCLES 7
#define FPU_MULD_CYCLES 8
#define FPU_SIMPLE_CYCLES 7            /* for fpu{add,sub,mov,neg,abs,xtoy */ 
#define FPU_DIVS_CYCLES 12
#define FPU_DIVD_CYCLES 17
#define FPU_SQRTS_CYCLES 14
#define FPU_SQRTD_CYCLES 21
#define MAX_CYCLES 21
#define FPU_CMP_CYCLES 6
#define FPU_SETUP_CYCLES 2	       /* two iu cycles to setup fpu inst. */
#define FPU_CHAIN_CYCLES 3

#define SET_NO_ALIAS( reg1, reg2 )	(indirect_pointer[(reg1)].no_alias |= (1<<(reg2)))

#define NO_ALIAS( reg1, reg2 )		(indirect_pointer[(reg1)].no_alias & (1<<(reg2)))

#define CHAINNABLE(np)  ((np)->n_opcp->o_func.f_subgroup==FSG_FADD ||\
			 (np)->n_opcp->o_func.f_subgroup==FSG_FSUB ||\
			 (np)->n_opcp->o_func.f_subgroup==FSG_FMUL)

#define MAX( a, b )    ( a > b ) ? a : b             

extern LIST_NODE *free_iu_nodes, *free_fpu_nodes, *delay_slot_lnode;
extern LIST_NODE *branches_with_nop;
extern RESOURCE resources[];
extern INDIRECT_POINTER indirect_pointer[];
extern Node *last_node, *branch_node, *trap_node;
extern int number_of_nodes, free_node_count;

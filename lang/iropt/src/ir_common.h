/* @(#)ir_common.h 1.1 94/10/31 Copyr 1986 Sun Micro */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* version identification: for SunOS 4.1, major = 4, minor = 1 */

#define IR_MAJOR_VERS 4
#define IR_MINOR_VERS 1

typedef enum {
        IR_ERR=0, IR_ENTRYDEF, IR_EXITUSE, IR_IMPLICITDEF, IR_IMPLICITUSE,
        IR_PLUS,IR_MINUS,IR_MULT,IR_DIV,IR_REMAINDER, IR_AND, IR_OR, IR_XOR, IR_NOT,
        IR_LSHIFT, IR_RSHIFT, IR_SCALL, IR_FCALL, IR_EQ, IR_NE, IR_LE, IR_LT, IR_GE,
        IR_GT, IR_CONV, IR_COMPL, IR_NEG, IR_ADDROF, IR_IFETCH, IR_ISTORE, IR_GOTO,
        IR_CBRANCH, IR_SWITCH, IR_REPEAT, IR_ASSIGN,
        IR_PASS, IR_STMT, IR_LABELDEF, IR_INDIRGOTO, IR_FVAL, IR_LABELREF,
        IR_PARAM, IR_PCONV, IR_FOREFF
} IR_OP;

typedef enum { IR_FALSE, IR_TRUE } BOOLEAN;
typedef enum { VAR_LEAF=1, ADDR_CONST_LEAF=2, CONST_LEAF=3 } LEAF_CLASS;
typedef enum { ISBLOCK=1, ISLEAF, ISTRIPLE } TAG;
typedef enum { EXT_FUNC, INTR_FUNC, SUPPORT_FUNC } FUNC_DESCR;
typedef enum { UNOPTIMIZED, OPTIMIZED } FILE_STATUS;
typedef enum { C, FORTRAN, PASCAL } LANG;

typedef enum { REG_SEG, STG_SEG } SEGCONTENT;
typedef enum { USER_SEG, BUILTIN_SEG } SEGBUILTIN;
typedef enum { LCLSTG_SEG, EXTSTG_SEG } SEGSTGTYPE;

#if TARGET == MC68000
typedef enum { ARG_SEG, BSS_SEG, DATA_SEG, AUTO_SEG, TEMP_SEG,
		DREG_SEG, AREG_SEG, FREG_SEG,
		HEAP_SEG } SEGCLASS;
#endif

#if TARGET == SPARC || TARGET == I386
typedef enum { ARG_SEG, BSS_SEG, DATA_SEG, AUTO_SEG, TEMP_SEG,
		DREG_SEG, FREG_SEG,
		HEAP_SEG } SEGCLASS;
#endif

typedef unsigned long TWORD;

struct	segdescr_st	{
	SEGCLASS class:	8;			/* segment class */
	SEGCONTENT content: 8;		/* registers or storage */
	SEGBUILTIN builtin: 8;		/* built in or user defined */
	SEGSTGTYPE external: 8;		/* external or local storage */
};

typedef struct segment {
	char *name;
	struct segdescr_st descr;
	short base;
	int align:4;
	BOOLEAN visited:1;
	long offset;
	long len;
	int segno;
	struct segment *next_seg;
	struct list *leaves;
} SEGMENT;

/* segment numbers for builtin segments : these segments are always present
** but may be empty
*/
# define ARG_SEGNO 0
# define BSS_SMALL_SEGNO 1
# define BSS_LARGE_SEGNO 2
# define DATA_SEGNO 3
# define AUTO_SEGNO 4
# define HEAP_SEGNO 5
# define DREG_SEGNO 6

#if TARGET == MC68000
# define AREG_SEGNO 7
# define FREG_SEGNO 8
#endif

#if TARGET == SPARC || TARGET == I386
# define FREG_SEGNO 7
#endif

# define N_BUILTIN_SEG (FREG_SEGNO + 1)

typedef struct address {
	struct segment * seg;
	long offset;
	long labelno;
	long length;
	short alignment;
} ADDRESS;

#define NOBASEREG -1
#define EXIT_LABELNO -1

union	const_u {
	char *cp;	/* Character constants */
	long i;		/* Integer constants */
	char *fp[2];	/* Strings for float real and complex consts */
	char *bytep[2];	/* bytes for binary constants (init only) */
};

typedef struct list {
	struct list *next;
	union list_u *datap;	/* union list_u defined by application */
} LIST;

/*
 * IR type structure.   For most types, the size and alignment fields
 * indicate the type's size and its alignment requirements, both quantities
 * in units of bytes.  (alignment of a structured variable depends on the
 * types of its members.)  For bitstring types, align is the bit offset
 * within the word containing the string, and size is the length of the
 * string in units of bits.
 */
typedef struct irtype {
	TWORD	tword;		/* PCC type encoding */
	int align:5;		/* type alignment */
	int size:27;		/* type size */
} TYPE;

typedef struct block {
	TAG tag:8;
	BOOLEAN is_ext_entry : 8; /* is the block an external entry point */
	BOOLEAN entry_is_global: 1;	/* if so, is the entry point global */
	int /*unused*/:15;
	int blockno;
	struct triple *last_triple;
	char *entryname;
	int labelno;
	struct block *next;		/* allocation defined order */
	BOOLEAN visited;
	struct list *pred, *succ;
# ifdef IROPT
	struct block *prev; 		/* reverse link of *next */
	struct block *dfonext, *dfoprev;	/* depth first search order */
	struct list *loops;
	int	loop_weight;
# define IROPT_BLOCK_FIELDS	5	/* # of longs specific to IROPT */
# endif
} BLOCK;

typedef union leaf_value {
	struct address addr;
	struct constant {
		BOOLEAN isbinary;
		union const_u c
	} const;
} LEAF_VALUE;

typedef struct leaf {
	TAG tag:8;
	LEAF_CLASS class:8;
	FUNC_DESCR func_descr:4;	/* attributes of known functions */
	BOOLEAN unknown_control_flow:1;	/* if called, flow graph is incorrect */
	BOOLEAN makes_regs_inconsistent:1; /* if called, ... */
#if TARGET == SPARC
	    /*
	     * must_store_arg == TRUE if an incoming argument must be
	     * stored in its ARG_SEG location as part of the routine prologue.
	     */
	    BOOLEAN must_store_arg:1;
	    int  /*unused*/:9;
#endif
#if TARGET == MC68000 || TARGET == I386
	    int  /*unused*/:10;
#endif
	int  leafno;
	TYPE	type;
	LEAF_VALUE val;
	BOOLEAN	visited; /* used as a flag and/or pointer to auxiliary information*/
	char *pass1_id;
	struct leaf *next_leaf;
	struct list *overlap;
	struct list *neighbors;
	int  pointerno;
	int  elvarno;
	struct leaf *addressed_leaf;	/* object addressed by ADDR_CONST */
	BOOLEAN no_reg;	/* indicate this leaf can not be in the register */
# ifdef IROPT
	struct triple *entry_define, *exit_use;
	struct  var_ref *references;
	struct	var_ref *ref_tail;
	struct list *dependent_exprs; /*list of exprs that depend on this leaf*/
	struct list *kill_copy;	/* list of copies that will be killed if this */
				/* leaf has been redefined */
# define IROPT_LEAF_FIELDS 	6	/* # of fields specific to IROPT */
# endif
} LEAF;

typedef struct triple {
	TAG tag:8;
	IR_OP	op:8;
	BOOLEAN is_stcall:1;	/* FCALL triple returns structured result */
	BOOLEAN inactive:1;	/* = IR_TRUE if marked for deletion */
	BOOLEAN chk_ovflo:1;	/* = IR_TRUE if overflow checking enabled */
	int tripleno;
	TYPE	type;
	struct triple *tprev, *tnext;
	union node_u *left,*right;
	BOOLEAN	visited;	/* flag/ptr used repeatedly by various phases */
	struct list *can_access;/* for ifetch or istore triples: the list */
				/* of leaves that may be affected */
# ifdef IROPT
	struct expr *expr;
	struct list *reachdef1; /* list of sites that may define the var or expr */
	struct list *reachdef2; /* used at this point */
	struct list *canreach;	/* list of sites that may use the var or expr */
				/* defined at this  point */
	struct var_ref *var_refs; /* the var_ref records created for a triple */
	struct list *implicit_use; /*implicit u records created for this triple */
	struct list *implicit_def;
# define IROPT_TRIPLE_FIELDS 	7	/* # of fields specific to IROPT */
# endif
} TRIPLE;


/*
 * OPERAND contains those fields which MUST be
 * identical in all variants of the IR_NODE union.
 */
typedef struct operand {
	TAG tag:8; 
	int /*overloaded*/: 24;
	int number;  /* align with blockno, tripleno, leafno */
	TYPE type;
} OPERAND;

typedef union node_u {
	struct leaf leaf;
	struct triple triple;
	struct block block;
	struct operand operand;
} IR_NODE;

typedef struct header {
	int major_vers:8;	/* IR major version number */
	int minor_vers:8;	/* IR minor version number */
	long triple_offset;	/* file-relative offset to triples table */
	long block_offset;	/* file-relative offset to basic block list */
	long leaf_offset;	/* file-relative offset to leaf table */
	long seg_offset;		/* file-relative offset to segment table */
	long string_offset;	/* file-relative offset to string table */
	long list_offset;	/* file-relative offset to list nodes pool */
	long proc_size;		/* file-relative offset to end of proc data */
	int procno;	/* procedure id number; for back end communication */
	int nptrs;		/* number of trackable pointers in leaf table */
	int nels;		/* number of potential aliases in leaf table */
	char *procname;		/* procedure name */
	FILE_STATUS file_status; /* { UNOPTIMIZED, OPTIMIZED } */
	LANG lang;		/* source language - do aliasing if not F77 */
	int regmask;		/* regs reserved by front end or optimizer */
#if TARGET == SPARC
	int fregmask;		/* floating point registers reserved by optimizer */
#endif
        TYPE proc_type; /* type of fn result -- also, size of str fn result */
	char *source_file;	/* source file name -- for profiling, errors */
	int source_lineno;	/* source line no. of proc. entry -- same  */
	int opt_level;		/* optimization level (0 = disable) */
} HEADER;

typedef struct string_buff {
	struct string_buff *next;
	char *data, *top, *max;
} STRING_BUFF; 

# define STRING_BUFSIZE 1024

# define UN_OP (1<<0)		/* unary */
# define BIN_OP (1<<1)		/* binary */
# define VALUE_OP (1<<2)	/* computes a subexpression value */
# define BRANCH_OP (1<<3)	/* may alter control flow */
# define MOD_OP (1<<4)		/* may modify 1st operand */
# define USE1_OP (1<<5)		/* may use 1st operand */
# define USE2_OP (1<<6)		/* may use 2nd operand */
# define BOOL_OP (1<<7)		/* could be a boolean operation */
# define FLOAT_OP (1<<8)	/* could be a floating operation */
# define HOUSE_OP (1<<9)	/* housekeeping operator */
# define COMMUTE_OP (1<<10)	/* operator is commutative */
# define ROOT_OP (1<<11)	/* operator roots a pcc tree */
# define NTUPLE_OP (1<<12)	/* triple's right child holds additional triples */
# define ISOP(op,attribute) (op_descr[(int)(op)].flags & attribute)

extern struct opdescr_st {
	char name[20];		/* ascii name of operator */
	int flags;		/* attributes : arity, side-effects, etc. */
	struct {		/* MACHINE-DEPENDENT: register allocation */
		int d_weight, a_weight;
		} left, right;
	int f_weight;		/* MACHINE-DEPENDENT: register allocation */
} op_descr[];

#define LCAST(list,structname) ((structname*)((list)->datap))
#define LNULL   ((LIST*) NULL)
#define TNULL   ((TRIPLE*) NULL)
#define LFOR(list,head)\
	for((list)=(head); (list);\
	    ((list)=(list)->next,\
		((list)==(head)? (list)=LNULL : (list))))

#define TFOR(triple,head)\
	for((triple)=(head); (triple);\
	    ((triple)=(triple)->tnext,\
		((triple)==(head)? (triple)=TNULL : (triple))))

/*
 *	append an item to a circular list AND update the pointer to the list
 */
#define LAPPEND(after,item) {\
	register LIST *tmp1;\
	if((after) != LNULL) {\
		tmp1 = (item)->next;\
		(item)->next = (after)->next; \
		(after)->next = tmp1;\
	} (after) = (item);\
}

#define TAPPEND(after,item) {\
	register TRIPLE *tmp1, *tmp2;\
	if((after) != TNULL) {\
		tmp1 = (item)->tnext;\
		tmp2 = (after)->tnext;\
		(after)->tnext = tmp1;\
		(item)->tnext = tmp2; \
		tmp1->tprev = (after);\
		tmp2->tprev  = (item);\
	} (after) = (item);\
}

extern STRING_BUFF *string_buff, *first_string_buff;
extern BLOCK *first_block;
extern LEAF *first_leaf;

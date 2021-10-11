/*	@(#)structs.h	1.1	10/31/94	*/

/* This file defines globally-used structures, 
 * and also defines many cross-file #define'd values and #define'd macros.
 *
 * Any #define'd symbols which begin with "_" are intended only for use
 * within this include file, and shouldn't need to be referenced in a ".c"
 * file.
 */

/* These allow declaration of bitfields with less-restrictive alignment
 * than declaring them "int" or "Bool".
 */
#define BITINT		unsigned char
#define BITBOOL		unsigned char

#define MASK(N) ((1<<(N)) -1)   /* a mask of N low-order '1' bits */

typedef	unsigned int	Bool;
#undef FALSE
#undef TRUE
#define FALSE	((Bool)0)
#define TRUE	((Bool)1)

typedef enum
{
	OPER_NONE=0,		/* NULL value */

	/* values associated with TOK_HILO */
	OPER_HILO_NONE, OPER_HILO_HI, OPER_HILO_LO,

	/* values associated with TOK_ADD */
	OPER_ADD_PLUS, OPER_ADD_MINUS,

	/* values associated with TOK_MULT */
	OPER_MULT_MULT, OPER_MULT_DIV, OPER_MULT_MOD,

	/* values associated with TOK_SHIFT */
	OPER_SHIFT_RIGHT, OPER_SHIFT_LEFT,

	/* value assciated with EQUATE's in certain circumstances */
	OPER_EQUATE,

	/* value associated with emit-time or post-emit-time evaluation of
	 * special symbol "."
	 */
	OPER_DOT,

	/* values associated with TOK_LOG */
	OPER_LOG_AND, OPER_LOG_OR, OPER_LOG_XOR,

	/* value associated with */
	OPER_BITNOT
	
}  OperatorType;

	/* assembler-internal segment #s */
typedef int SEGMENT;
#define ASSEG_ABS	((SEGMENT)(-1))	/* special-case segment # */
#define ASSEG_FILENAME	((SEGMENT)(-2))	/* special-case segment # */
#define ASSEG_UNKNOWN	((SEGMENT)(-3))	/* special-case segment # */
#define ASSEG_DCOMMON	((SEGMENT)(-4))	/* special-case segment # */
#define ASSEG_SDCOMMON	((SEGMENT)(-5))	/* special-case segment # */

typedef enum
{
	/**REL_ABS,**/
	REL_NONE=0/* (used internally only, in actions*.c) */,

	REL_8, REL_16, REL_32,
	REL_LO10, REL_13, REL_22, REL_HI22, REL_DISP22, REL_DISP30,

	REL_SFA_BR,	/* Short-form addressing base register		*/
	REL_SFA_OF,	/* Short-form addressing (13-bit) offset	*/

	REL_SFA,	/* Short-Form Addressing: internal shorthand for
			 *	BOTH of the above (SFA_BR+SFA_OF).
			 */

			/* Following are PIC code relocations. */
	REL_BASE10, REL_BASE13, REL_BASE22, REL_PC10, REL_PC22, REL_JMP_TBL,
} RELTYPE;

typedef int		    LINENO;	/* for storage of input line#s	      */
typedef unsigned long int   NODESEQNO;	/* sequence # for nodes		      */
typedef int		    STDIOCHAR;	/* char returned by std-i/o routines  */
typedef unsigned int	    TOKEN;	/* token value, for lex & parser      */

typedef unsigned char	    U8BITS;	/* unsigned 8-bit quantity	      */
typedef unsigned short int  U16BITS;	/* unsigned 16-bit quantity	      */
typedef unsigned long int   U32BITS;	/* unsigned 32-bit quantity	      */
typedef /*signed*/short int S16BITS;	/*   signed 16-bit quantity	      */
typedef /*signed*/long  int S32BITS;	/*   signed 32-bit quantity	      */
typedef double		    FLOATVAL;	/* for floating-point values	      */

typedef U32BITS		LOC_COUNTER;	/* for storage of location counter(s) */
typedef U32BITS		INSTR;		/* 32-bit instruction word 	      */
typedef U32BITS		IGROUP;		/* bit vector for instruction groups  */

typedef unsigned int	UINT;		/* convenience typedef - abbr. uns.int*/

typedef struct
{
	/* values for an OPTIONS variable */
	BITBOOL	opt_preproc :1;	/* option to run C preprocessor on input file */
	BITBOOL	opt_L_lbls  :1;	/* option to save 'L' labels in output sym tbl*/
} Options;		/* bit vector for ass'y options	      */

typedef enum {VRM_NONE, VRM_LO10, VRM_HI22} RELMODE;
typedef U16BITS STRLEN;

struct value
{
	long int v_value;	/* The value of interest.  If v_symp == NULL,
				 *	then this is an absolute value;
				 *	otherwise an offset from the value of
				 *	of the symbol referenced by v_symp.
				 */
	char	*v_strptr;	/* ptr to char string if value is from a
				 * 	quoted string  (NULL if not)
				 */
	STRLEN	 v_strlen;	/* length of string pointed to by v_strptr
				 *	(undefined if v_strptr == NULL)
				 */
	RELMODE  v_relmode:8;	/* relocation flags (whether %lo or %hi
				 *	are to be applied to this value)
				 */
	struct symbol *v_symp;	/* ptr to symbol, whose value should be
				 *	added to this (NULL if none)
				 */
	struct value *v_next;	/* ptr to next value in list, if any.
				 *	(list of values used by Pseudo's)
				 */
};

#define VALUE_IS_ABSOLUTE(vp)		((vp)->v_symp == NULL)
#define VALUE_IS_QUOTED_STRING(vp)	((vp)->v_strptr != NULL)

typedef S32BITS SymbolIndex;

struct symbol
{
	char	     *s_symname;/* ptr to null-terminated Ascii symbol name*/

	SEGMENT	      s_segment;/* segment in which symbol is defined */
	LOC_COUNTER   s_value;	/* if (s_segment == ASSEG_ABS)
				 *	then, some absolute value.
				 *	else, symbol's offset from beginning
				 *		of its segment.
				 */

	U8BITS	      s_attr;	/* symbol attributes, #defined below	*/
#define SA_DEFN_SEEN	0x01	/* defnition for symbol has been seen in the
				 *    current module (this only used during
				 *    input pass)
				 */
#define SA_DEFINED	0x02	/* symbol has been defined in current module
				 *    (used during assembly pass)
				 */
#define SA_GLOBAL	0x04	/* symbol has been declared to be global      */
#define SA_LOCAL	0x08	/* symbol is local ONLY; don't put in .o file */
#define SA_COMMON	0x10	/* symbol is a COMMON symbol		      */
#define SA_EXPRESSION	0x20	/* symbol is really an EXPRESSION, whose
				 *    evaluation must be delayed until after
				 *    optimization, but done before assembly.
				 */
#define SA_DYN_EVAL     0x40    /* Each copy of this symbol has a unique value;
				 *    not even kept in sym table.  Needed for
				 *    '.' and expressions involving it.
				 */
#define SA_NOWRITE	0x80	/* do not write symbol to the object file
				 *    (either because it should not be written
				 *     out at all, or it has already been
				 *     written out)
				 */

	OperatorType  s_opertype:8;/* operator type, valid only when
				 *    SA_EXPRESSION is on; identifies operation
				 *    to be performed on s_def_node/s_def_node2.
				 */

	struct node *s_def_node;/* ptr to primary node which defines this sym*/
	struct node *s_def_node2;/*ptr to secondary node used in defining
				 *    this symbol; valid only when SA_EXPRES-
				 *    SION is on.  It is the second symbol
				 *    referenced in a binary expression.
				 */

	U16BITS	     s_ref_count;/* # of nodes in s_ref_list		 */
	struct node *s_ref_list;/* ptr to list of nodes which reference (not
				 *	define) this symbol.  The nodes structs
				 *	are chained by their "n_symref_next"
				 *	fields.
				 */
	struct basic_block
		    *s_blockname;/* for labels only, the pointer to the basic
				  *     block which this label dominates.
				  */

	SymbolIndex   s_symidx;	/* symbol index in output obj file;
				 *	invalid (value = SYMIDX_NONE) until
				 *	sym has been written out to file
				 */
#define SYMIDX_NONE (-1)	/* "not-yet-valid" value for s_symidx	*/

	struct symbol *s_chain;	/* ptr to next symbol in hash chain	*/
};

#define SYMBOL_IS_ABSOLUTE_VALUE(symp)	(symp->s_segment == ASSEG_ABS)


/* functional groups, mainly for use by (optional) optimization pass */
enum functional_group
{
	FG_error=0,	/* don't want a REAL func group to be zero by accident*/

	FG_NONE,	/* in no functional group */

	FG_MARKER,	/* for a beginning-of-node-list Marker node */

	FG_COMMENT,	/* for a comment node */

	FG_SYM_REF,	/* just a place-holder node for a symbol reference */

	/* for Integer instructions */
	FG_LD, FG_ST, FG_LDST, FG_ARITH, FG_WR, FG_SETHI, FG_BR, FG_TRAP,
	FG_CALL, FG_JMP, FG_JMPL, FG_RETT, FG_IFLUSH, FG_UNIMP, FG_RD,

	/* for 2- and 3-operand Floating-Point instructions */
	FG_FLOAT2, FG_FLOAT3,

	/* for Coprocessor instructions */
	FG_CP,

	/* for various pseudo-instructions */
	FG_RET, FG_NOP,
	FG_LABEL, FG_SET, FG_EQUATE,
	
	/* for pseudo-operations */
	FG_PS/*pseudo*/,

	/* for ".word" in an optimizable segment	  */
	FG_CSWITCH,
};

/* functional groups, mainly for use by (optional) optimization pass */
enum functional_subgroup
{
	FSG_error=0,	/* don't want one to be zero by accident */

	FSG_NONE,	/* in no functional subgroup */

	FSG_common,	/* The mutually-exclusive subgroup lists can all
			 * start with this value.
			 * This allows use to put an enum of this type into a
			 * potentially narrower bitfield.
			 */

	/* Subgroups for FG_LDST functional group */
	FSG_LSB/*=FSG_common*/,	/* LDSTUB, LDSTUBA	*/
	FSG_SWAP,		/* SWAP, SWAPA		*/
#ifdef CHIPFABS /* only in fab#1 chip */
	FSG_LSW,		/* LDST, LDSTA		*/
#endif /*CHIPFABS*/

	/* Subgroups for FG_ARITH functional group, i.e. Integer
	 * arithmetic/logical/save/restore instructions.
	 */
	FSG_ADD/*=FSG_common*/,
	FSG_SUB,  FSG_MULS,
	FSG_AND,  FSG_ANDN, FSG_OR,   FSG_ORN,  FSG_XOR,  FSG_XORN,
	FSG_SLL,  FSG_SRL,  FSG_SRA,
	FSG_UMUL, FSG_SMUL, FSG_UDIV, FSG_SDIV, FSG_TADD, FSG_TSUB,
	FSG_SAVE, FSG_REST,

	/* Subgroups for FG_RD functional group.
	 */
	FSG_RDY/*=FSG_common*/,
	FSG_RDPSR,
	FSG_RDWIM,
	FSG_RDTBR,

	/* Subgroups for FG_WR functional group.
	 */
	FSG_WRY/*=FSG_common*/,
	FSG_WRPSR,
	FSG_WRWIM,
	FSG_WRTBR,

	/* Subgroups for FG_FLOAT2 functional group,
	 * i.e. 2-operand Floating-Point instructions.
	 */
	FSG_FCVT/*=FSG_common*/,
	FSG_FMOV, FSG_FNEG, FSG_FABS,
	FSG_FCMP,
	FSG_FSQT,

	/* Subgroups for FG_FLOAT3 functional group,
	 * i.e. 3-operand Floating-Point arithmetic instructions.
	 */
	FSG_FADD/*=FSG_common*/,
	FSG_FSUB, FSG_FMUL, FSG_FDIV,

	/* Subgroups of FG_CP for cpop1 and cpop2 */
	FSG_CP1, FSG_CP2,

	/* Subgroups for FG_RET functional group.  */
	FSG_RET/*=FSG_common*/,
	FSG_RETL,

	/* Subgroups for FG_PS functional group, i.e. pseudo operations and
	 * pseudo-instructions.
	 */
	FSG_SEG/*=FSG_common*/,/* ".seg"	*/
	FSG_OPTIM,	/* ".optim"	*/
	FSG_ASCII,	/* ".ascii"	*/
	FSG_ASCIZ,	/* ".asciz"	*/
	FSG_GLOBAL,	/* ".global"	*/
	FSG_COMMON,	/* ".common"	*/
	FSG_RESERVE,	/* ".reserve"	*/
	FSG_EMPTY,	/* ".empty"	*/
	FSG_BOF,	/* "*.bof" (internal: begin of file) */
	FSG_STAB,	/* ".stab[dns]"	*/
	FSG_SGL,	/* ".single"	*/
	FSG_DBL,	/* ".double"	*/
	FSG_QUAD,	/* ".quad"	*/
			/*--------------*/
	FSG_BYTE,	/* ".byte"	*/
	FSG_HALF,	/* ".half"	*/
	FSG_WORD,	/* ".word"	*/
	FSG_ALIGN,	/* ".word", ".half", ".byte" with no args */
	FSG_SKIP,	/* ".skip"	*/
	FSG_PROC,	/* ".proc"	*/
	FSG_NOALIAS,	/* ".noalias"	*/
	FSG_ALIAS,	/* ".alias"	*/
        FSG_IDENT,      /* ".ident"     */

	/* Subgroups for FG_BR functional group.  */
			/*-------------- shared by integer & floating branches*/
	FSG_N/*=FSG_common*/,/* never	*/
	FSG_A,		/* always	*/
			/*-------------- integer branches only */
	FSG_E,		/* ==		*/
	FSG_LE,		/* <=		*/
	FSG_L,		/* <		*/
	FSG_LEU,	/* <=, unsigned	*/
	FSG_CS,		/* carry set	*/
	FSG_NEG,	/* < 0		*/
	FSG_VS,		/* overflow set	*/
	FSG_NE,		/* !=		*/
	FSG_G,		/* >		*/
	FSG_GE,		/* >=		*/
	FSG_GU,		/* >, unsigned	*/
	FSG_CC,		/* carry clear	*/
	FSG_POS,	/* >= 0		*/
	FSG_VC,		/* overflow clr	*/
			/*-------------- floating branches only */
	FSG_FU,		/* unordered		*/
	FSG_FG,		/* >			*/
	FSG_FUG,	/* unordered or >	*/
	FSG_FL,		/* <			*/
	FSG_FUL,	/* unordered or <	*/
	FSG_FLG,	/* < or >		*/
	FSG_FNE,	/* !=			*/
	FSG_FE,		/* ==			*/
	FSG_FUE,	/* unordered or ==	*/
	FSG_FGE,	/* >=			*/
	FSG_FUGE,	/* unordered or >=	*/
	FSG_FLE,	/* <=			*/
	FSG_FULE,	/* unordered or <=	*/
	FSG_FO,		/* ordered		*/
			/*-------------- coprocessor branches only */
	FSG_C3,		/* unordered		*/
	FSG_C2,		/* >			*/
	FSG_C23,	/* unordered or >	*/
	FSG_C1,		/* <			*/
	FSG_C13,	/* unordered or <	*/
	FSG_C12,	/* < or >		*/
	FSG_C123,	/* !=			*/
	FSG_C0,		/* ==			*/
	FSG_C03,	/* unordered or ==	*/
	FSG_C02,	/* >=			*/
	FSG_C023,	/* unordered or >=	*/
	FSG_C01,	/* <=			*/
	FSG_C013,	/* unordered or <=	*/
	FSG_C012,	/* ordered		*/
};

/* "enum node_union_used" tells which element of "n_union" in a "struct node"
 * is valid.
 */
enum node_union_used { NU_NONE=0, NU_VLHP, NU_OPS, NU_FLP, NU_STRG };

/* "enum operand_width" tells the width of an operand in bytes. */
typedef unsigned int OperandWidth;
#define SN	0	/*abbreviation for SIZE:NONE for an OperandWidth field*/

/* converts # of bytes to # of words, rounded up */
#define BYTES_TO_WORDS(n)       (((n)+3)/4)

/* "enum cond_codes" tells which set of condition codes: ICC, FCC, or CP CC */
enum cond_codes { CC_NONE, CC_I, CC_F, CC_C };
#define CN  CC_NONE


struct func_grp
{
	/* The following fields apply to all opcode mnemonics.
	 * Any changes to this structure must be reflected in file
	 * "functional_groups.h".
	 */

	enum functional_group
		f_group       :8;/* instruction's general functional grouping */
	enum functional_subgroup
		f_subgroup    :8;/* sub-group within above functional group   */
	enum node_union_used
		f_node_union  :3;/* which subfield in the n_union field is used
				  * by this opcode when in a node.
				  */
	BITBOOL	f_has_delay_slot:1;/* 1=instruction has delay slot, 0=doesn't */
	BITBOOL f_setscc      :1;/* 0=CC's unaffected, 1=CC's are set by instr*/
	BITBOOL f_generates_code:1;/* 1=this instr generates code, and should
				  * therefore be deleted in sections of dead
				  * code.
				  */
	BITBOOL f_accesses_fpu:1;/* 1=this instr access the FPU */

	/* would have made each of the following groups share space via a UNION
	 * if we could have initialized a union at compile time (the tables in
	 * "lex_tables.c").
	 */

	/* Following for f_group == FG_LD, FG_LDST, FG_ST, and FG_FLOAT* only;
	 * stored widths are in Bytes, which are 4..16 (1..4 Words) for
	 * Floating-point operations, and 1..8 Bytes for Integer operations
	 * (for Bytes to DoubleWords).  The value "SN" is used when there is
	 * no size associated with the operand.
	 */
	OperandWidth f_rs1_width :5;	/* width of RS1 operand.	*/
	OperandWidth f_rs2_width :5;	/* width of RS2 operand.	*/
	OperandWidth f_rd_width  :5;	/* width of RD  operand.	*/

	/* Following for f_group == FG_LD, FG_LDST, and FG_ST only*/
	BITBOOL	  f_ldst_alt  :1;	/* 0=regular, 1=alt-space    */

	/* Following for f_group == FG_LD only */
	BITBOOL	  f_ld_signed :1;	/* 0=unsigned, 1=signed load */

	/* Following for f_group == FG_ARITH only	*/
	BITBOOL	  f_ar_commutative:1;	/* 0=oper'n not commutative; 1=it is */

	/* Following for f_group == FG_BR and FG_TRAP only	*/
	enum cond_codes f_br_cc:2;	/* branch on ICC, FCC, or CP CC's*/

	/* Following for f_group == FG_BR only	*/
	BITBOOL	  f_br_annul   :1;	/* 0=annul not used, 1=annul used    */

#ifdef CHIPFABS
	BITBOOL	  f_fp_1164    :1;	/* 1=exec's on Weitek 1164 chip */
	BITBOOL	  f_fp_1165    :1;	/* 1=exec's on Weitek 1165 chip */
#endif /*CHIPFABS*/
};

enum binary_fmt
{
	BF_NONE=0,	/* for pseudo-instructions, etc, which should never get
			 * as far as being included in an instruction node (at
			 * least not without having been converted to something
			 * else).
			 */

	BF_IGNORE,	/* for nodes which should get as far as emit_*(),
			 * but should just be ignored at that point.
			 */

	/* see "instructions.h" for description of how the following are named*/
	BF_0,
	BF_1,     BF_2a,    BF_2b,
	BF_3ab,
	BF_3cd11, BF_3cd00, BF_3cd01, BF_3cd10, BF_3cd1X,
	BF_3e11,  BF_3e10,  BF_3e01, BF_3e11_CPOP,

	BF_LABEL,	/* for label nodes			*/
	BF_SET,		/* for "set" pseudo-instruction nodes	*/
	BF_EQUATE,	/* for "=" pseudo-instruction nodes	*/
	BF_MARKER,	/* for the "marker" node at the start of each segment */
	BF_PS,		/* for pseudo-operations (note:not pseudo-instructions)
			 * of functional group FG_PS, which DO get put into a
			 * node, but of course don't assemble into actual
			 * instructions.
			 */
	BF_CSWITCH,	/* for "*.cswitch" pseudo-instruction nodes	*/
	BF_FMOV2, 	/* for fmovd{d,x} pseudo instruction */
	BF_LDST2,	/* for ld2/st2 pseudo instructions   */
};

struct opcode
{
	char	       *o_name;	/* ptr to null-terminated opcode mnemonic    */
	TOKEN		o_token;/* token value for this opcode		     */
	INSTR		o_instr;/* prototype binary opcode for this instr.   */
	/*OLD*/IGROUP	o_igroup;/* instruction group it belongs to [**OLD**]*/
	/**NEW, not yet implemented, in place of o_igroup.
	/** U32BITS	o_syntax;/* allowable instruction syntax (i.e. parse
	/**			 * rules) for this opcode mnemonic.
	/**			 */
	/** **/
	struct func_grp	o_func;	/* instruction's functional grouping, for
				 * use in the (optional) optimization pass.
				 */
	enum binary_fmt	o_binfmt:8;/* instruction's binary format, for use
				 * during assembly into object code.
				 */
	struct opcode  *o_chain;/* ptr to next opcode in sym-tbl hash chain. */
	char	       *o_synonym;/* ptr --> opcode to which this maps,if any.*/
};

/* purely convenience macros */
#define NODE_HAS_DELAY_SLOT(np)   ((np)->n_opcp->o_func.f_has_delay_slot)
#define NODE_SETSCC(np)		  ((np)->n_opcp->o_func.f_setscc)
#define NODE_READS_I_CC(np) 	  ((np)->n_opcp->o_func.f_br_cc == CC_I)
#define NODE_GENERATES_CODE(np)   ((np)->n_opcp->o_func.f_generates_code)
#define NODE_ACCESSES_FPU(np)     ((np)->n_opcp->o_func.f_accesses_fpu)
#define NODE_IS_MARKER_NODE(np)   ((np)->n_opcp->o_func.f_group   ==FG_MARKER)
#define NODE_IS_CSWITCH(np)	  ((np)->n_opcp->o_func.f_group   ==FG_CSWITCH)
#define NODE_IS_LABEL(np)	  ((np)->n_opcp->o_func.f_group   ==FG_LABEL)
#define NODE_IS_NOP(np)		  ((np)->n_opcp->o_func.f_group   ==FG_NOP)
#define NODE_IS_BRANCH(np)	  ((np)->n_opcp->o_func.f_group   ==FG_BR)
#define NODE_IS_FBRANCH(np)	 (NODE_IS_BRANCH(np) && \
				  ((np)->n_opcp->o_func.f_br_cc == CC_F) )
#define NODE_IS_FCMP(np)	  ((np)->n_opcp->o_func.f_group   ==FG_FLOAT2 && \
				   (np)->n_opcp->o_func.f_subgroup   ==FSG_FCMP)
#define NODE_IS_TRAP(np)	  ((np)->n_opcp->o_func.f_group   ==FG_TRAP)
#define NODE_IS_ARITH(np)	  ((np)->n_opcp->o_func.f_group   ==FG_ARITH)
#define NODE_IS_SAVE(np)	 (NODE_IS_ARITH(np) && \
				  ((np)->n_opcp->o_func.f_subgroup==FSG_SAVE) )
#define NODE_IS_RESTORE(np)	 (NODE_IS_ARITH(np) && \
				  ((np)->n_opcp->o_func.f_subgroup==FSG_REST) )
#define NODE_IS_PSEUDO(np)	  ((np)->n_opcp->o_func.f_group   ==FG_PS)
#define NODE_IS_EMPTY_PSEUDO(np) (NODE_IS_PSEUDO(np) && \
				  ((np)->n_opcp->o_func.f_subgroup==FSG_EMPTY) )
#define NODE_IS_PROC_PSEUDO(np)	 (NODE_IS_PSEUDO(np) && \
				  ((np)->n_opcp->o_func.f_subgroup==FSG_PROC) )
#define NODE_IS_SEG_PSEUDO(np)	 (NODE_IS_PSEUDO(np) && \
				  ((np)->n_opcp->o_func.f_subgroup==FSG_SEG) )
#define NODE_IS_OPTIM_PSEUDO(np) (NODE_IS_PSEUDO(np) && \
				  ((np)->n_opcp->o_func.f_subgroup==FSG_OPTIM) )
#define NODE_IS_CALL(np)	  ((np)->n_opcp->o_func.f_group   ==FG_CALL || \
				    (np)->n_opcp->o_func.f_group   ==FG_JMPL)
#define NODE_IS_PIC_CALL(np)	  (NODE_IS_CALL(np) && \
				   (np)->n_operands.op_symp != NULL &&  \
				   (np)->n_operands.op_symp->s_def_node != NULL &&  \
				   (np)->n_next != NULL && \
				   (np)->n_operands.op_symp->s_def_node == np->n_next->n_next )	
#define NODE_IS_JMP(np)		  ((np)->n_opcp->o_func.f_group   ==FG_JMP)
#define NODE_IS_RET(np)		  ((np)->n_opcp->o_func.f_group   ==FG_RET )
#define NODE_IS_UNIMP(np)	  ((np)->n_opcp->o_func.f_group   ==FG_UNIMP )
#define NODE_IS_EQUATE(np)   	  ((np)->n_opcp->o_func.f_group   ==FG_EQUATE)
#define NODE_IS_MULTIPLY(np)	  ((np)->n_opcp->o_func.f_subgroup==FSG_FMUL )

#define NODE_IS_ALIAS(np)	  (( NODE_IS_PSEUDO(np)) && \
					(((np)->n_opcp->o_func.f_subgroup==FSG_ALIAS) || \
					 ((np)->n_opcp->o_func.f_subgroup==FSG_NOALIAS)))

	/* NODE_IS_LOAD() inlcudes F.P. LOAD, but not LOAD_STORE */
#define NODE_IS_LOAD(np)	  ((np)->n_opcp->o_func.f_group   ==FG_LD )
#define NODE_IS_STORE(np)	  ((np)->n_opcp->o_func.f_group   ==FG_ST )
#define NODE_IS_FMOVE(np)	  ((np)->n_opcp->o_func.f_subgroup==FSG_FMOV )
#define NODE_IS_FNEG(np)	  ((np)->n_opcp->o_func.f_subgroup==FSG_FNEG )
#define NODE_IS_FABS(np)	  ((np)->n_opcp->o_func.f_subgroup==FSG_FABS )
#define NODE_BYPASS_FPU(np)	  ((NODE_IS_LOAD(np)) || (NODE_IS_STORE(np)))
/* Whatch for the meaning of this it may not be what you think.  It is ONLY used
 * carefully in opt_resequence.c
 */
#define NODE_IS_DOUBLE_FLOAT(np)  (((np)->n_opcp->o_func.f_rd_width == 8) && \
				   ((np)->n_opcp->o_func.f_subgroup != FSG_FCVT))

/* a structure referring to a list of "struct value"s */
struct val_list_head
{
	UINT	 vlh_listlen;	/* # of elements in the list		*/
	struct value *vlh_head;/* ptr to the first element in the list*/
	struct value *vlh_tail;/* ptr to the last  element in the list*/
};


struct filelist
{
	char   *fl_fnamep;	/* ptr to name of input file		 */
	Options fl_options;	/* ass'y options in effect for this file */
	struct filelist *fl_next;/* ptr to next input file info		 */
};

/*---------------------------------------------------------------------------*/

/* The types "regno" and "register_set" (and the _REG_* macros) are really
 * associated with file "registers.h", but are needed here because of their
 * usage in the "opcode" and "node" structures below.
 *
 * See "registers.h" for lots more information on them.
 */

typedef unsigned char Regno;	/* Holds a register #, i.e. one of the RN_*
				 * values defined in registers.h .
				 */

/*
 * The register_set type is manipulated by routines in opt_regsets.c .
 * The macros defined here should be considered to be exported from that
 * file, and in-lined as an effeciency hack.
 * This type is a struct so that it can be manipulated by value, because
 * in C, a mere array cannot be.
 */

	/* Number of Bits for ALL RESources -- see registers.h for its
	 * derivation.
	 */
#define _NB_ALL_RES 107

	/* Number of Long Ints needed to represent every register with 1 bit */
#define	_NLI_FOR_ALLREGS ((_NB_ALL_RES+31)/32)

typedef struct { long unsigned int r[_NLI_FOR_ALLREGS]; }	register_set;

	/* The following definition MUST be an exact copy of the one in
	 * "registers.h":
	 */
#define _REG_nflags	3	/* 3 flags per reg: Read, Written, Live */

/*---------------------------------------------------------------------------*/

struct operands	/* operands (registers + one symbol value) */
{
	/* following are index values for op_regs[] */
#define O_RS1 0	/* RS1 register operand	*/
#define O_RS2 1	/* RS2 register operand	*/
#define O_RD  2	/* RD register operand	*/
#define O_N_OPS 3 /* max # of register operands per stmt */
	Regno		op_regs[O_N_OPS];
	BITBOOL		op_imm_used:1;	/* TRUE if an immediate value is given;
					 *	needed to distinguish between
					 *	rs1+%g0 and rs1+0 in format-3
					 *	instructions.
					 * Also, if TRUE, then op_regs[O_RS2]
					 *	is unused.
					 */
	BITBOOL		op_asi_used:1;	/* TRUE if ASI is given		*/
	BITBOOL		op_call_oreg_count_used:1;
					/* TRUE if op_call_oreg_count is used */
	BITINT		/*unused*/ :5;

#ifdef DEBUG	/* easier to DEBUG (e.g. with DBX) without the darned union! */
		BITINT	op_asi     :8;	/* Alternate Space Index, if given */
		BITINT	op_call_oreg_count :8;
					/* Count (number between 0 and 6) of
					 * args in OUT-regs, for CALL nodes.
					 */
	        BITINT  op_cp_opcode:8; /* Keeps the coprocessor opcode */
#else /*DEBUG*/
	union
	{
		U8BITS	op_u_asi;	/* Alternate Space Index, if given */
		U8BITS	op_u_call_oreg_count;
					/* Count (number between 0 and 6) of
					 * args in OUT-regs, for CALL nodes.
					 */
		U8BITS op_u_cp_opcode;  /* Keeps the coprocessor opcode */
# define op_asi			op_union.op_u_asi
# define op_call_oreg_count	op_union.op_u_call_oreg_count
# define op_cp_opcode           op_union.op_u_cp_opcode
	} op_union;
#endif /*DEBUG*/

	RELTYPE		op_reltype :8;	/* if p_vp != NULL:
					 * type of relocation needed for p_symp
					 */
	struct symbol  *op_symp;	/* symbol, if any (NULL if none)  */
	long int	op_addend;	/* constant,or addend value for symbol*/
};

typedef unsigned char   OptimizationNumber;

struct node
{
	/* information generated during Phase 1 (input) and used in Phases
	 * 2 (optimization) and 3 (producing machine code).
	 */
		/* info about the source code from which this instruction came*/
	struct opcode	*n_opcp;	/* ptr to opcode-mnemonic info.	      */
	LINENO		n_lineno;	/* input line # of instruction 	      */
	BITBOOL		n_internal :1;	/* =TRUE: node was created by assembler,
					 *    not explicitly from user input.
					 */
	BITBOOL		n_mark	   :1;	/* used in optimization phases to note
					 * nodes already queued for processing.
					 */
#ifdef DEBUG	/* easier to DEBUG (e.g. with DBX) without the darned union! */
	OptimizationNumber n_optno;	/* for debugging: if this node was
					 *    created during an optimization,
					 *    which optimization was it?
					 */
		struct operands	      n_operands;/*see comments on these below*/
		struct val_list_head *n_vlhp;
		struct filelist      *n_flp;
		char		     *n_string;
#else /*DEBUG*/
	union
	{
		struct operands n_u_ops;/* The operands, for an actual machine
					 * instruction, LABEL, EQUATE, or SET.
					 * Valid when
					 * opcp->o_func.f_node_union == NU_OPS
					 */
		struct val_list_head *n_u_vlhp;
					/* Pointer to list of values, used for
					 * most pseudo-instruction nodes.
					 * Valid when
					 * opcp->o_func.f_node_union == NU_VLHP.
					 */
		struct filelist *n_u_flp;
					/* Pointer to "filelist" structure,
					 * used for internal "*.bof" pseudo-
					 * instruction node.
					 * Valid when
					 * opcp->o_func.f_node_union == NU_FLP.
					 */
		char	*n_u_string;	/* Pointer to a character string.
					 * Used for name of segment for a
					 * beginning-of-segment marker node,
					 * and for comment nodes.
					 * Valid when
					 * opcp->o_func.f_node_union == NU_STRG.
					 */
# define n_operands n_union.n_u_ops
# define n_vlhp     n_union.n_u_vlhp
# define n_flp      n_union.n_u_flp
# define n_string   n_union.n_u_string
	} n_union;
#endif /*DEBUG*/
		/* instruction sequence information	*/
	NODESEQNO	n_seq_no;	/* sequence # of node		      */
	struct node	*n_prev;	/* previous-node ptr		*/
	struct node	*n_next;	/* next-node ptr		*/

		/* ptr for linked list of nodes which reference the same sym */
	struct node	*n_symref_next;	/* ptr to next node which ref's sym */

	/* information used only during Phase 2: optimization */
					/* Read,Written,and Live reg bits*/
	register_set	n_regs[_REG_nflags];

	/* Instruction Scheduling */
	struct list_node *depend_list;/*list of insts that depend on this node*/
	short	dependences;	/*# of nodes this inst is still DEPENDED on by*/
	short	inst_fpu_dep;	/*# of FPU insts that depend on this node     */
	short	inst_iu_dep;	/*# of IU insts that depend on this node      */
	short	delay_candidate;/*# of delay slots, this inst are qualified to*/
				/* fill in.  It's qualify to fill inst A's    */
				/* delay slot IFF A is directly depend on this*/
				/* inst and is "exchangable" (in opt_scheduling.c) */
				/* with A */
	struct list_node *cti_node;/* list of cti of this candidate node */
};

typedef struct node	Node;

/* Convenience macro:
 *	opcode function description; has f_group, f_subgroup, etc.
 */
#define OP_FUNC(np)	((np)->n_opcp->o_func)
/* Other convenience macro */
#define BYTE_OFFSET(n)	     (n == 24)
#define HALFW_OFFSET(n)	     (n == 16)
#define BYTE_OR_HW_OFFSET(n) (BYTE_OFFSET(n) || HALFW_OFFSET(n))


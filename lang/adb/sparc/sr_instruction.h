/* "@(#)sr_instruction.h 1.1 94/10/31 Copyr 1986 Sun Micro";
 * From Will Brown's "sas" Sparc Architectural Simulator,
 * Version "@:-)instruction.h	3.1 11/11/85
 */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* instruction.h renamed to sr_instruction.h
	Instruction formats, and some FAB1 -> FAB2 differences.
*/

/* All formats are 32 bits. All instructions fit into an unsigned integer */

/* General 32-bit sign extension macro */
/* The (long) cast is necessary in case we're given unsigned arguments */
#define SR_SEXT(sex,size) ((((long)(sex)) << (32 - size)) >> (32 - size))

/* Change a word address to a byte address (call & branches need this) */
#define SR_WA2BA(woff) ((woff) << 2)

/* Format 1: CALL instruction only */

/* ifdef is used to provide machine independence */

/* Instruction Field Width definitions */
#define IFW_OP		2
#define IFW_DISP30	30

#ifdef LOW_END_FIRST

struct fmt1Struct {
	u_int disp30		:IFW_DISP30;	/* PC relative, word aligned,
								disp. */
	u_int op		:IFW_OP;	/* op field = 00 for format 1 */
};

#else

struct fmt1Struct {
	u_int op		:IFW_OP;	/* op field = 00 for format 1 */
	u_int disp30		:IFW_DISP30;	/* PC relative, word aligned,
								disp. */
};

#endif

/* extraction macros. These are portable, but don't work for registers */
/* with sun's C compiler, they are also quite efficient */

#define X_OP(x)		( ((struct fmt1Struct *)&(x))->op )	
#define OP		X_OP(instruction)

#define X_DISP30(x)	( ((struct fmt1Struct *)&(x))->disp30 )
#define DISP30		X_DISP30(instruction)

/* Format 2: SETHI, Bicc, Bfcc */

#define IFW_RD		5
#define IFW_OP2	3
#define IFW_DISP22	22

#define SR_SEX22( sex ) SR_SEXT(sex, IFW_DISP22 )
#define SR_HI(disp22) ((disp22) << (32 - IFW_DISP22) )

#ifdef LOW_END_FIRST

struct fmt2Struct {
	u_int disp22		:IFW_DISP22;	/* 22 bit displacement */
	u_int op2		:IFW_OP2;	/* opcode for format 2
							instructions */
	u_int rd		:IFW_RD;	/* destination register */
	u_int op		:IFW_OP;	/* op field = 01 for format 2 */
};

#else

struct fmt2Struct {
	u_int op		:IFW_OP;	/* op field = 01 for format 2 */
	u_int rd		:IFW_RD;	/* destination register */
	u_int op2		:IFW_OP2;	/* opcode for format 2
							instructions */
	u_int disp22		:IFW_DISP22;	/* 22 bit displacement */
};

#endif;

#define X_RD(x)		( ((struct fmt2Struct *)&(x))->rd )
#define RD		X_RD(instruction)

#define X_OP2(x)	( ((struct fmt2Struct *)&(x))->op2 )
#define OP2		X_OP2(instruction)

#define X_DISP22(x)	( ((struct fmt2Struct *)&(x))->disp22 )
#define DISP22		X_DISP22(instruction)

/* The rd field of formats 2, 3, and 3I also has another form: */

#define IFW_ANNUL	1
#define IFW_COND	4


#ifdef LOW_END_FIRST

struct condStruct {
	u_int disp22		:IFW_DISP22;	/* use format 2 definition */
	u_int op2		:IFW_OP2;	/* because it is simplest */
	u_int cond		:IFW_COND;	/* cond field */
	u_int annul		:IFW_ANNUL;	/* annul field for BICC,
								and BFCC */
	u_int op		:IFW_OP;	/* common to all instructions */
};

#else

struct condStruct {
	u_int op		:IFW_OP;
	u_int annul		:IFW_ANNUL;	/* annul field for BICC,
								and BFCC */
	u_int cond		:IFW_COND;	/* cond field */
	u_int op2		:IFW_OP2;
	u_int disp22		:IFW_DISP22;
};

#endif

#define X_ANNUL(x)	( ((struct condStruct *)&(x))->annul )
#define ANNUL		X_ANNUL(instruction)

#define X_COND(x)	( ((struct condStruct *)&(x))->cond )
#define COND		X_COND(instruction)

/* format 3: all other instructions including FPop with a register source 2 */

#define IFW_OP3		6
#define IFW_RS1		5
#define IFW_IMM		1
#define IFW_OPF		8
#define IFW_RS2		5

#ifdef LOW_END_FIRST

struct fmt3Struct {
	u_int rs2		:IFW_RS2;	/* register source 2 */
	u_int opf		:IFW_OPF;	/* floating-point opcode */
	u_int imm		:IFW_IMM;	/* immediate. distinguishes fmt3
						and fmt3I */
	u_int rs1		:IFW_RS1;	/* register source 1 */
	u_int op3		:IFW_OP3;	/* opcode distinguishing
							fmt3 instrns */
	u_int rd		:IFW_RD;	/* destination register */
	u_int op		:IFW_OP;	/* op field = 1- for format 3 */
};

#else

struct fmt3Struct {
	u_int op		:IFW_OP;	/* op field = 1- for format 3 */
	u_int rd		:IFW_RD;	/* destination register */
	u_int op3		:IFW_OP3;	/* opcode distinguishing
							fmt3 instrns */
	u_int rs1		:IFW_RS1;	/* register source 1 */
	u_int imm		:IFW_IMM;	/* immediate. distinguishes fmt3
						and fmt3I */
	u_int opf		:IFW_OPF;	/* floating-point opcode */
	u_int rs2		:IFW_RS2;	/* register source 2 */
};

#endif

#define X_OP3(x)	( ((struct fmt3Struct *)&(x))->op3 )
#define OP3		X_OP3(instruction)

#define X_RS1(x)	( ((struct fmt3Struct *)&(x))->rs1 )
#define RS1		X_RS1(instruction)

#define X_IMM(x)	( ((struct fmt3Struct *)&(x))->imm )
#define IMM		X_IMM(instruction)

#define X_OPF(x)	( ((struct fmt3Struct *)&(x))->opf )
#define OPF		X_OPF(instruction)

#define X_RS2(x)	( ((struct fmt3Struct *)&(x))->rs2 )
#define RS2		X_RS2(instruction)

/* format 3I: all other instructions (except FPop) with immediate source 2 */

#define IFW_SIMM13	13

#define SR_SEX13( sex ) SR_SEXT(sex, 13 )

#ifdef LOW_END_FIRST

struct fmt3IStruct {
	u_int simm13		:IFW_SIMM13;	/* immediate source 2 */
	u_int imm		:IFW_IMM;	/* immediate.
							distinguishes fmt3
						and fmt3I */
	u_int rs1		:IFW_RS1;	/* register source 1 */
	u_int op3		:IFW_OP3;	/* opcode distinguishing
							fmt3 instrns */
	u_int rd		:IFW_RD;	/* destination register */
	u_int op		:IFW_OP;	/* op field = 1- for format
									3I */
};

#else

struct fmt3IStruct {
	u_int op		:IFW_OP;	/* op field = 1- for format
									3I */
	u_int rd		:IFW_RD;	/* destination register */
	u_int op3		:IFW_OP3;	/* opcode distinguishing
								fmt3 instrns */
	u_int rs1		:IFW_RS1;	/* register source 1 */
	u_int imm		:IFW_IMM;	/* immediate. distinguishes fmt3
						and fmt3I */
	u_int simm13		:IFW_SIMM13;	/* immediate source 2 */
};

#endif

#define X_SIMM13(x)	( ((struct fmt3IStruct *)&(x))->simm13 )
#define SIMM13		X_SIMM13(instruction)

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */
/*
 * #Defined constants for some of the operations
 * that the debuggers need to know about.
 */
#define SR_UNIMP_OP             0
#define SR_BICC_OP              2
#define SR_FBCC_OP              6
#define SR_SETHI_OP             4
#define SR_OR_OP 	        2

/* @(#)sas_includes.h 1.1 94/10/31 SMI" */
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * This file is from Will Brown's "sas" Sparc Architectural Simulator.
 * It is the concatenation of several header files; eventually I'll remove
 * all of the unnecessary definitions.
 */

/*
 **** **** **** **** **** **** **** **** **** **** **** **** **** ****
 * sas-file general.h (from 3.1 11/11/85)
 */

typedef unsigned long int Word;	/* Word is 32 bits, and unsigned */
typedef          long int SWord;/* SWord is 32 bits and signed */

typedef unsigned int Bool;
#define FALSE 0
#define TRUE  1

extern char *strsave();
/* compute positive powers of 2. 2^n */
#define pow2(n)  (((SWord)1) << (n) )

extern char eBuf[];	/* error message buffer */

extern char *malloc(), *realloc(), *calloc();

/*
 **** **** **** **** **** **** **** **** **** **** **** **** **** ****
 * sas-file indextables.h (from 3.1 11/11/85)
 */

/*
	The variable names for the index tables used to decode instructions
	in sas.
*/

/*
	SWITCH INDEX LOOKUP TABLES FOR DECODING INSTRUCTIONS

	The following tables are allocated dynamically, and contain switch()
	statement indexes for decoding instructions. They are indexed
	with various pieces of the instruction.
*/

struct formatSwitchTableStruct { short	index; };
extern struct formatSwitchTableStruct *formatSwitchTable;
					/* index with op field */

struct fmt2SwitchTableStruct { short	index; };
extern struct fmt2SwitchTableStruct  *fmt2SwitchTable;
					/* indexed with op2 field */

struct fmt3SwitchTableStruct { short	index; };
extern struct fmt3SwitchTableStruct  *fmt3SwitchTable;
					/* indexed with op  <concat> op3*/

struct fpopSwitchTableStruct { short	index; };
extern struct fpopSwitchTableStruct  *fpopSwitchTable;
					/* indexed with opf */

struct iccSwitchTableStruct { short	index; };
extern struct iccSwitchTableStruct  *iccSwitchTable;
					/* indexed with cond */

struct fccSwitchTableStruct { short	index; };
extern struct fccSwitchTableStruct  *fccSwitchTable;
					/* indexed with cond */

/* procedures */
extern formatCheckRecord();
extern formatExtractKey();
extern formatTransRecord();

extern fmt2CheckRecord();
extern fmt2ExtractKey();
extern fmt2TransRecord();

extern fmt3CheckRecord();
extern fmt3ExtractKey();
extern fmt3TransRecord();

extern fpopCheckRecord();
extern fpopExtractKey();
extern fpopTransRecord();

extern iccCheckRecord();
extern iccExtractKey();
extern iccTransRecord();

extern fccCheckRecord();
extern fccExtractKey();
extern fccTransRecord();


/* MNEMONIC LOOKUP TABLES FOR DIS-ASSEMBLER */

/* format 1 table will only be used to find the CALL instruction mnemonic */
struct fmt1MnemonicTableStruct { char *mnemonic; };
extern struct fmt1MnemonicTableStruct *fmt1MnemonicTable;
					/* indexed with op */

/* format 2 table will only be used to find the SETHI instruction mnemonic */
struct fmt2MnemonicTableStruct { char *mnemonic; };
extern struct fmt2MnemonicTableStruct *fmt2MnemonicTable;
					/* indexed with opx1 = op2 */

/* format 3 table will be used to find mnemonics for all format 3 instructions,
	except TICC, and FPOPs */
struct fmt3MnemonicTableStruct { char *mnemonic; };
extern struct fmt3MnemonicTableStruct *fmt3MnemonicTable;
				/* indexed with opx1 = op <concat> op3 */

/* bicc mnemonic table */
struct biccMnemonicTableStruct { char *mnemonic; };
extern struct biccMnemonicTableStruct *biccMnemonicTable;
				/* indexed with opx2 = cond */

/* bfcc mnemonic table */
struct bfccMnemonicTableStruct { char *mnemonic; };
extern struct bfccMnemonicTableStruct *bfccMnemonicTable;
				/* indexed with opx2 = cond */

/* ticc mnemonic table */
struct ticcMnemonicTableStruct { char *mnemonic; };
extern struct ticcMnemonicTableStruct *ticcMnemonicTable;
				/* indexed with opx2 = cond */

/* fpop mnemonic table */
struct fpopMnemonicTableStruct { char *mnemonic; };
extern struct fpopMnemonicTableStruct *fpopMnemonicTable;
				/* indexed with opx2 = opf */

/* pseudo-instruction mnemonic table */
struct pseudoMnemonicTableStruct { char *mnemonic; };
extern struct pseudoMnemonicTableStruct *pseudoMnemonicTable;
				/* indexed with parseToken */

/* procedures */

extern fmt1MnCheckRecord();
extern fmt1MnExtractKey();
extern fmt1MnTransRecord();

extern fmt2MnCheckRecord();
extern fmt2MnExtractKey();
extern fmt2MnTransRecord();

extern fmt3MnCheckRecord();
extern fmt3MnExtractKey();
extern fmt3MnTransRecord();

extern biccMnCheckRecord();
extern biccMnExtractKey();
extern biccMnTransRecord();

extern bfccMnCheckRecord();
extern bfccMnExtractKey();
extern bfccMnTransRecord();

extern ticcMnCheckRecord();
extern ticcMnExtractKey();
extern ticcMnTransRecord();

extern fpopMnCheckRecord();
extern fpopMnExtractKey();
extern fpopMnTransRecord();

extern pseudoMnCheckRecord();
extern pseudoMnExtractKey();
extern pseudoMnTransRecord();


/*
 **** **** **** **** **** **** **** **** **** **** **** **** **** ****
 * sas-file instruction.h (from 3.1 11/11/85)
 *
 * Instruction formats, and some FAB1 -> FAB2 differences.  All formats
 * are 32 bits.  All instructions fit into an unsigned integer
 */


/* General 32-bit sign extension macro */
#define SR_SEXT(sex,size) ((sex << (32 - size)) >> (32 - size))

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
#define	IFW_RD	5

#ifdef FAB1
#   define IFW_OP2	2
#   define IFW_DISP223	23
#else
#   define IFW_OP2	3
#   define IFW_DISP223	22
#endif

#define SR_SEX223( sex ) SR_SEXT(sex, IFW_DISP223 )
#define SR_HI(disp223) ((disp223) << (32 - IFW_DISP223) )

#ifdef LOW_END_FIRST

struct fmt2Struct {
	u_int disp223		:IFW_DISP223;	/* 22 or 23 bit displacement */
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
	u_int disp223		:IFW_DISP223;	/* 22 or 23 bit displacement */
};

#endif;

#define X_RD(x)		( ((struct fmt2Struct *)&(x))->rd )
#define RD		X_RD(instruction)

#define X_OP2(x)	( ((struct fmt2Struct *)&(x))->op2 )
#define OP2		X_OP2(instruction)

#define X_DISP223(x)	( ((struct fmt2Struct *)&(x))->disp223 )
#define DISP223		X_DISP223(instruction)

/* The rd field of formats 2, 3, and 3I also has another form: */

#define IFW_ANNUL	1
#define IFW_COND	4


#ifdef LOW_END_FIRST

struct condStruct {
	u_int disp223		:IFW_DISP223;	/* use format 2 definition */
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
	u_int disp223		:IFW_DISP223;
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

#define SR_SEX13( sex ) SR_SEXT(sex, 13)

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

/*
 **** **** **** **** **** **** **** **** **** **** **** **** **** ****
 * sas-file opcodetable.h (from 3.1 11/11/85)
 */

/*
	The structure of the opcode tables and some useful type
	constants are in this file
*/

/*
	switch-index values of NONE (0) indicate table entries that are of no
	interest to the simulator sas.

	similarly, parse token values of NONE (0) indicate table entries that
	are of no interest to the assembler.
*/

struct opcodeTableStruct {
	short		o_switchIndex;	/* a switch() index used in sas */
	short		o_type;		/* a type constant for this entry */
	unsigned char	o_op;		/* the value of the op field */
	unsigned char	o_opx1;		/* the value of a second op field */
	unsigned char	o_opx2;		/* the value of a third op field */
	char		*o_mnemonic;	/* The assembler mnemonic */
};

/* types */

#define NONE		0	/* indicates an unused entry in the table */

#define OT_F1		1	/* entry describes a format 1 instruction
					used by:	sas, sasm
					switchIndex:	used
					op:		op
					opx1:		NONE
					opx2:		NONE
					mnemonic:	NONE
				*/
#define OT_F2		2	/* format 2 instruction
					used by:	sas, sasm
					switchIndex:	used
					op:		op
					opx1:		op2
					opx2:		cond for bicc and bfcc
					mnemonic:	used
				*/
#define OT_F3		3	/* format 3 instruction
					used by:	sas, sasm
					switchIndex:	used
					op:		op
					opx1:		op <concat> op3
					opx2:		cond for ticc
					mnemonic:	used
				*/
#define OT_IC		4	/* Integer condition code field
					used by:	sas
					switchIndex:	used
					op:		cond
					opx1:		NONE
					opx2:		NONE
					mnemonic:	NONE
				*/
#define OT_FC		5	/* Floating condition code field
					used by:	sas
					switchIndex:	used
					op:		cond
					opx1:		NONE
					opx2:		NONE
					mnemonic:	NONE
				*/
#define OT_FOP		6	/* OPF field for FPOP instructions
					used by:	sas, sasm
					switchIndex:	used
					op:		op
					opx1:		op3 (FPOP)
					opx2:		opf
					mnemonic:	used
				*/
#define OT_PSEUDO	7	/* PSEUDO Instructions (assembler mnemonics)
					used by:	sas
					switchIndex:	NONE
					op:		NONE
					opx1:		NONE
					opx2:		NONE
					mnemonic	used
				*/

/*
	constants which may be returned from the routines which
	look at a structure of the above type
*/

#define LAST_RECORD		-1 /* The last record has type field NONE */
#define INTERESTING_RECORD	1
#define BORING_RECORD		0

/* procedures */

extern char *makeTable();

/* The actual opcode table */

extern struct opcodeTableStruct opcodeTable[];


/*
 **** **** **** **** **** **** **** **** **** **** **** **** **** ****
 * sas-file processor.h (from 3.1 11/11/85)
 */

/*
	Descriptions and declarations for the processor registers including:

	the overlapping window registers	reg(n)
	the floating point registers		freg(n)
	the Program counters			pc, npc
	the Y register				yr
	The Window Invald Mask Register		wim
	the Processor Status Register		psr
	the Trap Base Register			tbr
	the Floating-point Status Register	fsr
*/

/* processor implementation number and version number */

extern int IMPLEMENTATION;
extern int VERSION;

/* The R registers (window registers) */

/* architecture parameters. */

#define MAX_NW		32		/* maximum number of register windows */
extern int R_NW;			/* default number of register windows */
#define R_NGLOBALS	8		/* number of global registers */
#define R_NOVERLAP	8		/* number of ins and outs */
#define R_NLOCALS	8		/* number of local registers */

#define R_NREGISTERS	(R_NGLOBALS + R_NW*(R_NOVERLAP + R_NLOCALS))

extern Word *regPtrArray[MAX_NW][R_NGLOBALS + 2*R_NOVERLAP + R_NLOCALS];
extern Word regFile[R_NGLOBALS + MAX_NW*(R_NOVERLAP + R_NLOCALS)];

#define winReg(w, n)	(*regPtrArray[w][n])
#define reg(n)		winReg(CWP, n)

/* The F Registers for floating-point */

#define F_NR		32		/* number of F-registers */
extern Word floatReg[F_NR];
#define freg(n)		(floatReg[n])

/* The program counters */
extern Word pc, npc;

/* the Y register */
extern Word yr;

/* The WIM */

extern Word wim;

#ifdef LOW_END_FIRST

struct wimStruct {
	u_int vwim;
};

#else

struct wimStruct {
	u_int vwim;
};

#endif

#define X_WIM(x)	( ((struct wimStruct *)&(x))->vwim )
#define WIM		X_WIM(wim)

/* The PSR */

extern Word psr;

#ifdef LOW_END_FIRST

struct psrStruct {
	u_int cwp		:5;	/* saved window pointer */
	u_int et		:1;	/* enable traps */
	u_int psm		:1;	/* previous s */
	u_int sm		:1;	/* supervisor mode */
	u_int pil		:4;	/* processor interrupt level */
	u_int df		:1;	/* disable floating-point */
	u_int icc		:4;	/* the integer condition codes */
	u_int psr_reserved	:7;	/* not used currently */
	u_int ver		:4;	/* implementation version number */
	u_int impl		:4;	/* implementation */
};

#else

struct psrStruct {
	u_int impl		:4;	/* implementation */
	u_int ver		:4;	/* implementation version number */
	u_int psr_reserved	:7;	/* not used currently */
	u_int icc		:4;	/* the integer condition codes */
	u_int df		:1;	/* disable floating-point */
	u_int pil		:4;	/* processor interrupt level */
	u_int sm		:1;	/* supervisor mode */
	u_int psm		:1;	/* previous s */
	u_int et		:1;	/* enable traps */
	u_int cwp		:5;	/* saved window pointer */
};

#endif

/* ICC sub-fields */

#ifdef LOW_END_FIRST

struct iccStruct {
	u_int cwp		:5;	/* saved window pointer */
	u_int et		:1;	/* enable traps */
	u_int psm		:1;	/* previous s */
	u_int sm		:1;	/* supervisor mode */
	u_int pil		:4;	/* processor interrupt level */
	u_int df		:1;	/* disable floating-point */

	u_int carry		:1;	/* carry */
	u_int over		:1;	/* overflow */
	u_int zero		:1;	/* zero */
	u_int neg		:1;	/* negative */

	u_int psr_reserved	:7;	/* not used currently */
	u_int ver		:4;	/* implementation version number */
	u_int impl		:4;	/* implementation */
};

#else

struct iccStruct {
	u_int impl		:4;	/* implementation */
	u_int ver		:4;	/* implementation version number */
	u_int psr_reserved	:7;	/* not used currently */
					
	u_int neg		:1;	/* negative */
	u_int zero		:1;	/* zero */
	u_int over		:1;	/* overflow */
	u_int carry		:1;	/* carry */

	u_int df		:1;	/* disable floating-point */
	u_int pil		:4;	/* processor interrupt level */
	u_int sm		:1;	/* supervisor mode */
	u_int psm		:1;	/* previous s */
	u_int et		:1;	/* enable traps */
	u_int cwp		:5;	/* saved window pointer */
};

#endif

/* extraction macros. These are portable, but don't work for registers */
/* using sun's C compiler for the 68010, they are quite efficient */

#define X_PSR_RES(x)	( ((struct psrStruct *)&(x))->psr_reserved )
#define PSR_RES		X_PSR_RES(psr)

#define X_CWP(x)	( ((struct psrStruct *)&(x))->cwp )
#define CWP		X_CWP(psr)

#define X_ET(x)		( ((struct psrStruct *)&(x))->et )
#define ET		X_ET(psr)

#define X_PSM(x)	( ((struct psrStruct *)&(x))->psm )
#define PSM		X_PSM(psr)

#define X_SM(x)		( ((struct psrStruct *)&(x))->sm )
#define SM		X_SM(psr)

#define X_PIL(x)	( ((struct psrStruct *)&(x))->pil )
#define PIL		X_PIL(psr)

#define X_DF(x)		( ((struct psrStruct *)&(x))->df )
#define DF		X_DF(psr)

#define X_ICC(x)	( ((struct psrStruct *)&(x))->icc )
#define ICC		X_ICC(psr)

#define X_VER(x)	( ((struct psrStruct *)&(x))->ver )
#define VER		X_VER(psr)

#define X_IMPL(x)	( ((struct psrStruct *)&(x))->impl )
#define IMPL		X_IMPL(psr)

#define X_NEG(x)	( ((struct iccStruct *)&(x))->neg )
#define NEG		X_NEG(psr)

#define X_ZERO(x)	( ((struct iccStruct *)&(x))->zero )
#define ZERO		X_ZERO(psr)

#define X_OVER(x)	( ((struct iccStruct *)&(x))->over )
#define OVER		X_OVER(psr)

#define X_CARRY(x)	( ((struct iccStruct *)&(x))->carry )
#define CARRY		X_CARRY(psr)

/* The Trap Base (TBR) register */

extern Word tbr;

#ifdef LOW_END_FIRST

struct tbrStruct {
	u_int tbr_zero		:4;	/* four trailing zeros */
	u_int tt		:8;	/* trap type */
	u_int tba		:20;	/* trap base address */
};

#else

struct tbrStruct {
	u_int tba		:20;	/* trap base address */
	u_int tt		:8;	/* trap type */
	u_int tbr_reserved	:4;	/* four trailing zeros */
};

#endif

#define X_TT(x)		( ((struct tbrStruct *)&(x))->tt )
#define TT		X_TT(tbr)

#define X_TBA(x)	( ((struct tbrStruct *)&(x))->tba )
#define TBA		X_TBA(tbr)

#define X_TBR_RES(x)	( ((struct tbrStruct *)&(x))->tbr_reserved )
#define TBR_RES		X_TBR_RES(tbr)


/* The FSR */

Word fsr;

#ifdef LOW_END_FIRST

struct fsrStruct {
	u_int cexc		:5;	/* current exceptions */
	u_int fcc		:2;	/* floating condition codes */
	u_int qne		:1;	/* queue not empty */
	u_int ftt		:3;	/* floating-point trap type */
	u_int pr		:1;	/* something to do with FREM, FQUOT */
	u_int fsr_reserved	:15;	/* currently unused */
	u_int nxe		:1;	/* inexact exception enable */
	u_int rp		:2;	/* rounding precision (extended) */
	u_int rnd		:2;	/* rounding direction */
};

#else

struct fsrStruct {
	u_int rnd		:2;	/* rounding direction */
	u_int rp		:2;	/* rounding precision (extended) */
	u_int nxe		:1;	/* inexact exception enable */
	u_int fsr_reserved	:15;	/* currently unused */
	u_int pr		:1;	/* something to do with FREM, FQUOT */
	u_int ftt		:3;	/* floating-point trap type */
	u_int qne		:1;	/* queue not empty */
	u_int fcc		:2;	/* floating condition codes */
	u_int cexc		:5;	/* current exceptions */
};

#endif

#define X_RND(x)	( ((struct fsrStruct *)&(x))->rnd )
#define RND		X_RND(fsr)

#define X_RP(x)		( ((struct fsrStruct *)&(x))->rp )
#define RP		X_RP(fsr)

#define X_NXE(x)	( ((struct fsrStruct *)&(x))->nxe )
#define NXE		X_NXE(fsr)

#define X_FSR_RES(x)	( ((struct fsrStruct *)&(x))->fsr_reserved )
#define FSR_RES		X_FSR_RES(fsr)

#define X_PR(x)	( ((struct fsrStruct *)&(x))->pr )
#define PR		X_PR(fsr)

#define X_FTT(x)	( ((struct fsrStruct *)&(x))->ftt )
#define FTT		X_FTT(fsr)

/* values for the FTT field of the FSR */
#define FTT_NO_TRAP	0
#define FTT_IEEE	1
#define FTT_UNFINISHED	2
#define FTT_UNIMP	3
#define FTT_FPU_ERROR	4

#define X_QNE(x)	( ((struct fsrStruct *)&(x))->qne )
#define QNE		X_QNE(fsr)

#define X_FCC(x)	( ((struct fsrStruct *)&(x))->fcc )
#define FCC		X_FCC(fsr)

/* possible values for the FCC field, and their meanings */
#define FCC_FEQ		0	/* Floating equal */
#define FCC_FLT		1	/* Floating Less Than */
#define FCC_FGT		2	/* Floating Greater Than */
#define FCC_FU		3	/* Floating Unordered */

#define X_CEXC(x)	( ((struct fsrStruct *)&(x))->cexc )
#define CEXC		X_CEXC(fsr)

/* values for the CEXC field */
#define FEXC_NV		0x10
#define FEXC_OF		0x08
#define FEXC_UF		0x04
#define FEXC_DZ		0x02
#define FEXC_NX		0x01

/* the following sub-field struct further reduces cexc
	into component subfields, and provides extraction macros */

#ifdef LOW_END_FIRST

struct fsrSubStruct {
	u_int nx		:1;	/* inexact */
	u_int dz		:1;	/* Divide-by-zero */
	u_int uf		:1;	/* underflow */
	u_int of		:1;	/* overflow */
	u_int nv		:1;	/* invalid */

	u_int fcc		:2;	/* floating condition codes */
	u_int qne		:1;	/* queue not empty */
	u_int ftt		:3;	/* floating-point trap type */
	u_int pr		:1;	/* something to do with FREM, FQUOT */
	u_int fsr_reserved	:15;	/* currently unused */
	u_int nxe		:1;	/* inexact exception enable */
	u_int rp		:2;	/* rounding precision (extended) */
	u_int rnd		:2;	/* rounding direction */
};

#else

struct fsrSubStruct {
	u_int rnd		:2;	/* rounding direction */
	u_int rp		:2;	/* rounding precision (extended) */
	u_int nxe		:1;	/* inexact exception enable */
	u_int fsr_reserved	:15;	/* currently unused */
	u_int pr		:1;	/* something to do with FREM, FQUOT */
	u_int ftt		:3;	/* floating-point trap type */
	u_int qne		:1;	/* queue not empty */
	u_int fcc		:2;	/* floating condition codes */

	u_int nv		:1;	/* invalid */
	u_int of		:1;	/* overflow */
	u_int uf		:1;	/* underflow */
	u_int dz		:1;	/* Divide-by-zero */
	u_int nx		:1;	/* inexact */
};

#endif

#define X_NV(x)		( ((struct fsrSubStruct *)&(x))->nv )
#define NV		X_NV(fsr)

#define X_OF(x)		( ((struct fsrSubStruct *)&(x))->of )
#define OF		X_OF(fsr)

#define X_UF(x)		( ((struct fsrSubStruct *)&(x))->uf )
#define UF		X_UF(fsr)

#define X_DZ(x)		( ((struct fsrSubStruct *)&(x))->dz )
#define DZ		X_DZ(fsr)

#define X_NX(x)		( ((struct fsrSubStruct *)&(x))->nx )
#define NX		X_NX(fsr)


/*
 **** **** **** **** **** **** **** **** **** **** **** **** **** ****
 * sas-file sw_trap.h (from 3.2 12/11/85)
 */

/* reserved software trap types (subject to change by software folks)
	There are 128 software trap types, numbered 128 to 255. */

#define ST_BREAKPOINT	255 /* breakPoint trap */
#define ST_SYSCALL	128

#define ST_PUTCONSOLE	253
#define ST_GETCONSOLE	252

#define ST_BOOTENTRY	251 /* used in machine simulation as bootstrap entry */

#define ST_PAUSE	200 /* (temporary) SAS-pause trap		*/
#define ST_DIV0		190 /* (temporary) Division-by-Zero trap	*/
#define ST_SYSCALL_MACH	210 /* (temporary) Kernel system call,
						for device driver emulation */
#define ST_DUP_CTXT0	209 /* (temporary) Machiner mode only,
				duplicate context 0 mapping in all contexts */


/*
 **** **** **** **** **** **** **** **** **** **** **** **** **** ****
 * sas-file switchindexes.h (from 3.1 11/11/85)
 */

/*
	This file defines the values for indices which are used in
	switch() statements to decode instructions. Values in this
	table are unrelated to the architecture. At best, they should be
	chosen to optimize the efficiency of the switch() statements,
	which has been done by choosing sequential numbers. Sun
	Microsystems's C compiler will see a dense, large switch
	table for decoding format 3 instructions and generate a jump
	table, which is efficient.

	It is also necessary that the values for ALL instructions
	be unique, because sas generates a table which is searched
	with these switch indeces, regardless of instruction format.

	The index value 0 is reserved to force the the default case.
*/


/*						INTEGER CONDITION CODES */
#define ICN	1
#define ICNE	2
#define ICEQ	3
#define ICGT	4
#define ICLE	5
#define ICGE	6
#define ICLT	7
#define ICHI	8
#define ICLS	9
#define ICCC	10
#define ICCS	11
#define ICPL	12
#define ICMI	13
#define ICVC	14
#define ICVS	15
#define ICA	16

/*						FLOATING CONDITION CODES */
#define FCN	1
#define FCUN	2
#define FCGT	3
#define FCUG	4
#define FCLT	5
#define FCUL	6
#define FCLG	7
#define FCNE	8
#define FCEQ	9
#define FCUE	10
#define FCGE	11
#define FCUGE	12
#define FCLE	13
#define FCULE	14
#define FCLEG	15
#define FCA	16

/*****************************************************************************

	STARTING HERE, ALL #define'd numbers MUST BE UNIQUE

*****************************************************************************/

/*						PSEUDO INSTRUCTIONS */

#define NOP	1
#define CMP	2
#define TST	3
#define MOV	4
#define RET	5


/*						FORMAT 1 INSTRUCTIONS */

/*	Call Instruction */
#define CALL	6

/*						FORMAT 2 INSTRUCTIONS */

/*	Conditional Branch Instructions	*/
#define BICC	7
#define BFCC	8

/*	Sethi Instruction */
#define SETHI	9

/*						FORMAT 3 INSTRUCTIONS */
/*	Load instructions */

#define LDSB	10
#define LDSH	11
#define LDUB	12
#define LDUH	13
#define LD	14
#define LDF	15
#define LDFD	16
#define LDDC	17
#define LASB	18
#define LASH	19
#define LAUB	20
#define LAUH	21
#define LDA	22
#define LDC	23

#define LDFSR	24
#define LDCSR	25

/*	Store Instructions */

#define STB	26
#define STH	27
#define ST	28
#define STF	29
#define STFD	30
#define STDC	31
#define SAB	32
#define SAH	33
#define STA	34
#define STC	35

#define STFSR	36
#define STCSR	37
#define STFQ	38
#define STDCQ	39

/*	Atomic Load-Store Instructions */

#define ALS	40
#define AALS	41

/*	Arithmetic Instructions */

#define ADD	42
#define ADDcc	43
#define ADDX	44
#define ADDXcc	45
#define TADDcc	46
#define TADDccTV	47

#define SUB	48
#define SUBcc	49
#define SUBX	50
#define SUBXcc	51
#define TSUBcc	52
#define TSUBccTV	53

#define MULScc	54

/*	Logical Instructions */

#define AND	55
#define ANDN	56
#define OR	57
#define ORN	58
#define XOR	59
#define XORN	60

#define ANDcc	61
#define ANDNcc	62
#define ORcc	63
#define ORNcc	64
#define XORcc	65
#define XORNcc	66

/*	Shift Instructions	*/

#define SLL	67
#define SRL	68
#define SRA	69


/*	Window Instructions	*/

#define SAVE	70
#define RESTORE	71
#define RETT	72


/*	Jump Instruction */

#define JUMP	73

/*	Trap Instruction */

#define TICC	74

/*	Read Register Instructions */

#define RDY	75
#define RDPSR	76
#define RDTBR	77
#define RDWIM	78

/*	Write Register Instructions */

#define WRY	79
#define WRPSR	80
#define WRWIM	81
#define WRTBR	82

/*	Instruction Cache flush */

#define IFLUSH	83

/*	Floating-Point Operation Instructions */

#define FPOP	84
#define FPCMP	85

/*						FPOP INSTRUCTIONS */

#define FCVTis		86
#define FCVTid		87
#define FCVTie		88

#define FCVTsi		89
#define FCVTdi		90
#define FCVTei		91

#define FCVTsiz		92
#define FCVTdiz		93
#define FCVTeiz		94

#define FINTs		95
#define FINTd		96
#define FINTe		97

#define FINTsz		98
#define FINTdz		99
#define FINTez		100

#define FCVTsd		101
#define FCVTse		102
#define FCVTds		103
#define FCVTde		104
#define FCVTes		105
#define FCVTed		106

#define FMOVE		107
#define FNEG		108
#define FABS		109

#define FCLSs		110
#define FCLSd		111
#define FCLSe		112

#define FEXPs		113
#define FEXPd		114
#define FEXPe		115

#define FSQRs		116
#define FSQRd		117
#define FSQRe		118

#define FADDs		119
#define FADDd		120
#define FADDe		121

#define FSUBs		122
#define FSUBd		123
#define FSUBe		124

#define FMULs		125
#define FMULd		126
#define FMULe		127

#define FDIVs		128
#define FDIVd		129
#define FDIVe		130

#define FSCLs		131
#define FSCLd		132
#define FSCLe		133

#define FREMs		134
#define FREMd		135
#define FREMe		136

#define FQUOs		137
#define FQUOd		138
#define FQUOe		139

#define FCMPs		140
#define FCMPd		141
#define FCMPe		142

#define FCMPXs		143
#define FCMPXd		144
#define FCMPXe		145

#define UNIMP		146
/* 			NONE	USED OUT OF SEQUENCE */

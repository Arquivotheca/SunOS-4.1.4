/*	@(#)instructions.h	1.1	10/31/94	*/
/*
 *	This file contains prototype codes for all instructions,
 *	plus classification codes for grouping the instructions.
 */

/*
 * Binary instruction formats:
 *	The names of the formats used here are derived from the Formats in the
 *	Scalable Processor Architecture (SPARC) Manual.
 *
 *	Format 0 is a ficticious "what you see is what you get" format; the
 *	entire instruction is treated like an opcode.
 *
 *	Formats 1, 2a, and 2b are referred to verbatim.
 *
 *	Formats 3a&3b and 3c&3d are treated as pairs because a given
 *	instruction can assemble either of the two formats, depending on its
 *	operands.  3ab/0 refers to Format 3ab with i=0; 3ab/1 refers to 3ab
 *	with i=1.
 *
 *	There are minor variations on instruction formats 3cd and 3e, where
 *	some field(s) are ignored.  These are indicated in the name by trailing
 *	0's and 1's which tell whether a field is ignored (0) or used(1).
 *	For example, in 3cd10, the RD field is used but the asi field is
 *	ignored.
 *
 *	In one case, the entire lower 19 bits is considered a large immediate
 *	field which is either used (the UNIMP instruction) or ignored (the RD
 *	instruction).   That format is named 3cd1X (the 1 refers to the RD
 *	field which is not ignored).
 *
 */

/*
 * Binary instruction types for use in opcode table are enumerated in
 * "structs.h".
 */

/*
 * Macros to help form instructions, for use only in this file:
 */
	/* Instruction format 1: CALL only		*/
/******************************************************************************
3130 29 28272625 2423 22212019 1817161514 13 1211100908070605 0403020100
-op- ---------------------------disp30---------------------------------- 1
 ******************************************************************************/
#define IF1(op)		  (((op)<<30)                                      )

	/* Instruction format 2a: SETHI	*/
/******************************************************************************
3130 29 28272625 242322 212019 1817161514 13 1211100908070605 0403020100
-op- -----rd---- -op2-- --------------------disp22---------------------- 2a
 ******************************************************************************/
#define IF2a(op,op2)	  (((op)<<30)|             ((op2)<<22)             )

	/* Instruction format 2b: Bicc, FBfcc, CBccc	*/
/******************************************************************************
3130 29 28272625 242322 212019 1817161514 13 1211100908070605 0403020100
-op- a- --cond-- -op2-- --------------------disp22---------------------- 2b
 ******************************************************************************/
#define IF2b(op,cond,op2) (((op)<<30)|((cond)<<25)|((op2)<<22)             )

	/* Instruction format 3cd11[i=0]: LD's and ST's			 */
	/* Instruction format 3cd11[i=1]: LD's and ST's			 */
	/* Instruction format 3cd00[i=0]: WR				 */
	/* Instruction format 3cd00[i=1]: WR				 */
        /* Instruction format 3cd01[i=0]: LD{FSR,CSR} and ST{FSR,CSR,FQ,CQ} */
        /* Instruction format 3cd01[i=1]: LD{FSR,CSR} and ST{FSR,CSR,FQ,CQ} */
	/* Instruction format 3cd10[i=0]: remaining non-FPop instructions*/
	/* Instruction format 3cd10[i=1]: remaining non-FPop instructions*/
	/* Instruction format 3cd1X[i=0]: RD				 */
	/* Instruction format 3cd1X[i=1]: UNIMP				 */
/******************************************************************************
3130 29 28272625 2423 22212019 1817161514 13 1211100908070605 0403020100
-op- -----rd---- ------op3---- ----rs1--- i=0 ------asi------ ---rs2---- 3cd11/0
-op- -----rd---- ------op3---- ----rs1--- i=1 ----------simm13---------- 3cd11/1
-op- -<ignored>- ------op3---- ----rs1--- i=0 ---<ignored>--- ---rs2---- 3cd00/0
-op- -<ignored>- ------op3---- ----rs1--- i=1 ----------simm13---------- 3cd00/1
-op- ---<zero>-- ------op3---- ----rs1--- i=0 ---<ignored>--- ---rs2---- 3cd01/0
-op- ---<zero>-- ------op3---- ----rs1--- i=1 ----------simm13---------- 3cd01/1
-op- -----rd---- ------op3---- ----rs1--- i=0 ---<ignored>--- ---rs2---- 3cd10/0
-op- -----rd---- ------op3---- ----rs1--- i=1 ----------simm13---------- 3cd10/1
-op- -----rd---- ------op3---- ---------------<ignored>----------------- 3cd1X/0
-op- -----rd---- ------op3---- -----------------imm19------------------- 3cd1X/1
 ******************************************************************************/
#define IF3cd11(op,op3)	(((op)<<30)|             ((op3)<<19)         )
#define IF3cd00(op,op3)	(((op)<<30)|             ((op3)<<19)         )
#define IF3cd01(op,op3) (((op)<<30)|		 ((op3)<<19)	     )
#define IF3cd10(op,op3)	(((op)<<30)|             ((op3)<<19)         )
#define IF3cd1X(op,op3)	(((op)<<30)|             ((op3)<<19)         )

	/* Instruction format 3e11: 3-operand floating-point instructions */
	/* Instruction format 3e10: most 2-operand floating-point instructions*/
	/* Instruction format 3e01: (2-operand) f-p Compare instructions  */
/******************************************************************************
 * 3130 29 28272625 2423 22212019 1817161514 13 1211100908070605 0403020100
 * -op- -----rd---- ------op3---- ----rs1--- --------opf-------- ---rs2---- 3e11
 * -op- -----rd---- ------op3---- -<ignored> --------opf-------- ---rs2---- 3e10
 * -op- -<ignored>- ------op3---- ----rs1--- --------opf-------- ---rs2---- 3e01
 ******************************************************************************/
#define IF3e11(op3,opf)	  (((02)<<30)|             ((op3)<<19)|((opf)<<5)  )
#define IF3e10(op3,opf)	  (((02)<<30)|             ((op3)<<19)|((opf)<<5)  )
#define IF3e01(op3,opf)	  (((02)<<30)|             ((op3)<<19)|((opf)<<5)  )

/******************************************************************************
 * 3130 29 28272625 2423 22212019 1817161514 13 1211100908070605 0403020100
 * -op- -----rd---- ------op3---- ----rs1--- --------opf-------- ---rs2---- 3e11
 ******************************************************************************/
#define IF3e11_CPOP(op3)  (((02)<<30)|		   ((op3)<<19))
	/* Instruction format 3ab: trap instructions	*/
/******************************************************************************
3130 29 28272625 2423 22212019 1817161514 13 1211100908070605 0403020100
-op- ign --cond- ------op3---- ----rs1--- i=0 ---<ignored>--- ---rs2----  3ab/0
-op- ign --cond- ------op3---- ----rs1--- i=1 ----------simm13----------  3ab/1
 ******************************************************************************/
#define IF3ab(op,cond,op3) (((op)<<30)|((cond)<<25)|((op3)<<19)         )

/* used in floating-point instructions */
#define INT10  0x00	/* integer  type, in bits 1:0 */
#define SGL10  0x01	/* single   type, in bits 1:0 */
#define DBL10  0x02	/* double   type, in bits 1:0 */
#define QUAD10 0x03	/* quad     type, in bits 1:0 */
#define INT32  0x00	/* integer  type, in bits 3:2 */
#define SGL32  0x04	/* single   type, in bits 3:2 */
#define DBL32  0x08	/* double   type, in bits 3:2 */
#define QUAD32 0x0c	/* quad     type, in bits 3:2 */

/*----------------------------------------------------------------------------
 *	Instruction groups (grouped purely by SYNTAX, not semantics).
 *
 *	These are used to differentiate (e.g. in actions.c) between
 *	semantically different instructions which happened to share the
 *	same assembly-language syntax.
 *
 *	IG  = Instruction Group
 *	ISG = Instruction SubGroup (within a Group)
 *----------------------------------------------------------------------------
 */

#define IG_GROUP_MASK	0xfffffff0	/* bits which are used for Group */
#define IG_SUBGRP_MASK	~(IG_GROUP_MASK)/* bits which can be used
					 * to differentiate among subgroups.
					 */

#define IG_NONE		         0	/*(where no instr'n group is needed) */

#define IG_MOV		0x00000010	/* mov			*/
#define IG_CMP		0x00000020	/* cmp			*/
#define IG_SETHI	0x00000040	/* sethi		*/

#define IG_TST		0x00000080	/* tst			*/
#define IG_BR		0x00000100	/* branches & f.p. branches */
#define IG_CALL		0x00000200	/* call			*/
#define IG_JRI		0x00000400	/* jmp, rett, iflush	*/
#define IG_TICC		0x00000800	/* traps		*/

#define IG_ARITH	0x00001000	/* arith. instructions	*/
#define IG_SVR		0x00002000	/* save & restore	*/
#define IG_RD		0x00004000	/* rd			*/
#define IG_WR		0x00008000	/* wr			*/

#define IG_LD		0x00010000	/* ld*			*/
#define IG_ST		0x00020000	/* st*			*/
#define IG_DLD		0x00040000	/* ldd*			*/
#define IG_DST		0x00080000	/* std*			*/

#define IG_FP		0x00100000	/* f* (fl-pt, except branches).	*/
#define	  ISG_FCMP	       0x2	/*	fpcmp*		*/

#define IG_UNIMP	0x00200000	/* the "unimp" instr.	*/

#define IG_NN		0x00400000	/* not & neg		*/
#define	  ISG_NOT	       0x1	/*	not		*/
#define	  ISG_NEG	       0x2	/*	neg		*/

#define IG_BIT		0x00800000	/* btst,bset,bclr,btog	*/
#define   ISG_BT	       0x0	/*	btst		*/
#define   ISG_BS	       0x1	/*	bset		*/
#define   ISG_BC 	       0x2	/*	bclr		*/
#define   ISG_BG 	       0x3	/*	btog		*/

#define IG_CLR		0x01000000	/* clrb,clrh,clr	*/
#define   ISG_CLRB	       0x1	/*	clrb		*/
#define   ISG_CLRH	       0x2	/*	clrh		*/
#define   ISG_CLR	       0x4	/*	clr		*/

#define IG_SET		0x02000000	/* set			*/

#define IG_ID		0x04000000	/* inc,dec		*/
#define   ISG_I		       0x0	/*	inc		*/
#define   ISG_IC	       0x1	/*	inccc		*/
#define   ISG_D 	       0x2	/*	dec		*/
#define   ISG_DC 	       0x3	/*	deccc		*/

#define IG_JMPL		0x08000000	/* jmpl			*/

#define IG_NOALIAS	0x10000000	/* .noalias		*/
#define IG_CP           0x40000000      /* cpop1, cpop2         */
#define   ISG_CP1              0x0      /* cpop1                */
#define   ISG_CP2              0x1      /* cpop2                */

/* If we run out of bits for instruction groups,
 * we'll have to combine some existing (mutually-exclusive) groups,
 * and/or take some bits away from the Subgroups.
 */


/*----------------------------------------------------------------------------
 *	the Annul bit, for use in Branch w/Annul instructions
 *----------------------------------------------------------------------------
 */
#define ANNUL_BIT 0x20000000L

/*----------------------------------------------------------------------------
 *	prototype instruction codes for instructions:
 *----------------------------------------------------------------------------
 */

	/* please leave these 2 #define's right here, as a Shell script
	 * uses them to locate the beginning of the instruction #define's
	 */
#define INSTR_NONE	((INSTR)0)
#define INSTR_ERROR	((INSTR)0)

#define INSTR_CALL	IF1(1)

#define INSTR_BN	IF2b(0,0x0,0x2)
#define INSTR_BE	IF2b(0,0x1,0x2)
#define INSTR_BLE	IF2b(0,0x2,0x2)
#define INSTR_BL	IF2b(0,0x3,0x2)
#define INSTR_BLEU	IF2b(0,0x4,0x2)
#define INSTR_BCS	IF2b(0,0x5,0x2)
#define INSTR_BNEG	IF2b(0,0x6,0x2)
#define INSTR_BVS	IF2b(0,0x7,0x2)
#define INSTR_BA	IF2b(0,0x8,0x2)
#define INSTR_BNE	IF2b(0,0x9,0x2)
#define INSTR_BG	IF2b(0,0xa,0x2)
#define INSTR_BGE	IF2b(0,0xb,0x2)
#define INSTR_BGU	IF2b(0,0xc,0x2)
#define INSTR_BCC	IF2b(0,0xd,0x2)
#define INSTR_BPOS	IF2b(0,0xe,0x2)
#define INSTR_BVC	IF2b(0,0xf,0x2)

#define INSTR_BN_A	(INSTR_BN   | ANNUL_BIT)
#define INSTR_BE_A	(INSTR_BE   | ANNUL_BIT)
#define INSTR_BLE_A	(INSTR_BLE  | ANNUL_BIT)
#define INSTR_BL_A	(INSTR_BL   | ANNUL_BIT)
#define INSTR_BLEU_A	(INSTR_BLEU | ANNUL_BIT)
#define INSTR_BCS_A	(INSTR_BCS  | ANNUL_BIT)
#define INSTR_BNEG_A	(INSTR_BNEG | ANNUL_BIT)
#define INSTR_BVS_A	(INSTR_BVS  | ANNUL_BIT)
#define INSTR_BA_A	(INSTR_BA   | ANNUL_BIT)
#define INSTR_BNE_A	(INSTR_BNE  | ANNUL_BIT)
#define INSTR_BG_A	(INSTR_BG   | ANNUL_BIT)
#define INSTR_BGE_A	(INSTR_BGE  | ANNUL_BIT)
#define INSTR_BGU_A	(INSTR_BGU  | ANNUL_BIT)
#define INSTR_BCC_A	(INSTR_BCC  | ANNUL_BIT)
#define INSTR_BPOS_A	(INSTR_BPOS | ANNUL_BIT)
#define INSTR_BVC_A	(INSTR_BVC  | ANNUL_BIT)

#define INSTR_FBN	IF2b(0,0x0,0x6)
#define INSTR_FBU	IF2b(0,0x7,0x6)
#define INSTR_FBG	IF2b(0,0x6,0x6)
#define INSTR_FBUG	IF2b(0,0x5,0x6)
#define INSTR_FBL	IF2b(0,0x4,0x6)
#define INSTR_FBUL	IF2b(0,0x3,0x6)
#define INSTR_FBLG	IF2b(0,0x2,0x6)
#define INSTR_FBNE	IF2b(0,0x1,0x6)
#define INSTR_FBE	IF2b(0,0x9,0x6)
#define INSTR_FBUE	IF2b(0,0xa,0x6)
#define INSTR_FBGE	IF2b(0,0xb,0x6)
#define INSTR_FBUGE	IF2b(0,0xc,0x6)
#define INSTR_FBLE	IF2b(0,0xd,0x6)
#define INSTR_FBULE	IF2b(0,0xe,0x6)
#define INSTR_FBO	IF2b(0,0xf,0x6)
#define INSTR_FBA	IF2b(0,0x8,0x6)

#define INSTR_FBN_A	(INSTR_FBN   | ANNUL_BIT)
#define INSTR_FBU_A	(INSTR_FBU   | ANNUL_BIT)
#define INSTR_FBG_A	(INSTR_FBG   | ANNUL_BIT)
#define INSTR_FBUG_A	(INSTR_FBUG  | ANNUL_BIT)
#define INSTR_FBL_A	(INSTR_FBL   | ANNUL_BIT)
#define INSTR_FBUL_A	(INSTR_FBUL  | ANNUL_BIT)
#define INSTR_FBLG_A	(INSTR_FBLG  | ANNUL_BIT)
#define INSTR_FBNE_A	(INSTR_FBNE  | ANNUL_BIT)
#define INSTR_FBE_A	(INSTR_FBE   | ANNUL_BIT)
#define INSTR_FBUE_A	(INSTR_FBUE  | ANNUL_BIT)
#define INSTR_FBGE_A	(INSTR_FBGE  | ANNUL_BIT)
#define INSTR_FBUGE_A	(INSTR_FBUGE | ANNUL_BIT)
#define INSTR_FBLE_A	(INSTR_FBLE  | ANNUL_BIT)
#define INSTR_FBULE_A	(INSTR_FBULE | ANNUL_BIT)
#define INSTR_FBO_A	(INSTR_FBO   | ANNUL_BIT)
#define INSTR_FBA_A	(INSTR_FBA   | ANNUL_BIT)

#define INSTR_CBN	IF2b(0,0x0,0x7)
#define INSTR_CB3	IF2b(0,0x7,0x7)
#define INSTR_CB2	IF2b(0,0x6,0x7)
#define INSTR_CB23	IF2b(0,0x5,0x7)
#define INSTR_CB1	IF2b(0,0x4,0x7)
#define INSTR_CB13	IF2b(0,0x3,0x7)
#define INSTR_CB12	IF2b(0,0x2,0x7)
#define INSTR_CB123	IF2b(0,0x1,0x7)
#define INSTR_CB0	IF2b(0,0x9,0x7)
#define INSTR_CB03	IF2b(0,0xa,0x7)
#define INSTR_CB02	IF2b(0,0xb,0x7)
#define INSTR_CB023	IF2b(0,0xc,0x7)
#define INSTR_CB01	IF2b(0,0xd,0x7)
#define INSTR_CB013	IF2b(0,0xe,0x7)
#define INSTR_CB012	IF2b(0,0xf,0x7)
#define INSTR_CBA	IF2b(0,0x8,0x7)

#define INSTR_CBN_A	(INSTR_CBN   | ANNUL_BIT)
#define INSTR_CB3_A	(INSTR_CB3   | ANNUL_BIT)
#define INSTR_CB2_A	(INSTR_CB2   | ANNUL_BIT)
#define INSTR_CB23_A	(INSTR_CB23  | ANNUL_BIT)
#define INSTR_CB1_A	(INSTR_CB1   | ANNUL_BIT)
#define INSTR_CB13_A	(INSTR_CB13  | ANNUL_BIT)
#define INSTR_CB12_A	(INSTR_CB12  | ANNUL_BIT)
#define INSTR_CB123_A	(INSTR_CB123 | ANNUL_BIT)
#define INSTR_CB0_A	(INSTR_CB0   | ANNUL_BIT)
#define INSTR_CB03_A	(INSTR_CB03  | ANNUL_BIT)
#define INSTR_CB02_A	(INSTR_CB02  | ANNUL_BIT)
#define INSTR_CB023_A	(INSTR_CB023 | ANNUL_BIT)
#define INSTR_CB01_A	(INSTR_CB01  | ANNUL_BIT)
#define INSTR_CB013_A	(INSTR_CB013 | ANNUL_BIT)
#define INSTR_CB012_A	(INSTR_CB012 | ANNUL_BIT)
#define INSTR_CBA_A	(INSTR_CBA   | ANNUL_BIT)

#define INSTR_SETHI	IF2a(0,0x4)

#define INSTR_LDSB      IF3cd11(3,0x09)
#define INSTR_LDSH	IF3cd11(3,0x0a)
#define INSTR_LDUB	IF3cd11(3,0x01)
#define INSTR_LDUH	IF3cd11(3,0x02)
#define INSTR_LD	IF3cd11(3,0x00)
#define INSTR_LDD	IF3cd11(3,0x03)

#define INSTR_STB	IF3cd11(3,0x05)
#define INSTR_STH	IF3cd11(3,0x06)
#define INSTR_ST	IF3cd11(3,0x04)
#define INSTR_STD	IF3cd11(3,0x07)
#define INSTR_LDST	IF3cd11(3,0x0c)
#define INSTR_LDSTUB	IF3cd11(3,0x0d)
#define INSTR_SWAP	IF3cd11(3,0x0f)

#define INSTR_LDSBA	IF3cd11(3,0x19)
#define INSTR_LDSHA	IF3cd11(3,0x1a)
#define INSTR_LDUBA	IF3cd11(3,0x11)
#define INSTR_LDUHA	IF3cd11(3,0x12)
#define INSTR_LDA	IF3cd11(3,0x10)
#define INSTR_LDDA	IF3cd11(3,0x13)

#define INSTR_STBA	IF3cd11(3,0x15)
#define INSTR_STHA	IF3cd11(3,0x16)
#define INSTR_STA	IF3cd11(3,0x14)
#define INSTR_STDA	IF3cd11(3,0x17)
#define INSTR_LDSTA	IF3cd11(3,0x1c)
#define INSTR_LDSTUBA	IF3cd11(3,0x1d)
#define INSTR_SWAPA	IF3cd11(3,0x1f)

#define INSTR_LDF	IF3cd11(3,0x20)
#define INSTR_LDFSR	IF3cd01(3,0x21)
#define INSTR_LDDF	IF3cd11(3,0x23)

#define INSTR_STF	IF3cd11(3,0x24)
#define INSTR_STFSR	IF3cd01(3,0x25)
#define INSTR_STDFQ	IF3cd01(3,0x26)
#define INSTR_STDF	IF3cd11(3,0x27)

#define INSTR_LDC	IF3cd11(3,0x30)
#define INSTR_LDCSR	IF3cd01(3,0x31)
#define INSTR_LDDC	IF3cd11(3,0x33)

#define INSTR_STC	IF3cd11(3,0x34)
#define INSTR_STCSR	IF3cd01(3,0x35)
#define INSTR_STDCQ	IF3cd01(3,0x36)
#define INSTR_STDC	IF3cd11(3,0x37)

#ifdef CHIPFABS	/* only in fab#1 chip */
# define INSTR_LDFA	INSTR_LDC
# define INSTR_LDFSRA	INSTR_LDCSR
# define INSTR_LDDFA	INSTR_LDDC

# define INSTR_STFA	INSTR_STC
# define INSTR_STFSRA	INSTR_STCSR
# define INSTR_STDFQA	INSTR_STDCQ
# define INSTR_STDFA	INSTR_STDC
#endif /*CHIPFABS*/

#define INSTR_LDF2	IF3cd11(3,0x28)
#define INSTR_LDF3	IF3cd11(3,0x29)
#define INSTR_STF2	IF3cd11(3,0x2c)
#define INSTR_STF3	IF3cd11(3,0x2d)

#define INSTR_LDC2	IF3cd11(3,0x38)
#define INSTR_LDC3	IF3cd11(3,0x39)
#define INSTR_STC2	IF3cd11(3,0x3c)
#define INSTR_STC3	IF3cd11(3,0x3d)

#define INSTR_ADD	IF3cd10(2,0x00)
#define INSTR_SUB	IF3cd10(2,0x04)
#define INSTR_ADDX	IF3cd10(2,0x08)
#define INSTR_SUBX	IF3cd10(2,0x0c)
#define INSTR_AND	IF3cd10(2,0x01)
#define INSTR_ANDN	IF3cd10(2,0x05)
#define INSTR_OR	IF3cd10(2,0x02)
#define INSTR_ORN	IF3cd10(2,0x06)
#define INSTR_XOR	IF3cd10(2,0x03)
#define INSTR_XNOR	IF3cd10(2,0x07)

#define INSTR_ADDCC	IF3cd10(2,0x10)
#define INSTR_SUBCC	IF3cd10(2,0x14)
#define INSTR_ADDXCC	IF3cd10(2,0x18)
#define INSTR_SUBXCC	IF3cd10(2,0x1c)
#define INSTR_ANDCC	IF3cd10(2,0x11)
#define INSTR_ANDNCC	IF3cd10(2,0x15)
#define INSTR_ORCC	IF3cd10(2,0x12)
#define INSTR_ORNCC	IF3cd10(2,0x16)
#define INSTR_XORCC	IF3cd10(2,0x13)
#define INSTR_XNORCC	IF3cd10(2,0x17)

#define INSTR_TADDCC	IF3cd10(2,0x20)
#define INSTR_TSUBCC	IF3cd10(2,0x21)
#define INSTR_TADDCCTV	IF3cd10(2,0x22)
#define INSTR_TSUBCCTV	IF3cd10(2,0x23)

#define INSTR_MULSCC	IF3cd10(2,0x24)
#define INSTR_SLL	IF3cd10(2,0x25)
#define INSTR_SRL	IF3cd10(2,0x26)
#define INSTR_SRA	IF3cd10(2,0x27)

#define INSTR_UMUL      IF3cd10(2,0x0a)
#define INSTR_UMULCC    IF3cd10(2,0x1a)
#define INSTR_SMUL      IF3cd10(2,0x0b)
#define INSTR_SMULCC    IF3cd10(2,0x1b)
#define INSTR_UDIV      IF3cd10(2,0x0e)
#define INSTR_UDIVCC    IF3cd10(2,0x1e)
#define INSTR_SDIV      IF3cd10(2,0x0f)
#define INSTR_SDIVCC    IF3cd10(2,0x1f)

#define INSTR_RD_Y	IF3cd1X(2,0x28)
#define INSTR_RD_PSR	IF3cd1X(2,0x29)
#define INSTR_RD_WIM	IF3cd1X(2,0x2a)
#define INSTR_RD_TBR	IF3cd1X(2,0x2b)

#define INSTR_WR_Y	IF3cd00(2,0x30)
#define INSTR_WR_PSR	IF3cd00(2,0x31)
#define INSTR_WR_WIM	IF3cd00(2,0x32)
#define INSTR_WR_TBR	IF3cd00(2,0x33)

#define INSTR_JMPL	IF3cd00(2,0x38)
#define INSTR_RETT	IF3cd00(2,0x39)

#define INSTR_TN	IF3ab(2,0x0,0x3a)
#define INSTR_TE	IF3ab(2,0x1,0x3a)
#define INSTR_TLE	IF3ab(2,0x2,0x3a)
#define INSTR_TL	IF3ab(2,0x3,0x3a)
#define INSTR_TLEU	IF3ab(2,0x4,0x3a)
#define INSTR_TCS	IF3ab(2,0x5,0x3a)
#define INSTR_TNEG	IF3ab(2,0x6,0x3a)
#define INSTR_TVS	IF3ab(2,0x7,0x3a)
#define INSTR_TA	IF3ab(2,0x8,0x3a)
#define INSTR_TNE	IF3ab(2,0x9,0x3a)
#define INSTR_TG	IF3ab(2,0xa,0x3a)
#define INSTR_TGE	IF3ab(2,0xb,0x3a)
#define INSTR_TGU	IF3ab(2,0xc,0x3a)
#define INSTR_TCC	IF3ab(2,0xd,0x3a)
#define INSTR_TPOS	IF3ab(2,0xe,0x3a)
#define INSTR_TVC	IF3ab(2,0xf,0x3a)

#define INSTR_IFLUSH	IF3cd00(2,0x3b)

#define INSTR_SAVE	IF3cd10(2,0x3c)
#define INSTR_RESTORE	IF3cd10(2,0x3d)

#define INSTR_FITOS	IF3e10(0x34,0xc0|INT10|SGL32)
#define INSTR_FITOD	IF3e10(0x34,0xc0|INT10|DBL32)
#define INSTR_FITOQ	IF3e10(0x34,0xc0|INT10|QUAD32)
#define INSTR_FSTOI	IF3e10(0x34,0xd0|SGL10|INT32)
#define INSTR_FDTOI	IF3e10(0x34,0xd0|DBL10|INT32)
#define INSTR_FQTOI	IF3e10(0x34,0xd0|QUAD10|INT32)
#define INSTR_FSTOD	IF3e10(0x34,0xc0|SGL10|DBL32)
#define INSTR_FSTOQ	IF3e10(0x34,0xc0|SGL10|QUAD32)
#define INSTR_FDTOS	IF3e10(0x34,0xc0|DBL10|SGL32)
#define INSTR_FDTOQ	IF3e10(0x34,0xc0|DBL10|QUAD32)
#define INSTR_FQTOD	IF3e10(0x34,0xc0|QUAD10|DBL32)
#define INSTR_FQTOS	IF3e10(0x34,0xc0|QUAD10|SGL32)

#define INSTR_FINTS	IF3e10(0x34,0x20|SGL10)
#define INSTR_FINTD	IF3e10(0x34,0x20|DBL10)
#define INSTR_FINTX	IF3e10(0x34,0x20|QUAD10)

#define INSTR_FMOVS	IF3e10(0x34,0x00|SGL10)
#define INSTR_FNEGS	IF3e10(0x34,0x04|SGL10)
#define INSTR_FABSS	IF3e10(0x34,0x08|SGL10)

#define INSTR_FSQRTS	IF3e10(0x34,0x28|SGL10)
#define INSTR_FSQRTD	IF3e10(0x34,0x28|DBL10)
#define INSTR_FSQRTQ	IF3e10(0x34,0x28|QUAD10)

#define INSTR_FADDS	IF3e11(0x34,0x40|SGL10)
#define INSTR_FADDD	IF3e11(0x34,0x40|DBL10)
#define INSTR_FADDQ	IF3e11(0x34,0x40|QUAD10)
#define INSTR_FSUBS	IF3e11(0x34,0x44|SGL10)
#define INSTR_FSUBD	IF3e11(0x34,0x44|DBL10)
#define INSTR_FSUBQ	IF3e11(0x34,0x44|QUAD10)
#define INSTR_FMULS	IF3e11(0x34,0x48|SGL10)
#define INSTR_FMULD	IF3e11(0x34,0x48|DBL10)
#define INSTR_FMULQ	IF3e11(0x34,0x48|QUAD10)
#define INSTR_FDIVS	IF3e11(0x34,0x4c|SGL10)
#define INSTR_FDIVD	IF3e11(0x34,0x4c|DBL10)
#define INSTR_FDIVQ	IF3e11(0x34,0x4c|QUAD10)
#define INSTR_FREMS	IF3e11(0x34,0x60|SGL10)
#define INSTR_FREMD	IF3e11(0x34,0x60|DBL10)
#define INSTR_FREMQ	IF3e11(0x34,0x60|QUAD10)
#define INSTR_FQUOS	IF3e11(0x34,0x64|SGL10)
#define INSTR_FQUOD	IF3e11(0x34,0x64|DBL10)
#define INSTR_FQUOQ	IF3e11(0x34,0x64|QUAD10)
#define INSTR_FSMULD	IF3e11(0x34,0x68|SGL10)
#define INSTR_FDMULQ	IF3e11(0x34,0x6c|DBL10)

#define INSTR_FCMPS	IF3e01(0x35,0x50|SGL10)
#define INSTR_FCMPD	IF3e01(0x35,0x50|DBL10)
#define INSTR_FCMPQ	IF3e01(0x35,0x50|QUAD10)
#define INSTR_FCMPES	IF3e01(0x35,0x54|SGL10)
#define INSTR_FCMPED	IF3e01(0x35,0x54|DBL10)
#define INSTR_FCMPEQ	IF3e01(0x35,0x54|QUAD10)

#define INSTR_CPOP1	IF3e11_CPOP(0x36)
#define INSTR_CPOP2	IF3e11_CPOP(0x37)

#define INSTR_UNIMP	IF3cd1X(0,0x00)

/* pseudo-instruction(s):	*/
#define INSTR_RET	(INSTR_JMPL|(31<<14)|(1<<13)|8)	/* jmp %i7+8	*/
#define INSTR_RETL	(INSTR_JMPL|(15<<14)|(1<<13)|8)	/* jmp %o7+8	*/
#define INSTR_NOP	(INSTR_SETHI|(0<<25))		/* sethi 0,%g0	*/

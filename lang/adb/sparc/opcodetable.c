#ifndef lint
static  char sccsid[] = "@(#)opcodetable.c 1.1 94/10/31 Copyr 1986 Sun Micro";
/*
 * From Will Brown's "sas" Sparc Architectural Simulator,
 * Version "@:-)opcodetable.c	3.3	2/4/86"
 */
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* opcodetable.c
	This file contains the static initialization for the opcode
	table which is common to sas and sasm, and adb-for-sparc.

	This file also contains the "makeBkptInstr" routine.
*/

#include <sys/types.h>
#include "sr_general.h"
#include "sr_switchindexes.h"
#include "sr_opcodetable.h"
#include "sr_instruction.h"
#include "sr_sw_trap.h"		/* software trap types */

/*
	switch-index values of 0 (NONE) indicate table entries that are of no
	interest to the simulator sas.

	similarly, parse token values of 0 (NONE) indicate table entries that
	are of no interest to the assembler
*/

struct opcodeTableStruct opcodeTable[] = {
/*
switch-index	type	op	opx1	opx2	mnemonic
									*/
/*	Integer Condition codes */
ICN,		OT_IC,	0x0,	NONE,	NONE,	NONE,
ICNE,		OT_IC,	0x9,	NONE,	NONE,	NONE,
ICEQ,		OT_IC,	0x1,	NONE,	NONE,	NONE,
ICGT,		OT_IC,	0xa,	NONE,	NONE,	NONE,
ICLE,		OT_IC,	0x2,	NONE,	NONE,	NONE,
ICGE,		OT_IC,	0xb,	NONE,	NONE,	NONE,
ICLT,		OT_IC,	0x3,	NONE,	NONE,	NONE,
ICHI,		OT_IC,	0xc,	NONE,	NONE,	NONE,
ICLS,		OT_IC,	0x4,	NONE,	NONE,	NONE,
ICCC,		OT_IC,	0xd,	NONE,	NONE,	NONE,
ICCS,		OT_IC,	0x5,	NONE,	NONE,	NONE,
ICPL,		OT_IC,	0xe,	NONE,	NONE,	NONE,
ICMI,		OT_IC,	0x6,	NONE,	NONE,	NONE,
ICVC,		OT_IC,	0xf,	NONE,	NONE,	NONE,
ICVS,		OT_IC,	0x7,	NONE,	NONE,	NONE,
ICA,		OT_IC,	0x8,	NONE,	NONE,	NONE,


/*	Floating Condition codes */
FCN,		OT_FC,	0x0,	NONE,	NONE,	NONE,
FCUN,		OT_FC,	0x7,	NONE,	NONE,	NONE,
FCGT,		OT_FC,	0x6,	NONE,	NONE,	NONE,
FCUG,		OT_FC,	0x5,	NONE,	NONE,	NONE,
FCLT,		OT_FC,	0x4,	NONE,	NONE,	NONE,
FCUL,		OT_FC,	0x3,	NONE,	NONE,	NONE,
FCLG,		OT_FC,	0x2,	NONE,	NONE,	NONE,
FCNE,		OT_FC,	0x1,	NONE,	NONE,	NONE,
FCEQ,		OT_FC,	0x9,	NONE,	NONE,	NONE,
FCUE,		OT_FC,	0xa,	NONE,	NONE,	NONE,
FCGE,		OT_FC,	0xb,	NONE,	NONE,	NONE,
FCUGE,		OT_FC,	0xc,	NONE,	NONE,	NONE,
FCLE,		OT_FC,	0xd,	NONE,	NONE,	NONE,
FCULE,		OT_FC,	0xe,	NONE,	NONE,	NONE,
FCLEG,		OT_FC,	0xf,	NONE,	NONE,	NONE,
FCA,		OT_FC,	0x8,	NONE,	NONE,	NONE,


/*	Bicc Instructions */
/*	Their OP2 opcodes changed from FAB1 to FAB2, so they're symbolic. */

BICC,		OT_F2,	0,  SR_BICC_OP, 0x0,	"bn",
BICC,		OT_F2,	0,  SR_BICC_OP, 0x9,	"bne",
BICC,		OT_F2,	0,  SR_BICC_OP, 0x1,	"be",	/* change to beq? */
BICC,		OT_F2,	0,  SR_BICC_OP, 0xa,	"bg",	/* change to bgt? */
BICC,		OT_F2,	0,  SR_BICC_OP, 0x2,	"ble",
BICC,		OT_F2,	0,  SR_BICC_OP, 0xb,	"bge",
BICC,		OT_F2,	0,  SR_BICC_OP, 0x3,	"bl",	/* change to blt? */
BICC,		OT_F2,	0,  SR_BICC_OP, 0xc,	"bgu",
BICC,		OT_F2,	0,  SR_BICC_OP, 0x4,	"bleu",
BICC,		OT_F2,	0,  SR_BICC_OP, 0xd,	"bgeu",
BICC,		OT_F2,	0,  SR_BICC_OP, 0x5,	"blu",
BICC,		OT_F2,	0,  SR_BICC_OP, 0xe,	"bpos",
BICC,		OT_F2,	0,  SR_BICC_OP, 0x6,	"bneg",
BICC,		OT_F2,	0,  SR_BICC_OP, 0xf,	"bvc",
BICC,		OT_F2,	0,  SR_BICC_OP, 0x7,	"bvs",
BICC,		OT_F2,	0,  SR_BICC_OP, 0x8,	"ba",

/*	Bfcc Instructions */

FBFCC,		OT_F2,	0,  SR_FBCC_OP, 0x0,	"fbn",
FBFCC,		OT_F2,	0,  SR_FBCC_OP, 0x7,	"fbu",
FBFCC,		OT_F2,	0,  SR_FBCC_OP, 0x6,	"fbg",	/* change to fbgt? */
FBFCC,		OT_F2,	0,  SR_FBCC_OP, 0x5,	"fbug",
FBFCC,		OT_F2,	0,  SR_FBCC_OP, 0x4,	"fbl",	/* change to fblt? */
FBFCC,		OT_F2,	0,  SR_FBCC_OP, 0x3,	"fbul",
FBFCC,		OT_F2,	0,  SR_FBCC_OP, 0x2,	"fblg",
FBFCC,		OT_F2,	0,  SR_FBCC_OP, 0x1,	"fbne",
FBFCC,		OT_F2,	0,  SR_FBCC_OP, 0x9,	"fbe",	/* change to fbeq? */
FBFCC,		OT_F2,	0,  SR_FBCC_OP, 0xa,	"fbue",
FBFCC,		OT_F2,	0,  SR_FBCC_OP, 0xb,	"fbge",
FBFCC,		OT_F2,	0,  SR_FBCC_OP, 0xc,	"fbuge",
FBFCC,		OT_F2,	0,  SR_FBCC_OP, 0xd,	"fble",
FBFCC,		OT_F2,	0,  SR_FBCC_OP, 0xe,	"fbule",
FBFCC,		OT_F2,	0,  SR_FBCC_OP, 0xf,	"fbo",
FBFCC,		OT_F2,	0,  SR_FBCC_OP, 0x8,	"fba",

/*	Load Instructions */

LDSB,		OT_F3,	3,	3<<IFW_OP3 | 0x09,	NONE,	"ldsb",
LDSBA,		OT_F3,	3,	3<<IFW_OP3 | 0x19,	NONE,	"ldsba",
LDSH,		OT_F3,	3,	3<<IFW_OP3 | 0x0a,	NONE,	"ldsh",
LDSHA,		OT_F3,	3,	3<<IFW_OP3 | 0x1a,	NONE,	"ldsha",
LDUB,		OT_F3,	3,	3<<IFW_OP3 | 0x01,	NONE,	"ldub",
LDUBA,		OT_F3,	3,	3<<IFW_OP3 | 0x11,	NONE,	"lduba",
LDUH,		OT_F3,	3,	3<<IFW_OP3 | 0x02,	NONE,	"lduh",
LDUHA,		OT_F3,	3,	3<<IFW_OP3 | 0x12,	NONE,	"lduha",
LD,		OT_F3,	3,	3<<IFW_OP3 | 0x00,	NONE,	"ld",
LDA,		OT_F3,	3,	3<<IFW_OP3 | 0x10,	NONE,	"lda",
LDD,		OT_F3,	3,	3<<IFW_OP3 | 0x03,	NONE,	"ldd",
LDDA,		OT_F3,	3,	3<<IFW_OP3 | 0x13,	NONE,	"ldda",

LDF,		OT_F3,	3,	3<<IFW_OP3 | 0x20,	NONE,	"ld",
LDDF,		OT_F3,	3,	3<<IFW_OP3 | 0x23,	NONE,	"ldd",
LDFSR,		OT_F3,	3,	3<<IFW_OP3 | 0x21,	NONE,	"ld",

LDC,		OT_F3,	3,	3<<IFW_OP3 | 0x30,	NONE,	"ld",
LDDC,		OT_F3,	3,	3<<IFW_OP3 | 0x33,	NONE,	"ldd",
LDCSR,		OT_F3,	3,	3<<IFW_OP3 | 0x31,	NONE,	"ld",

/*	Store Instructions */

STB,		OT_F3,	3,	3<<IFW_OP3 | 0x05,	NONE,	"stb",
STBA,		OT_F3,	3,	3<<IFW_OP3 | 0x15,	NONE,	"stba",
STH,		OT_F3,	3,	3<<IFW_OP3 | 0x06,	NONE,	"sth",
STHA,		OT_F3,	3,	3<<IFW_OP3 | 0x16,	NONE,	"stha",
ST,		OT_F3,	3,	3<<IFW_OP3 | 0x04,	NONE,	"st",
STA,		OT_F3,	3,	3<<IFW_OP3 | 0x14,	NONE,	"sta",
STD,		OT_F3,	3,	3<<IFW_OP3 | 0x07,	NONE,	"std",
STDA,		OT_F3,	3,	3<<IFW_OP3 | 0x17,	NONE,	"stda",

STF,		OT_F3,	3,	3<<IFW_OP3 | 0x24,	NONE,	"st",
STDF,		OT_F3,	3,	3<<IFW_OP3 | 0x27,	NONE,	"std",
STFSR,		OT_F3,	3,	3<<IFW_OP3 | 0x25,	NONE,	"st",
STDFQ,		OT_F3,	3,	3<<IFW_OP3 | 0x26,	NONE,	"std",

STC,		OT_F3,	3,	3<<IFW_OP3 | 0x34,	NONE,	"st",
STDC,		OT_F3,	3,	3<<IFW_OP3 | 0x37,	NONE,	"std",
STCSR,		OT_F3,	3,	3<<IFW_OP3 | 0x35,	NONE,	"st",
STDCQ,		OT_F3,	3,	3<<IFW_OP3 | 0x36,	NONE,	"std",

/*	Atomic Load-Store Instructions */

LDSTUB,		OT_F3,	3,	3<<IFW_OP3 | 0x0d,	NONE,	"ldstub",
LDSTUBA,	OT_F3,	3,	3<<IFW_OP3 | 0x1d,	NONE,	"ldstuba",
SWAP,		OT_F3,	3,	3<<IFW_OP3 | 0x0f,	NONE,	"swap",
SWAPA,		OT_F3,	3,	3<<IFW_OP3 | 0x1f,	NONE,	"swapa",

/*	Arithmetic Instructions */

ADD,		OT_F3,	2,	2<<IFW_OP3 | 0x00,	NONE,	"add",
ADDcc,		OT_F3,	2,	2<<IFW_OP3 | 0x10,	NONE,	"addcc",
ADDX,		OT_F3,	2,	2<<IFW_OP3 | 0x08,	NONE,	"addx",
ADDXcc,		OT_F3,	2,	2<<IFW_OP3 | 0x18,	NONE,	"addxcc",
TADDcc,		OT_F3,  2,	2<<IFW_OP3 | 0x20,      NONE,   "taddcc",
TADDccTV,	OT_F3,  2,	2<<IFW_OP3 | 0x22,      NONE,   "taddcctv",
SUB,		OT_F3,	2,	2<<IFW_OP3 | 0x04,	NONE,	"sub",
SUBcc,		OT_F3,	2,	2<<IFW_OP3 | 0x14,	NONE,	"subcc",
SUBX,		OT_F3,	2,	2<<IFW_OP3 | 0x0c,	NONE,	"subx",
SUBXcc,		OT_F3,	2,	2<<IFW_OP3 | 0x1c,	NONE,	"subxcc",
TSUBcc,		OT_F3,	2,	2<<IFW_OP3 | 0x21,	NONE,	"tsubcc",
TSUBccTV,	OT_F3,	2,	2<<IFW_OP3 | 0x23,	NONE,	"tsubcctv",
MULScc,		OT_F3,	2,	2<<IFW_OP3 | 0x24,	NONE,	"mulscc",

/*	Logical Instructions */


AND,		OT_F3,	2,	2<<IFW_OP3 | 0x01,	NONE,	"and",
ANDcc,		OT_F3,	2,	2<<IFW_OP3 | 0x11,	NONE,	"andcc",
ANDN,		OT_F3,	2,	2<<IFW_OP3 | 0x05,	NONE,	"andn",
ANDNcc,		OT_F3,	2,	2<<IFW_OP3 | 0x15,	NONE,	"andncc",
OR,		OT_F3,	2,	2<<IFW_OP3 | 0x02,	NONE,	"or",
ORcc,		OT_F3,	2,	2<<IFW_OP3 | 0x12,	NONE,	"orcc",
ORN,		OT_F3,	2,	2<<IFW_OP3 | 0x06,	NONE,	"orn",
ORNcc,		OT_F3,	2,	2<<IFW_OP3 | 0x16,	NONE,	"orncc",
XOR,		OT_F3,	2,	2<<IFW_OP3 | 0x03,	NONE,	"xor",
XORcc,		OT_F3,	2,	2<<IFW_OP3 | 0x13,	NONE,	"xorcc",
XORN,		OT_F3,	2,	2<<IFW_OP3 | 0x07,	NONE,	"xnor",
XORNcc,		OT_F3,	2,	2<<IFW_OP3 | 0x17,	NONE,	"xnorcc",

/*	Shift Instructions */

SLL,		OT_F3,	2,	2<<IFW_OP3 | 0x25,	NONE,	"sll",
SRL,		OT_F3,	2,	2<<IFW_OP3 | 0x26,	NONE,	"srl",
SRA,		OT_F3,	2,	2<<IFW_OP3 | 0x27,	NONE,	"sra",

/*	Sethi Instruction */
SETHI,		OT_F2,	0,	SR_SETHI_OP,		NONE,	"sethi",

/*      The "unimplemented" Instruction -- used by cc to return structs */
UNIMP,		OT_F2,	0,	SR_UNIMP_OP,		NONE,	"unimp",

/*	Procedure Instructions */

SAVE,		OT_F3,	2,	2<<IFW_OP3 | 0x3c,	NONE,	"save",
RESTORE,	OT_F3,	2,	2<<IFW_OP3 | 0x3d,	NONE,	"restore",
CALL,		OT_F1,	1,	NONE,			NONE,	"call",
RETT,		OT_F3,	2,	2<<IFW_OP3 | 0x39,	NONE,	"rett",

/*	Jump Instruction */

JUMP,		OT_F3,	2,	2<<IFW_OP3 | 0x38,	NONE,	"jmp",


/*	Ticc Instructions */

TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0x0,	"tn",
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0x9,	"tne",
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0x1,	"te", /* change to teq? */
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0xa,	"tg", /* change to tgt? */
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0x2,	"tle",
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0xb,	"tge",
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0x3,	"tl", /* change to tlt? */
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0xc,	"tgu",
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0x4,	"tleu",
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0xd,	"tgeu",
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0x5,	"tlu",
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0xe,	"tpos",
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0x6,	"tneg",
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0xf,	"tvc",
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0x7,	"tvs",
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0x8,	"ta",
							/* WARNING!!!
	When changing the mnemonic for TRAP ALWAYS, be sure to change
	the appearance in procedure makeBkptInstr() in file breakpoint.c */

/* Read Register Instructions */

RDY,		OT_F3,	2,	2<<IFW_OP3 | 0x28,	NONE,	"rd",
RDPSR,		OT_F3,	2,	2<<IFW_OP3 | 0x29,	NONE,	"rd",
RDTBR,		OT_F3,	2,	2<<IFW_OP3 | 0x2b,	NONE,	"rd",
RDWIM,		OT_F3,	2,	2<<IFW_OP3 | 0x2a,	NONE,	"rd",

/* Write Register Instructions */

WRY,		OT_F3,	2,	2<<IFW_OP3 | 0x30,	NONE,	"wr",
WRPSR,		OT_F3,	2,	2<<IFW_OP3 | 0x31,	NONE,	"wr",
WRWIM,		OT_F3,	2,	2<<IFW_OP3 | 0x32,	NONE,	"wr",
WRTBR,		OT_F3,	2,	2<<IFW_OP3 | 0x33,	NONE,	"wr",

/* I cache Flush instruction. Thank You bill joy. */

IFLUSH,		OT_F3,	2,	2<<IFW_OP3 | 0x3b,	NONE,	"iflush",

/*	Floating-point Operate Instructions */

FPOP,		OT_F3,	2,	2<<IFW_OP3 | 0x34,	NONE,	NONE,
FPCMP,		OT_F3,	2,	2<<IFW_OP3 | 0x35,	NONE,	NONE,

/*	Pseudo Instructions (assembler generates real instructions)	*/

NOP,		OT_PSEUDO,	NONE,	NONE,	NONE,	"nop",
CMP,		OT_PSEUDO,	NONE,	NONE,	NONE,	"cmp",
TST,		OT_PSEUDO,	NONE,	NONE,	NONE,	"tst",
MOV,		OT_PSEUDO,	NONE,	NONE,	NONE,	"mov",
CLR,		OT_PSEUDO,      NONE,   NONE,   NONE,   "clr",
RET,		OT_PSEUDO,	NONE,	NONE,	NONE,	"ret",

/*	FPOP Instructions */
FCVTis,		OT_FOP,	2,	0x34,	0xc4,	"fitos",
FCVTid,		OT_FOP,	2,	0x34,	0xc8,	"fitod",
FCVTie,		OT_FOP,	2,	0x34,	0xcc,	"fitox",

FCVTsi,		OT_FOP,	2,	0x34,	0xc1,	"fstoir",
FCVTdi,		OT_FOP,	2,	0x34,	0xc2,	"fdtoir",
FCVTei,		OT_FOP,	2,	0x34,	0xc3,	"fxtoir",

FCVTsiz,	OT_FOP,	2,	0x34,	0xd1,	"fstoi",
FCVTdiz,	OT_FOP,	2,	0x34,	0xd2,	"fdtoi",
FCVTeiz,	OT_FOP,	2,	0x34,	0xd3,	"fxtoi",

FINTs,		OT_FOP,	2,	0x34,	0x21,	"fints",
FINTd,		OT_FOP,	2,	0x34,	0x22,	"fintd",
FINTe,		OT_FOP,	2,	0x34,	0x23,	"fintx",

FINTsz,		OT_FOP,	2,	0x34,	0x25,	"fintrzs",
FINTdz,		OT_FOP,	2,	0x34,	0x26,	"fintrzd",
FINTez,		OT_FOP,	2,	0x34,	0x27,	"fintrzx",

FCVTsd,		OT_FOP,	2,	0x34,	0xc9,	"fstod",
FCVTse,		OT_FOP,	2,	0x34,	0xcd,	"fstox",
FCVTds,		OT_FOP,	2,	0x34,	0xc6,	"fdtos",
FCVTde,		OT_FOP,	2,	0x34,	0xce,	"fdtox",
FCVTes,		OT_FOP,	2,	0x34,	0xc7,	"fxtos",
FCVTed,		OT_FOP,	2,	0x34,	0xcb,	"fxtod",

FMOVE,		OT_FOP,	2,	0x34,	0x01,	"fmovs",
FNEG,		OT_FOP,	2,	0x34,	0x05,	"fnegs",
FABS,		OT_FOP,	2,	0x34,	0x09,	"fabss",

FCLSs,		OT_FOP,	2,	0x34,	0xe1,	"fclasss",
FCLSd,		OT_FOP,	2,	0x34,	0xe2,	"fclassd",
FCLSe,		OT_FOP,	2,	0x34,	0xe3,	"fclassx",

FEXPs,		OT_FOP,	2,	0x34,	0xf1,	"fexpos",
FEXPd,		OT_FOP,	2,	0x34,	0xf2,	"fexpod",
FEXPe,		OT_FOP,	2,	0x34,	0xf3,	"fexpox",

FSQRs,		OT_FOP,	2,	0x34,	0x29,	"fsqrts",
FSQRd,		OT_FOP,	2,	0x34,	0x2a,	"fsqrtd",
FSQRe,		OT_FOP,	2,	0x34,	0x2b,	"fsqrtx",

FADDs,		OT_FOP,	2,	0x34,	0x41,	"fadds",
FADDd,		OT_FOP,	2,	0x34,	0x42,	"faddd",
FADDe,		OT_FOP,	2,	0x34,	0x43,	"faddx",

FSUBs,		OT_FOP,	2,	0x34,	0x45,	"fsubs",
FSUBd,		OT_FOP,	2,	0x34,	0x46,	"fsubd",
FSUBe,		OT_FOP,	2,	0x34,	0x47,	"fsubx",

FMULs,		OT_FOP,	2,	0x34,	0x49,	"fmuls",
FMULd,		OT_FOP,	2,	0x34,	0x4a,	"fmuld",
FMULe,		OT_FOP,	2,	0x34,	0x4b,	"fmulx",

FDIVs,		OT_FOP,	2,	0x34,	0x4d,	"fdivs",
FDIVd,		OT_FOP,	2,	0x34,	0x4e,	"fdivd",
FDIVe,		OT_FOP,	2,	0x34,	0x4f,	"fdivx",

FSCLs,		OT_FOP,	2,	0x34,	0x84,	"fscales",
FSCLd,		OT_FOP,	2,	0x34,	0x88,	"fscaled",
FSCLe,		OT_FOP,	2,	0x34,	0x8c,	"fscalex",

FREMs,		OT_FOP,	2,	0x34,	0x61,	"frems",
FREMd,		OT_FOP,	2,	0x34,	0x62,	"fremd",
FREMe,		OT_FOP,	2,	0x34,	0x63,	"fremx",

FQUOs,		OT_FOP,	2,	0x34,	0x65,	"fquos",
FQUOd,		OT_FOP,	2,	0x34,	0x66,	"fquod",
FQUOe,		OT_FOP,	2,	0x34,	0x67,	"fquox",

FsMULd,		OT_FOP, 2, 	0x34,	0x69,	"fsmuld",
FdMULe,		OT_FOP, 2, 	0x34,	0x6E,	"fdmulx",

FCMPs,		OT_FOP,	2,	0x35,	0x51,	"fcmps",
FCMPd,		OT_FOP,	2,	0x35,	0x52,	"fcmpd",
FCMPe,		OT_FOP,	2,	0x35,	0x53,	"fcmpx",

FCMPXs,		OT_FOP,	2,	0x35,	0x55,	"fcmpes",
FCMPXd,		OT_FOP,	2,	0x35,	0x56,	"fcmped",
FCMPXe,		OT_FOP,	2,	0x35,	0x57,	"fcmpex",

/*	Multiply and Divide instruction from version 8 sparc */

UMUL,		OT_F3,	2,	2<<IFW_OP3 | 0xa,	NONE,	"umul",
SMUL,		OT_F3,	2,	2<<IFW_OP3 | 0xb,	NONE,   "smul",
UMULcc,		OT_F3,	2,	2<<IFW_OP3 | 0x1a,	NONE,   "umulcc",
SMULcc,		OT_F3,	2,	2<<IFW_OP3 | 0x1b,	NONE,   "smulcc",

UDIV,		OT_F3,	2,	2<<IFW_OP3 | 0xe,	NONE,	"udiv",
SDIV,		OT_F3,	2,	2<<IFW_OP3 | 0xf,	NONE,	"sdiv",
UDIVcc,		OT_F3,	2,	2<<IFW_OP3 | 0x1e,	NONE,   "udivcc",
SDIVcc,		OT_F3,	2,	2<<IFW_OP3 | 0x1f,	NONE,   "sdivcc",

/*	End of Table Marker */

NONE,			NONE,	NONE,	NONE,	NONE,	NONE
};


Word bkptInstr = 0;		/* Value of the breakpoint instruction */


#include <stdio.h>	/* for now q?*/
/* This should be done statically */
makeBkptInstr()
/*
	This routine creates the break point instruction
*/
{
	Word instruction = 0;
	struct opcodeTableStruct *opPtr;

	opPtr = opcodeTable;

	/* look for the trap-always instruction in the opcode table */
	while(opPtr->o_type != NONE){
		if(opPtr->o_switchIndex == TICC &&
			(strcmp("ta", opPtr->o_mnemonic)  == 0))break;
		++opPtr;
	}
	if(opPtr->o_type == NONE) {	/* you can take this out */
		fprintf(stderr,"makeBkptInstr(): Can't find necessary opcode");
		exit( 1 );
	}

	COND = opPtr->o_opx2;
	OP = opPtr->o_op;
	OP3 = opPtr->o_opx1 & (pow2(IFW_OP3) - 1);
	IMM = 1;
	SIMM13 = ST_BREAKPOINT;

	bkptInstr = instruction;
}


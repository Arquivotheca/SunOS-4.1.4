/* "@(#)sr_switchindexes.h 1.1 94/10/31 Copyr 1986 Sun Micro"
 * From Will Brown's "sas" Sparc Architectural Simulator,
 * Version "@:-)switchindexes.h	3.1 11/11/85
 */
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* switchindexes.h renamed to sr_switchindexes.h
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
	with these switch indices, regardless of instruction format.

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
#define CLR	5
#define RET	6
	
/*						"PSEUDO" NON-INSTRUCTION */
#define UNIMP	7


/*						FORMAT 1 INSTRUCTIONS */

/*	Call Instruction */
#define CALL	8

/*						FORMAT 2 INSTRUCTIONS */

/*	Conditional Branch Instructions	*/
#define BICC	9
#define FBFCC	10

/*	Sethi Instruction */
#define SETHI	11

/*						FORMAT 3 INSTRUCTIONS */
/*	Load instructions */

#define LDSB	12
#define LDSBA	13
#define LDSH	14
#define LDSHA	15
#define LDUB	16
#define LDUBA	17
#define LDUH	18
#define LDUHA	19
#define LD	20
#define LDA	21
#define LDD	22
#define LDDA	23

#define LDF	24
#define LDDF	25
#define LDFSR	26

#define LDC	27
#define LDDC	28
#define LDCSR	29

/*	Store Instructions */

#define STB	30
#define STBA	31
#define STH	32
#define STHA	33
#define ST	34
#define STA	35
#define STD	36
#define STDA	37

#define STF	38
#define STDF	39
#define STFSR	40
#define STDFQ	41

#define STC	42
#define STDC	43
#define STCSR	44
#define STDCQ	45

/*	Atomic Load-Store Instructions */

#define LDSTUB	46
#define LDSTUBA	47
#define SWAP	48
#define SWAPA	49

/*	Arithmetic Instructions */

#define ADD	50
#define ADDcc	51
#define ADDX	52
#define ADDXcc	53
#define TADDcc	54
#define TADDccTV 55

#define SUB	56
#define SUBcc	57
#define SUBX	58
#define SUBXcc	59
#define TSUBcc	60
#define TSUBccTV	61

#define MULScc	62

/*	Logical Instructions */

#define AND	63
#define ANDcc	64
#define ANDN	65
#define ANDNcc	66
#define OR	67
#define ORcc	68
#define ORN	69
#define ORNcc	70
#define XOR	71
#define XORcc	72
#define XORN	73
#define XORNcc	74

/*	Shift Instructions	*/

#define SLL	75
#define SRL	76
#define SRA	77


/*	Window Instructions	*/

#define SAVE	78
#define RESTORE	79
#define RETT	80


/*	Jump Instruction */

#define JUMP	81

/*	Trap Instruction */

#define TICC	82

/*	Read Register Instructions */

#define RDY	83
#define RDPSR	84
#define RDTBR	85
#define RDWIM	86

/*	Write Register Instructions */

#define WRY	87
#define WRPSR	88
#define WRWIM	89
#define WRTBR	90

/*	Instruction Cache flush */

#define IFLUSH	91

/*	Floating-Point Operation Instructions */

#define FPOP	92
#define FPCMP	93

/*						FPOP INSTRUCTIONS */

#define FCVTis		94
#define FCVTid		95
#define FCVTie		96

#define FCVTsi		97
#define FCVTdi		98
#define FCVTei		99

#define FCVTsiz		100
#define FCVTdiz		101
#define FCVTeiz		102

#define FINTs		103
#define FINTd		104
#define FINTe		105

#define FINTsz		106
#define FINTdz		107
#define FINTez		108

#define FCVTsd		109
#define FCVTse		110
#define FCVTds		111
#define FCVTde		112
#define FCVTes		113
#define FCVTed		114

#define FMOVE		115
#define FNEG		116
#define FABS		117

#define FCLSs		118
#define FCLSd		119
#define FCLSe		120

#define FEXPs		121
#define FEXPd		122
#define FEXPe		123

#define FSQRs		124
#define FSQRd		125
#define FSQRe		126

#define FADDs		127
#define FADDd		128
#define FADDe		129

#define FSUBs		130
#define FSUBd		131
#define FSUBe		132

#define FMULs		133
#define FMULd		134
#define FMULe		135

#define FDIVs		136
#define FDIVd		137
#define FDIVe		138

#define FSCLs		139
#define FSCLd		140
#define FSCLe		141

#define FREMs		142
#define FREMd		143
#define FREMe		144

#define FQUOs		145
#define FQUOd		146
#define FQUOe		147

#define FCMPs		148
#define FCMPd		149
#define FCMPe		150

#define FCMPXs		151
#define FCMPXd		152
#define FCMPXe		153


/* following instructions come from Version 8 of sparc */
/* multiply instructions */

#define UMUL		154
#define SMUL		155
#define UMULcc		156
#define SMULcc		157

/* divide instructions */

#define UDIV		158
#define SDIV		159
#define UDIVcc		160
#define SDIVcc		161

/* V8 fpop instructions */

#define	FsMULd		162
#define	FdMULe		163
/* 			NONE	USED OUT OF SEQUENCE */

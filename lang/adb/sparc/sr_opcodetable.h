/* "@(#)sr_opcodetable.h 1.1 94/10/31 Copyr 1986 Sun Micro";
 * From Will Brown's "sas" Sparc Architectural Simulator,
 * Version "@:-)opcodetable.h	3.1 11/11/85
 */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* opcodetable.h renamed to sr_opcodetable.h
	The structure of the opcode tables and some useful type
	constants are in this file
*/

/*
	switch-index values of NONE (0) indicate table entries that are of no
	interest to adb's disassembler.

	similarly, parse token values of NONE (0) indicate table entries that
	are of no interest to the assembler.
*/

struct opcodeTableStruct {
	short		o_switchIndex;	/* a switch() index used in disasm */
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

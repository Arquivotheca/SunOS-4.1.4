/* "@(#)sr_indextables.h 1.1 94/10/31 Copyr 1986 Sun Micro";
 * From Will Brown's "sas" Sparc Architectural Simulator,
 * Version "@:-)indextables.h	3.1 11/11/85
 */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* indextables.h renamed to sr_indextables.h --
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

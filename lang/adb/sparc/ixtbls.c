#ifndef lint
static  char sccsid[] = "@(#)ixtbls.c 1.1 94/10/31 Copyr 1986 Sun Micro";
/*
 * From Will Brown's "sas" Sparc Architectural Simulator,
 * Version "@:-)indextables.c	3.1\t11/11/85"
 */
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Name changed to ixtbls.c from indextables.c
 * To get a printout of the tables created here,
 * compile this file with -Ddebug_tables.
 */

#include <sys/types.h>
#include <stdio.h>		/* will fail for kadb? */
#include "sr_general.h"
#include "sr_opcodetable.h"
#include "sr_indextables.h"
#include "sr_instruction.h"
#include "sr_switchindexes.h"

/*
 * sr_indextables.c -- This file contains the code which builds
 * the tables for use by the instruction disassembler.  It was snitched
 * whole from the "sas" source files "indextables.c" and "maketables.c",
 * with some pieces of other files added to it.  Note that for adb,
 * mkDisAsmTables is the only public routine in this file.
 */
	
/* taken from in-line code in "sas" main.c */
/* q?  can I remove all of the "instruction decoding" tables? */
#define dbg(routine) printf( "Calling %s\n", routine )
mkDisAsmTables ( )
{
	/* make the necessary index tables for instruction decoding */
	makeFormatTable();
	makeFmt2Table();
	makeFmt3Table();
	makeIccTable();
	makeFccTable();
	makeFpopTable();

	/* make the break-point instruction */
	makeBkptInstr();

	/* make the mnemonic lookup tables for the dis-assembler */
	makeFmt1MnemonicTable();
	makeFmt2MnemonicTable();
	makeFmt3MnemonicTable();
	makeBiccMnemonicTable();
	makeBfccMnemonicTable();
	makeTiccMnemonicTable();
	makeFpopMnemonicTable();
	makePseudoMnemonicTable();

#	ifdef debug_tables
		/* Temporary -- print the tables */
		printDisAsmTables();
#	endif debug_tables

} /* end mkDisAsmTables */


/*	INSTRUCTION FORMAT DISCRIMINATION TABLE	*/
/*
	The table built with the following procedure is indexed with the op
	field of any instruction. The table contains switch indices
	for decoding during execution, or dis-assembly of instructions
*/

struct formatSwitchTableStruct  *formatSwitchTable;

static
makeFormatTable()
{
	formatSwitchTable = (struct formatSwitchTableStruct *)
		makeTable(opcodeTable, sizeof(opcodeTable[0]),
		formatCheckRecord,
		formatExtractKey,
		formatTransRecord,
		sizeof(formatSwitchTable[0]),
		pow2(IFW_OP));
}

static
printFormatTable()
{
	int i;

	printf("Instruction Format Switch Table\n");
	printf("OP		INDEX\n");

	for(i = 0; i < pow2(IFW_OP); ++i)
		printf("0x%x (%d)		%d\n",
			i, i, (formatSwitchTable + i)->index);
}

static
formatCheckRecord(rec)
struct opcodeTableStruct *rec;
{
	if(rec->o_type == NONE)return(LAST_RECORD);
	if(rec->o_type == OT_F1 || rec->o_type == OT_F2 || rec->o_type == OT_F3)
		return(INTERESTING_RECORD);
	return(BORING_RECORD);
}

static
formatExtractKey(rec)
struct opcodeTableStruct *rec;
{
	return((int)(rec->o_op));
}

static
formatTransRecord(rec, newPlace)
struct opcodeTableStruct *rec;
struct formatSwitchTableStruct *newPlace;
{
	newPlace->index = rec->o_type;	/* o_type is used as a switch index */
}


/*	FORMAT 2 INSTRUCTIONS	*/
/*
	The table built using these procedures is to be indexed with the
	op2 field of format 2 instructions during decoding for dis-assembly
	or during execution. The table is an array
	which contains switch() statement indices.
*/

struct fmt2SwitchTableStruct  *fmt2SwitchTable;

static
makeFmt2Table()
{
	fmt2SwitchTable = (struct fmt2SwitchTableStruct *)
		makeTable(opcodeTable, sizeof(opcodeTable[0]),
		fmt2CheckRecord,
		fmt2ExtractKey,
		fmt2TransRecord,
		sizeof(fmt2SwitchTable[0]),
		pow2(IFW_OP2));
}

static
printFmt2Table()
{
	int i;

	printf("Format 2 Instruction Switch Table\n");
	printf("OP2		INDEX\n");

	for(i = 0; i < pow2(IFW_OP2); ++i)
		printf("0x%x (%d)		%d\n",
			i, i, (fmt2SwitchTable + i)->index);
}

static
fmt2CheckRecord(rec)
struct opcodeTableStruct *rec;
{
	if(rec->o_type == NONE)return(LAST_RECORD);
	if(rec->o_switchIndex != NONE && rec->o_type == OT_F2)
		return(INTERESTING_RECORD);
	return(BORING_RECORD);
}

static
fmt2ExtractKey(rec)
struct opcodeTableStruct *rec;
{
	return((int)(rec->o_opx1));
}

static
fmt2TransRecord(rec, newPlace)
struct opcodeTableStruct *rec;
struct fmt2SwitchTableStruct *newPlace;
{
	newPlace->index = rec->o_switchIndex;
}

/*	FORMAT 3 INSTRUCTIONS	*/

/*
	The table built using these procedures is to be indexed with 
	op  <concat> op3 of format 3 instructions during decoding for
	dis-assembly or during execution.
	The table is an array which contains switch()
	statement indices.
*/

struct fmt3SwitchTableStruct *fmt3SwitchTable;

static
makeFmt3Table()
{
	fmt3SwitchTable = (struct fmt3SwitchTableStruct *)
		makeTable(opcodeTable, sizeof(opcodeTable[0]),
		fmt3CheckRecord,
		fmt3ExtractKey,
		fmt3TransRecord,
		sizeof(fmt3SwitchTable[0]),
		pow2(IFW_OP + IFW_OP3));
}

static
printFmt3Table()
{
	int i;

	printf("Format 3 Instruction Switch Table\n");
	printf("OP	OP3		INDEX\n");

	for(i = 0; i < pow2(IFW_OP + IFW_OP3); ++i)
		printf("0x%x	0x%x		%d\n",
			i>>IFW_OP3, i & (pow2(IFW_OP3)-1),
			(fmt3SwitchTable + i)->index);
}

static
fmt3CheckRecord(rec)
struct opcodeTableStruct *rec;
{
	if(rec->o_type == NONE)return(LAST_RECORD);
	if(rec->o_switchIndex != NONE && rec->o_type == OT_F3)
		return(INTERESTING_RECORD);
	return(BORING_RECORD);
}

static
fmt3ExtractKey(rec)
struct opcodeTableStruct *rec;
{
	return((int)(rec->o_opx1));	/* op <concat> op3 */
}

static
fmt3TransRecord(rec, newPlace)
struct opcodeTableStruct *rec;
struct fmt3SwitchTableStruct *newPlace;
{
	newPlace->index = rec->o_switchIndex;
}

/*	FPOP INSTRUCTIONS	*/

/*
	The table built using these procedures is to be indexed with 
	the opf field of format 3 instructions during decoding for
	dis-assembly or during execution.
	The table is simply an array which contains switch()
	statement indices.
*/

struct fpopSwitchTableStruct *fpopSwitchTable;

static
makeFpopTable()
{
	fpopSwitchTable = (struct fpopSwitchTableStruct *)
		makeTable(opcodeTable, sizeof(opcodeTable[0]),
		fpopCheckRecord,
		fpopExtractKey,
		fpopTransRecord,
		sizeof(fpopSwitchTable[0]),
		pow2(IFW_OPF));
}

static
printFpopTable()
{
	int i;

	printf("FPOP Instruction Switch Table\n");
	printf("OPF		INDEX\n");

	for(i = 0; i < pow2(IFW_OPF); ++i)
		printf("0x%x (%d)		%d\n",
			i, i, (fpopSwitchTable + i)->index);
}

static
fpopCheckRecord(rec)
struct opcodeTableStruct *rec;
{
	if(rec->o_type == NONE)return(LAST_RECORD);
	if(rec->o_switchIndex != NONE && rec->o_type == OT_FOP)
		return(INTERESTING_RECORD);
	return(BORING_RECORD);
}

static
fpopExtractKey(rec)
struct opcodeTableStruct *rec;
{
	return((int)(rec->o_opx2));	/* opf */
}

static
fpopTransRecord(rec, newPlace)
struct opcodeTableStruct *rec;
struct fpopSwitchTableStruct *newPlace;
{
	newPlace->index = rec->o_switchIndex;
}

/*	INTEGER CONDITION CODE TABLE	*/

/*
	The table built using these procedures is to be indexed with 
	the cond field of bicc and ticc instructions during decoding for
	dis-assembly or during execution.
	The table is simply an array which contains switch()
	statement indices.
*/

struct iccSwitchTableStruct *iccSwitchTable;

static
makeIccTable()
{
	iccSwitchTable = (struct iccSwitchTableStruct *)
		makeTable(opcodeTable, sizeof(opcodeTable[0]),
		iccCheckRecord,
		iccExtractKey,
		iccTransRecord,
		sizeof(iccSwitchTable[0]),
		pow2(IFW_COND));
}

static
printIccTable()
{
	int i;

	printf("ICC Switch Table\n");
	printf("I COND		INDEX\n");

	for(i = 0; i < pow2(IFW_COND); ++i)
		printf("0x%x (%d)		%d\n",
			i, i, (iccSwitchTable + i)->index);
}

static
iccCheckRecord(rec)
struct opcodeTableStruct *rec;
{
	if(rec->o_type == NONE)return(LAST_RECORD);
	if(rec->o_switchIndex != NONE && rec->o_type == OT_IC)
		return(INTERESTING_RECORD);
	return(BORING_RECORD);
}

static
iccExtractKey(rec)
struct opcodeTableStruct *rec;
{
	return((int)(rec->o_op));	/* cond */
}

static
iccTransRecord(rec, newPlace)
struct opcodeTableStruct *rec;
struct iccSwitchTableStruct *newPlace;
{
	newPlace->index = rec->o_switchIndex;
}

/*	FLOAT CONDITION CODE TABLE	*/

/*
	The table built using these procedures is to be indexed with 
	the cond field of bfcc instructions during decoding for
	dis-assembly or during execution.
	The table is simply an array which contains switch()
	statement indices.
*/

struct fccSwitchTableStruct *fccSwitchTable;

static
makeFccTable()
{
	fccSwitchTable = (struct fccSwitchTableStruct *)
		makeTable(opcodeTable, sizeof(opcodeTable[0]),
		fccCheckRecord,
		fccExtractKey,
		fccTransRecord,
		sizeof(fccSwitchTable[0]),
		pow2(IFW_COND));
}

static
printFccTable()
{
	int i;

	printf("FCC Switch Table\n");
	printf("F COND		INDEX\n");

	for(i = 0; i < pow2(IFW_COND); ++i)
		printf("0x%x (%d)		%d\n",
			i, i, (fccSwitchTable + i)->index);
}

static
fccCheckRecord(rec)
struct opcodeTableStruct *rec;
{
	if(rec->o_type == NONE)return(LAST_RECORD);
	if(rec->o_switchIndex != NONE && rec->o_type == OT_FC)
		return(INTERESTING_RECORD);
	return(BORING_RECORD);
}

static
fccExtractKey(rec)
struct opcodeTableStruct *rec;
{
	return((int)(rec->o_op));	/* cond */
}

static
fccTransRecord(rec, newPlace)
struct opcodeTableStruct *rec;
struct fccSwitchTableStruct *newPlace;
{
	newPlace->index = rec->o_switchIndex;
}



/* MNEMONIC LOOKUP TABLES FOR THE DIS-ASSEMBLER */

/*
	format 1 mnemonic table
*/

struct fmt1MnemonicTableStruct *fmt1MnemonicTable;

static
makeFmt1MnemonicTable()
{
	fmt1MnemonicTable = (struct fmt1MnemonicTableStruct *)
		makeTable(opcodeTable, sizeof(opcodeTable[0]),
		fmt1MnCheckRecord,
		fmt1MnExtractKey,
		fmt1MnTransRecord,
		sizeof(fmt1MnemonicTable[0]),
		pow2(IFW_OP));
}

static
printFmt1MnemonicTable()
{
	int i;

	printf("Format 1 Mnemonic Switch Table\n");
	printf("OP		MNEMONIC\n");

	for(i = 0; i < pow2(IFW_OP); ++i)
		printf("0x%x (%d)		%s\n",
			i, i, (fmt1MnemonicTable + i)->mnemonic);
}

static
fmt1MnCheckRecord(rec)
struct opcodeTableStruct *rec;
{
	if(rec->o_type == NONE)return(LAST_RECORD);
	if(rec->o_mnemonic != NONE && rec->o_type == OT_F1)
		return(INTERESTING_RECORD);
	return(BORING_RECORD);
}

static
fmt1MnExtractKey(rec)
struct opcodeTableStruct *rec;
{
	return((int)(rec->o_op));
}

static
fmt1MnTransRecord(rec, newPlace)
struct opcodeTableStruct *rec;
struct fmt1MnemonicTableStruct *newPlace;
{
	newPlace->mnemonic = rec->o_mnemonic;
}


/*
	format 2 mnemonic table
*/

struct fmt2MnemonicTableStruct *fmt2MnemonicTable;

static
makeFmt2MnemonicTable()
{
	fmt2MnemonicTable = (struct fmt2MnemonicTableStruct *)
		makeTable(opcodeTable, sizeof(opcodeTable[0]),
		fmt2MnCheckRecord,
		fmt2MnExtractKey,
		fmt2MnTransRecord,
		sizeof(fmt2MnemonicTable[0]),
		pow2(IFW_OP2));
}

static
printFmt2MnemonicTable()
{
	int i;

	printf("Format 2 Mnemonic Switch Table\n");
	printf("OP2		MNEMONIC\n");

	for(i = 0; i < pow2(IFW_OP2); ++i)
		printf("0x%x (%d)		%s\n",
			i, i, (fmt2MnemonicTable + i)->mnemonic);
}

static
fmt2MnCheckRecord(rec)
struct opcodeTableStruct *rec;
{
	if(rec->o_type == NONE)return(LAST_RECORD);
	if(rec->o_mnemonic != NONE && rec->o_type == OT_F2)
		return(INTERESTING_RECORD);
	return(BORING_RECORD);
}

static
fmt2MnExtractKey(rec)
struct opcodeTableStruct *rec;
{
	return((int)(rec->o_opx1));
}

static
fmt2MnTransRecord(rec, newPlace)
struct opcodeTableStruct *rec;
struct fmt2MnemonicTableStruct *newPlace;
{
	newPlace->mnemonic = rec->o_mnemonic;
}


struct fmt3MnemonicTableStruct *fmt3MnemonicTable;

static
makeFmt3MnemonicTable()
{
	fmt3MnemonicTable = (struct fmt3MnemonicTableStruct *)
		makeTable(opcodeTable, sizeof(opcodeTable[0]),
		fmt3MnCheckRecord,
		fmt3MnExtractKey,
		fmt3MnTransRecord,
		sizeof(fmt3MnemonicTable[0]),
		pow2(IFW_OP + IFW_OP3));
}

static
printFmt3MnemonicTable()
{
	int i;

	printf("Format 3 Mnemonic Table\n");
	printf("OP		MNEMONIC\n");

	for(i = 0; i < pow2(IFW_OP + IFW_OP3); ++i)
		printf("0x%x	0x%x		%s\n",
			i>>IFW_OP3, i & (pow2(IFW_OP3)-1),
			(fmt3MnemonicTable + i)->mnemonic);
}

static
fmt3MnCheckRecord(rec)
struct opcodeTableStruct *rec;
{
	if(rec->o_type == NONE)return(LAST_RECORD);
	if(rec->o_mnemonic != NONE && rec->o_type == OT_F3)
		return(INTERESTING_RECORD);
	return(BORING_RECORD);
}

static
fmt3MnExtractKey(rec)
struct opcodeTableStruct *rec;
{
	return((int)(rec->o_opx1));
}

static
fmt3MnTransRecord(rec, newPlace)
struct opcodeTableStruct *rec;
struct fmt3MnemonicTableStruct *newPlace;
{
	newPlace->mnemonic = rec->o_mnemonic;
}


struct biccMnemonicTableStruct *biccMnemonicTable;

static
makeBiccMnemonicTable()
{
	biccMnemonicTable = (struct biccMnemonicTableStruct *)
		makeTable(opcodeTable, sizeof(opcodeTable[0]),
		biccMnCheckRecord,
		biccMnExtractKey,
		biccMnTransRecord,
		sizeof(biccMnemonicTable[0]),
		pow2(IFW_COND));
}

static
printBiccMnemonicTable()
{
	int i;

	printf("Bicc Mnemonic Table\n");
	printf("COND		MNEMONIC\n");

	for(i = 0; i < pow2(IFW_COND); ++i)
		printf("0x%x		%s\n",
			i, (biccMnemonicTable + i)->mnemonic);
}

static
biccMnCheckRecord(rec)
struct opcodeTableStruct *rec;
{
	if(rec->o_type == NONE)return(LAST_RECORD);
	if(rec->o_mnemonic != NONE && rec->o_switchIndex == BICC &&
		rec->o_type == OT_F2)
		return(INTERESTING_RECORD);
	return(BORING_RECORD);
}

static
biccMnExtractKey(rec)
struct opcodeTableStruct *rec;
{
	return((int)(rec->o_opx2));
}

static
biccMnTransRecord(rec, newPlace)
struct opcodeTableStruct *rec;
struct biccMnemonicTableStruct *newPlace;
{
	newPlace->mnemonic = rec->o_mnemonic;
}


struct bfccMnemonicTableStruct *bfccMnemonicTable;

static
makeBfccMnemonicTable()
{
	bfccMnemonicTable = (struct bfccMnemonicTableStruct *)
		makeTable(opcodeTable, sizeof(opcodeTable[0]),
		bfccMnCheckRecord,
		bfccMnExtractKey,
		bfccMnTransRecord,
		sizeof(bfccMnemonicTable[0]),
		pow2(IFW_COND));
}

static
printBfccMnemonicTable()
{
	int i;

	printf("Bfcc Mnemonic Table\n");
	printf("COND		MNEMONIC\n");

	for(i = 0; i < pow2(IFW_COND); ++i)
		printf("0x%x		%s\n",
			i, (bfccMnemonicTable + i)->mnemonic);
}

static
bfccMnCheckRecord(rec)
struct opcodeTableStruct *rec;
{
	if(rec->o_type == NONE)return(LAST_RECORD);
	if(rec->o_mnemonic != NONE && rec->o_switchIndex == FBFCC &&
		rec->o_type == OT_F2)
		return(INTERESTING_RECORD);
	return(BORING_RECORD);
}

static
bfccMnExtractKey(rec)
struct opcodeTableStruct *rec;
{
	return((int)(rec->o_opx2));
}

static
bfccMnTransRecord(rec, newPlace)
struct opcodeTableStruct *rec;
struct bfccMnemonicTableStruct *newPlace;
{
	newPlace->mnemonic = rec->o_mnemonic;
}


struct ticcMnemonicTableStruct *ticcMnemonicTable;

static
makeTiccMnemonicTable()
{
	ticcMnemonicTable = (struct ticcMnemonicTableStruct *)
		makeTable(opcodeTable, sizeof(opcodeTable[0]),
		ticcMnCheckRecord,
		ticcMnExtractKey,
		ticcMnTransRecord,
		sizeof(ticcMnemonicTable[0]),
		pow2(IFW_COND));
}

static
printTiccMnemonicTable()
{
	int i;

	printf("Ticc Mnemonic Table\n");
	printf("COND		MNEMONIC\n");

	for(i = 0; i < pow2(IFW_COND); ++i)
		printf("0x%x		%s\n",
			i, (ticcMnemonicTable + i)->mnemonic);
}

static
ticcMnCheckRecord(rec)
struct opcodeTableStruct *rec;
{
	if(rec->o_type == NONE)return(LAST_RECORD);
	if(rec->o_mnemonic != NONE && rec->o_switchIndex == TICC &&
		rec->o_type == OT_F3)
		return(INTERESTING_RECORD);
	return(BORING_RECORD);
}

static
ticcMnExtractKey(rec)
struct opcodeTableStruct *rec;
{
	return((int)(rec->o_opx2));
}

static
ticcMnTransRecord(rec, newPlace)
struct opcodeTableStruct *rec;
struct ticcMnemonicTableStruct *newPlace;
{
	newPlace->mnemonic = rec->o_mnemonic;
}


struct fpopMnemonicTableStruct *fpopMnemonicTable;

static
makeFpopMnemonicTable()
{
	fpopMnemonicTable = (struct fpopMnemonicTableStruct *)
		makeTable(opcodeTable, sizeof(opcodeTable[0]),
		fpopMnCheckRecord,
		fpopMnExtractKey,
		fpopMnTransRecord,
		sizeof(fpopMnemonicTable[0]),
		pow2(IFW_OPF));
}

static
printFpopMnemonicTable()
{
	int i;

	printf("Fpop Mnemonic Table\n");
	printf("OPF		MNEMONIC\n");

	for(i = 0; i < pow2(IFW_OPF); ++i)
		printf("0x%x		%s\n",
			i, (fpopMnemonicTable + i)->mnemonic);
}

static
fpopMnCheckRecord(rec)
struct opcodeTableStruct *rec;
{
	if(rec->o_type == NONE)return(LAST_RECORD);
	if(rec->o_mnemonic != NONE && rec->o_type == OT_FOP)
		return(INTERESTING_RECORD);
	return(BORING_RECORD);
}

static
fpopMnExtractKey(rec)
struct opcodeTableStruct *rec;
{
	return((int)(rec->o_opx2));
}

static
fpopMnTransRecord(rec, newPlace)
struct opcodeTableStruct *rec;
struct fpopMnemonicTableStruct *newPlace;
{
	newPlace->mnemonic = rec->o_mnemonic;
}


/*			PSEUDO INSTRUCTIONS MNEMONIC TABLE
	This table is different because it is indexed with numbers from
	the switchIndex column of the opcode table. Only the OT_PSEUDO
	type entries in the table are put into this index table.
	This table is used by the dis-assembler to find mnemonics for
	pseudo instructions when it recognizes them.
*/


struct pseudoMnemonicTableStruct *pseudoMnemonicTable;

static
makePseudoMnemonicTable()
{
	int size = 0;
	struct opcodeTableStruct *rec;

	/*
		have to find the range of this table by scanning the
		opcodeTable for the largest index (largest switchIndex).
	*/
	rec = opcodeTable;
	while(rec->o_type != NONE){
		if(rec->o_type == OT_PSEUDO){
			if(rec->o_switchIndex > size)
				size = rec->o_switchIndex;
		}
		++rec;
	}
	++size;

	pseudoMnemonicTable = (struct pseudoMnemonicTableStruct *)
		makeTable(opcodeTable, sizeof(opcodeTable[0]),
		pseudoMnCheckRecord,
		pseudoMnExtractKey,
		pseudoMnTransRecord,
		sizeof(pseudoMnemonicTable[0]),
		size);
}

static
printPseudoMnemonicTable()
{
	int i;
	int size = 0;
	struct opcodeTableStruct *rec;

	/*
		have to find the range of this table by scanning the
		opcodeTable for the largest index (largest parseToken).
	*/
	rec = opcodeTable;
	while(rec->o_type != NONE){
		if(rec->o_type == OT_PSEUDO){
			if(rec->o_switchIndex > size)
				size = rec->o_switchIndex;
		}
		++rec;
	}
	++size;

	printf("Pseudo Mnemonic Table\n");
	printf("SWITCH-INDEX		MNEMONIC\n");

	for(i = 0; i < size; ++i)
		printf("0x%x		%s\n",
			i, (pseudoMnemonicTable + i)->mnemonic);
}

static
pseudoMnCheckRecord(rec)
struct opcodeTableStruct *rec;
{
	if(rec->o_type == NONE)return(LAST_RECORD);
	if(rec->o_mnemonic != NONE && rec->o_type == OT_PSEUDO)
		return(INTERESTING_RECORD);
	return(BORING_RECORD);
}

static
pseudoMnExtractKey(rec)
struct opcodeTableStruct *rec;
{
	return((int)(rec->o_switchIndex));
}

static
pseudoMnTransRecord(rec, newPlace)
struct opcodeTableStruct *rec;
struct pseudoMnemonicTableStruct *newPlace;
{
	newPlace->mnemonic = rec->o_mnemonic;
}



/*
	The following procedure makeTable() is intended to be a general
	purpose routine to build index tables from a data table.
	It is passed procedures which select interesting records from the
	data table, choose an index key out of a record, and finally
	copy the needed parts of the record into the indexed location of
	a new table. 
*/

static char *
makeTable(table, recordSize,
	checkRecord, extractKey, transRecord,
	newRecordSize, newTableSize)
char *table;	/* pointer to data table */
int recordSize;	/* size of records in the data table (in bytes) */
int (*checkRecord)();	/* procedure to check a record for interestingness */
unsigned int (*extractKey)();	/* procedure to extract a key from a record */
int (*transRecord)();	/* procedure to translate a record */
int newRecordSize;	/* size of a record in the new table */
int newTableSize;	/* number of records in the new table */
/*
	This procedure selects the interesting records from a data table
	(as determined by checkRecord()), and converts these records
	into a new table which is indexed by a key in the old table
	which is selected by extractKey().
	----
	The returned value points to the new table.
*/
{
	char *record;
	char *newTable;
	int recordFlag, newSize;
	unsigned int key, max, min;

	/* allocate memory for new table */

	newSize = newTableSize * newRecordSize;
	newTable = malloc(newSize);
	if(newTable == NULL) {
		fprintf( stderr, "ran out of memory for instruction table.\n");
		exit(1);
	}

	/* zero the new table */
	bzero(newTable, newSize);

	/* build new table */

	for ( record = table;
	      ((recordFlag = (*checkRecord)(record)) != LAST_RECORD);
	      record += recordSize
	    )
	{
		if(recordFlag == INTERESTING_RECORD){

			key = (*extractKey)(record);
			(*transRecord)(record, newTable + key*newRecordSize);
		}
	}

	return(newTable);
}

#ifdef debug_tables
/* Temporary! -- print the tables */
printDisAsmTables () {
	printFormatTable();
	printFmt2Table();
	printFmt3Table();
	printFpopTable();
	printIccTable();
	printFccTable();
	printFmt1MnemonicTable();
	printFmt2MnemonicTable();
	printFmt3MnemonicTable();
	printBiccMnemonicTable();
	printBfccMnemonicTable();
	printTiccMnemonicTable();
	printFpopMnemonicTable();
	printPseudoMnemonicTable();
}
#endif debug_tables

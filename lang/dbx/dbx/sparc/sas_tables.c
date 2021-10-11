#ifndef lint
static	char sccsid[] = "@(#)sas_tables.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * This file is from Will Brown's "sas" Sparc Architectural Simulator.
 * It is the concatenation of indextables.c and opcodetable.c and dribs
 * and drabs from elsewhere.
 */

#include <sys/types.h>
#include <stdio.h>
#include "sas_includes.h"

/*
 **** **** **** **** **** **** **** **** **** **** **** **** **** ****
 * indextables.c -- (from 3.1	11/11/85)
 * This file contains the code which builds the tables for use
 * by the instruction disassembler.
 **** **** **** **** **** **** **** **** **** **** **** **** **** ****
 */
	
/* taken from in-line code in "sas" main.c */
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
	if(rec->o_mnemonic != NONE && rec->o_switchIndex == BFCC &&
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




/*
 **** **** **** **** **** **** **** **** **** **** **** **** **** ****
 * opcodetable.c -- (from 3.3	2/4/86)
 *	This file contains the static initialization for the opcode
 *	table which is common to sas and sasm, and adb-for-sparc.
 *
 *	This file also contains the "makeBkptInstr" routine.
 **** **** **** **** **** **** **** **** **** **** **** **** **** ****
 */

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
BICC,		OT_F2,	0,  SR_BICC_OP, 0x1,	"be",
BICC,		OT_F2,	0,  SR_BICC_OP, 0xa,	"bg",
BICC,		OT_F2,	0,  SR_BICC_OP, 0x2,	"ble",
BICC,		OT_F2,	0,  SR_BICC_OP, 0xb,	"bge",
BICC,		OT_F2,	0,  SR_BICC_OP, 0x3,	"bl",
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

BFCC,		OT_F2,	0,  SR_FBCC_OP, 0x0,	"fbn",
BFCC,		OT_F2,	0,  SR_FBCC_OP, 0x7,	"fbu",
BFCC,		OT_F2,	0,  SR_FBCC_OP, 0x6,	"fbg",
BFCC,		OT_F2,	0,  SR_FBCC_OP, 0x5,	"fbug",
BFCC,		OT_F2,	0,  SR_FBCC_OP, 0x4,	"fbl",
BFCC,		OT_F2,	0,  SR_FBCC_OP, 0x3,	"fbul",
BFCC,		OT_F2,	0,  SR_FBCC_OP, 0x2,	"fblg",
BFCC,		OT_F2,	0,  SR_FBCC_OP, 0x1,	"fbne",
BFCC,		OT_F2,	0,  SR_FBCC_OP, 0x9,	"fbe",
BFCC,		OT_F2,	0,  SR_FBCC_OP, 0xa,	"fbue",
BFCC,		OT_F2,	0,  SR_FBCC_OP, 0xb,	"fbge",
BFCC,		OT_F2,	0,  SR_FBCC_OP, 0xc,	"fbuge",
BFCC,		OT_F2,	0,  SR_FBCC_OP, 0xd,	"fble",
BFCC,		OT_F2,	0,  SR_FBCC_OP, 0xe,	"fbule",
BFCC,		OT_F2,	0,  SR_FBCC_OP, 0xf,	"fbo",
BFCC,		OT_F2,	0,  SR_FBCC_OP, 0x8,	"fba",

/*	Load Instructions */
/*	Some of the mnemonics changed between FAB1 and FAB2, */
/*	so they're symbolic here, defined in sr_instruction.h. */

LDSB,		OT_F3,	3,	3<<IFW_OP3 | 0x09,	NONE,	"ldsb",
LDSH,		OT_F3,	3,	3<<IFW_OP3 | 0x0a,	NONE,	"ldsh",
LDUB,		OT_F3,	3,	3<<IFW_OP3 | 0x01,	NONE,	"ldub",
LDUH,		OT_F3,	3,	3<<IFW_OP3 | 0x02,	NONE,	"lduh",
LD,		OT_F3,	3,	3<<IFW_OP3 | 0x00,	NONE,	"ld",
LDF,		OT_F3,	3,	3<<IFW_OP3 | 0x20,	NONE,	"ld",
LDFD,		OT_F3,	3,	3<<IFW_OP3 | 0x23,	NONE,	"ldd",
LDDC,		OT_F3,	3,	3<<IFW_OP3 | 0x33,	NONE,	"lddc",
LASB,		OT_F3,	3,	3<<IFW_OP3 | 0x19,	NONE,	"ldsba",
LASH,		OT_F3,	3,	3<<IFW_OP3 | 0x1a,	NONE,	"ldsha",
LAUB,		OT_F3,	3,	3<<IFW_OP3 | 0x11,	NONE,	"lduba",
LAUH,		OT_F3,	3,	3<<IFW_OP3 | 0x12,	NONE,	"lduha",
LDA,		OT_F3,	3,	3<<IFW_OP3 | 0x10,	NONE,	"lda",
LDC,		OT_F3,	3,	3<<IFW_OP3 | 0x30,	NONE,	"ldc",

LDFSR,		OT_F3,	3,	3<<IFW_OP3 | 0x21,	NONE,	"ld",
LDCSR,		OT_F3,	3,	3<<IFW_OP3 | 0x31,	NONE,	"ldcsr",

/*	Store Instructions */

ST,		OT_F3,	3,	3<<IFW_OP3 | 0x04,	NONE,	"st",
ST,             OT_F3,  3,      3<<IFW_OP3 | 0x07,      NONE,   "std",
STB,		OT_F3,	3,	3<<IFW_OP3 | 0x05,	NONE,	"stb",
STH,		OT_F3,	3,	3<<IFW_OP3 | 0x06,	NONE,	"sth",
STF,		OT_F3,	3,	3<<IFW_OP3 | 0x24,	NONE,	"st",
STFD,		OT_F3,	3,	3<<IFW_OP3 | 0x27,	NONE,	"std",
STDC,		OT_F3,	3,	3<<IFW_OP3 | 0x37,	NONE,	"stdc",
SAB,		OT_F3,	3,	3<<IFW_OP3 | 0x15,	NONE,	"stba",
SAH,		OT_F3,	3,	3<<IFW_OP3 | 0x16,	NONE,	"stha",
STA,		OT_F3,	3,	3<<IFW_OP3 | 0x14,	NONE,	"sta",
STC,		OT_F3,	3,	3<<IFW_OP3 | 0x34,	NONE,	"stc",

STFSR,		OT_F3,	3,	3<<IFW_OP3 | 0x25,	NONE,	"st",
STCSR,		OT_F3,	3,	3<<IFW_OP3 | 0x35,	NONE,	"stcsr",

STFQ,		OT_F3,	3,	3<<IFW_OP3 | 0x26,	NONE,	"std",
STDCQ,		OT_F3,	3,	3<<IFW_OP3 | 0x36,	NONE,	"stdcq",

/*	Atomic Load-Store Instructions */

ALS,		OT_F3,	3,	3<<IFW_OP3 | 0x0c,	NONE,	"ldst",
AALS,		OT_F3,	3,	3<<IFW_OP3 | 0x1c,	NONE,	"ldsta",

/*	Arithmetic Instructions */

ADD,		OT_F3,	2,	2<<IFW_OP3 | 0x00,	NONE,	"add",
ADDcc,		OT_F3,	2,	2<<IFW_OP3 | 0x10,	NONE,	"addcc",
ADDX,		OT_F3,	2,	2<<IFW_OP3 | 0x08,	NONE,	"addx",
ADDXcc,		OT_F3,	2,	2<<IFW_OP3 | 0x18,	NONE,	"addxcc",
TADDcc,		OT_F3,  2,      2<<IFW_OP3 | 0x20,      NONE,   "taddcc",
TADDccTV,	OT_F3,  2,      2<<IFW_OP3 | 0x22,      NONE,   "taddcctv",
SUB,		OT_F3,	2,	2<<IFW_OP3 | 0x04,	NONE,	"sub",
SUBcc,		OT_F3,	2,	2<<IFW_OP3 | 0x14,	NONE,	"subcc",
SUBX,		OT_F3,	2,	2<<IFW_OP3 | 0x0c,	NONE,	"subx",
SUBXcc,		OT_F3,	2,	2<<IFW_OP3 | 0x1c,	NONE,	"subxcc",
TSUBcc,		OT_F3,	2,	2<<IFW_OP3 | 0x21,	NONE,	"tsubcc",
TSUBccTV,	OT_F3,	2,	2<<IFW_OP3 | 0x23,	NONE,	"tsubcctv",
MULScc,		OT_F3,	2,	2<<IFW_OP3 | 0x24,	NONE,	"mulscc",

/*	Logical Instructions */

AND,		OT_F3,	2,	2<<IFW_OP3 | 0x01,	NONE,	"and",
ANDN,		OT_F3,	2,	2<<IFW_OP3 | 0x05,	NONE,	"andn",
OR,		OT_F3,	2,	2<<IFW_OP3 | 0x02,	NONE,	"or",
ORN,		OT_F3,	2,	2<<IFW_OP3 | 0x06,	NONE,	"orn",
XOR,		OT_F3,	2,	2<<IFW_OP3 | 0x03,	NONE,	"xor",
XORN,		OT_F3,	2,	2<<IFW_OP3 | 0x07,	NONE,	"xnor",
ANDcc,		OT_F3,	2,	2<<IFW_OP3 | 0x11,	NONE,	"andcc",
ANDNcc,		OT_F3,	2,	2<<IFW_OP3 | 0x15,	NONE,	"andncc",
ORcc,		OT_F3,	2,	2<<IFW_OP3 | 0x12,	NONE,	"orcc",
ORNcc,		OT_F3,	2,	2<<IFW_OP3 | 0x16,	NONE,	"orncc",
XORcc,		OT_F3,	2,	2<<IFW_OP3 | 0x13,	NONE,	"xorcc",
XORNcc,		OT_F3,	2,	2<<IFW_OP3 | 0x17,	NONE,	"xnorcc",

/*	Shift Instructions */

SLL,		OT_F3,	2,	2<<IFW_OP3 | 0x25,	NONE,	"sll",
SRL,		OT_F3,	2,	2<<IFW_OP3 | 0x26,	NONE,	"srl",
SRA,		OT_F3,	2,	2<<IFW_OP3 | 0x27,	NONE,	"sra",

/*	Sethi Instruction */
SETHI,		OT_F2,	0,	SR_SETHI_OP,		NONE,	"sethi",

/*	The "unimplemented" Instruction -- used by cc to return structs */
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
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0x1,	"te",
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0xa,	"tg",
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0x2,	"tle",
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0xb,	"tge",
TICC,		OT_F3,	2,	2<<IFW_OP3 | 0x3a,	0x3,	"tl",
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

FCMPs,		OT_FOP,	2,	0x35,	0x51,	"fcmps",
FCMPd,		OT_FOP,	2,	0x35,	0x52,	"fcmpd",
FCMPe,		OT_FOP,	2,	0x35,	0x53,	"fcmpe",

FCMPXs,		OT_FOP,	2,	0x35,	0x55,	"fcmpes",
FCMPXd,		OT_FOP,	2,	0x35,	0x56,	"fcmped",
FCMPXe,		OT_FOP,	2,	0x35,	0x57,	"fcmpex",

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

#ifndef lint
static  char sccsid[] = "@(#)disasm.c 1.1 94/10/31 Copyr 1986 Sun Micro";
/*
 * From Will Brown's "sas" Sparc Architectural Simulator,
 * Version "@:-)disassembler.c	3.1\t11/11/85"
 */
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <strings.h>
#include "sas_includes.h"

/*
	External variables are used to hold all of the possible fields
	that might be printed out for a dis-assembled instruction.

	Most of these are created prior to decoding the instruction
	for dis-assembly. After decode, disassembly is a matter
	of printing out the right combination of fields.
*/

static char *mnemonic;	/* pointer to the mnemonic string */
static int const223;	/* 22 (fab2) or 23 (fab1) bit constant,
				extracted from format 2 */
static char target[64];	/* symbolic target of call, or branch,
					created after decode */

static char rd[6];	/* destination r register string ( eg: "%l5") */
static char fd[6];	/* destination f register string ( eg: "%f23") */
static char rs1[6], fs1[6];	/* register source 1 */
static char rs2[6], fs2[6];	/* register source 2 */
static int simm13;	/* signed immediate constant from format 3 */
static int asi;		/* address space identifier */

static char src2[20];	/* either "%rs2" or "simm13" */
static char source[30];	/* "%rs1 + src2" */

static char specialReg[10];	/* fsr, psr, y, tbr, wim:	after decode */


/* toSymbolic is now called ssymoff, and is in machine.c */
extern char *ssymoff( /* addr_t address */ );


/*
	The dis-assembler puts the final result of dis-assembly
	into the following string buffer, without a newline, for
	printing.
*/
static char dResult[128];

/*	Illegal opcodes result in the following dis-assembly string */
static char unknownInstr[] = "???";

/*
	Routines to extract register specifiers
*/

static
char *convertReg(r)
unsigned int r;
{
	char bank;	/* register bank g, o, l, or i */
	static char name[6];

	/* detect %sp and %fp */
	if(r == 14){
		strcpy(name, "%sp");
		return(name);
	}
	if(r == 30){
		strcpy(name, "%fp");
		return(name);
	}

	if(r < R_NGLOBALS)
		bank = 'g'; 
	else if(r < (R_NGLOBALS + R_NOVERLAP)){
		bank = 'o';
		r -= R_NGLOBALS;
	}
	else if(r < (R_NGLOBALS + R_NOVERLAP + R_NLOCALS)){
		bank = 'l';
		r -= R_NGLOBALS + R_NOVERLAP;
	}
	else{
		bank = 'i';
		r -= R_NGLOBALS + R_NOVERLAP + R_NLOCALS;
	}

	sprintf(name, "%%%c%d", bank, r);
	return(name);
}

static
char *convertFreg(r)
unsigned int r;
{
	static char name[6];

	sprintf(name, "%%f%d", r);
	return(name);
}

static
extractRd(instruction)
unsigned int instruction;
{
	strcpy(rd, convertReg(RD));
}

static
extractRs1(instruction)
unsigned int instruction;
{
	strcpy(rs1, convertReg(RS1));
}

static
extractRs2(instruction)
unsigned int instruction;
{
	strcpy(rs2, convertReg(RS2));
}

static
extractFd(instruction)
unsigned int instruction;
{
	strcpy(fd, convertFreg(RD));
}

static
extractFs1(instruction)
unsigned int instruction;
{
	strcpy(fs1, convertFreg(RS1));
}

static
extractFs2(instruction)
unsigned int instruction;
{
	strcpy(fs2, convertFreg(RS2));
}


/*
	There are about 24 "templates' for dis-assembly, and these
	are defined as procedures below.
*/

static
loadTemplate()
/*
	ldsb, ldsh, ldub, lduh, ld, ldst
*/
{
	sprintf(dResult, "%-7s	[%s], %s", mnemonic, source, rd);
}

static
loadATemplate()
/*
	ldsba, ldsha, lduba, lduha, lda, ldsta
*/
{
	sprintf(dResult, "%-7s	[%s] 0x%x, %s", mnemonic, source, asi, rd);
}

static
loadFTemplate()
/*
	ldf
*/
{
	sprintf(dResult, "%-7s	[%s], %s", mnemonic, source, fd);
}

static
loadCPTemplate()
/*
	ldc, lddc
*/
{
	sprintf(dResult, "%-7s	[%s] 0x%x, %s", mnemonic, source, asi, fd);
}
	
static
ldfsrTemplate()
/*
	ldfsr
*/
{
	sprintf(dResult, "%-7s	[%s], %%fsr", mnemonic, source);
}

static
ldcsrTemplate()
/*
	ldcsr (fab2)
*/
{
#ifdef FAB1
	sprintf(dResult, "%-7s	[%s] 0x%x, %%fsr", mnemonic, source, asi);
#else !FAB1
	sprintf(dResult, "%-7s	[%s] 0x%x, %%csr", mnemonic, source, asi);
#endif FAB1
}

static
storeTemplate()
/*
	stb, sth, st
*/
{
	sprintf(dResult, "%-7s	%s, [%s]", mnemonic, rd, source);
}

static
storeATemplate()
/*
	sta, stba, stha,
*/
{
	sprintf(dResult, "%-7s	%s, [%s] 0x%x", mnemonic, rd, source, asi);
}

static
storeCTemplate()
/*
	stc
*/
{
	sprintf(dResult, "%-7s	%s, [%s] 0x%x", mnemonic, rd, source, asi);
}

static
storeFTemplate()
/*
	stf
*/
{
	sprintf(dResult, "%-7s	%s, [%s]", mnemonic, fd, source);
}

static
storeDCTemplate()
/*
	stfa
*/
{
	sprintf(dResult, "%-7s	%s, [%s] 0x%x", mnemonic, fd, source, asi);
}

static
stfsrTemplate()
/*
	stfsr
*/
{
	sprintf(dResult, "%-7s	%%fsr, [%s]", mnemonic, source);
}

static
stcsrTemplate()
/*
	stcsr
*/
{
#ifdef FAB1
	sprintf(dResult, "%-7s	%%fsr, [%s] 0x%x", mnemonic, source, asi);
#else !FAB1
	sprintf(dResult, "%-7s	%%csr, [%s] 0x%x", mnemonic, source, asi);
#endif FAB1
}

static
stfqTemplate()
/*
	stfq
*/
{
	sprintf(dResult, "%-7s	%%fq, [%s]", mnemonic, source);
}

static
stdcqTemplate()
/*
	stdcq
*/
{
	sprintf(dResult, "%-7s	%%fq, [%s] 0x%x", mnemonic, source, asi);
}

static
threeOpTemplate()
/*
	most arithmetic, logical, shift, save, restore
*/
{
	sprintf(dResult, "%-7s	%s, %s, %s", mnemonic, rs1, src2, rd);
}

static
fpTwoOpTemplate()
/*
	fcvt, fint, fmov, fneg, fabs, fcls, fexp, fsqr
*/
{
	sprintf(dResult, "%-7s	%s, %s", mnemonic, fs2, fd);
}

static
fpThreeOpTemplate()
/*
	fadd, fsub, fmul, fdiv, fscl, frem, fquot
*/
{
	sprintf(dResult, "%-7s	%s, %s, %s", mnemonic, fs1, fs2, fd);
}

static
fpCmpTemplate()
/*
	fcmp, fcmpx
*/
{
	sprintf(dResult, "%-7s	%s, %s", mnemonic, fs1, fs2);
}

static
movTemplate()
/*
	integer mov (which is really an or instruction)
*/
{
	sprintf(dResult, "%-7s	%s, %s", mnemonic, src2, rd);
}

static
anotherMovTemplate()
/*
	This one is for the following cases:
		or	%x, 0, %y	! immediate value of 0
		or	%x, %g0, %y	! rs2 is %g0
*/
{
	sprintf(dResult, "%-7s	%s, %s", mnemonic, rs1, rd);
}

static
cmpTemplate()
/*
	integer cmp (which is really a subcc instruction)
*/
{
	sprintf(dResult, "%-7s	%s, %s", mnemonic, rs1, src2);
}

static
tstTemplate()
/*
	integer tst (which is really a subcc instruction)
*/
{
	sprintf(dResult, "%-7s	%s", mnemonic, rs1);
}

static
nopTemplate()
/*
	"ret" and "nop" ("nop" can be an "or", or a sethi %hi(0), %g0)
*/
{
	sprintf(dResult, "%-7s", mnemonic);
}

static
unimplTemplate()
/*
	"unimpl" (the explicitly unimplemented instruction) is currently
	used by our compiler upon return from a structure-valued function.
*/
{
	sprintf(dResult, "%-7s	%s", mnemonic, target);
}

static
sethiTemplate()
/*
	sethi
*/
{
	sprintf(dResult, "%-7s	%%hi(0x%x), %s", mnemonic,
			SR_HI( const223 ), rd);
}

static
branchTemplate()
/*
	bicc, bfcc, call
*/
{
	sprintf(dResult, "%-7s	%s", mnemonic, target);
}

static
jumpTemplate()
/*
	jump when r[rd] is meaningful (i.e., not %g0)
*/
{
	sprintf(dResult, "%-7s	%s, %s", mnemonic, source, rd );
}

static
retticcTemplate()
/*
	rett, ticc, jump when r[rd] is %g0
*/
{
	sprintf(dResult, "%-7s	%s", mnemonic, source);
}

static
readTemplate()
/*
	rdpsr, rdwim, rdtbr, rdy
*/
{
	sprintf(dResult, "%-7s	%s, %s", mnemonic, specialReg, rd);
}

static
writeTemplate()
/*
	wrpsr, wry, wrwim, wrtbr
*/
{
	sprintf(dResult, "%-7s	%s, %s, %s", mnemonic, rs1, src2, specialReg);
}

static
wrSrc2Template()
/*
	wrpsr, wry
*/
{
	sprintf(dResult, "%-7s	%s, %s", mnemonic, src2, specialReg);
}

static
wrRs1Template()
/*
	wrpsr, wry
*/
{
	sprintf(dResult, "%-7s	%s, %s", mnemonic, rs1, specialReg);
}

static
char *lookupMnemonic(instruction)
Word instruction;
/*
	This procedure looks up the mnemonic for the given
	instruction. It performs a partial decode, and uses
	mnemonic tables. If there is no mnemonic for the
	given instruction, a NULL pointer is returned.
*/
{
	char *opcode;
	short index;

	/* first decode the instruction format */
	switch((formatSwitchTable + OP)->index){

	case OT_F1:
		opcode = (fmt1MnemonicTable + OP)->mnemonic;
		break;

	case OT_F2:
		switch((fmt2SwitchTable + OP2)->index){

		case BICC:
			opcode = (biccMnemonicTable + COND)->mnemonic;
			break;

		case BFCC:
			opcode = (bfccMnemonicTable + COND)->mnemonic;
			break;

		case UNIMP:
			opcode = (fmt2MnemonicTable + OP2)->mnemonic;
			break;

		case SETHI:
			opcode = (fmt2MnemonicTable + OP2)->mnemonic;
			break;
		
		default:
			return(NULL);

		}
		break;

	case OT_F3:
		
		index = (fmt3SwitchTable + ((OP << IFW_OP3) |  OP3))->index;
		if(index == FPOP || index == FPCMP)
			opcode = (fpopMnemonicTable + OPF)->mnemonic;
		else if(index == TICC)
			opcode = (ticcMnemonicTable + COND)->mnemonic;
		else
			opcode = (fmt3MnemonicTable +
				((OP << IFW_OP3) |  OP3))->mnemonic;
		break;

	default:
		return(NULL);
	
	}

	/* copy the opcode string into external variable mnemonic */
	mnemonic = opcode;
	return(opcode);
}

static
extractFields(instruction)
unsigned int instruction;
{
	(void) lookupMnemonic(instruction);
			/* this sets the mnemonic char pointer */

	const223 = DISP223;	/* extraction macros are in sr_instruction.h */

	extractRd(instruction);
	extractRs1(instruction);
	extractRs2(instruction);

	extractFd(instruction);
	extractFs1(instruction);
	extractFs2(instruction);

	simm13 = SIMM13;
	simm13 = SR_SEX13( simm13 );	/* sign extend */

	asi = OPF;

	if(IMM)
		sprintf(src2, "%d", simm13);
	else
		strcpy(src2, rs2);
	
	if((IMM && simm13 == 0) || (!IMM && RS2 == 0) )
		strcpy(source, rs1);
	else if(RS1 == 0)
		strcpy(source, src2);
	else{
		strcpy(source, rs1);
		strcat(source, " + ");
		strcat(source, src2);
	}
}

/*
 * Here, we have some special instruction-recognizing routines,
 * for use when decoding prologs to see if we have leaf routines,
 * and for use by the nextaddr stuff (the "next" instruction).
 */
#define OP_MASK    0xC0000000
#define OP_AND_OP2 0xC1C00000
#define OP_AND_OP3 0xC1F80000
#define RD_MASK    0x3E000000

#define BICC_OP    0x00800000
#define FBCC_OP    0x01800000
#define CALL_OPS   0x40000000
#define JMPL_OPS   0x81C00000
#define SAVE_OPS   0x81600000
#define TRAP_OPS   0x81500000
#define UNIMP_OPS  0x00000000

is_br_instr ( ins ) {
    if(((ins & OP_AND_OP2) == BICC_OP ) ||
      ((ins & OP_AND_OP2) == FBCC_OP ) ) 
	return 1;
    else
	return 0;
}

is_call_instr ( ins ) {
    if( (ins & OP_MASK) == CALL_OPS )
	return 1;
    else
	return 0;
}

/*
 * Note -- this can be a call (jmpl with rd != 0),
 * a return (jmpl with rd == 0, to rs1 == %i7 + imm == 8),
 * or any other simple jump.
 */
is_jmpl_instr ( ins ) {
    if( (ins & OP_AND_OP3) == JMPL_OPS )
	return 1;
    else
	return 0;
}

/*
 * Given a jmpl, is this one that acts like a call?
 * (I.e., is rd != 0?)
 */
is_jmpl_call ( ins ) {
    return ( (ins & RD_MASK) != 0 );
}

is_save_instr ( ins ) {
    if( (ins & OP_AND_OP3) == SAVE_OPS )
	return 1;
    else
	return 0;
}

is_trap_instr ( ins ) {
    if( (ins & OP_AND_OP3) == TRAP_OPS )
	return 1;
    else
	return 0;
}

is_unimp_instr ( ins ) {
    if( (ins & OP_AND_OP2) == UNIMP_OPS )
	return 1;
    else
	return 0;
}

char *
disassemble(instruction, pc)
unsigned int instruction, pc;
/*
	The given instruction is dis-assembled into the static
	character array "dResult" and a pointer to the dis-assembly
	string is returned.

	It is necessary to know the program counter for the instruction
	so that absolute addresses can be computed where PC-relative
	displacements exist.

	Where an immediate address is expected (branch, and call instructions),
	An attempt is made to look up the address in the symbol table 
	so that a symbolic target can be printed.
*/
{
	/* If we have no formatSwitchTable, we gotta make tables */
	if( formatSwitchTable == NULL ) {
		mkDisAsmTables();
		if( formatSwitchTable == NULL ) {
			return "Cannot build instruction disassembly tables.\n";
		}

	}

	/* first extract various parts of the instruction */
	extractFields(instruction);

	/* now decode and dis-assemble the instruction */

	switch((formatSwitchTable + OP)->index){

	case OT_F1:
		disassembleFmt1(instruction, pc);
		break;

	case OT_F2:
		disassembleFmt2(instruction, pc);
		break;

	case OT_F3:
		disassembleFmt3(instruction, pc);
		break;

	default:
		strcpy(dResult, unknownInstr);
		break;

	}
	return(dResult);
}


static
disassembleFmt1(instruction, pc)
unsigned int instruction, pc;
{
	unsigned int absAddr;

		/*
			CALL instruction. Must convert PC-relative constant
			into an absolute address.
		*/

		absAddr = pc + SR_WA2BA(DISP30);

		/* lookup symbolic equivalent */

		strcpy(target, ssymoff(absAddr));
		if(target[0] == '\0')sprintf(target, "0x%x", absAddr);

		/* produce dis-assembled instruction */

		branchTemplate();
}


static
disassembleFmt2(instruction, pc)
unsigned int instruction, pc;
{

	/* decode format 2 instructions */
	switch((fmt2SwitchTable + OP2)->index){

	case BICC:
	case BFCC:
	    { int sex;	/* used for sign extension */
	      unsigned int absAddr;

		/*
			Branch instruction. Must convert PC-relative constant
			into an absolute address.
		*/

		sex = DISP223;
		sex = SR_SEX223( sex );		/* sign extend */
		absAddr = pc + SR_WA2BA(sex);	/* convert to absolute addr */

		/* lookup symbolic equivalent */

		strcpy(target, ssymoff(absAddr));
		if(target[0] == '\0')sprintf(target, "0x%x", absAddr);

		/* produce dis-assembled instruction */
		strcpy(source, mnemonic);	/* use source[] as a temp buf */
		mnemonic = source;
		if(ANNUL)
			strcat(mnemonic, ",a");
		branchTemplate();
	    }
	    break;

	case SETHI:
		if( const223 == 0  &&  strcmp(rd, "%g0")==0 ) {
		    mnemonic = "nop" ;	    /* sethi %hi(0), %g0 */
		    nopTemplate();
		} else {
    		    sethiTemplate();
		}
		break;

	case UNIMP:
	    { int v;

		/*
		 * The constant that our compiler currently puts into
		 * the immediate field (imm22) of the 'unimpl' instruction
		 * is the length of a structure being returned.  Don't want
		 * it printed symbolically.
		 */
		v = DISP223;
		if( v < 10 )	sprintf( target, "%x", v );
		else		sprintf( target, "0x%x", v );
		unimplTemplate();
	    }
	    break;

	default:
		strcpy(dResult, unknownInstr);
		break;

	}
}


static
disassembleFmt3(instruction, pc)
unsigned int instruction, pc;
{
	unsigned int absAddr;

	/* decode format 3 instructions */
	switch((fmt3SwitchTable + ((OP << IFW_OP3) |  OP3))->index){
		
/***************************  LOAD INSTRUCTIONS  ******************************/

	case LDSB:
	case LDSH:
	case LDUB:
	case LDUH:
	case LD:
	case ALS:
		loadTemplate();
		break;

	case LASB:
	case LASH:
	case LAUB:
	case LAUH:
	case LDA:
	case AALS:
		loadATemplate();
		break;
	
	case LDF:
	case LDFD:
		loadFTemplate();
		break;
	
	case LDC:
	case LDDC:
		loadCPTemplate();
		break;
	
	case LDFSR:
		ldfsrTemplate();
		break;
	
	case LDCSR:
		ldcsrTemplate();
		break;

		
/**************************  STORE INSTRUCTIONS  ******************************/

	case STB:
	case STH:
	case ST:
		storeTemplate();
		break;

	case SAB:
	case SAH:
	case STA:
		storeATemplate();
		break;
	
	case STF:
	case STFD:
		storeFTemplate();
		break;

	case STC:
		storeCTemplate();
		break;
	
	case STDC:
		storeDCTemplate();
		break;
	
	case STFSR:
		stfsrTemplate();
		break;
	
	case STCSR:
		stcsrTemplate();
		break;

	case STFQ:
		stfqTemplate();
		break;

	case STDCQ:
		stdcqTemplate();
		break;

/************  ARITMETIC/LOGICAL/SHIFT/SAVE/RESTORE INSTRUCTIONS  *************/

	case ADD:
	case ADDcc:
	case ADDX:
	case ADDXcc:
	case TADDcc:
	case TADDccTV:
	case SUB:
	case SUBX:
	case SUBXcc:
	case TSUBcc:
	case TSUBccTV:
	case MULScc:
	case AND:
	case ANDcc:
	case ANDN:
	case ANDNcc:
	case ORcc:
	case ORN:
	case ORNcc:
	case XOR:
	case XORN:
	case XORcc:
	case XORNcc:
	case SLL:
	case SRL:
	case SRA:
		threeOpTemplate();
		break;

	case SAVE:
	case RESTORE:
		/* detect default register specifiers */
		if(!IMM && RS1 == 0 && RS2 == 0 && RD == 0){
			nopTemplate();
			break;
		}
		threeOpTemplate();
		break;

	case OR:
		/* detect nop */
		if(!IMM && (RS1 == RS2) && (RD == 0 || RD == RS1)){
			mnemonic = (pseudoMnemonicTable + NOP)->mnemonic;
			nopTemplate();
			break;
		}

		/* detect mov */
		if(RS1 == 0){
			mnemonic = (pseudoMnemonicTable + MOV)->mnemonic;
			movTemplate();
			break;
		}
		else if((IMM && SIMM13 == 0) || (!IMM && RS2 == 0)){
			mnemonic = (pseudoMnemonicTable + MOV)->mnemonic;
			anotherMovTemplate();
			break;
		}
		threeOpTemplate();
		break;

	case SUBcc:
		/* detect cmp */
		if(( (IMM && simm13 != 0 ) || (!IMM && RS2 != 0) ) && RD == 0){
			mnemonic = (pseudoMnemonicTable + CMP)->mnemonic;
			cmpTemplate();
			break;
		}
		/* detect tst */
		if(( (IMM && simm13 == 0 ) || (!IMM && RS2 == 0) ) && RD == 0){
			mnemonic = (pseudoMnemonicTable + TST)->mnemonic;
			tstTemplate();
			break;
		}
		threeOpTemplate();
		break;

/************************  JUMP, RETT, and TICC INSTRUCTIONS  *****************/


	case JUMP:
		/* detect ret */
		if(RS1 == 31 && IMM && simm13 == 8){
			mnemonic = (pseudoMnemonicTable + RET)->mnemonic;
			nopTemplate();
			break;
		}
		/*
		 * Separate JMP from the others because it might have
		 * a meaningful destination register (it saves PC into
		 * r[rd] if r[rd] isn't %g0).
		 *
		 * What about when it's "jmp r[rs1]+r[rs2]"?
		 * Is this handled?
		 */
		if( strcmp( rd, "%g0" ) != 0 )
			jumpTemplate();
		else	
			retticcTemplate();
		break;

/************************  RETT and TICC INSTRUCTIONS  ************************/
	case RETT:
	case TICC:
	case IFLUSH:
		retticcTemplate();
		break;
	
/************************* READ PROCESSOR STATE REGISTERS *********************/

	case RDY:
		strcpy(specialReg, "%y");
		readTemplate();
		break;

	case RDPSR:
		strcpy(specialReg, "%psr");
		readTemplate();
		break;

	case RDTBR:
		strcpy(specialReg, "%tbr");
		readTemplate();
		break;

	case RDWIM:
		strcpy(specialReg, "%wim");
		readTemplate();
		break;


/***********************  WRITE PROCESSOR STATE REGISTERS  ********************/

	case WRY:
		strcpy(specialReg, "%y");
		goto writeSpecial;

	case WRWIM:
		strcpy(specialReg, "%wim");
		goto writeSpecial;

	case WRPSR:
		strcpy(specialReg, "%psr");
		goto writeSpecial;

	case WRTBR:
		strcpy(specialReg, "%tbr");
	
	writeSpecial:
		if(!IMM && RS1 == 0){
			mnemonic = (pseudoMnemonicTable + MOV)->mnemonic;
			wrSrc2Template();
		}
		else if(!IMM && RS2 == 0){
			mnemonic = (pseudoMnemonicTable + MOV)->mnemonic;
			wrRs1Template();
		}
		else if(IMM && simm13 == 0){
			mnemonic = (pseudoMnemonicTable + MOV)->mnemonic;
			wrRs1Template();
		}
		else if(IMM && RS1 == 0){
			mnemonic = (pseudoMnemonicTable + MOV)->mnemonic;
			wrSrc2Template();
		}
		else
			writeTemplate();
		break;
	
	case FPOP:
	case FPCMP:
		disassembleFpop(instruction);
		break;


	default:
		strcpy(dResult, unknownInstr);
		break;
	}
}

static
disassembleFpop(instruction)
unsigned int instruction;
{
	/* decode fpops */
	switch((fpopSwitchTable + OPF)->index){

	case FCVTis:
	case FCVTid:
	case FCVTie:

	case FCVTsi:
	case FCVTdi:
	case FCVTei:

	case FCVTsiz:
	case FCVTdiz:
	case FCVTeiz:

	case FINTs:
	case FINTd:
	case FINTe:

	case FINTsz:
	case FINTdz:
	case FINTez:

	case FCVTsd:
	case FCVTse:
	case FCVTds:
	case FCVTde:
	case FCVTes:
	case FCVTed:

	case FMOVE:
	case FNEG:
	case FABS:

	case FCLSs:
	case FCLSd:
	case FCLSe:

	case FEXPs:
	case FEXPd:
	case FEXPe:

	case FSQRs:
	case FSQRd:
	case FSQRe:

		fpTwoOpTemplate();
		break;
	
	case FADDs:
	case FADDd:
	case FADDe:

	case FSUBs:
	case FSUBd:
	case FSUBe:

	case FMULs:
	case FMULd:
	case FMULe:

	case FDIVs:
	case FDIVd:
	case FDIVe:

	case FSCLs:
	case FSCLd:
	case FSCLe:

	case FREMs:
	case FREMd:
	case FREMe:

	case FQUOs:
	case FQUOd:
	case FQUOe:

		fpThreeOpTemplate();
		break;
	
	case FCMPs:
	case FCMPd:
	case FCMPe:

	case FCMPXs:
	case FCMPXd:
	case FCMPXe:
		fpCmpTemplate();
		break;

	default:
		strcpy(dResult, unknownInstr);
		break;
	}
}

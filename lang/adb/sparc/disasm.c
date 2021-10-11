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

/*
 * Name changed to disasm.c from disassembler.c
 */

#include <stdio.h>
#include <sys/types.h>
#include <strings.h>
#include "sr_general.h"
#include "sr_instruction.h"
#include "sr_indextables.h"
#include "sr_switchindexes.h"
#include "sr_opcodetable.h"
#include "sr_processor.h" /* source of register file size parameters */

/*
	External variables are used to hold all of the possible fields
	that might be printed out for a dis-assembled instruction.

	Most of these are created prior to decoding the instruction
	for dis-assembly. After decode, disassembly is a matter
	of printing out the right combination of fields.
*/

static char *mnemonic;	/* pointer to the mnemonic string */
static int const22;	/* 22 bit constant, extracted from format 2 */
static char target[64];	/* symbolic target of call, or branch,
					created after decode */

static char rd[6];	/* destination IU register string ( eg: "%l5") */
static char fd[6];	/* destination FPU register string ( eg: "%f23") */
static char cd[6];	/* dest coprocessor register string ( eg: "%c23") */
static char rs1[6], fs1[6], cs1[6];	/* register source 1 */
static char rs2[6], fs2[6], cs2[6];	/* register source 2 */
static int simm13;	/* signed immediate constant from format 3 */
static int asi;		/* address space identifier */

static char src2[20];	/* either "%rs2" or "simm13" */
static char source[30];	/* "%rs1 + src2" */

static char specialReg[10];	/* fsr, psr, y, tbr, wim:	after decode */


char *toSymbolic(addr)
Word addr;
/*
	convert an address to a string of the form:

		symbolName + offset
*/
{
	static char buf[64];

	/* The "1" shd be DSYM or ISYM */
	(void) ssymoff( addr, 1, buf );
	return(buf);
}



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
char *convertCPreg(r)
unsigned int r;
{
	static char name[6];

	sprintf(name, "%%c%d", r);
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

/*
 * Extract floating point fields
 */

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
 * Extract coprocessor fields
 */

static
extractCd(instruction)
unsigned int instruction;
{
	strcpy(cd, convertCPreg(RD));
}

static
extractCs1(instruction)
unsigned int instruction;
{
	strcpy(cs1, convertCPreg(RS1));
}

static
extractCs2(instruction)
unsigned int instruction;
{
	strcpy(cs2, convertCPreg(RS2));
}


/*
	There are about 34 "templates' for dis-assembly, and these
	are defined as procedures below.
*/
static
loadTemplate()
/*
	ldsb, ldsh, ldub, lduh, ld, ldd, ldstub
*/
{
	sprintf(dResult, "%-7s	[%s], %s", mnemonic, source, rd);
}

static
loadATemplate()
/*
	ldsba, ldsha, lduba, lduha, lda, ldda, ldstuba
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
	sprintf(dResult, "%-7s	[%s], %s", mnemonic, source, cd);
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
	ldcsr
*/
{
	sprintf(dResult, "%-7s	[%s], %%csr", mnemonic, source);
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
	sprintf(dResult, "%-7s	%s, [%s]", mnemonic, cd, source);
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
	sprintf(dResult, "%-7s	%%csr, [%s]", mnemonic, source);
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
	sprintf(dResult, "%-7s	%%cq, [%s]", mnemonic, source);
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
clrTemplate()
/*
	clr is really "mov %g0, dest", which is really an "or" instruction
*/
{
	sprintf(dResult, "%-7s	%s", mnemonic, rd);
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
unimpTemplate()
/*
	"unimp" (the explicitly unimplemented instruction) is currently
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
			SR_HI( const22 ), rd);
}

static
branchTemplate()
/*
	Bicc, FBfcc, call
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

		case FBFCC:
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

#define COMMENT_COL 32

/*
 * This routine appends the comment (for any instruction following
 * a sethi) that contains the put-back-together constant.  Used only
 * by sethi_reassemble.
 */
static void
cat_column ( res, comment ) register char *res, *comment; {
  int n, m;

        n  = strlen( res );
	while( n < COMMENT_COL ) {
	    res[ n++ ] = ' ';
	}
	res[ n++ ] = ' ';
	strcpy( res+n, comment );

} /* end cat_column */


/*
 * These variables are for keeping track of a constant
 * that is split between a sethi and its "or".
 */
static char sethi_jam[ 80 ];
static Word is_sethi, was_sethi, sethi_half, sethi_rd;

static
sethi_reassemble ( instruction )
unsigned int instruction;
{

	/*
	 * Heuristics for re-assembling constants from sethi instructions,
	 * where possible:
	 * IF previous instr was a sethi AND current instr is not a sethi
	 *  AND current instr is immediate AND they have the same destination,
	 * THEN print the re-assembled constant.
	 */
	if( was_sethi  &&  !is_sethi  &&  sethi_half  &&
	    IMM  &&  RD == sethi_rd )
	{
	  long c32;

	    c32 = sethi_half + simm13;
	    if( c32 == -1 ) {
		strcpy( sethi_jam, "! -1" );
	    } else if( c32 < 0 ) {
		sprintf( sethi_jam, "! -0x%x", -c32 );
	    } else {
		sprintf( sethi_jam, "! %s", toSymbolic( c32 ) );
	    }

	    cat_column( dResult, sethi_jam );
	}
}

#ifdef IMM_DECIMAL
static char *imm_base = "%d";
#else
static char *imm_base = "0x%x";
#endif

static
extractFields(instruction)
unsigned int instruction;
{
	(void) lookupMnemonic(instruction);
			/* this sets the mnemonic char pointer */

	const22 = DISP22;	/* extraction macros are in sr_instruction.h */

	extractRd(instruction);
	extractRs1(instruction);
	extractRs2(instruction);

	extractFd(instruction);
	extractFs1(instruction);
	extractFs2(instruction);

	extractCd(instruction);
	extractCs1(instruction);
	extractCs2(instruction);

	simm13 = SIMM13;
	simm13 = SR_SEX13( simm13 );	/* sign extend */

	asi = OPF;

	if(IMM) {
		if( simm13 < 0 )
		    sprintf(src2, "-0x%x", -simm13);
		else
		    sprintf(src2, imm_base, simm13);
	} else {
		strcpy(src2, rs2);
	}
	
	if((IMM && simm13 == 0) || (!IMM && RS2 == 0) )
		strcpy(source, rs1);
	else if(RS1 == 0)
		strcpy(source, src2);
	else if( src2[0] == '-' ) {
		strcpy(source, rs1);
		strcat(source, " - ");
		strcat(source, &src2[1] );
	} else {
		strcpy(source, rs1);
		strcat(source, " + ");
		strcat(source, src2);
	}
}

char *
disassemble(instruction, pc)
unsigned int instruction, pc;
/*
	The given instruction is dis-assembled into the static
	character array "dResult" and a pointer to the dis-assembly
	string is returned.

	It is necessary to know the program counter for the instruction
	so that absolute addresses can be computed where PC relative
	displacements exist.

	Where an immediate address is expected (branch, and call instructions),
	An attempt is made to look up the address in the symbol table 
	so that a symbolic target can be printed.
*/
{

 	/* If we have no formatSwitchTable, we gotta make tables */
 	if( formatSwitchTable == NULL ) {
		was_sethi = 0;
		mkDisAsmTables();
		if( formatSwitchTable == NULL ) {
			return "Cannot build instruction disassembly tables.\n";
		}

	}

	/* first extract various parts of the instruction */
	is_sethi = 0;
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

	if( was_sethi ) {
		sethi_reassemble( instruction );
	}

	was_sethi = is_sethi;
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

		strcpy(target, toSymbolic(absAddr));
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
	case FBFCC:
	    { int sex;	/* used for sign extension */
	      unsigned int absAddr;

		/*
			Branch instruction. Must convert PC-relative constant
			into an absolute address.
		*/

		sex = DISP22;
		sex = SR_SEX22( sex );		/* sign extend */
		absAddr = pc + SR_WA2BA(sex);	/* convert to absolute addr */

		/* lookup symbolic equivalent */

		strcpy(target, toSymbolic(absAddr));
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
		if( const22 == 0  &&  strcmp(rd, "%g0")==0 ) {
		    mnemonic = "nop" ;	    /* sethi %hi(0), %g0 */
		    nopTemplate();
		} else {
    		    sethiTemplate();
		    /*
		     * Save the high part, so that we can re-assemble the
		     * long constant in the process of decoding the next
		     * instruction, if we can determine whether it's relevant.
		     */
		    sethi_half = SR_HI( const22 );
		    sethi_rd = RD;
		    is_sethi = 1;
		}
		break;

	case UNIMP:
		/*
		 * The constant that our compiler currently puts into
		 * the immediate field (imm22) of the 'unimp' instruction
		 * is the length of a structure being returned.  Don't want
		 * it printed symbolically.
		 */
		sprintf( target, "0x%x", DISP22 );
		unimpTemplate();
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
	case LDD:
	case LDSTUB:
	case SWAP:
		loadTemplate();
		break;

	case LDSBA:
	case LDSHA:
	case LDUBA:
	case LDUHA:
	case LDA:
	case LDDA:
	case LDSTUBA:
	case SWAPA:
		loadATemplate();
		break;
	
	case LDF:
	case LDDF:
		loadFTemplate();
		break;
	
	case LDFSR:
		ldfsrTemplate();
		break;
	
	case LDC:
	case LDDC:
		loadCPTemplate();
		break;
	
	case LDCSR:
		ldcsrTemplate();
		break;

		
/**************************  STORE INSTRUCTIONS  ******************************/

	case STB:
	case STH:
	case ST:
	case STD:
		storeTemplate();
		break;

	case STBA:
	case STHA:
	case STA:
	case STDA:
		storeATemplate();
		break;
	
	case STF:
	case STDF:
		storeFTemplate();
		break;
	
	case STFSR:
		stfsrTemplate();
		break;

	case STDFQ:
		stfqTemplate();
		break;

	case STC:
	case STDC:
		storeCTemplate();
		break;
	
	case STCSR:
		stcsrTemplate();
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
	case UDIV:
	case SDIV:
	case UDIVcc:
	case SDIVcc:
	case UMUL:
	case SMUL:
	case UMULcc:
	case SMULcc:
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
		    if( (!IMM  &&  RS2 == 0)  ||  (IMM  && SIMM13 == 0 ) ) {
			mnemonic = (pseudoMnemonicTable + CLR)->mnemonic;
			clrTemplate();
			break;
		    } else {
			mnemonic = (pseudoMnemonicTable + MOV)->mnemonic;
			movTemplate();
			break;
		    }
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
		if( strcmp( rd, "%g0" ) != 0 ) {
			mnemonic = "jmpl";	/* unclean */
			jumpTemplate();
		} else {
			retticcTemplate();
		}
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

	case FsMULd:
	case FdMULe:

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

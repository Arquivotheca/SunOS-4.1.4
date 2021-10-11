static char sccs_id[] = "@(#)opcode_ptrs.c	1.1\t10/31/94";

#include <stdio.h>
#include "structs.h"
#include "sym.h"
#include "opcode_ptrs.h"

/* This is all purely here for efficiency reasons:
 * 	with the "opcode_ptr" array, we can get the pointer to a given opcode
 *	very cheaply, instead of doing an "opc_lookup()" every time.
 */

struct opcode *(opcode_ptr[N_OPs]);


void
init_opcode_pointers()
{
#ifdef DEBUG
	register int i;
#endif
	opcode_ptr[OP_NOP]	= opc_lookup("nop");

	opcode_ptr[OP_ST]	= opc_lookup("st");
	opcode_ptr[OP_STH]	= opc_lookup("sth");
	opcode_ptr[OP_STB]	= opc_lookup("stb");
	opcode_ptr[OP_LDF]	= opc_lookup("*ldf");
	opcode_ptr[OP_STF]	= opc_lookup("*stf");
	opcode_ptr[OP_LDFSR]	= opc_lookup("*ldfsr");
	opcode_ptr[OP_LDFSRA]	= opc_lookup("*ldfsra");
	opcode_ptr[OP_STFSR]	= opc_lookup("*stfsr");
	opcode_ptr[OP_STFSRA]	= opc_lookup("*stfsra");
	opcode_ptr[OP_LDC]	= opc_lookup("*ldc");
	opcode_ptr[OP_STC]	= opc_lookup("*stc");
	opcode_ptr[OP_LDCSR]	= opc_lookup("*ldcsr");
	opcode_ptr[OP_STCSR]	= opc_lookup("*stcsr");
	opcode_ptr[OP_LDDF]	= opc_lookup("*lddf");
	opcode_ptr[OP_STDF]	= opc_lookup("*stdf");
	opcode_ptr[OP_LDDC]	= opc_lookup("*lddc");
	opcode_ptr[OP_STDC]	= opc_lookup("*stdc");
	opcode_ptr[OP_STDFQ]	= opc_lookup("*stdfq");
	opcode_ptr[OP_STDCQ]	= opc_lookup("*stdcq");
	opcode_ptr[OP_ADD]	= opc_lookup("add");
	opcode_ptr[OP_ADDCC]	= opc_lookup("addcc");
	opcode_ptr[OP_SUB]	= opc_lookup("sub");
	opcode_ptr[OP_SUBCC]	= opc_lookup("subcc");
	opcode_ptr[OP_ANDCC]	= opc_lookup("andcc");
	opcode_ptr[OP_OR]	= opc_lookup("or");
	opcode_ptr[OP_ANDN]	= opc_lookup("andn");
	opcode_ptr[OP_XOR]	= opc_lookup("xor");
	opcode_ptr[OP_XNOR]	= opc_lookup("xnor");
	opcode_ptr[OP_SETHI]	= opc_lookup("sethi");
	opcode_ptr[OP_CALL]	= opc_lookup("call");
	opcode_ptr[OP_JMPL]	= opc_lookup("jmpl");
	opcode_ptr[OP_WRY]	= opc_lookup("*wry");
	opcode_ptr[OP_WRPSR]	= opc_lookup("*wrpsr");
	opcode_ptr[OP_WRWIM]	= opc_lookup("*wrwim");
	opcode_ptr[OP_WRTBR]	= opc_lookup("*wrtbr");
	opcode_ptr[OP_RDY]	= opc_lookup("*rdy");
	opcode_ptr[OP_RDPSR]	= opc_lookup("*rdpsr");
	opcode_ptr[OP_RDWIM]	= opc_lookup("*rdwim");
	opcode_ptr[OP_RDTBR]	= opc_lookup("*rdtbr");
	opcode_ptr[OP_UNIMP]	= opc_lookup("unimp");
	opcode_ptr[OP_PSEUDO_ALIGN]	= opc_lookup(".align");
	opcode_ptr[OP_PSEUDO_SKIP]	= opc_lookup(".skip");
	opcode_ptr[OP_PSEUDO_SEG]	= opc_lookup(".seg");
	opcode_ptr[OP_PSEUDO_BOF]	= opc_lookup("*.bof");
	opcode_ptr[OP_PSEUDO_CSWITCH]	= opc_lookup("*.cswitch");

	opcode_ptr[OP_BN]	= opc_lookup("bn");
	opcode_ptr[OP_BE]	= opc_lookup("be");
	opcode_ptr[OP_BZ]	= opc_lookup("bz");	/* synonym */
	opcode_ptr[OP_BLE]	= opc_lookup("ble");
	opcode_ptr[OP_BL]	= opc_lookup("bl");
	opcode_ptr[OP_BLEU]	= opc_lookup("bleu");
	opcode_ptr[OP_BCS]	= opc_lookup("bcs");
	opcode_ptr[OP_BNEG]	= opc_lookup("bneg");
	opcode_ptr[OP_BVS]	= opc_lookup("bvs");
	opcode_ptr[OP_BA]	= opc_lookup("ba");
	opcode_ptr[OP_BNE]	= opc_lookup("bne");
	opcode_ptr[OP_BNZ]	= opc_lookup("bnz");	/* synonym */
	opcode_ptr[OP_BG]	= opc_lookup("bg");
	opcode_ptr[OP_BGE]	= opc_lookup("bge");
	opcode_ptr[OP_BGU]	= opc_lookup("bgu");
	opcode_ptr[OP_BCC]	= opc_lookup("bcc");
	opcode_ptr[OP_BPOS]	= opc_lookup("bpos");
	opcode_ptr[OP_BVC]	= opc_lookup("bvc");

	opcode_ptr[OP_FBN]	= opc_lookup("fbn");
	opcode_ptr[OP_FBU]	= opc_lookup("fbu");
	opcode_ptr[OP_FBG]	= opc_lookup("fbg");
	opcode_ptr[OP_FBUG]	= opc_lookup("fbug");
	opcode_ptr[OP_FBL]	= opc_lookup("fbl");
	opcode_ptr[OP_FBUL]	= opc_lookup("fbul");
	opcode_ptr[OP_FBLG]	= opc_lookup("fblg");
	opcode_ptr[OP_FBNE]	= opc_lookup("fbne");
	opcode_ptr[OP_FBNZ]	= opc_lookup("fbnz");	/* synonym */
	opcode_ptr[OP_FBE]	= opc_lookup("fbe");
	opcode_ptr[OP_FBZ]	= opc_lookup("fbz");	/* synonym */
	opcode_ptr[OP_FBUE]	= opc_lookup("fbue");
	opcode_ptr[OP_FBGE]	= opc_lookup("fbge");
	opcode_ptr[OP_FBUGE]	= opc_lookup("fbuge");
	opcode_ptr[OP_FBLE]	= opc_lookup("fble");
	opcode_ptr[OP_FBULE]	= opc_lookup("fbule");
	opcode_ptr[OP_FBO]	= opc_lookup("fbo");
	opcode_ptr[OP_FBA]	= opc_lookup("fba");

	opcode_ptr[OP_CBN]	= opc_lookup("cbn");
	opcode_ptr[OP_CB3]	= opc_lookup("cb3");
	opcode_ptr[OP_CB2]	= opc_lookup("cb2");
	opcode_ptr[OP_CB23]	= opc_lookup("cb23");
	opcode_ptr[OP_CB1]	= opc_lookup("cb1");
	opcode_ptr[OP_CB13]	= opc_lookup("cb13");
	opcode_ptr[OP_CB12]	= opc_lookup("cb12");
	opcode_ptr[OP_CB123]	= opc_lookup("cb123");
	opcode_ptr[OP_CB0]	= opc_lookup("cb0");
	opcode_ptr[OP_CB03]	= opc_lookup("cb03");
	opcode_ptr[OP_CB02]	= opc_lookup("cb02");
	opcode_ptr[OP_CB023]	= opc_lookup("cb023");
	opcode_ptr[OP_CB01]	= opc_lookup("cb01");
	opcode_ptr[OP_CB013]	= opc_lookup("cb013");
	opcode_ptr[OP_CB012]	= opc_lookup("cb012");
	opcode_ptr[OP_CBA]	= opc_lookup("cba");

	opcode_ptr[OP_RET]	= opc_lookup("ret");
	opcode_ptr[OP_RETL]	= opc_lookup("retl");

	opcode_ptr[OP_EQUATE]	= opc_lookup("*EQUATE");
	opcode_ptr[OP_LABEL]	= opc_lookup("*LABEL");
	opcode_ptr[OP_MARKER]	= opc_lookup("*MARKER");
	opcode_ptr[OP_COMMENT]	= opc_lookup("*COMMENT");
	opcode_ptr[OP_SYM_REF]	= opc_lookup("*SYM_REF");

	opcode_ptr[OP_ORCC]	= opc_lookup("orcc");
	opcode_ptr[OP_JMP]	= opc_lookup("jmp");

#ifdef CHIPFABS	/* only for fab#1 IU chip */
	opcode_ptr[OP_FMOVS]	= opc_lookup("fmovs");
	opcode_ptr[OP_LDFA]	= opc_lookup("*ldfa");
	opcode_ptr[OP_STFA]	= opc_lookup("*stfa");
#endif /*CHIPFABS*/

/** #ifdef DEBUG
/** 	for (i = 0;   i < N_OPs;   i++)
/** 	{
/** 		if (opcode_ptr[i] == NULL)
/** 		{
/** 			internal_error("init_opcode_ptrs(): lookup err @%d", i);
/** 			/*NOTREACHED*/
/** 		}
/** 	}
/** #endif
 **/
}

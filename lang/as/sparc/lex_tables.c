static char sccs_id[] = "@(#)lex_tables.c	1.1\t10/31/94";

#include <stdio.h>	/* for "NULL" def'n */
#include "structs.h"
#include "y.tab.h"
#include "instructions.h"
#include "lex.h"
#include "functional_groups.h"

struct symbol init_syms[] =	/* reserved symbols */
{
/*{ "symname",         segment   , value,    attr   , opertype, ... }*/

/* No symbols defined here, currently.
 * Note: symbol '.' is "defined" in actions_expr.c:symbol_to_value().
 */

{ NULL }
};

struct opcode init_opcs[] =	/* opcode & pseudo-op mnemonics  */
{
/* Opcode mnemonics ("o_name"s) beginning with "*" are not accesible
 * to the user; they are used internally by the assembler.
 */

/*
*{ "o_name",  o_token ,   o_instr ,   o_igroup, o_func, o_binfmt,o_chain}
*/

{ "ldsb",    TOK_LD,    INSTR_LDSB,   IG_LD, FGLD(1,0,1),BF_3cd11,NULL},
{ "ldub",    TOK_LD,    INSTR_LDUB,   IG_LD, FGLD(1,0,0),BF_3cd11,NULL},
{ "ldsh",    TOK_LD,    INSTR_LDSH,   IG_LD, FGLD(2,0,1),BF_3cd11,NULL},
{ "lduh",    TOK_LD,    INSTR_LDUH,   IG_LD, FGLD(2,0,0),BF_3cd11,NULL},
{ "ld",      TOK_LD,    INSTR_LD,     IG_LD, FGLD(4,0,1),BF_3cd11,NULL},
{ "ldd",     TOK_LD,    INSTR_LDD,    IG_DLD,FGLD(8,0,0),BF_3cd11,NULL},

{ "ldsba",   TOK_LDA,   INSTR_LDSBA,  IG_LD, FGLD(1,1,1),BF_3cd11,NULL},
{ "lduba",   TOK_LDA,   INSTR_LDUBA,  IG_LD, FGLD(1,1,0),BF_3cd11,NULL},
{ "ldsha",   TOK_LDA,   INSTR_LDSHA,  IG_LD, FGLD(2,1,1),BF_3cd11,NULL},
{ "lduha",   TOK_LDA,   INSTR_LDUHA,  IG_LD, FGLD(2,1,0),BF_3cd11,NULL},
{ "lda",     TOK_LDA,   INSTR_LDA,    IG_LD, FGLD(4,1,1),BF_3cd11,NULL},
{ "ldda",    TOK_LDA,   INSTR_LDDA,   IG_DLD,FGLD(8,1,0),BF_3cd11,NULL},

#ifdef CHIPFABS	/* only in fab#1 chip */
{ "ldst",    TOK_LD,    INSTR_LDST,   IG_LD, FGLDST(0),  BF_3cd11,NULL},
{ "ldsta",   TOK_LDA,   INSTR_LDSTA,  IG_LD, FGLDST(1),  BF_3cd11,NULL},
#endif /*CHIPFABS*/
{ "ldstub",  TOK_LD,    INSTR_LDSTUB, IG_LD, FGLDSTUB(0),BF_3cd11,NULL},
{ "swap",    TOK_LD,    INSTR_SWAP,   IG_LD, FGSWAP(0),  BF_3cd11,NULL},
{ "ldstuba", TOK_LDA,   INSTR_LDSTUBA,IG_LD, FGLDSTUB(1),BF_3cd11,NULL},
{ "swapa",   TOK_LDA,   INSTR_SWAPA,  IG_LD, FGSWAP(1),  BF_3cd11,NULL},

{ "stb",     TOK_ST,    INSTR_STB,    IG_ST, FGST(1,0), BF_3cd11, NULL},
{ "sth",     TOK_ST,    INSTR_STH,    IG_ST, FGST(2,0), BF_3cd11, NULL},
{ "st",      TOK_ST,    INSTR_ST,     IG_ST, FGST(4,0), BF_3cd11, NULL},
{ "std",     TOK_ST,    INSTR_STD,    IG_DST,FGST(8,0), BF_3cd11, NULL},

{ "stba",    TOK_STA,   INSTR_STBA,   IG_ST, FGST(1,1), BF_3cd11, NULL},
{ "stha",    TOK_STA,   INSTR_STHA,   IG_ST, FGST(2,1), BF_3cd11, NULL},
{ "sta",     TOK_STA,   INSTR_STA,    IG_ST, FGST(4,1), BF_3cd11, NULL},
{ "stda",    TOK_STA,   INSTR_STDA,   IG_DST,FGST(8,1), BF_3cd11, NULL},

{ "*ldf",    TOK_NONE,  INSTR_LDF,    IG_LD,FGLDF(4,0,1),BF_3cd11,NULL},
{ "*ldfsr",  TOK_NONE,  INSTR_LDFSR,  IG_LD,FGLDF(4,0,1),BF_3cd01,NULL},
{ "*lddf",   TOK_NONE,  INSTR_LDDF,   IG_LD,FGLDF(8,0,0),BF_3cd11,NULL},

{ "*stf",    TOK_NONE,  INSTR_STF,    IG_ST, FGSTF(4,0), BF_3cd11,NULL},
{ "*stfsr",  TOK_NONE,  INSTR_STFSR,  IG_ST, FGSTF(4,0), BF_3cd01,NULL},
{ "*stdfq",  TOK_NONE,  INSTR_STDFQ,  IG_DST,FGSTF(8,0), BF_3cd01,NULL},
{ "*stdf",   TOK_NONE,  INSTR_STDF,   IG_DST,FGSTF(8,0), BF_3cd11,NULL},

{ "*ldc",    TOK_NONE,  INSTR_LDC,    IG_LD, FGLD(4,0,1),BF_3cd11,NULL},
{ "*ldcsr",  TOK_NONE,  INSTR_LDCSR,  IG_LD, FGLD(4,0,1),BF_3cd01,NULL},
{ "*lddc",   TOK_NONE,  INSTR_LDDC,   IG_LD, FGLD(8,0,0),BF_3cd11,NULL},

{ "*stc",    TOK_NONE,  INSTR_STC,    IG_ST, FGST(4,0),  BF_3cd11,NULL},
{ "*stcsr",  TOK_NONE,  INSTR_STCSR,  IG_ST, FGST(4,0),  BF_3cd01,NULL},
{ "*stdcq",  TOK_NONE,  INSTR_STDCQ,  IG_DST,FGST(8,0),  BF_3cd01,NULL},
{ "*stdc",   TOK_NONE,  INSTR_STDC,   IG_DST,FGST(8,0),  BF_3cd11,NULL},

#ifdef CHIPFABS	/* only in fab#1 chip */
{ "*ldfa",   TOK_NONE,  INSTR_LDFA,   IG_LD, FGLD(4,1,1),BF_3cd11,NULL},
{ "*ldfsra", TOK_NONE,  INSTR_LDFSRA, IG_LD, FGLD(4,1,1),BF_3cd11,NULL},
{ "*lddfa",  TOK_NONE,  INSTR_LDDFA,  IG_LD, FGLD(8,1,0),BF_3cd11,NULL},

{ "*stfa",   TOK_NONE,  INSTR_STFA,   IG_ST, FGST(4,1),  BF_3cd11,NULL},
{ "*stfsra", TOK_NONE,  INSTR_STFSRA, IG_ST, FGST(4,1),  BF_3cd11,NULL},
{ "*stdfqa", TOK_NONE,  INSTR_STDFQA, IG_DST,FGST(8,1),  BF_3cd11,NULL},
{ "*stdfa",  TOK_NONE,  INSTR_STDFA,  IG_DST,FGST(8,1),  BF_3cd11,NULL},
#endif /*CHIPFABS*/

{ "add",     TOK_ARITH, INSTR_ADD,    IG_ARITH,FGA(ADD,0,1), BF_3cd10, NULL},
{ "addcc",   TOK_ARITH, INSTR_ADDCC,  IG_ARITH,FGA(ADD,1,1), BF_3cd10, NULL},
{ "addx",    TOK_ARITH, INSTR_ADDX,   IG_ARITH,FGAX(ADD,0,1),BF_3cd10, NULL},
{ "addxcc",  TOK_ARITH, INSTR_ADDXCC, IG_ARITH,FGAX(ADD,1,1),BF_3cd10, NULL},
{ "sub",     TOK_ARITH, INSTR_SUB,    IG_ARITH,FGA(SUB,0,0), BF_3cd10, NULL},
{ "subcc",   TOK_ARITH, INSTR_SUBCC,  IG_ARITH,FGA(SUB,1,0), BF_3cd10, NULL},
{ "subx",    TOK_ARITH, INSTR_SUBX,   IG_ARITH,FGAX(SUB,0,0),BF_3cd10, NULL},
{ "subxcc",  TOK_ARITH, INSTR_SUBXCC, IG_ARITH,FGAX(SUB,1,0),BF_3cd10, NULL},
{ "mulscc",  TOK_ARITH, INSTR_MULSCC, IG_ARITH,FGA(MULS,1,0),BF_3cd10, NULL},
{ "and",     TOK_ARITH, INSTR_AND,    IG_ARITH,FGA(AND,0,1), BF_3cd10, NULL},
{ "andcc",   TOK_ARITH, INSTR_ANDCC,  IG_ARITH,FGA(AND,1,1), BF_3cd10, NULL},
{ "andn",    TOK_ARITH, INSTR_ANDN,   IG_ARITH,FGA(ANDN,0,0),BF_3cd10, NULL},
{ "andncc",  TOK_ARITH, INSTR_ANDNCC, IG_ARITH,FGA(ANDN,1,0),BF_3cd10, NULL},
{ "or",      TOK_ARITH, INSTR_OR,     IG_ARITH,FGA(OR, 0,1), BF_3cd10, NULL},
{ "orcc",    TOK_ARITH, INSTR_ORCC,   IG_ARITH,FGA(OR, 1,1), BF_3cd10, NULL},
{ "orn",     TOK_ARITH, INSTR_ORN,    IG_ARITH,FGA(ORN,0,0), BF_3cd10, NULL},
{ "orncc",   TOK_ARITH, INSTR_ORNCC,  IG_ARITH,FGA(ORN,1,0), BF_3cd10, NULL},
{ "xor",     TOK_ARITH, INSTR_XOR,    IG_ARITH,FGA(XOR,0,1), BF_3cd10, NULL},
{ "xorcc",   TOK_ARITH, INSTR_XORCC,  IG_ARITH,FGA(XOR,1,1), BF_3cd10, NULL},
{ "xnor",    TOK_ARITH, INSTR_XNOR,   IG_ARITH,FGA(XORN,0,1),BF_3cd10, NULL},
{ "xnorcc",  TOK_ARITH, INSTR_XNORCC, IG_ARITH,FGA(XORN,1,1),BF_3cd10, NULL},
{ "sll",     TOK_ARITH, INSTR_SLL,    IG_ARITH,FGA(SLL,0,0), BF_3cd10, NULL},
{ "srl",     TOK_ARITH, INSTR_SRL,    IG_ARITH,FGA(SRL,0,0), BF_3cd10, NULL},
{ "sra",     TOK_ARITH, INSTR_SRA,    IG_ARITH,FGA(SRA,0,0), BF_3cd10, NULL},
{ "umul",    TOK_ARITH, INSTR_UMUL,   IG_ARITH,FGA(UMUL,0,1),BF_3cd10, NULL},
{ "umulcc",  TOK_ARITH, INSTR_UMULCC, IG_ARITH,FGA(UMUL,1,1),BF_3cd10, NULL},
{ "smul",    TOK_ARITH, INSTR_SMUL,   IG_ARITH,FGA(SMUL,0,1),BF_3cd10, NULL},
{ "smulcc",  TOK_ARITH, INSTR_SMULCC, IG_ARITH,FGA(SMUL,1,1),BF_3cd10, NULL},
{ "udiv",    TOK_ARITH, INSTR_UDIV,   IG_ARITH,FGA(UDIV,0,0),BF_3cd10, NULL},
{ "udivcc",  TOK_ARITH, INSTR_UDIVCC, IG_ARITH,FGA(UDIV,1,0),BF_3cd10, NULL},
{ "sdiv",    TOK_ARITH, INSTR_SDIV,   IG_ARITH,FGA(SDIV,0,0),BF_3cd10, NULL},
{ "sdivcc",  TOK_ARITH, INSTR_SDIVCC, IG_ARITH,FGA(SDIV,1,0),BF_3cd10, NULL},

{ "taddcc",  TOK_ARITH, INSTR_TADDCC, IG_ARITH,FGA(TADD,1,1), BF_3cd10, NULL},
{ "taddcctv",TOK_ARITH, INSTR_TADDCCTV,IG_ARITH,FGA(TADD,0,1), BF_3cd10, NULL},
{ "tsubcc",  TOK_ARITH, INSTR_TSUBCC, IG_ARITH,FGA(TSUB,1,0), BF_3cd10, NULL},
{ "tsubcctv",TOK_ARITH, INSTR_TSUBCCTV,IG_ARITH,FGA(TSUB,0,0), BF_3cd10, NULL},

{ "save",    TOK_ARITH, INSTR_SAVE,   IG_SVR,FGA(SAVE,0,1), BF_3cd10, NULL},
{ "restore", TOK_ARITH, INSTR_RESTORE,IG_SVR,FGA(REST,0,1), BF_3cd10, NULL},

{ "sethi",   TOK_2OPS,  INSTR_SETHI,  IG_SETHI,FGSETHI, BF_2a,    NULL},

/* Following mnemonics marked "synonym" are synonyms for the basic
 * mnemonics, to make life easier for assembly-language programmers.
 * They are disassembled using the same name the programmer originally
 * used.
 */

{ "bn",      TOK_1OPS,  INSTR_BN,     IG_BR,FGBR(N),      BF_2b, NULL},
{ "be",      TOK_1OPS,  INSTR_BE,     IG_BR,FGBRI(E),     BF_2b, NULL},
{ "bz",      TOK_1OPS,  INSTR_BE,     IG_BR,FGBRI(E),     BF_2b, NULL}, /*synonym*/
{ "ble",     TOK_1OPS,  INSTR_BLE,    IG_BR,FGBRI(LE),    BF_2b, NULL},
{ "bl",      TOK_1OPS,  INSTR_BL,     IG_BR,FGBRI(L),     BF_2b, NULL},
{ "bleu",    TOK_1OPS,  INSTR_BLEU,   IG_BR,FGBRI(LEU),   BF_2b, NULL},
{ "bcs",     TOK_1OPS,  INSTR_BCS,    IG_BR,FGBRI(CS),    BF_2b, NULL},
{ "blu",     TOK_1OPS,  INSTR_BCS,    IG_BR,FGBRI(CS),    BF_2b, NULL}, /*synonym*/
{ "bneg",    TOK_1OPS,  INSTR_BNEG,   IG_BR,FGBRI(NEG),   BF_2b, NULL},
{ "bvs",     TOK_1OPS,  INSTR_BVS,    IG_BR,FGBRI(VS),    BF_2b, NULL},
{ "ba",      TOK_1OPS,  INSTR_BA,     IG_BR,FGBR(A),      BF_2b, NULL},
{ "b",       TOK_1OPS,  INSTR_BA,     IG_BR,FGBR(A),      BF_2b, NULL}, /*synonym*/
{ "bne",     TOK_1OPS,  INSTR_BNE,    IG_BR,FGBRI(NE),    BF_2b, NULL},
{ "bnz",     TOK_1OPS,  INSTR_BNE,    IG_BR,FGBRI(NE),    BF_2b, NULL}, /*synonym*/
{ "bg",      TOK_1OPS,  INSTR_BG,     IG_BR,FGBRI(G),     BF_2b, NULL},
{ "bge",     TOK_1OPS,  INSTR_BGE,    IG_BR,FGBRI(GE),    BF_2b, NULL},
{ "bgu",     TOK_1OPS,  INSTR_BGU,    IG_BR,FGBRI(GU),    BF_2b, NULL},
{ "bcc",     TOK_1OPS,  INSTR_BCC,    IG_BR,FGBRI(CC),    BF_2b, NULL},
{ "bgeu",    TOK_1OPS,  INSTR_BCC,    IG_BR,FGBRI(CC),    BF_2b, NULL}, /*synonym*/
{ "bpos",    TOK_1OPS,  INSTR_BPOS,   IG_BR,FGBRI(POS),   BF_2b, NULL},
{ "bvc",     TOK_1OPS,  INSTR_BVC,    IG_BR,FGBRI(VC),    BF_2b, NULL},

{ "bn,a",    TOK_1OPS,  INSTR_BN_A,   IG_BR,FGBR_A(N),    BF_2b, NULL},
{ "be,a",    TOK_1OPS,  INSTR_BE_A,   IG_BR,FGBRI_A(E),   BF_2b, NULL},
{ "bz,a",    TOK_1OPS,  INSTR_BE_A,   IG_BR,FGBRI_A(E),   BF_2b, NULL}, /*synonym*/
{ "ble,a",   TOK_1OPS,  INSTR_BLE_A,  IG_BR,FGBRI_A(LE),  BF_2b, NULL},
{ "bl,a",    TOK_1OPS,  INSTR_BL_A,   IG_BR,FGBRI_A(L),   BF_2b, NULL},
{ "bleu,a",  TOK_1OPS,  INSTR_BLEU_A, IG_BR,FGBRI_A(LEU), BF_2b, NULL},
{ "bcs,a",   TOK_1OPS,  INSTR_BCS_A,  IG_BR,FGBRI_A(CS),  BF_2b, NULL},
{ "blu,a",   TOK_1OPS,  INSTR_BCS_A,  IG_BR,FGBRI_A(CS),  BF_2b, NULL}, /*synonym*/
{ "bneg,a",  TOK_1OPS,  INSTR_BNEG_A, IG_BR,FGBRI_A(NEG), BF_2b, NULL},
{ "bvs,a",   TOK_1OPS,  INSTR_BVS_A,  IG_BR,FGBRI_A(VS),  BF_2b, NULL},
{ "ba,a",    TOK_1OPS,  INSTR_BA_A,   IG_BR,FGBR_A(A),    BF_2b, NULL},
{ "b,a",     TOK_1OPS,  INSTR_BA_A,   IG_BR,FGBR_A(A),    BF_2b, NULL}, /*synonym*/
{ "bne,a",   TOK_1OPS,  INSTR_BNE_A,  IG_BR,FGBRI_A(NE),  BF_2b, NULL},
{ "bnz,a",   TOK_1OPS,  INSTR_BNE_A,  IG_BR,FGBRI_A(NE),  BF_2b, NULL}, /*synonym*/
{ "bg,a",    TOK_1OPS,  INSTR_BG_A,   IG_BR,FGBRI_A(G),   BF_2b, NULL},
{ "bge,a",   TOK_1OPS,  INSTR_BGE_A,  IG_BR,FGBRI_A(GE),  BF_2b, NULL},
{ "bgu,a",   TOK_1OPS,  INSTR_BGU_A,  IG_BR,FGBRI_A(GU),  BF_2b, NULL},
{ "bcc,a",   TOK_1OPS,  INSTR_BCC_A,  IG_BR,FGBRI_A(CC),  BF_2b, NULL},
{ "bgeu,a",  TOK_1OPS,  INSTR_BCC_A,  IG_BR,FGBRI_A(CC),  BF_2b, NULL}, /*synonym*/
{ "bpos,a",  TOK_1OPS,  INSTR_BPOS_A, IG_BR,FGBRI_A(POS), BF_2b, NULL},
{ "bvc,a",   TOK_1OPS,  INSTR_BVC_A,  IG_BR,FGBRI_A(VC),  BF_2b, NULL},

{ "fbn",     TOK_1OPS,  INSTR_FBN,    IG_BR,FGBR(N),      BF_2b, NULL},
{ "fbu",     TOK_1OPS,  INSTR_FBU,    IG_BR,FGBRF(FU),    BF_2b, NULL},
{ "fbg",     TOK_1OPS,  INSTR_FBG,    IG_BR,FGBRF(FG),    BF_2b, NULL},
{ "fbug",    TOK_1OPS,  INSTR_FBUG,   IG_BR,FGBRF(FUG),   BF_2b, NULL},
{ "fbl",     TOK_1OPS,  INSTR_FBL,    IG_BR,FGBRF(FL),    BF_2b, NULL},
{ "fbul",    TOK_1OPS,  INSTR_FBUL,   IG_BR,FGBRF(FUL),   BF_2b, NULL},
{ "fblg",    TOK_1OPS,  INSTR_FBLG,   IG_BR,FGBRF(FLG),   BF_2b, NULL},
{ "fbne",    TOK_1OPS,  INSTR_FBNE,   IG_BR,FGBRF(FNE),   BF_2b, NULL},
{ "fbnz",    TOK_1OPS,  INSTR_FBNE,   IG_BR,FGBRF(FNE),   BF_2b, NULL}, /*synonym*/
{ "fbe",     TOK_1OPS,  INSTR_FBE,    IG_BR,FGBRF(FE),    BF_2b, NULL},
{ "fbz",     TOK_1OPS,  INSTR_FBE,    IG_BR,FGBRF(FE),    BF_2b, NULL}, /*synonym*/
{ "fbue",    TOK_1OPS,  INSTR_FBUE,   IG_BR,FGBRF(FUE),   BF_2b, NULL},
{ "fbge",    TOK_1OPS,  INSTR_FBGE,   IG_BR,FGBRF(FGE),   BF_2b, NULL},
{ "fbuge",   TOK_1OPS,  INSTR_FBUGE,  IG_BR,FGBRF(FUGE),  BF_2b, NULL},
{ "fble",    TOK_1OPS,  INSTR_FBLE,   IG_BR,FGBRF(FLE),   BF_2b, NULL},
{ "fbule",   TOK_1OPS,  INSTR_FBULE,  IG_BR,FGBRF(FULE),  BF_2b, NULL},
{ "fbo",     TOK_1OPS,  INSTR_FBO,    IG_BR,FGBRF(FO),    BF_2b, NULL},
{ "fba",     TOK_1OPS,  INSTR_FBA,    IG_BR,FGBR(A),      BF_2b, NULL},

{ "fbn,a",   TOK_1OPS,  INSTR_FBN_A,  IG_BR,FGBR_A(N),    BF_2b, NULL},
{ "fbu,a",   TOK_1OPS,  INSTR_FBU_A,  IG_BR,FGBRF_A(FU),  BF_2b, NULL},
{ "fbg,a",   TOK_1OPS,  INSTR_FBG_A,  IG_BR,FGBRF_A(FG),  BF_2b, NULL},
{ "fbug,a",  TOK_1OPS,  INSTR_FBUG_A, IG_BR,FGBRF_A(FUG), BF_2b, NULL},
{ "fbl,a",   TOK_1OPS,  INSTR_FBL_A,  IG_BR,FGBRF_A(FL),  BF_2b, NULL},
{ "fbul,a",  TOK_1OPS,  INSTR_FBUL_A, IG_BR,FGBRF_A(FUL), BF_2b, NULL},
{ "fblg,a",  TOK_1OPS,  INSTR_FBLG_A, IG_BR,FGBRF_A(FLG), BF_2b, NULL},
{ "fbne,a",  TOK_1OPS,  INSTR_FBNE_A, IG_BR,FGBRF_A(FNE), BF_2b, NULL},
{ "fbnz,a",  TOK_1OPS,  INSTR_FBNE_A, IG_BR,FGBRF_A(FNE), BF_2b, NULL}, /*synonym*/
{ "fbe,a",   TOK_1OPS,  INSTR_FBE_A,  IG_BR,FGBRF_A(FE),  BF_2b, NULL},
{ "fbz,a",   TOK_1OPS,  INSTR_FBE_A,  IG_BR,FGBRF_A(FE),  BF_2b, NULL}, /*synonym*/
{ "fbue,a",  TOK_1OPS,  INSTR_FBUE_A, IG_BR,FGBRF_A(FUE), BF_2b, NULL},
{ "fbge,a",  TOK_1OPS,  INSTR_FBGE_A, IG_BR,FGBRF_A(FGE), BF_2b, NULL},
{ "fbuge,a", TOK_1OPS,  INSTR_FBUGE_A,IG_BR,FGBRF_A(FUGE),BF_2b, NULL},
{ "fble,a",  TOK_1OPS,  INSTR_FBLE_A, IG_BR,FGBRF_A(FLE), BF_2b, NULL},
{ "fbule,a", TOK_1OPS,  INSTR_FBULE_A,IG_BR,FGBRF_A(FULE),BF_2b, NULL},
{ "fbo,a",   TOK_1OPS,  INSTR_FBO_A,  IG_BR,FGBRF_A(FO),  BF_2b, NULL},
{ "fba,a",   TOK_1OPS,  INSTR_FBA_A,  IG_BR,FGBR_A(A),    BF_2b, NULL},

{ "cbn",     TOK_1OPS,  INSTR_CBN,    IG_BR,FGBR(N),      BF_2b, NULL},
{ "cb3",     TOK_1OPS,  INSTR_CB3,    IG_BR,FGBRC(C3),    BF_2b, NULL},
{ "cb2",     TOK_1OPS,  INSTR_CB2,    IG_BR,FGBRC(C2),    BF_2b, NULL},
{ "cb23",    TOK_1OPS,  INSTR_CB23,   IG_BR,FGBRC(C23),   BF_2b, NULL},
{ "cb1",     TOK_1OPS,  INSTR_CB1,    IG_BR,FGBRC(C1),    BF_2b, NULL},
{ "cb13",    TOK_1OPS,  INSTR_CB13,   IG_BR,FGBRC(C13),   BF_2b, NULL},
{ "cb12",    TOK_1OPS,  INSTR_CB12,   IG_BR,FGBRC(C12),   BF_2b, NULL},
{ "cb123",   TOK_1OPS,  INSTR_CB123,  IG_BR,FGBRC(C123),  BF_2b, NULL},
{ "cb0",     TOK_1OPS,  INSTR_CB0,    IG_BR,FGBRC(C0),    BF_2b, NULL},
{ "cb03",    TOK_1OPS,  INSTR_CB03,   IG_BR,FGBRC(C03),   BF_2b, NULL},
{ "cb02",    TOK_1OPS,  INSTR_CB02,   IG_BR,FGBRC(C02),   BF_2b, NULL},
{ "cb023",   TOK_1OPS,  INSTR_CB023,  IG_BR,FGBRC(C023),  BF_2b, NULL},
{ "cb01",    TOK_1OPS,  INSTR_CB01,   IG_BR,FGBRC(C01),   BF_2b, NULL},
{ "cb013",   TOK_1OPS,  INSTR_CB013,  IG_BR,FGBRC(C013),  BF_2b, NULL},
{ "cb012",   TOK_1OPS,  INSTR_CB012,  IG_BR,FGBRC(C012),  BF_2b, NULL},
{ "cba",     TOK_1OPS,  INSTR_CBA,    IG_BR,FGBR(A),      BF_2b, NULL},

{ "cbn,a",   TOK_1OPS,  INSTR_CBN_A,  IG_BR,FGBR_A(N),    BF_2b, NULL},
{ "cb3,a",   TOK_1OPS,  INSTR_CB3_A,  IG_BR,FGBRC_A(C3),  BF_2b, NULL},
{ "cb2,a",   TOK_1OPS,  INSTR_CB2_A,  IG_BR,FGBRC_A(C2),  BF_2b, NULL},
{ "cb23,a",  TOK_1OPS,  INSTR_CB23_A, IG_BR,FGBRC_A(C23), BF_2b, NULL},
{ "cb1,a",   TOK_1OPS,  INSTR_CB1_A,  IG_BR,FGBRC_A(C1),  BF_2b, NULL},
{ "cb13,a",  TOK_1OPS,  INSTR_CB13_A, IG_BR,FGBRC_A(C13), BF_2b, NULL},
{ "cb12,a",  TOK_1OPS,  INSTR_CB12_A, IG_BR,FGBRC_A(C12), BF_2b, NULL},
{ "cb123,a", TOK_1OPS,  INSTR_CB123_A,IG_BR,FGBRC_A(C123),BF_2b, NULL},
{ "cb0,a",   TOK_1OPS,  INSTR_CB0_A,  IG_BR,FGBRC_A(C0),  BF_2b, NULL},
{ "cb03,a",  TOK_1OPS,  INSTR_CB03_A, IG_BR,FGBRC_A(C03), BF_2b, NULL},
{ "cb02,a",  TOK_1OPS,  INSTR_CB02_A, IG_BR,FGBRC_A(C02), BF_2b, NULL},
{ "cb023,a", TOK_1OPS,  INSTR_CB023_A,IG_BR,FGBRC_A(C023),BF_2b, NULL},
{ "cb01,a",  TOK_1OPS,  INSTR_CB01_A, IG_BR,FGBRC_A(C01), BF_2b, NULL},
{ "cb013,a", TOK_1OPS,  INSTR_CB013_A,IG_BR,FGBRC_A(C013),BF_2b, NULL},
{ "cb012,a", TOK_1OPS,  INSTR_CB012_A,IG_BR,FGBRC_A(C012),BF_2b, NULL},
{ "cba,a",   TOK_1OPS,  INSTR_CBA_A,  IG_BR,FGBR_A(A),    BF_2b, NULL},

{ "tn",      TOK_1OPS,  INSTR_TN,     IG_TICC, FGTRAP, BF_3ab, NULL},
{ "te",      TOK_1OPS,  INSTR_TE,     IG_TICC, FGTRAPI,BF_3ab, NULL},
{ "tz",      TOK_1OPS,  INSTR_TE,     IG_TICC, FGTRAPI,BF_3ab, NULL}, /*synonym*/
{ "tle",     TOK_1OPS,  INSTR_TLE,    IG_TICC, FGTRAPI,BF_3ab, NULL},
{ "tl",      TOK_1OPS,  INSTR_TL,     IG_TICC, FGTRAPI,BF_3ab, NULL},
{ "tleu",    TOK_1OPS,  INSTR_TLEU,   IG_TICC, FGTRAPI,BF_3ab, NULL},
{ "tcs",     TOK_1OPS,  INSTR_TCS,    IG_TICC, FGTRAPI,BF_3ab, NULL},
{ "tlu",     TOK_1OPS,  INSTR_TCS,    IG_TICC, FGTRAPI,BF_3ab, NULL}, /*synonym*/
{ "tneg",    TOK_1OPS,  INSTR_TNEG,   IG_TICC, FGTRAPI,BF_3ab, NULL},
{ "tvs",     TOK_1OPS,  INSTR_TVS,    IG_TICC, FGTRAPI,BF_3ab, NULL},
{ "ta",      TOK_1OPS,  INSTR_TA,     IG_TICC, FGTRAP, BF_3ab, NULL},
{ "t",       TOK_1OPS,  INSTR_TA,     IG_TICC, FGTRAP, BF_3ab, NULL}, /*synonym*/
{ "tne",     TOK_1OPS,  INSTR_TNE,    IG_TICC, FGTRAPI,BF_3ab, NULL},
{ "tnz",     TOK_1OPS,  INSTR_TNE,    IG_TICC, FGTRAPI,BF_3ab, NULL}, /*synonym*/
{ "tg",      TOK_1OPS,  INSTR_TG,     IG_TICC, FGTRAPI,BF_3ab, NULL},
{ "tge",     TOK_1OPS,  INSTR_TGE,    IG_TICC, FGTRAPI,BF_3ab, NULL},
{ "tgu",     TOK_1OPS,  INSTR_TGU,    IG_TICC, FGTRAPI,BF_3ab, NULL},
{ "tcc",     TOK_1OPS,  INSTR_TCC,    IG_TICC, FGTRAPI,BF_3ab, NULL},
{ "tgeu",    TOK_1OPS,  INSTR_TCC,    IG_TICC, FGTRAPI,BF_3ab, NULL}, /*synonym*/
{ "tpos",    TOK_1OPS,  INSTR_TPOS,   IG_TICC, FGTRAPI,BF_3ab, NULL},
{ "tvc",     TOK_1OPS,  INSTR_TVC,    IG_TICC, FGTRAPI,BF_3ab, NULL},

{ "call",    TOK_1OPS,  INSTR_CALL,   IG_CALL, FGCALL, BF_1,   NULL},

{ "jmpl",    TOK_1OPS,  INSTR_JMPL,   IG_JMPL, FGJMPL, BF_3cd10, NULL},

{ "rett",    TOK_1OPS,  INSTR_RETT,   IG_JRI,  FGRETT, BF_3cd00, NULL},
{ "iflush",  TOK_1OPS,  INSTR_IFLUSH, IG_JRI,  FGIFLUSH,BF_3cd00,NULL},

{ "unimp",   TOK_1OPS,  INSTR_UNIMP,  IG_UNIMP,FGUNIMP,BF_3cd1X, NULL},

{ "rd",      TOK_2OPS,  INSTR_NONE,   IG_RD,   FGNONE,   BF_3cd1X,NULL},
{ "*rdy",    TOK_NONE,  INSTR_RD_Y,   IG_RD,   FGRD(Y),  BF_3cd1X,NULL},
{ "*rdpsr",  TOK_NONE,  INSTR_RD_PSR, IG_RD,   FGRD(PSR),BF_3cd1X,NULL},
{ "*rdwim",  TOK_NONE,  INSTR_RD_WIM, IG_RD,   FGRD(WIM),BF_3cd1X,NULL},
{ "*rdtbr",  TOK_NONE,  INSTR_RD_TBR, IG_RD,   FGRD(TBR),BF_3cd1X,NULL},

{ "wr",      TOK_ARITH, INSTR_NONE,   IG_WR,   FGNONE,   BF_3cd00,NULL},
{ "*wry",    TOK_NONE,  INSTR_WR_Y,   IG_WR,   FGWR(Y),  BF_3cd00,NULL},
{ "*wrpsr",  TOK_NONE,  INSTR_WR_PSR, IG_WR,   FGWR(PSR),BF_3cd00,NULL},
{ "*wrwim",  TOK_NONE,  INSTR_WR_WIM, IG_WR,   FGWR(WIM),BF_3cd00,NULL},
{ "*wrtbr",  TOK_NONE,  INSTR_WR_TBR, IG_WR,   FGWR(TBR),BF_3cd00,NULL},

{ "fitos",   TOK_2OPS,INSTR_FITOS, IG_FP,FGFCVT( 4, 4), BF_3e10, NULL},
{ "fitod",   TOK_2OPS,INSTR_FITOD, IG_FP,FGFCVT( 4, 8), BF_3e10, NULL},
{ "fitoq",   TOK_2OPS,INSTR_FITOQ, IG_FP,FGFCVT( 4,16), BF_3e10, NULL},
{ "fstoi",   TOK_2OPS,INSTR_FSTOI, IG_FP,FGFCVT( 4, 4), BF_3e10, NULL},
{ "fdtoi",   TOK_2OPS,INSTR_FDTOI, IG_FP,FGFCVT( 8, 4), BF_3e10, NULL},
{ "fqtoi",   TOK_2OPS,INSTR_FQTOI, IG_FP,FGFCVT(16, 4), BF_3e10, NULL},
{ "fstod",   TOK_2OPS,INSTR_FSTOD, IG_FP,FGFCVT( 4, 8), BF_3e10, NULL},
{ "fstoq",   TOK_2OPS,INSTR_FSTOQ, IG_FP,FGFCVT( 4,16), BF_3e10, NULL},
{ "fdtos",   TOK_2OPS,INSTR_FDTOS, IG_FP,FGFCVT( 8, 4), BF_3e10, NULL},
{ "fdtoq",   TOK_2OPS,INSTR_FDTOQ, IG_FP,FGFCVT( 8,16), BF_3e10, NULL},
{ "fqtod",   TOK_2OPS,INSTR_FQTOD, IG_FP,FGFCVT(16, 8), BF_3e10, NULL},
{ "fqtos",   TOK_2OPS,INSTR_FQTOS, IG_FP,FGFCVT(16, 4), BF_3e10, NULL},

{ "fmovs",   TOK_2OPS,INSTR_FMOVS,  IG_FP,FGFMOV( 4),BF_3e10, NULL},
{ "fnegs",   TOK_2OPS,INSTR_FNEGS,  IG_FP,FGFNEG( 4),BF_3e10, NULL},
{ "fabss",   TOK_2OPS,INSTR_FABSS,  IG_FP,FGFABS( 4),BF_3e10, NULL},

{ "fsqrts",  TOK_2OPS,INSTR_FSQRTS, IG_FP,FGFSQT( 4),BF_3e10, NULL},
{ "fsqrtd",  TOK_2OPS,INSTR_FSQRTD, IG_FP,FGFSQT( 8),BF_3e10, NULL},
{ "fsqrtq",  TOK_2OPS,INSTR_FSQRTQ, IG_FP,FGFSQT(16),BF_3e10, NULL},


{ "fadds",   TOK_3OPS,INSTR_FADDS,  IG_FP,FGFADD( 4),BF_3e11,NULL},
{ "faddd",   TOK_3OPS,INSTR_FADDD,  IG_FP,FGFADD( 8),BF_3e11,NULL},
{ "faddq",   TOK_3OPS,INSTR_FADDQ,  IG_FP,FGFADD(16),BF_3e11,NULL},
{ "fsubs",   TOK_3OPS,INSTR_FSUBS,  IG_FP,FGFSUB( 4),BF_3e11,NULL},
{ "fsubd",   TOK_3OPS,INSTR_FSUBD,  IG_FP,FGFSUB( 8),BF_3e11,NULL},
{ "fsubq",   TOK_3OPS,INSTR_FSUBQ,  IG_FP,FGFSUB(16),BF_3e11,NULL},
{ "fmuls",   TOK_3OPS,INSTR_FMULS,  IG_FP,FGFMUL( 4),BF_3e11,NULL},
{ "fmuld",   TOK_3OPS,INSTR_FMULD,  IG_FP,FGFMUL( 8),BF_3e11,NULL},
{ "fmulq",   TOK_3OPS,INSTR_FMULQ,  IG_FP,FGFMUL(16),BF_3e11,NULL},
{ "fdivs",   TOK_3OPS,INSTR_FDIVS,  IG_FP,FGFDIV( 4),BF_3e11,NULL},
{ "fdivd",   TOK_3OPS,INSTR_FDIVD,  IG_FP,FGFDIV( 8),BF_3e11,NULL},
{ "fdivq",   TOK_3OPS,INSTR_FDIVQ,  IG_FP,FGFDIV(16),BF_3e11,NULL},

{ "fsmuld",  TOK_3OPS,INSTR_FSMULD, IG_FP,FGFMULCVT(4, 8),BF_3e11,NULL},
{ "fdmulq",  TOK_3OPS,INSTR_FDMULQ, IG_FP,FGFMULCVT(8,16),BF_3e11,NULL},

{ "fcmps",   TOK_2OPS,INSTR_FCMPS,  IG_FP|ISG_FCMP,FGFCMP( 4),BF_3e01,NULL},
{ "fcmpd",   TOK_2OPS,INSTR_FCMPD,  IG_FP|ISG_FCMP,FGFCMP( 8),BF_3e01,NULL},
{ "fcmpq",   TOK_2OPS,INSTR_FCMPQ,  IG_FP|ISG_FCMP,FGFCMP(16),BF_3e01,NULL},
{ "fcmpes",  TOK_2OPS,INSTR_FCMPES, IG_FP|ISG_FCMP,FGFCMP( 4),BF_3e01,NULL},
{ "fcmped",  TOK_2OPS,INSTR_FCMPED, IG_FP|ISG_FCMP,FGFCMP( 8),BF_3e01,NULL},
{ "fcmpeq",  TOK_2OPS,INSTR_FCMPEQ, IG_FP|ISG_FCMP,FGFCMP(16),BF_3e01,NULL},

{ "cpop1",   TOK_4OPS,INSTR_CPOP1,  IG_CP|ISG_CP1, FGCP(4,1),BF_3e11_CPOP, NULL},
{ "cpop2",   TOK_4OPS,INSTR_CPOP2,  IG_CP|ISG_CP2, FGCP(4,2),BF_3e11_CPOP, NULL},

	/* pseudo-instructions */
{ "ret",     TOK_0OPS,  INSTR_RET,   IG_NONE,   FGRET, BF_0,   NULL},
{ "retl",    TOK_0OPS,  INSTR_RETL,  IG_NONE,   FGRETL,BF_0,   NULL},
{ "nop",     TOK_0OPS,  INSTR_NOP,   IG_NONE,   FGNOP, BF_0,   NULL},

{ "jmp",     TOK_1OPS,  INSTR_JMPL,  IG_JRI,    FGJMP, BF_3cd10, NULL},

{ "mov",     TOK_2OPS,  INSTR_NONE,  IG_MOV,    FGNONE,BF_NONE,NULL},

{ "tst",     TOK_1OPS,  INSTR_NONE,  IG_TST,    FGNONE,BF_NONE,NULL},

{ "cmp",     TOK_2OPS,  INSTR_NONE,  IG_CMP,       FGNONE,BF_NONE,NULL},
{ "not",     TOK_2OPS,  INSTR_NONE,  IG_NN|ISG_NOT,FGNONE,BF_NONE,NULL},
{ "neg",     TOK_2OPS,  INSTR_NONE,  IG_NN|ISG_NEG,FGNONE,BF_NONE,NULL},
{ "inc",     TOK_2OPS,  INSTR_NONE,  IG_ID|ISG_I,  FGNONE,BF_NONE,NULL},
{ "inccc",   TOK_2OPS,  INSTR_NONE,  IG_ID|ISG_IC, FGNONE,BF_NONE,NULL},
{ "dec",     TOK_2OPS,  INSTR_NONE,  IG_ID|ISG_D,  FGNONE,BF_NONE,NULL},
{ "deccc",   TOK_2OPS,  INSTR_NONE,  IG_ID|ISG_DC, FGNONE,BF_NONE,NULL},
{ "btst",    TOK_2OPS,  INSTR_NONE,  IG_BIT|ISG_BT,FGNONE,BF_NONE,NULL},
{ "bset",    TOK_2OPS,  INSTR_NONE,  IG_BIT|ISG_BS,FGNONE,BF_NONE,NULL},
{ "bclr",    TOK_2OPS,  INSTR_NONE,  IG_BIT|ISG_BC,FGNONE,BF_NONE,NULL},
{ "btog",    TOK_2OPS,  INSTR_NONE,  IG_BIT|ISG_BG,FGNONE,BF_NONE,NULL},
{ "clr",     TOK_1OPS,  INSTR_ST,  IG_CLR|ISG_CLR, FGNONE,BF_NONE,NULL},
{ "clrh",    TOK_1OPS,  INSTR_STH, IG_CLR|ISG_CLRH,FGNONE,BF_NONE,NULL},
{ "clrb",    TOK_1OPS,  INSTR_STB, IG_CLR|ISG_CLRB,FGNONE,BF_NONE,NULL},

{ "set",     TOK_2OPS,  INSTR_NONE, IG_SET, FGSET,        BF_SET, NULL},

{ "fmovd",   TOK_2OPS,INSTR_FMOVS,  IG_FP,FGFMOV( 8),BF_FMOV2, NULL},
{ "fnegd",   TOK_2OPS,INSTR_FNEGS,  IG_FP,FGFNEG( 8),BF_3e10, NULL},
{ "fabsd",   TOK_2OPS,INSTR_FABSS,  IG_FP,FGFABS( 8),BF_3e10, NULL},
{ "fmovq",   TOK_2OPS,INSTR_FMOVS,  IG_FP,FGFMOV(16),BF_FMOV2, NULL}, 
{ "fnegq",   TOK_2OPS,INSTR_FNEGS,  IG_FP,FGFNEG(16),BF_3e10, NULL},
{ "fabsq",   TOK_2OPS,INSTR_FABSS,  IG_FP,FGFABS(16),BF_3e10, NULL}, 	
{ "ld2",     TOK_LD,  INSTR_LDD,  IG_DLD,FGLD(8,0,0),BF_LDST2,NULL},
{ "st2",     TOK_ST,  INSTR_STD,  IG_DST,FGST(8,0), BF_LDST2, NULL},
{ "*ld2",    TOK_NONE,INSTR_LDDF,  IG_DLD,FGLDF(8,0,0),BF_LDST2,NULL},
{ "*st2",    TOK_NONE,INSTR_STDF,  IG_DST,FGSTF(8,0), BF_LDST2, NULL},


	/* pseudo-ops (using n_union.n_u_vlhp) */
{ ".single", TOK_FP_PSEUDO,INSTR_NONE,    1,FGPSV(SGL,1),   BF_PS,NULL},
{ ".double", TOK_FP_PSEUDO,INSTR_NONE,    2,FGPSV(DBL,1),   BF_PS,NULL},
{ ".quad",   TOK_FP_PSEUDO,INSTR_NONE,    4,FGPSV(QUAD,1),   BF_PS,NULL},
{ ".ascii",  TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSV(ASCII,1),  BF_PS,NULL},
{ ".asciz",  TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSV(ASCIZ,1),  BF_PS,NULL},
{ ".global", TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSV(GLOBAL,0), BF_PS,NULL},
{ ".common", TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSV(COMMON,0), BF_PS,NULL},
{ ".reserve",TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSV(RESERVE,0),BF_PS,NULL},
{ ".stabd",  TOK_PSEUDO,INSTR_NONE,      3,FGPSV(STAB,0),   BF_PS,NULL},
{ ".stabn",  TOK_PSEUDO,INSTR_NONE,      4,FGPSV(STAB,0),   BF_PS,NULL},
{ ".stabs",  TOK_PSEUDO,INSTR_NONE,      5,FGPSV(STAB,0),   BF_PS,NULL},
	/* pseudo-ops (using n_union.n_u_flp) */
{ "*.bof",   TOK_NONE,  INSTR_NONE,IG_NONE,FGPSF(BOF,0),    BF_PS,NULL},
	/* pseudo-ops (using nothing in n_union) */
{ ".empty",  TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSN(EMPTY,1),  BF_PS,NULL},
{ ".alias",  TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSN(ALIAS,0),  BF_PS,NULL},
	/* pseudo-ops (using n_union.n_u_string) */
{ ".seg",    TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSS(SEG,0),    BF_PS,NULL},
{ ".optim",  TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSS(OPTIM,0),  BF_PS,NULL},
{ ".ident",  TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSS(IDENT,0),  BF_PS,NULL},
	/* pseudo-ops (using n_union.n_u_ops) */
{ ".byte",   TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSO(BYTE,1),   BF_PS,NULL},
{ ".half",   TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSO(HALF,1),   BF_PS,NULL},
{ ".word",   TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSO(WORD,1),   BF_PS,NULL},
{ "*.cswitch",TOK_NONE, INSTR_NONE,IG_NONE,FGCSWITCH,  BF_CSWITCH,NULL},
{ ".align",  TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSO(ALIGN,0),  BF_PS,NULL},
{ ".skip",   TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSO(SKIP,1),   BF_PS,NULL},
{ ".proc",   TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSO(PROC,0),   BF_PS,NULL},
{ ".noalias",TOK_2OPS,  INSTR_NONE,IG_NOALIAS,FGPSO(NOALIAS,0),BF_PS,NULL},

/* These three are for compatibility with Sun-3 680x0 assemblers...*/
{ ".text",   TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSS(SEG,0),    BF_PS,NULL},
{ ".data",   TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSS(SEG,0),    BF_PS,NULL},
{ ".bss",    TOK_PSEUDO,INSTR_NONE,IG_NONE,FGPSS(SEG,0),    BF_PS,NULL},

{ "*MARKER",          0,INSTR_NONE,  IG_NONE, FGMARKER, BF_MARKER,NULL},
{ "*COMMENT",         0,INSTR_NONE,  IG_NONE, FGCOMMENT,BF_IGNORE,NULL},
{ "*LABEL",           0,INSTR_NONE,  IG_NONE, FGLABEL,  BF_LABEL, NULL},
{ "*EQUATE",          0,INSTR_NONE,  IG_NONE, FGEQUATE, BF_EQUATE,NULL},
{ "*SYM_REF",         0,INSTR_NONE,  IG_NONE, FGSYMREF, BF_IGNORE,NULL},


/* The following mnemonics are synonyms for the basic mnemonics,
 * to make life easier for assembly-language programmers who may find
 * these names more familiar from some other instruction set.
 * They will map immediately to the "real" instruction mnemonics, and
 * will be disassembled using the "real" instruction mnemonics.
 *
 * The pseudo-op synonyms are useful for compatibility with Sun's
 * mc680x0 assemblers.  Programmers sometimes put "asm()" statements
 * in C code and want (expect) them to be machine-independent.
 */
/*
 *{"o_name",o_token,o_instr,o_igroup,o_func,o_binfmt,o_chain,o_synonnym}
 */
{ "beq",     TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "be"    },
{ "blt",     TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "bl"    },
{ "bgt",     TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "bg"    },
{ "bltu",    TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "blu"   },
{ "bgtu",    TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "bgu"   },
{ "beq,a",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "be,a"  },
{ "blt,a",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "bl,a"  },
{ "bgt,a",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "bg,a"  },
{ "bltu,a",  TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "blu,a" },
{ "bgtu,a",  TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "bgu,a" },

{ "teq",     TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "te"    },
{ "tlt",     TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "tl"    },
{ "tgt",     TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "tg"    },
{ "tltu",    TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "tlu"   },
{ "tgtu",    TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "tgu"   },

{ ".globl",  TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL,".global"},
{ ".comm",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL,".common"},
{ ".long",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL,".word"  },

/* The ones listed below are synonyms with a previous version of the 
 * SPARC assembly language.  They remain here for compatibility.
 */
{ "fitox",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "fitoq"},
{ "fxtoi",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "fqtoi"}, 
{ "fstox",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "fstoq"}, 
{ "fdtox",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "fdtoq"}, 
{ "fxtod",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "fqtod"}, 
{ "fxtos",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "fqtos"}, 
{ "fsqrtx",  TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL,"fsqrtq"}, 
{ "faddx",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "faddq"}, 
{ "fsubx",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "fsubq"}, 
{ "fmulx",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "fmulq"}, 
{ "fdivx",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "fdivq"}, 
{ "fdmulx",  TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "fdmulq"}, 
{ "fcmpx",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "fcmpq"}, 
{ "fcmpex",  TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "fcmpeq"}, 
{ "fmovx",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "fmovq"}, 
{ "fnegx",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "fnegq"}, 
{ "fasbx",   TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, "fabsq"}, 
{ ".ext",    TOK_NONE,INSTR_NONE,IG_NONE,FGNONE,BF_NONE,NULL, ".quad"}, 


{ NULL}
};


/*
*	"numbers" tells numeric value of chars which are digits in
*	base 8, 10, or 16.
*
*	(ASCII char set is assumed!)
*/

unsigned char numbers[] =
{
NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
0,	1,	2,	3,	4,	5,	6,	7,
8,	9,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
NOTNUM,	10,	11,	12,	13,	14,	15,	NOTNUM,
NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	10,	11,	12,	13,	14,	15,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,

	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,
	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM,	NOTNUM
};

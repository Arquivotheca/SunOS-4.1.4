/*	@(#)functional_groups.h	1.1	10/31/94	*/

/* This file defines Functional Group macros, used in setting up the opcode
 * tables.  It must correspond to the "struct func_grp" in "structs.h".
 */

/* ----------------------------------------------------------------------------
 * For convenience here (the real authority on this is "structs.h"), the
 * fields of "struct func_grp" comprise:
 *  {
 *	fgrp,fsubgrp,nodeunion,hasdelayslot,setscc,genscode,accesses_fpu,
 *	rs1sz,rs2sz,rdsz, LDSTalt,LDsigned, ARcomm, BRcc,BRannul,
 *  #ifdef CHIPFABS
 *	execs_on_w1164, execs_on_w1165,
 *  #endif
 *  }
 * ----------------------------------------------------------------------------
 */
/* Functional Group macro for beginning-of-node-list Marker node */
#define FGMARKER	\
	{ FG_MARKER,FSG_NONE,NU_STRG,0,0,0,0,    SN,SN,SN,0,0,  0,CN,0}

/* Functional Group macro for Comment node */
#define FGCOMMENT	\
	{ FG_COMMENT,FSG_NONE,NU_STRG,0,0,0,0,   SN,SN,SN,0,0,  0,CN,0}

/* Functional Group macro for Symbol-Reference node */
#define FGSYMREF	\
	{ FG_SYM_REF,FSG_NONE,NU_OPS,0,0,0,0, SN,SN,SN,0,0,  0,CN,0}

/* Functional Group macro for Integer-register LoaD instructions */
#define FGLD(sz,a,si)	\
	{ FG_LD,   FSG_NONE,NU_OPS, 0,0,1,0,SN,SN,sz,a,si, 0,CN,0}

/* Functional Group macro for F.P.-register LoaD instructions */
#define FGLDF(sz,a,si)	\
	{ FG_LD,   FSG_NONE,NU_OPS, 0,0,1,1,SN,SN,sz,a,si, 0,CN,0}

/* Functional Group macro for Integer-register STore instructions */
#define FGST(sz,a)	\
	{ FG_ST,   FSG_NONE,NU_OPS, 0,0,1,0,SN,SN,sz,a, 0, 0,CN,0}

/* Functional Group macro for F.P.-register STore instructions */
#define FGSTF(sz,a)	\
	{ FG_ST,   FSG_NONE,NU_OPS, 0,0,1,1,SN,SN,sz,a, 0, 0,CN,0}

#ifdef CHIPFABS	/* for fab#1 chip only */
  /* Functional Group macro for atomic LoaD-STore instructions */
# define FGLDST(a)	\
	{ FG_LDST, FSG_LSW, NU_OPS, 0,0,1,0,SN,SN,4,a, 0, 0, CN,0}
#endif /*CHIPFABS*/

/* Functional Group macro for atomic LoaD-STore Byte instructions */
#define FGLDSTUB(a)	\
	{ FG_LDST, FSG_LSB, NU_OPS, 0,0,1,0,SN,SN,1,a, 0, 0, CN,0}

/* Functional Group macro for SWAP instructions */
#define FGSWAP(a)	\
	{ FG_LDST, FSG_SWAP,NU_OPS, 0,0,1,0,SN,SN,4,a, 0, 0, CN,0}

/* Functional Group macro for Arithmetic instructions */
#define FGA(sg,cc,cm)	\
	{ FG_ARITH,FSG_/**/sg,NU_OPS, 0,cc,1,0,SN,SN,SN,0,0, cm, CN,0 }

/* Functional Group macro for Arithmetic instructions */
#define FGAX(sg,cc,cm)	\
	{ FG_ARITH,FSG_/**/sg,NU_OPS, 0,cc,1,0,SN,SN,SN,0,0, cm, CC_I,0 }

#define FGCP(sz,sg) \
       { FG_CP, FSG_CP/**/sg, NU_OPS, 0,0,1,0, sz, sz, sz, 0,0,0,0,0 }

/* Functional Group macros for Floating-Point instructions */
#ifdef CHIPFABS	/* ====== for fab#1 FPU bug workaround, only */
	/* FMUL executes on Weitek 1164 chip;
	 * FMOV, FABS, and FNEG execute on the FPU itself (no Weitek chip),
	 * and the rest execute on the Weitek 1165 chip.
	 */
# define FGFCMP(sz)	\
	{ FG_FLOAT2,  FSG_FCMP,NU_OPS, 0,1,1,1,  sz, sz, SN,0,0,  0,CN,0,0,1 }
# define FGFP2(sg,sz1,sz2,szd)	\
	{ FG_FLOAT2,FSG_/**/sg,NU_OPS, 0,0,1,1, sz1,sz2,szd,0,0,  0,CN,0,0,1 }
# define FGFP3(sg,sz1,sz2,szd,cm)	\
	{ FG_FLOAT3,FSG_/**/sg,NU_OPS, 0,0,1,1, sz1,sz2,szd,0,0, cm,CN,0,0,1 }
# define FGFMOV(sz)	\
	{ FG_FLOAT2,  FSG_FMOV,NU_OPS, 0,0,1,1,  SN, sz, sz,0,0,  0,CN,0,0,0 }
# define FGFNEG(sz)	\
	{ FG_FLOAT2,  FSG_FNEG,NU_OPS, 0,0,1,1,  SN, sz, sz,0,0,  0,CN,0,0,0 }
# define FGFABS(sz)	\
	{ FG_FLOAT2,  FSG_FABS,NU_OPS, 0,0,1,1,  SN, sz, sz,0,0,  0,CN,0,0,0 }
# define FGFMUL(sz)	\
	{ FG_FLOAT3,  FSG_FMUL,NU_OPS, 0,0,1,1,  sz, sz, sz,0,0,  1,CN,0,1,0 }
# define FGFMULCVT(s1,s2)	\
	{ FG_FLOAT3,  FSG_FMUL,NU_OPS, 0,0,1,1,  s1, s1, s2,0,0,  1,CN,0,1,0 }
#else /*======CHIPFABS======*/
# define FGFCMP(sz)	\
	{ FG_FLOAT2,  FSG_FCMP,NU_OPS, 0,1,1,1, sz,sz,SN,0,0,     0, CN,0 }
# define FGFP2(sg,sz1,sz2,szd)	\
	{ FG_FLOAT2,FSG_/**/sg,NU_OPS, 0,0,1,1, sz1,sz2,szd,0,0,  0, CN,0 }
# define FGFP3(sg,sz1,sz2,szd,cm)	\
	{ FG_FLOAT3,FSG_/**/sg,NU_OPS, 0,0,1,1, sz1,sz2,szd,0,0, cm, CN,0 }
# define FGFMOV(sz)	FGFP2(FMOV, SN,sz,sz)
# define FGFNEG(sz)	FGFP2(FNEG, SN,sz,sz)
# define FGFABS(sz)	FGFP2(FABS, SN,sz,sz)
# define FGFMUL(sz)	FGFP3(FMUL, sz,sz,sz, 4)
# define FGFMULCVT(s1,s2)	FGFP3(FMUL, s1,s1,s2, 4)
#endif /*======CHIPFABS======*/

#define FGFCVT(sz2,szd)	FGFP2(FCVT, SN,sz2,szd)
#define FGFSQT(sz)	FGFP2(FSQT, SN,sz, sz)

#define FGFADD(sz)	FGFP3(FADD, sz,sz,sz, 1)
#define FGFSUB(sz)	FGFP3(FSUB, sz,sz,sz, 0)
#define FGFDIV(sz)	FGFP3(FDIV, sz,sz,sz, 0)

/* Functional Group macro for SETHI instructions */
#define FGSETHI		\
	{ FG_SETHI,  FSG_NONE,NU_OPS, 0,0,1,0, SN,SN,SN,0,0, 0, CN,0 }

/* Functional Group macros for BRanch instructions */
	/* Branch Never or Always (no dependence on CC's) */
#define FGBR(sg)	\
	{ FG_BR,   FSG_/**/sg,NU_OPS, 1,0,1,0, SN,SN,SN,0,0, 0, CN,  0}

	/* Branch Never or Always (no dependence on CC's), with Annul */
#define FGBR_A(sg)	\
	{ FG_BR,   FSG_/**/sg,NU_OPS, 1,0,1,0, SN,SN,SN,0,0, 0, CN,  1}

	/* Branch on ICC */
#define FGBRI(sg)	\
	{ FG_BR,   FSG_/**/sg,NU_OPS, 1,0,1,0, SN,SN,SN,0,0, 0, CC_I,0}

	/* Branch on ICC with Annul */
#define FGBRI_A(sg)	\
	{ FG_BR,   FSG_/**/sg,NU_OPS, 1,0,1,0, SN,SN,SN,0,0, 0, CC_I,1}

	/* Branch on FCC */
#define FGBRF(sg)	\
	{ FG_BR,   FSG_/**/sg,NU_OPS, 1,0,1,1, SN,SN,SN,0,0, 0, CC_F,0}

	/* Branch on FCC with Annul */
#define FGBRF_A(sg)	\
	{ FG_BR,   FSG_/**/sg,NU_OPS, 1,0,1,1, SN,SN,SN,0,0, 0, CC_F,1}

	/* Branch on CCC (Coprocessor CC's) */
#define FGBRC(sg)	\
	{ FG_BR,   FSG_/**/sg,NU_OPS, 1,0,1,0, SN,SN,SN,0,0, 0, CC_C,0}

	/* Branch on CCC (Coprocessor CC's) with Annul */
#define FGBRC_A(sg)	\
	{ FG_BR,   FSG_/**/sg,NU_OPS, 1,0,1,0, SN,SN,SN,0,0, 0, CC_C,1}

/* Ticc: Trap on ICC's */
#define FGTRAPI		\
	{ FG_TRAP,   FSG_NONE,NU_OPS, 0,0,1,0, SN,SN,SN,0,0, 0, CC_I,0}

/* Ticc: Trap Never or Always (no dependence on CC's) */
#define FGTRAP		\
	{ FG_TRAP,   FSG_NONE,NU_OPS, 0,0,1,0, SN,SN,SN,0,0, 0, CN,0 }

/* Functional Group macros for other instructions */
#define FGCALL		\
	{ FG_CALL,   FSG_NONE,NU_OPS, 1,0,1,0, SN,SN,SN,0,0, 0, CN,0 }
#define FGJMP		\
	{ FG_JMP,    FSG_NONE,NU_OPS, 1,0,1,0, SN,SN,SN,0,0, 0, CN,0 }
#define FGJMPL		\
	{ FG_JMPL,   FSG_NONE,NU_OPS, 1,0,1,0, SN,SN,SN,0,0, 0, CN,0 }
#define FGRETT		\
	{ FG_RETT,   FSG_NONE,NU_OPS, 1,0,1,0, SN,SN,SN,0,0, 0, CN,0 }
#define FGIFLUSH	\
	{ FG_IFLUSH, FSG_NONE,NU_OPS, 0,0,1,0, SN,SN,SN,0,0, 0, CN,0 }
#define FGUNIMP		\
	{ FG_UNIMP,  FSG_NONE,NU_OPS, 0,0,1,0, SN,SN,SN,0,0, 0, CN,0 }
#define FGRD(sg)	\
	{ FG_RD, FSG_RD/**/sg,NU_OPS, 0,0,1,0, SN,SN,SN,0,0, 0, CN,0 }
#define FGWR(sg)	\
	{ FG_WR, FSG_WR/**/sg,NU_OPS, 0,0,1,0, SN,SN,SN,0,0, 0, CN,0 }
#define FGNOP		\
	{ FG_NOP,    FSG_NONE,NU_NONE,0,0,1,0, SN,SN,SN,0,0, 0, CN,0 }
#define FGRET		\
	{ FG_RET,    FSG_RET, NU_NONE,1,0,1,0, SN,SN,SN,0,0, 0, CN,0 }
#define FGRETL		\
	{ FG_RET,    FSG_RETL,NU_NONE,1,0,1,0, SN,SN,SN,0,0, 0, CN,0 }
#define FGLABEL		\
	{ FG_LABEL,  FSG_NONE,NU_OPS, 0,0,0,0, SN,SN,SN,0,0, 0, CN,0 }
#define FGSET		\
	{ FG_SET,    FSG_NONE,NU_OPS, 0,0,1,0, SN,SN,SN,0,0, 0, CN,0 }
#define FGEQUATE	\
	{ FG_EQUATE, FSG_NONE,NU_OPS, 0,0,0,0, SN,SN,SN,0,0, 0, CN,0 }
#define FGPSN(sg,gc)	\
	{ FG_PS,   FSG_/**/sg,NU_NONE,0,0,gc,0,SN,SN,SN,0,0, 0, CN,0 }
#define FGPSO(sg,gc)	\
	{ FG_PS,   FSG_/**/sg,NU_OPS, 0,0,gc,0,SN,SN,SN,0,0, 0, CN,0 }
#define FGPSF(sg,gc)	\
	{ FG_PS,   FSG_/**/sg,NU_FLP, 0,0,gc,0,SN,SN,SN,0,0, 0, CN,0 }
#define FGPSV(sg,gc)	\
	{ FG_PS,   FSG_/**/sg,NU_VLHP,0,0,gc,0,SN,SN,SN,0,0, 0, CN,0 }
#define FGPSS(sg,gc)	\
	{ FG_PS,   FSG_/**/sg,NU_STRG,0,0,gc,0,SN,SN,SN,0,0, 0, CN,0 }
#define FGCSWITCH	\
	{ FG_CSWITCH,FSG_NONE,NU_OPS, 0,0,1,0, SN,SN,SN,0,0, 0, CN,0 }
#define FGNONE		\
	{ FG_NONE,   FSG_NONE,NU_NONE,0,0,0,0, SN,SN,SN,0,0, 0, CN,0 }

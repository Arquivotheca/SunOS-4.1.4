/* "@(#)sr_processor.h 1.1 94/10/31 Copyr 1986 Sun Micro";
 * From Will Brown's "sas" Sparc Architectural Simulator,
 * Version "@:-)processor.h	3.1 11/11/85
 */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* processor.h renamed to sr_processor.h
	Descriptions and declarations for the processor registers including:

	the overlapping window registers	reg(n)
	the floating point registers		freg(n)
	the Program counters			pc, npc
	the Y register				yr
	The Window Invald Mask Register		wim
	the Processor Status Register		psr
	the Trap Base Register			tbr
	the Floating-point Status Register	fsr
*/

/* processor implementation number and version number */

extern int IMPLEMENTATION;
extern int VERSION;

/* The R registers (window registers) */

/* architecture parameters. */

#define MAX_NW		32		/* maximum number of register windows */
extern int R_NW;			/* default number of register windows */
#define R_NGLOBALS	8		/* number of global registers */
#define R_NOVERLAP	8		/* number of ins and outs */
#define R_NLOCALS	8		/* number of local registers */

#define R_NREGISTERS	(R_NGLOBALS + R_NW*(R_NOVERLAP + R_NLOCALS))

extern Word *regPtrArray[MAX_NW][R_NGLOBALS + 2*R_NOVERLAP + R_NLOCALS];
extern Word regFile[R_NGLOBALS + MAX_NW*(R_NOVERLAP + R_NLOCALS)];

#define winReg(w, n)	(*regPtrArray[w][n])
#define reg(n)		winReg(CWP, n)

/* The F Registers for floating-point */

#define F_NR		32		/* number of F-registers */
extern Word floatReg[F_NR];
#define freg(n)		(floatReg[n])

/* The program counters */
extern Word pc, npc;

/* the Y register */
extern Word yr;

/* The WIM */

extern Word wim;

#ifdef LOW_END_FIRST

struct wimStruct {
	u_int vwim;
};

#else

struct wimStruct {
	u_int vwim;
};

#endif

#define X_WIM(x)	( ((struct wimStruct *)&(x))->vwim )
#define WIM		X_WIM(wim)

/* The PSR */

extern Word psr;

#ifdef LOW_END_FIRST

struct psrStruct {
	u_int cwp		:5;	/* saved window pointer */
	u_int et		:1;	/* enable traps */
	u_int psm		:1;	/* previous s */
	u_int sm		:1;	/* supervisor mode */
	u_int pil		:4;	/* processor interrupt level */
	u_int df		:1;	/* disable floating-point */
	u_int icc		:4;	/* the integer condition codes */
	u_int psr_reserved	:7;	/* not used currently */
	u_int ver		:4;	/* implementation version number */
	u_int impl		:4;	/* implementation */
};

#else

struct psrStruct {
	u_int impl		:4;	/* implementation */
	u_int ver		:4;	/* implementation version number */
	u_int psr_reserved	:7;	/* not used currently */
	u_int icc		:4;	/* the integer condition codes */
	u_int df		:1;	/* disable floating-point */
	u_int pil		:4;	/* processor interrupt level */
	u_int sm		:1;	/* supervisor mode */
	u_int psm		:1;	/* previous s */
	u_int et		:1;	/* enable traps */
	u_int cwp		:5;	/* saved window pointer */
};

#endif

/* ICC sub-fields */

#ifdef LOW_END_FIRST

struct iccStruct {
	u_int cwp		:5;	/* saved window pointer */
	u_int et		:1;	/* enable traps */
	u_int psm		:1;	/* previous s */
	u_int sm		:1;	/* supervisor mode */
	u_int pil		:4;	/* processor interrupt level */
	u_int df		:1;	/* disable floating-point */

	u_int carry		:1;	/* carry */
	u_int over		:1;	/* overflow */
	u_int zero		:1;	/* zero */
	u_int neg		:1;	/* negative */

	u_int psr_reserved	:7;	/* not used currently */
	u_int ver		:4;	/* implementation version number */
	u_int impl		:4;	/* implementation */
};

#else

struct iccStruct {
	u_int impl		:4;	/* implementation */
	u_int ver		:4;	/* implementation version number */
	u_int psr_reserved	:7;	/* not used currently */
					
	u_int neg		:1;	/* negative */
	u_int zero		:1;	/* zero */
	u_int over		:1;	/* overflow */
	u_int carry		:1;	/* carry */

	u_int df		:1;	/* disable floating-point */
	u_int pil		:4;	/* processor interrupt level */
	u_int sm		:1;	/* supervisor mode */
	u_int psm		:1;	/* previous s */
	u_int et		:1;	/* enable traps */
	u_int cwp		:5;	/* saved window pointer */
};

#endif

/* extraction macros. These are portable, but don't work for registers */
/* using sun's C compiler for the 68010, they are quite efficient */

#define X_PSR_RES(x)	( ((struct psrStruct *)&(x))->psr_reserved )
#define PSR_RES		X_PSR_RES(psr)

#define X_CWP(x)	( ((struct psrStruct *)&(x))->cwp )
#define CWP		X_CWP(psr)

#define X_ET(x)		( ((struct psrStruct *)&(x))->et )
#define ET		X_ET(psr)

#define X_PSM(x)	( ((struct psrStruct *)&(x))->psm )
#define PSM		X_PSM(psr)

#define X_SM(x)		( ((struct psrStruct *)&(x))->sm )
#define SM		X_SM(psr)

#define X_PIL(x)	( ((struct psrStruct *)&(x))->pil )
#define PIL		X_PIL(psr)

#define X_DF(x)		( ((struct psrStruct *)&(x))->df )
#define DF		X_DF(psr)

#define X_ICC(x)	( ((struct psrStruct *)&(x))->icc )
#define ICC		X_ICC(psr)

#define X_VER(x)	( ((struct psrStruct *)&(x))->ver )
#define VER		X_VER(psr)

#define X_IMPL(x)	( ((struct psrStruct *)&(x))->impl )
#define IMPL		X_IMPL(psr)

#define X_NEG(x)	( ((struct iccStruct *)&(x))->neg )
#define NEG		X_NEG(psr)

#define X_ZERO(x)	( ((struct iccStruct *)&(x))->zero )
#define ZERO		X_ZERO(psr)

#define X_OVER(x)	( ((struct iccStruct *)&(x))->over )
#define OVER		X_OVER(psr)

#define X_CARRY(x)	( ((struct iccStruct *)&(x))->carry )
#define CARRY		X_CARRY(psr)

/* The Trap Base (TBR) register */

extern Word tbr;

#ifdef LOW_END_FIRST

struct tbrStruct {
	u_int tbr_zero		:4;	/* four trailing zeros */
	u_int tt		:8;	/* trap type */
	u_int tba		:20;	/* trap base address */
};

#else

struct tbrStruct {
	u_int tba		:20;	/* trap base address */
	u_int tt		:8;	/* trap type */
	u_int tbr_reserved	:4;	/* four trailing zeros */
};

#endif

#define X_TT(x)		( ((struct tbrStruct *)&(x))->tt )
#define TT		X_TT(tbr)

#define X_TBA(x)	( ((struct tbrStruct *)&(x))->tba )
#define TBA		X_TBA(tbr)

#define X_TBR_RES(x)	( ((struct tbrStruct *)&(x))->tbr_reserved )
#define TBR_RES		X_TBR_RES(tbr)


/* The FSR */

Word fsr;

#ifdef LOW_END_FIRST

struct fsrStruct {
	u_int cexc		:5;	/* current exceptions */
	u_int fcc		:2;	/* floating condition codes */
	u_int qne		:1;	/* queue not empty */
	u_int ftt		:3;	/* floating-point trap type */
	u_int pr		:1;	/* something to do with FREM, FQUOT */
	u_int fsr_reserved	:15;	/* currently unused */
	u_int nxe		:1;	/* inexact exception enable */
	u_int rp		:2;	/* rounding precision (extended) */
	u_int rnd		:2;	/* rounding direction */
};

#else

struct fsrStruct {
	u_int rnd		:2;	/* rounding direction */
	u_int rp		:2;	/* rounding precision (extended) */
	u_int nxe		:1;	/* inexact exception enable */
	u_int fsr_reserved	:15;	/* currently unused */
	u_int pr		:1;	/* something to do with FREM, FQUOT */
	u_int ftt		:3;	/* floating-point trap type */
	u_int qne		:1;	/* queue not empty */
	u_int fcc		:2;	/* floating condition codes */
	u_int cexc		:5;	/* current exceptions */
};

#endif

#define X_RND(x)	( ((struct fsrStruct *)&(x))->rnd )
#define RND		X_RND(fsr)

#define X_RP(x)		( ((struct fsrStruct *)&(x))->rp )
#define RP		X_RP(fsr)

#define X_NXE(x)	( ((struct fsrStruct *)&(x))->nxe )
#define NXE		X_NXE(fsr)

#define X_FSR_RES(x)	( ((struct fsrStruct *)&(x))->fsr_reserved )
#define FSR_RES		X_FSR_RES(fsr)

#define X_PR(x)	( ((struct fsrStruct *)&(x))->pr )
#define PR		X_PR(fsr)

#define X_FTT(x)	( ((struct fsrStruct *)&(x))->ftt )
#define FTT		X_FTT(fsr)

/* values for the FTT field of the FSR */
#define FTT_NO_TRAP	0
#define FTT_IEEE	1
#define FTT_UNFINISHED	2
#define FTT_UNIMP	3
#define FTT_FPU_ERROR	4

#define X_QNE(x)	( ((struct fsrStruct *)&(x))->qne )
#define QNE		X_QNE(fsr)

#define X_FCC(x)	( ((struct fsrStruct *)&(x))->fcc )
#define FCC		X_FCC(fsr)

/* possible values for the FCC field, and their meanings */
#define FCC_FEQ		0	/* Floating equal */
#define FCC_FLT		1	/* Floating Less Than */
#define FCC_FGT		2	/* Floating Greater Than */
#define FCC_FU		3	/* Floating Unordered */

#define X_CEXC(x)	( ((struct fsrStruct *)&(x))->cexc )
#define CEXC		X_CEXC(fsr)

/* values for the CEXC field */
#define FEXC_NV		0x10
#define FEXC_OF		0x08
#define FEXC_UF		0x04
#define FEXC_DZ		0x02
#define FEXC_NX		0x01

/* the following sub-field struct further reduces cexc
	into component subfields, and provides extraction macros */

#ifdef LOW_END_FIRST

struct fsrSubStruct {
	u_int nx		:1;	/* inexact */
	u_int dz		:1;	/* Divide-by-zero */
	u_int uf		:1;	/* underflow */
	u_int of		:1;	/* overflow */
	u_int nv		:1;	/* invalid */

	u_int fcc		:2;	/* floating condition codes */
	u_int qne		:1;	/* queue not empty */
	u_int ftt		:3;	/* floating-point trap type */
	u_int pr		:1;	/* something to do with FREM, FQUOT */
	u_int fsr_reserved	:15;	/* currently unused */
	u_int nxe		:1;	/* inexact exception enable */
	u_int rp		:2;	/* rounding precision (extended) */
	u_int rnd		:2;	/* rounding direction */
};

#else

struct fsrSubStruct {
	u_int rnd		:2;	/* rounding direction */
	u_int rp		:2;	/* rounding precision (extended) */
	u_int nxe		:1;	/* inexact exception enable */
	u_int fsr_reserved	:15;	/* currently unused */
	u_int pr		:1;	/* something to do with FREM, FQUOT */
	u_int ftt		:3;	/* floating-point trap type */
	u_int qne		:1;	/* queue not empty */
	u_int fcc		:2;	/* floating condition codes */

	u_int nv		:1;	/* invalid */
	u_int of		:1;	/* overflow */
	u_int uf		:1;	/* underflow */
	u_int dz		:1;	/* Divide-by-zero */
	u_int nx		:1;	/* inexact */
};

#endif

#define X_NV(x)		( ((struct fsrSubStruct *)&(x))->nv )
#define NV		X_NV(fsr)

#define X_OF(x)		( ((struct fsrSubStruct *)&(x))->of )
#define OF		X_OF(fsr)

#define X_UF(x)		( ((struct fsrSubStruct *)&(x))->uf )
#define UF		X_UF(fsr)

#define X_DZ(x)		( ((struct fsrSubStruct *)&(x))->dz )
#define DZ		X_DZ(fsr)

#define X_NX(x)		( ((struct fsrSubStruct *)&(x))->nx )
#define NX		X_NX(fsr)

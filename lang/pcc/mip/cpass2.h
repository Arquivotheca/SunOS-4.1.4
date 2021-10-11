/*	@(#)cpass2.h 1.1 94/10/31 SMI	*/

# include "mip.h"
# include "machdep.h"
# include "machdep2.h"

# ifdef ONEPASS
/*	bunch of stuff for putting the passes together... */
# define where where2
# define xdebug x2debug
# define tdebug t2debug
# define edebug e2debug
# define eprint e2print
# define filename ftitle
# endif

# define branch p2branch
# define getlab p2getlab
# define deflab p2deflab

/*	cookies, used as arguments to codgen */

# define FOREFF       01 /* compute for effects only */
# define INAREG       02 /* compute into a register */
# define INTAREG      04 /* compute into a scratch register */
# define INBREG      010 /* compute into a lvalue register */
# define INTBREG     020 /* compute into a scratch lvalue register */
# define INCREG      040 /* compute into a floating register */
# define INTCREG    0100 /* compute into a scratch floating register */
# define FORCC      0200 /* compute for condition codes only */
# define INTEMP  0200000 /* compute into a temporary location */
# define FORARG  0400000 /* compute for an argument of a function */
# define FORREW 01000000 /* search the table, looking for a rewrite rule */

	/* OP descriptors */
	/* the ASG operator may be used on some of these */

# define OPSIMP  010000 /* &, |, ^       */
# define OPCOMM  010002 /* +, &, |, ^    */
# define OPMUL   010004 /* *, /          */
# define OPDIV   010006 /* /, %          */
# define OPUNARY 010010 /* unary ops     */
# define OPLEAF  010012 /* leaves        */
# define OPANY   010014 /* any op...     */
# define OPLOG   010016 /* logical ops   */
# define OPFLOAT 010020 /* +, -, *, or / (for floats) */
# define OPSHFT  010022 /* <<, >>        */
# define OPLTYPE 010024 /* leaf type nodes (e.g, NAME, ICON, etc. ) */
# define OPINTR  010026 /* inline-compiled math intrinsics */
# define OPLIBR  010030 /* library math intrinsics */

	/* match returns */

# define MNOPE 010000
# define MDONE 010001

	/* shapes */

# define SANY     01	/* same as FOREFF   */
# define SAREG    02	/* same as INAREG   */
# define STAREG   04	/* same as INTAREG  */
# define SBREG   010	/* same as INBREG   */
# define STBREG  020	/* same as INTBREG  */
# define SCREG   040	/* same as INCREG   */
# define STCREG 0100	/* same as INTCREG  */
# define SCC    0200	/* same as FORCC    */
# define SNAME  0400
# define SCON  01000
# define SFLD  02000
# define SOREG 04000
/* indirection or wild card shapes */
# ifndef WCARD1
# define STARNM  010000
# endif
# ifndef WCARD2
# define STARREG 020000
# endif
# define SWADD   040000
# define SPECIAL 0100000
# define SZERO   SPECIAL
# define SONE   (SPECIAL|1)
# define SMONE  (SPECIAL|2)
# define SCCON  (SPECIAL|3)	/* -256 <= constant < 256 */
# define SSCON  (SPECIAL|4)	/* -32768 <= constant < 32768 */
# define SSOREG (SPECIAL|5)	/* non-indexed OREG */

	/* FORARG and INTEMP are carefully not conflicting with shapes */

	/* types */

# define TCHAR 01
# define TSHORT 02
# define TINT 04
# define TLONG 010
# define TFLOAT 020
# define TDOUBLE 040
# define TPOINT 0100
# define TUCHAR 0200
# define TUSHORT 0400
# define TUNSIGNED 01000
# define TULONG 02000
# define TPTRTO 04000  /* pointer to one of the above */
# define TANY 010000  /* matches anything within reason */
# define TSTRUCT 020000   /* structure or union */

	/* reclamation cookies */

# define RNULL 0    /* clobber result */
# define RLEFT 01
# define RRIGHT 02
# define RESC1 04
# define RESC2 010
# define RESC3 020
# define RESCC 04000
# define RESFCC 010000 /* floating coprocessor condition code */
# define RNOP 020000   /* DANGER: can cause loops.. */

	/* needs */

/*
 * The following #defines describe a structure that might be
 * expressed more clearly as a packed array of bitfields, if
 * such a thing existed in C.  Lacking such a facility, we
 * have the following:
 */

# define NAREG	  (0x1)		/* 1 unit of measurement in the A-regs */
# define NACOUNT  (0x3)		/* [1..7] => # A-regs or A-reg pairs needed */
# define NASL	  (0x8)		/* 1 => may share A-reg from left subtree */
# define NASR	  (0x10)	/* 1 => may share A-reg from right subtree */
# define NAPAIR	  (0x20)	/* 1 => even/odd A-reg pair required */
# define NAPAIRO  (0x40)	/* 1 => unaligned A-reg pair required */
# define NAMASK	  (0xff)	/* mask for A-regs 'needs' descriptor */

# define NBREG	  (NAREG<<8)	/* 1 unit of measurement in the B-regs */
# define NBCOUNT  (NACOUNT<<8)	/* [1..7] => # B-regs or B-reg pairs needed */
# define NBSL	  (NASL<<8)	/* 1 => may share B-reg from left subtree */
# define NBSR	  (NASR<<8)	/* 1 => may share B-reg from right subtree */
# define NBPAIR	  (NAPAIR<<8)	/* 1 => even/odd B-reg pair required */
# define NBMASK	  (NAMASK<<8)	/* mask for B-regs 'needs' descriptor */

# define NCREG	  (NAREG<<16)	/* 1 unit of measurement in the C-regs */
# define NCCOUNT  (NACOUNT<<16)	/* [1..7] => # C-regs or C-reg pairs needed */
# define NCSL	  (NASL<<16)	/* 1 => may share C-reg from left subtree */
# define NCSR	  (NASR<<16)	/* 1 => may share C-reg from right subtree */
# define NCPAIR	  (NAPAIR<<16)	/* 1 => even/odd C-reg pair required */
# define NCMASK	  (NAMASK<<16)	/* mask for C-regs 'needs' descriptor */

# define NTEMP    (0x01000000)	/* 1 unit of measurement in temp space */
# define NTMASK   (0x0f000000)	/* [1..15] => # words of temps needed */
# define REWRITE  (0x10000000)	/* 1 => rewrite the tree */
# define EITHER   (0x20000000)	/* "either" modifier for needs */

	/* .rall modifiers */

# define MUSTDO 010000   /* force register requirements */
# define NOPREF 020000  /* no preference for register assignment */

	/* register allocation */

extern int rstatus[];
extern int busy[];

extern struct respref { int cform; int mform; } respref[];

# define isareg(r) (rstatus[r]&SAREG)
# define isbreg(r) (rstatus[r]&SBREG)
# define iscreg(r) (rstatus[r]&SCREG)
# define istreg(r) (rstatus[r]&(STBREG|STAREG|STCREG))
# define istnode(p) (p->in.op==REG && istreg(p->tn.rval))

# define TBUSY 01000
# define REGLOOP(i) for(i=0;i<REGSZ;++i)

extern int stocook;
# define DELAYS 20
extern NODE *deltrees[DELAYS];
extern int deli;   /* mmmmm */

extern NODE *stotree;
extern int callflag;

extern struct optab {
	int op;
	int visit;
	int lshape;
	int ltype;
	int rshape;
	int rtype;
	int needs;
	int rewrite;
	char * cstring;
	}
	table[];

extern NODE resc[];

extern OFFSZ tmpoff;
extern OFFSZ maxoff;
extern OFFSZ baseoff;
extern OFFSZ maxtemp;
extern int maxtreg, maxfreg;
extern int ftnno;
extern int rtyflg;

extern int nrecur;  /* flag to keep track of recursions */

# define NRECUR 0x10000

#define ncopy(dest,srce) (*(dest) = *(srce))

extern NODE
	*talloc(),
	*eread(),
	*tcopy(),
	*getlr();

extern CONSZ rdin();

extern int eprint();

extern char *rnames[];

extern int lineno;
extern char *filename;
extern int fldshf, fldsz;
extern int lflag, xdebug, udebug, edebug, odebug, rdebug, radebug, tdebug, sdebug;

/* flags to generate position-indepent code */
extern int picflag;
extern int smallpic;

#if TARGET != I386
extern void picode();
#endif

/* flag to generate mis-aligned ld/st */
extern int malign_flag;

/* flag to generate ldd/std */
extern int dalign_flag;

#ifndef callchk
#define callchk(x) allchk()
#endif

#ifndef PUTCHAR
# define PUTCHAR(x) putchar(x)
#endif

	/* macros for doing double indexing */
# define R2PACK(x,y,z) (0200*((x)+1)+y+040000*z)
# define R2UPK1(x) ((((x)>>7)-1)&0177)
# define R2UPK2(x) ((x)&0177)
# define R2UPK3(x) (x>>14)
# define R2TEST(x) ((x)>=0200)

# ifdef MULTILEVEL

union mltemplate{
	struct ml_head{
		int tag; /* identifies class of tree */
		int subtag; /* subclass of tree */
		union mltemplate * nexthead; /* linked by mlinit() */
		} mlhead;
	struct ml_node{
		int op; /* either an operator or op description */
		int nshape; /* shape of node */
		/* both op and nshape must match the node.
		 * where the work is to be done entirely by
		 * op, nshape can be SANY, visa versa, op can
		 * be OPANY.
		 */
		int ntype; /* type descriptor from mfile2 */
		} mlnode;
	};

# endif

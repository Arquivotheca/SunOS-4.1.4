
/*	@(#)cpass1.h 1.1 94/10/31 SMI; from S5R2 1.8	*/

/*
 * Guy's changes from System V lint (12 Apr 88):
 * Updated "cpass1.h".  Variable added as part of S5 fix to eliminate
 * bogus "statement not reached" errors.
 */

# include "machdep.h"
# include "mip.h"
# include "symtab.h"

# ifdef BROWSER
# include <cb_ccom.h>
# endif

# define RELEASE 4.0

/*	location counters */
# define PROG 0
# define DATA 1
# define ADATA 2
# define STRNG 3
# define ISTRNG 4
# define STAB 5
# define BSS 6

# ifndef BUG1
extern char *scnames();
# endif

# ifndef FIXDEF
# define FIXDEF(p)
# endif
# ifndef FIXARG
# define FIXARG(p)
# endif
# ifndef FIXSTRUCT
# define FIXSTRUCT(a,b)
# endif

	/* alignment of initialized quantities */
# ifndef AL_INIT
# define AL_INIT ALINT
# endif

extern end;

/*
 * symbol table internals
 */

# define FIELD 0100
# define FLDSIZ 077

/* symbol table flags */

# define SMOS 01
# define SHIDDEN 02
# define SHIDES 04
# define SSET 010
# define SREF 020
# define SNONUNIQ 040
# define STAG 0100
# define SPRFORWARD 0200
# define SADDRESS 0400
# define S_UNKNOWN_CONTROL_FLOW 01000
# define S_MAKES_REGS_INCONSISTENT 02000
# define S_NO_REG 04000

# define symtab pcc_symtab

typedef struct list LIST;	/* details known only to IR routines */

struct symtab {
	char *sname;
	TWORD stype;  /* type word */
	char sclass;  /* storage class */
	char slevel;  /* scope level */
	short sflags; /* flags for set, use, hidden, mos, etc. */
	int offset;   /* offset or value */
	short dimoff; /* offset into the dimension table */
	short sizoff; /* offset into the size table */
	short suse;   /* line number of last use of the variable */
	int hashVal;  /* hash code computed for this symbol */
	char *sfile;  /* file we first saw symbol in */
        struct symtab *next;  /* link to next colliding symbol */
	LIST *symrefs; /* list of IR leaf nodes referring to this symbol */
	int  svarno;   /* variable id number, for aliasing computations */
};

extern struct symtab *stab[];

# ifdef ONEPASS
/* NOPREF must be defined for use in first pass tree machine */
# define NOPREF 020000  /* no preference for register assignment */
# endif

extern int optimize;
extern int longswtab;
extern struct sw *swtab;
extern struct sw *swp;
extern int swx;

extern int ftnno;
extern int blevel;
extern int instruct, stwart;

extern int lineno, nerrors;
#ifdef BROWSER
struct Expr {
	NODE * nodep;
	struct Cb_expr_tag * expr;
};
struct Name {
	int	id;
	int	lineno;
};
struct Type {
	NODE	*nodep;
	struct symtab *typdef;
	int	lineno;
};
struct Intval2 {
	int	i1;
	int	i2;
};
#endif
typedef union {
	int intval;
	NODE * nodep;
#ifdef BROWSER
	struct Intval2 intval2;
	struct Type type;
	struct Expr expr;
	struct Name name;
#endif
	} YYSTYPE;
extern YYSTYPE yylval;

extern CONSZ lastcon;
extern double dcon;

extern char *ftitle;
extern char  ititle[];
extern struct symtab *stab[];
extern int curftn;
extern int curclass;
extern int curdim;
extern int *dimtab;
extern int *paramstk;
extern int paramno;
extern int autooff, argoff, strucoff;
extern int regvar;
extern int minrvar;
extern int brkflag;
extern char yytext[];

extern int strflg;
extern int lxsizeof;	/* counts nested uses of "sizeof", so that */
			/* sizeof works in initializers */

extern OFFSZ inoff;

extern int reached;
extern int reachflg;

/*	tunnel to buildtree for name id's */

extern int idname;

extern int cflag, hflag, pflag, qflag;

/* flags to generate position-indepent code */
extern int picflag;
extern int smallpic;

/* various labels */
extern int brklab;
extern int contlab;
extern int flostat;
extern int retlab;
extern int retstat;
extern int *asavbc, *psavbc;

/*	flags used in structures/unions */

# define SEENAME 01
# define INSTRUCT 02
# define INUNION 04
# define FUNNYNAME 010
# define TAGNAME 020

/*	flags used in the (elementary) flow analysis ... */

# define FBRK 02
# define FCONT 04
# define FDEF 010
# define FLOOP 020

/*	flags used for return status */

# define RETVAL 1
# define NRETVAL 2

/*	used to mark a constant with no name field */

# define NONAME 040000

	/* mark an offset which is undefined */

# define NOOFFSET (-10201)

/*	declarations of various functions */

extern NODE
	*buildtree(),
	*bdty(),
	*mkty(),
	*rstruct(),
	*dclstruct(),
	*getstr(),
	*tymerge(),
	*stref(),
	*offcon(),
	*bcon(),
	*bpsize(),
	*convert(),
	*pconvert(),
	*oconvert(),
	*ptmatch(),
	*tymatch(),
	*makety(),
	*block(),
	*doszof(),
	*talloc(),
	*pcc_tcopy(),
	*optim(),
	*strargs(),
	*clocal();

OFFSZ	tsize(),
	psize();

TWORD	types();


double atof();

char *exname(), *exdcon();

# define checkst(x)

# ifndef CHARCAST
/* to make character constants into character connstants */
/* this is a macro to defend against cross-compilers, etc. */
# define CHARCAST(x) (char)(x)
# endif

# define SYMTSZ 307 /* size of the symbol table (was 1300) */
# define DIMTABSZ 2048 /* size of increments of dimension/size table */
# define PARAMSZ 512 /* size of parameter stack increment */
# define SWITSZ 512 /* size of switch table increment */
/*	special interfaces for yacc alone */
/*	These serve as abbreviations of 2 or more ops:
	ASOP	=, = ops
	RELOP	LE,LT,GE,GT
	EQUOP	EQ,NE
	DIVOP	DIV,MOD
	SHIFTOP	LS,RS
	ICOP	ICR,DECR
	UNOP	NOT,COMPL
	STROP	DOT,STREF

	*/

# define ASOP 25
# define RELOP 26
# define EQUOP 27
# define DIVOP 28
# define SHIFTOP 29
# define INCOP 30
# define UNOP 31
# define STROP 32

#ifndef LINT
/*
 * built in function names.  This looks a lot like a
 * similar hack in pass 2, but requires some information
 * that is gone by then.
 */

struct builtin_func {
	char *name;		/* name of builtin function */
	NODE *(*rewrite)();	/* tree rewriting function */
};

/*
 * local.c supplies an array of the above structures,
 * terminated by an entry with a NULL name.  clocal()
 * rewrites a call tree (p) by setting p = p1builtin_call(p).
 */
extern struct builtin_func p1builtin_funcs[];
extern NODE *p1builtin_call(/*p*/);     /* function to rewrite builtin calls */
extern NODE *floatargs_to_double(/*p*/); /*  float=>double arg conversions */
#endif

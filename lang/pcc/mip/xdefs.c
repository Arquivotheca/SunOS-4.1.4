
#ifndef lint
static	char sccsid[] = "@(#)xdefs.c 1.1 94/10/31 SMI";
#endif


# include "cpass1.h"

/*
 * Guy's changes from System V lint (12 Apr 88):
 *
 * Updated "xdefs.c".  An extra variable from the S5 "lint" was added as
 * part of the fix to eliminate bogus "statement not reached" messages.
 */

/*	communication between lexical routines	*/

char	*ftitle     = "";  	/* title of the file */
char	ititle[BUFSIZ];   	/* title of initial file */
int	lineno;		/* line number of the input file */

CONSZ lastcon;  /* the last constant read by the lexical analyzer */
double dcon;   /* the last double read by the lexical analyzer */


/*	symbol table maintainence */

struct symtab *stab[SYMTSZ];

int	curftn;  /* "current" function */
int	ftnno;  /* "current" function number */

int	curclass,	  /* current storage class */
	instruct,	/* "in structure" flag */
	stwart,		/* for accessing names which are structure members or names */
	blevel,		/* block level: 0 for extern, 1 for ftn args, >=2 inside function */
	curdim;		/* current offset into the dimension table */
	
int	*dimtab;

int	*paramstk;	/* used in the definition of function parameters */
int	paramno;	/* the number of parameters */
int	autooff,	/* the next unused automatic offset */
	argoff,		/* the next unused argument offset */
	strucoff;	/*  the next structure offset position */
int	regvar;		/* the next free register for register variables */
int	minrvar;	/* the smallest that regvar gets witing a function */
OFFSZ	inoff;		/* offset of external element being initialized */
int	brkflag = 0;	/* complain about break statements not reached */

struct sw *swtab = NULL;  /* table for cases within a switch */
struct sw *swp = NULL;  /* pointer to next free entry in swtab */
int swx = 0;  /* index of beginning of cases for current switch */

/* debugging flag */
int xdebug = 0;

int strflg;  /* if on, strings are to be treated as lists */

int reached;	/* true if statement can be reached... */
int reachflg;	/* true if NOTREACHED encountered in function */

int idname;	/* tunnel to buildtree for name id's */


NODE node[TREESZ];

int cflag = 0;  /* do we check for funny casts */
int hflag = 0;  /* do we check for various heuristics which may indicate errors */
int pflag = 0;  /* do we check for portable constructions */
int qflag = 0;  /* do we disable some checks useless for Sun C */
int picflag = 0;  /* do we generate pic */
int smallpic = 0; /* do we assume data size is small i.e. within 64k */
int malign_flag =0; /* do we generate mis-aligned ld/st */
int dalign_flag =0; /* do we generate ldd/std */

int brklab;
int contlab;
int flostat;
int retlab = NOLAB;
int retstat;

/* save array for break, continue labels, and flostat */

int *asavbc = NULL;
int *psavbc = NULL;

static char *
ccnames[] = { /* names of storage classes */
	"SNULL",
	"AUTO",
	"EXTERN",
	"STATIC",
	"REGISTER",
	"EXTDEF",
	"LABEL",
	"ULABEL",
	"MOS",
	"PARAM",
	"STNAME",
	"MOU",
	"UNAME",
	"TYPEDEF",
	"FORTRAN",
	"ENAME",
	"MOE",
	"UFORTRAN",
	"USTATIC",
	};

char * scnames( c ) register c; {
	/* return the name for storage class c */
	static char buf[20];
	if( c&FIELD ){
		sprintf( buf, "FIELD[%d]", c&FLDSIZ );
		return( buf );
		}
	if( (unsigned)c > USTATIC ){
		sprintf( buf, "0x%x", c );
		return( buf );
		}
	return( ccnames[c] );
	}


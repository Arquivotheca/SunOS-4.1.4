/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)cpp.c 1.1 94/10/31 SMI"; /* from S5R3.1 1.41 */
#endif


#ifdef FLEXNAMES
#	define NCPS	128
#else
#	define NCPS	8
#endif
int ncps = NCPS;		/* default name length */

# include "stdio.h"
# include "ctype.h"
#ifdef SUNPRO
# include <sys/file.h>
# include <vroot.h>
# include <report.h>
#endif
#ifdef BROWSER
# include <sys/types.h>
# include <sys/stat.h>
# include "cb_cpp_protocol.h"
# include "cb_line_id.h"
#else
#include "-DBROWSER should be left ON!"
#endif
/* C command - C preprocessor */

#define STATIC

#define CLASSCODE	27	/* exit status if #class seen */

#define READ	"r"
#define WRITE	"w"
#define SALT	'#'

#  define PAGESIZ 8192

/*
 * The last token printed on the "# linenumber filename" lines.
 */
#define NOINCL	 ""		/* Has nothing to do with an include file */
#define PUSHINCL "1"		/* Have just entered an include file */
#define POPINCL  "2"		/* Have just exited an include file */

char *pbeg, *pbuf, *pend;
char *outp, *inp;
char *newp;
char cinit;

int cxref = 0;
char *xcopy();
FILE *outfp;
static int ready = 0;
int xline;
int predefining = 1;
int default_include = 1;

/* some code depends on whether characters are sign or zero extended */
/*	#if '\377' < 0		not used here, old cpp doesn't understand */
#if pdp11 | vax | mc68000 | sparc | i386
#	define COFF 128
#else
#	define COFF 0
#endif

# if gcos
#	define ALFSIZ 512	/* alphabet size */
# else
#	define ALFSIZ 256	/* alphabet size */
# endif
char macbit[ ALFSIZ + 11 ];
char toktyp[ ALFSIZ ];
#define BLANK 1
#define IDENT 2
#define NUMBR 3

/* a superimposed code is used to reduce the number of calls to the
/* symbol table lookup routine.  (if the kth character of an identifier
/* is 'a' and there are no macro names whose kth character is 'a'
/* then the identifier cannot be a macro name, hence there is no need
/* to look in the symbol table.)  'scw1' enables the test based on
/* single characters and their position in the identifier.  'scw2'
/* enables the test based on adjacent pairs of characters and their
/* position in the identifier.  scw1 typically costs 1 indexed fetch,
/* an AND, and a jump per character of identifier, until the identifier
/* is known as a non-macro name or until the end of the identifier.
/* scw1 is inexpensive.  scw2 typically costs 4 indexed fetches,
/* an add, an AND, and a jump per character of identifier, but it is also
/* slightly more effective at reducing symbol table searches.
/* scw2 usually costs too much because the symbol table search is
/* usually short; but if symbol table search should become expensive,
/* the code is here.
/* using both scw1 and scw2 is of dubious value.
*/
#define scw1 1
#define scw2 0

#if scw2
char t21[ ALFSIZ ], t22[ ALFSIZ ], t23[ ALFSIZ + NCPS ];
#endif

#if scw1
#	define b0 1
#	define b1 2
#	define b2 4
#	define b3 8
#	define b4 16
#	define b5 32
#	define b6 64
#	define b7 128
#endif

#define IB 1
#define SB 2
#define NB 4
#define CB 8
#define QB 16
#define WB 32
char fastab[ ALFSIZ ];
char slotab[ ALFSIZ ];
char *ptrtab;
#define isslo		( ptrtab == ( slotab + COFF ) )
#define isid(a)		( ( fastab + COFF )[a] & IB )
#define isspc(a)	( ptrtab[a] & SB )
#define isnum(a)	( ( fastab + COFF )[a] & NB )
#define iscom(a)	( ( fastab + COFF )[a] & CB )
#define isquo(a)	( ( fastab + COFF )[a] & QB )
#define iswarn(a)	( ( fastab + COFF )[a] & WB )

#define eob(a)		( ( a ) >= pend )
#define bob(a)		( pbeg >= ( a ) )

char buffer[ NCPS + PAGESIZ + PAGESIZ + NCPS ];

# define MAXNEST 12	/* max number of nested #include's */
# define MAXFRE 14	/* max buffers of macro pushback */

# define SBSIZE 65536

char	*sbf = NULL;
char	*savch = NULL;

char	*malloc(), *calloc();

# define DROP '\376' /* special character not legal ASCII or EBCDIC */
# define WARN DROP
# define SAME 0
# define MAXINC 100	/* max number of directories for -I options */
# define MAXFRM 31	/* max number of formals/actuals to a macro */

#ifdef BROWSER
# define CBCHUNK_POWER	9	/* Line id blocks as a power of 2 */
# define CBCHUNK_SIZE ( 1 << CBCHUNK_POWER ) /* No. line id blocks/chunk */
# define CBCHUNK_MASK ( CBCHUNK_SIZE - 1 ) /* Mask of low order bits */
Cb_line_id cbchunks[ PAGESIZ / CBCHUNK_SIZE ];
int cbcount = 0;		/* Number of line ids to flush */
#endif

static char warnc = WARN;

int mactop, fretop;
char *instack[ MAXFRE ], *bufstack[ MAXFRE ], *endbuf[ MAXFRE ];

int plvl;	/* parenthesis level during scan for macro actuals */
int maclin;	/* line number of macro call requiring actuals */
char *macfil;	/* file name of macro call requiring actuals */
char *macnam;	/* name of macro requiring actuals */
int maclvl;	/* # calls since last decrease in nesting level */
char *macforw;	/* pointer which must be exceeded to decrease nesting level */
int macdam;	/* offset to macforw due to buffer shifting */

#if tgp
	int tgpscan;	/* flag for dump(); */
#endif

STATIC	int	inctop[ MAXNEST ];
STATIC	char	*fnames[ MAXNEST ];
STATIC	char	*realnames[ MAXNEST ];
STATIC	char	*dirnams[ MAXNEST ];	/* actual directory of #include files */
STATIC	FILE	*fins[ MAXNEST ];
STATIC	int	lineno[ MAXNEST ];
#ifdef BROWSER
STATIC	int	cbhashes[ MAXNEST ];	/* Current hash value for line */
STATIC	int	cblengths[ MAXNEST ];	/* Current lengths for line */
#endif

#ifdef SUNPRO
STATIC	pathcellpt	dirs= NULL;
#else
STATIC	char	*dirs[ MAXINC ];	/* -I and <> directories */
#endif
STATIC	char	*dfltdir = (char *) 0;	/* user-supplied default (like /usr/include) */
char *copy(), *subst(), *trmdir(), *strchr(), *strrchr();
char *chkend();
struct symtab *stsym();
STATIC	FILE	*fin	= stdin;
STATIC	FILE	*fout	= stdout;
STATIC	int	nd	= 1;
STATIC	int	pflag;	/* don't put out lines "# 12 foo.c" */
STATIC	int	ignoreline;	/* ignore "#line" entries */
STATIC	int	passcom;	/* don't delete comments */
STATIC	int	eolcom;		/* allow // ... \n comments */
STATIC	int	incomm;	/* set while in comment so that EOF can be detected */
STATIC	int	rflag;	/* allow macro recursion */
STATIC	int	mflag;	/* generate makefile dependencies */
STATIC	char	*infile;	/* name of .o file to build dependencies from */
STATIC 	FILE	*mout;	/* file to place dependencies on */
STATIC	int	print_incs;	/* set to print out included filenames */
STATIC	int	port_flag;	/* set to check for unportable constructs */
STATIC	int	ifno;
STATIC	int	exfail;
#ifdef BROWSER
STATIC	int	cbflag;		/* Pass code browser info to ccom */
STATIC	int	cbsuppress;	/* Suppress code browser info */
STATIC	char	*cbmacend;	/* End of macro expansion (or NULL) */
STATIC	int	cbifline;	/* Lineno for last #if */
STATIC	int	cbifsay;	/* Lineno for last sayline() */
STATIC	int	cbfirst = 1;	/* First time through sayline */
STATIC	char	cbsuffix;	/* Suffix for source file */
STATIC	int	cbinsubst = 0;	/* 1 => inside of subst() */
STATIC	int	cbversionsend = 1; /* 1 => send version number */
#endif

struct symtab
{
	char	*name;
	char	*value;
	struct symtab *next;	/* pointer to next chain entry */
} *lastsym, *lookup(), *slookup();

# if gcos
#	include <setjmp.h>
	static jmp_buf env;
#	define main	mainpp
#	undef exit
#	define exit(S)	longjmp(env, 1)
#	define symsiz 500
#	define LINEFORM	"# %d %s %s\n"
#	define ERRFORM	"*%c*   %s, line "
# else
#	define symsiz 500
#	define LINEFORM	"# %d \"%s\" %s\n"
#	define ERRFORM	"*%c*   \"%s\", line "
# endif
STATIC	struct symtab *stab[ symsiz ];

#define BKTINC	50		/* max space obtainable by calloc */
#define ELEMSIZ	sizeof(struct symtab)

STATIC	struct symtab *sfree = NULL;	/* pointer to next free space */
STATIC	int	nfree = 0;		/* number of free cells */
 
STATIC	struct symtab *defloc;
STATIC	struct symtab *udfloc;
STATIC	struct symtab *incloc;
STATIC	struct symtab *ifloc;
STATIC	struct symtab *elsloc;
STATIC	struct symtab *eifloc;
STATIC	struct symtab *elifloc;
STATIC	struct symtab *ifdloc;
STATIC	struct symtab *ifnloc;
STATIC	struct symtab *lneloc;
STATIC	struct symtab *ulnloc;
STATIC	struct symtab *uflloc;
STATIC	struct symtab *clsloc;
STATIC	struct symtab *idtloc;
STATIC	struct symtab *pragloc;
STATIC	int	trulvl;
STATIC	int	flslvl;
STATIC	char   *expanding_params;
#define MAX_DEPTH	500		/* max of trulvl + flslvl */
#define SEEN_ELSE	0x1
#define TRUE_ELIF	0x2
STATIC	char	ifelstk[MAX_DEPTH];

sayline( incl )
	char *incl;
{
	if ( pflag == 0 )
	{
#ifdef BROWSER
		/*
		 * The code browser generates more saylines, hence a more
		 * compact representation used.  Also the first sayline for
		 * a file contains the file timestamp.
		 */
		if ( cbflag )
		{
			if ( ( incl[0] == '\0' ) && ( !cbfirst ) )
			{
				putc( CB_CHR_PREFIX, fout );
				putc( CB_CHR_LINENO, fout );
				cbputlineno( fout, lineno[ifno] );
			}
			else
			{
				if ( cbfirst )
					cbfirst = 0;
				else
				{
					putc( CB_CHR_PREFIX, fout );
					putc( CB_CHR_SAYLINE, fout );
				}
				fprintf( fout, "# %d \"%s\" %s\n",
					 lineno[ifno], fnames[ifno], incl );
			}
			if ( cbversionsend )
			{
				cbversionsend = 0;
				fprintf( fout, "%c%c%d%c", CB_CHR_PREFIX,
					 CB_CHR_VERSION,
					 CB_CPP_PROTOCOL_VERSION,
					 CB_CHR_END_ID );
				
			}
		}
		else
#endif
			fprintf( fout, LINEFORM,
				 lineno[ifno], fnames[ifno], incl );
	}
}

/* data structure guide
/*
/* most of the scanning takes place in the buffer:
/*
/*  (low address)                                             (high address)
/*  pbeg                           pbuf                                 pend
/*  |      <-- PAGESIZ chars -->      |         <-- PAGESIZ chars -->        |
/*  _______________________________________________________________________
/* |_______________________________________________________________________|
/*          |               |               |
/*          |<-- waiting -->|               |<-- waiting -->
/*          |    to be      |<-- current -->|    to be
/*          |    written    |    token      |    scanned
/*          |               |               |
/*          outp            inp             p
/*
/*  *outp   first char not yet written to output file
/*  *inp    first char of current token
/*  *p      first char not yet scanned
/*
/* macro expansion: write from *outp to *inp (chars waiting to be written),
/* ignore from *inp to *p (chars of the macro call), place generated
/* characters in front of *p (in reverse order), update pointers,
/* resume scanning.
/*
/* symbol table pointers point to just beyond the end of macro definitions;
/* the first preceding character is the number of formal parameters.
/* the appearance of a formal in the body of a definition is marked by
/* 2 chars: the char WARN, and a char containing the parameter number.
/* the first char of a definition is preceded by a zero character.
/*
/* when macro expansion attempts to back up over the beginning of the
/* buffer, some characters preceding *pend are saved in a side buffer,
/* the address of the side buffer is put on 'instack', and the rest
/* of the main buffer is moved to the right.  the end of the saved buffer
/* is kept in 'endbuf' since there may be nulls in the saved buffer.
/*
/* similar action is taken when an 'include' statement is processed,
/* except that the main buffer must be completely emptied.  the array
/* element 'inctop[ifno]' records the last side buffer saved when
/* file 'ifno' was included.  these buffers remain dormant while
/* the file is being read, and are reactivated at end-of-file.
/*
/* instack[0 : mactop] holds the addresses of all pending side buffers.
/* instack[inctop[ifno]+1 : mactop-1] holds the addresses of the side
/* buffers which are "live"; the side buffers instack[0 : inctop[ifno]]
/* are dormant, waiting for end-of-file on the current file.
/*
/* space for side buffers is obtained from 'savch' and is never returned.
/* bufstack[0:fretop-1] holds addresses of side buffers which
/* are available for use.
*/

dump()
{
	/* write part of buffer which lies between  outp  and  inp .
	/* this should be a direct call to 'write', but the system slows
	/* to a crawl if it has to do an unaligned copy.  thus we buffer.
	/*? this silly loop is 15% of the total time, thus even the 'putc'
	/*? macro is too slow.
	*/
	register char *p1;
	register FILE *f;
#if tgp
	register char *p2;
#endif

	if ( ( p1 = outp ) == inp || flslvl != 0 )
#ifdef BROWSER
	{
		if ( ( cbmacend != NULL ) && ( outp >= cbmacend ) &&
		     ( *inp != '\0' ) && ( !cbinsubst ) )
		{
			putc( CB_CHR_PREFIX, fout );
			putc( CB_CHR_MACRO_REF_END, fout );
			cbmacend = NULL;
		}
		return;
	}
#else
		return;
#endif
#if tgp
#	define MAXOUT 80
	if ( !tgpscan )		/* scan again to insure <= MAXOUT
	{			/* chars between linefeeds */
		register char c, *pblank;
		char savc, stopc, brk;

		tgpscan = 1;
		brk = stopc = pblank = 0;
		p2 = inp;
		savc = *p2;
		*p2 = '\0';
		while ( c = *p1++ )
		{
			if ( c == '\\' )
				c = *p1++;
			if ( stopc == c )
				stopc = 0;
			else if ( c == '"' || c == '\'' )
				stopc = c;
			if ( p1 - outp > MAXOUT && pblank != 0 )
			{
				*pblank++ = '\n';
				inp = pblank;
				dump();
				brk = 1;
				pblank = 0;
			}
			if ( c == ' ' && stopc == 0 )
				pblank = p1 - 1;
		}
		if ( brk )
			sayline( NOINCL );
		*p2 = savc;
		inp = p2;
		p1 = outp;
		tgpscan = 0;
	}
#endif
	f = fout;
# if gcos
	/* filter out "$ program c" card if first line of input */
	/* gmatch is a simple pattern matcher in the GCOS Standard Library */
	{
		static int gmfirst = 0;

		if ( !gmfirst )
		{
			++gmfirst;
			if ( gmatch( p1, "^$*program[ \t]*c*") )
				p1 = strchr( p1, '\n' );
		}
	}
# endif
	if ( p1 < inp )
		p1 += fwrite( p1, sizeof( char ), inp - p1, f );
	outp = p1;
#ifdef BROWSER
	/*
	 * Do not output CB_CHR_MACRO_REF_END when the input buffer is empty.
	 * It may be the case that a compiler lexeme splits (such as "->")
	 * across the buffer boundary.  Some compilers can not cope with
	 * code browser information in the middle of lexemes.
	 */
	if ( ( cbmacend != NULL ) && ( outp >= cbmacend ) &&
	     ( *inp != '\0' ) && ( !cbinsubst ) )
	{
		putc( CB_CHR_PREFIX, fout );
		putc( CB_CHR_MACRO_REF_END, fout );
		cbmacend = NULL;
	}
#endif
}

#ifdef BROWSER
cbflushids()
{
	register int i, count;

	dump();
	count = cbcount;
	putc( CB_CHR_PREFIX, fout );
	putc( CB_CHR_LINE_ID, fout );
	putc( count & 255, fout );
	putc( count >> 8, fout );
	i = 0;
	while ( count > 0 )
		count -= fwrite( cbchunks[ i++ ], sizeof( Cb_line_id_rec ),
				 ( count >= CBCHUNK_SIZE ) ?
					CBCHUNK_SIZE : count, fout );
	cbcount = count;
}

/*
 * Pre-scan the buffer and compute line id's for the code browser.
 */
cbprescan()
{
	register char c, *p;
	register int length, hash, count;
	register Cb_line_id line_id;
	Cb_line_id chunk;

	if ( cbcount > 0 )
	{
		perror( "line ids for -cb flag are messed up" );
		exit( 2 );
	}
	p = pbuf;
	hash = cbhashes[ifno];
	length = cblengths[ifno];
	count = 0;
	for ( ;; )
	{
		if ( ( ( c = *p++ ) == '\0' ) && ( p >= pend ) )
		{
			cbhashes[ifno] = hash;
			cblengths[ifno] = length;
			cbcount = count;
			return;
		}
		else if ( c == '\n' )
		{
			if ( ( count & CBCHUNK_MASK ) == 0 )
			{
				if ( cbchunks[ count >> CBCHUNK_POWER ] == 0 )
				{
					chunk = (Cb_line_id_rec *) malloc(
					    CBCHUNK_SIZE * sizeof( Cb_line_id_rec ) );
					if ( chunk == NULL )
					{
						perror( "Out of memory" );
						exit( 1 );
					}
					cbchunks[ count >> CBCHUNK_POWER ] =
							chunk;
				}
				chunk = cbchunks[ count >> CBCHUNK_POWER ];
			}
			line_id = &chunk[ count & CBCHUNK_MASK ];
			line_id->hash = hash;
			line_id->length = length;
			line_id->is_inactive = 0;
			count++;
			hash = 0;
			length = 0;
		}
		else
		{
			hash += c;
			length++;
		}
	}
}
#endif

char *
refill( p )
	register char *p;
{
	/* dump buffer.  save chars from inp to p.  read into buffer at pbuf,
	/* contiguous with p.  update pointers, return new p.
	*/
	register char *np, *op;
	register int ninbuf;
#ifdef BROWSER
	register int d;
#endif

	dump();
	np = pbuf - ( p - inp );
	op = inp;
#ifdef BROWSER
	if ( cbmacend != NULL )
		d = cbmacend - p;
#endif
	if ( bob( np + 1 ) )
	{
		pperror( "token too long" );
		np = pbeg;
		p = inp + PAGESIZ;
	}
	macdam += np - inp;
	outp = inp = np;
	while ( op < p )
		*np++ = *op++;
	p = np;
#ifdef BROWSER
	if ( cbmacend != NULL )
		cbmacend = p + d;
#endif
	for ( ;; )
	{
			/* retrieve hunk of pushed-back macro text */
		if ( mactop > inctop[ifno] )
		{
			op = instack[ --mactop ];
			np = pbuf;
			do
			{
				while ( *np++ = *op++ )
					;
			}
			while ( op < endbuf[mactop] );
			pend = np - 1;
			/* make buffer space avail for 'include' processing */
			if ( fretop < MAXFRE )
				bufstack[fretop++] = instack[mactop];
			return( p );
		}
		else 	/* get more text from file(s) */
		{
			maclvl = 0;
			if ( 0 < ( ninbuf = fread( pbuf, sizeof( char ), PAGESIZ, fin ) ) )
			{
				pend = pbuf + ninbuf;
				*pend = '\0';
#ifdef BROWSER
				if ( cbflag )
					cbprescan();
#endif
				return( p );
			}
			if ( ferror( fin ) )
			{
				fprintf( stderr, "cpp: Error reading ");
				perror( realnames[ifno] );
				exit(2);
			}
			/* end of #include file */
			if ( ifno == 0 )	/* end of input */
			{
				if ( plvl != 0 )
				{
					int n = plvl, tlin = lineno[ifno];
					char *tfil = fnames[ifno];

					lineno[ifno] = maclin;
					fnames[ifno] = macfil;
					pperror( "%s: unterminated macro call",
						macnam );
					lineno[ifno] = tlin;
					fnames[ifno] = tfil;
					np = p;
					/*shut off unterminated quoted string*/
					*np++ = '\n';
						/* supply missing parens */
					while ( --n >= 0 )
						*np++ = ')';
					pend = np;
					*np = '\0';
					if ( plvl < 0 )
						plvl = 0;
					return( p );
				}
				if ( incomm )
				{
					if ( !passcom )
						--flslvl;
					pperror( "missing */" );
				}
				if ( expanding_params ) {
					pperror( "%s: argument mismatch",
						expanding_params );
					--flslvl;
				}
				if ( trulvl +flslvl > 0 )
					pperror( "missing #endif" );
				inp = p;
				dump();
				if ( fout && ferror( fout ) )
				{
					perror( "cpp: Can't write output file" );
					if ( exfail < CLASSCODE - 1 )
						++exfail;
				}
#ifdef BROWSER
				if ( cbflag && ( cbmacend != NULL ) )
				{
					putc( CB_CHR_PREFIX, fout );
					putc( CB_CHR_MACRO_REF_END, fout );
					cbmacend = NULL;
				}
#endif
				exit( exfail ? ( exfail ==
					CLASSCODE ? CLASSCODE : 2 ) : 0 );
			}
			fclose( fin );
			fin = fins[--ifno];
#ifdef SUNPRO
			add_dir_to_path(dirnams[ifno], &dirs, 0);
#else
			dirs[0] = dirnams[ifno];
#endif
#ifdef BROWSER
			if ( cbcount > 0 )
				cbflushids();
#endif
			sayline( POPINCL );
			if ( cxref )
				fprintf(outfp, "\"%s\"\n", fnames[ifno]);
		}
	}
}

#define BEG 0
#define LF 1

char *
cotoken( p )
	register char *p;
{
	register int c, i;
	char quoc;
	static int state = BEG;

	if ( state != BEG )
		goto prevlf;
	for ( ;; )
	{
	again:
#ifdef BROWSER
		if ( ( cbmacend != NULL ) && ( p >= cbmacend ) )
		{
			dump();	/* Force out end macro reference. */
		}
#endif
		while ( !isspc( *p++ ) )
			;
		switch ( *( inp = p - 1 ) )
		{
		case 0:
			if ( eob( --p ) )
			{
				p = refill( p );
				goto again;
			}
			else
				++p;	/* ignore null byte */
			break;
		case '|':
		case '&':
			for ( ;; )	/* sloscan only */
			{
				if ( *p++ == *inp )
					break;
				if ( eob( --p ) )
					p = refill( p );
				else
					break;
			}
			break;
		case '=':
		case '!':
			for ( ;; )	/* sloscan only */
			{
				if ( *p++ == '=' )
					break;
				if ( eob( --p ) )
					p = refill( p );
				else
					break;
			}
			break;
		case '<':
		case '>':
			for ( ;; )	/* sloscan only */
			{
				if ( *p++ == '=' || p[-2] == p[-1] )
					break;
				if ( eob( --p ) )
					p = refill( p );
				else
					break;
			}
			break;
		case '\\':
			for ( ;; )
			{
				if ( *p++ == '\n' )
				{
					++lineno[ifno];
					break;
				}
				if ( eob( --p ) )
					p = refill( p );
				else
				{
					++p;
					break;
				}
			}
			break;
		case '/':
			for ( ;; )
			{
				if ( *p == '/' && eolcom )
				{
					p++;
  					incomm++;
		  			if ( !passcom )
					{
						inp = p - 2;
						dump();
						++flslvl;
					}
		  			for ( ;; )
					{
						while (*p && *p != '\n')
							p++;
						if ( p[0] == '\n' )
						{
							goto endcpluscom;
						}
						else if ( eob( p ) )
						{
							if ( !passcom )
							{
								inp = p;
								p = refill( p );
							}
							else if ( (p - inp) >= BUFSIZ ) /* split long comment */
							{
								inp = p;
								refill( p );
							}
							else
								p = refill( p );
						}
						else
							++p; /* ignore null byte */
					}
				endcpluscom:
					if ( !passcom )
					{
						outp = inp = p;
						--flslvl;
					}
					incomm--;
					break;
				}
				else if ( *p++ == '*' )	/* comment */
				{
					incomm = 1;
					if ( !passcom )
					{
						inp = p - 2;
						dump();
						++flslvl;
					}
					for ( ;; )
					{
						while ( !iscom( *p++ ) )
							;
						if ( p[-1] == '*' )
							for ( ;; )
							{
								if ( *p++ == '/' )
									goto endcom;
								if ( eob( --p ) )
								{
									if ( !passcom )
									{
										inp = p;
										p = refill( p );
									}
									/* split long comment */
									else if ( ( p - inp ) >= PAGESIZ )
									{
										*p++ = '*';
										*p++ = '/';
										inp = p;
										p = refill( p );
										outp = inp = p -= 2;
										*p++ = '/';
										*p++ = '*';
									}
									else
										p = refill( p );
								}
								else
									break;
							}
						else if ( p[-1] == '\n' )
						{
							++lineno[ifno];
							if ( !passcom && flslvl <= 1 )
								putc( '\n', fout );
						}
						else if ( eob( --p ) )
						{
							if ( !passcom )
							{
								inp = p;
								p = refill( p );
							}
							/* split long comment */
							else if ( ( p - inp ) >= PAGESIZ )
							{
								*p++ = '*';
								*p++ = '/';
								inp = p;
								p = refill( p );
								outp = inp = p -= 2;
								*p++ = '/';
								*p++ = '*';
							}
							else
								p = refill( p );
						}
						else
							++p;	/* ignore null byte */
					}
				endcom:
					incomm = 0;
					if ( !passcom )
					{
						outp = inp = p;
						--flslvl;
						goto again;
					}
					break;
				}
				if ( eob( --p ) )
					p = refill( p );
				else
					break;
			}
			break;
# if gcos
		case '`':
# endif
		case '"':
		case '\'':
			quoc = p[-1];
			for ( ;; )
			{
				while ( !isquo( *p++ ) )
					;
#ifdef BROWSER
				if ( cbflag && ( p[-1] == CB_CHR_PREFIX ) &&
				   ( flslvl == 0 ) )
				{
					inp = p;
					dump();
					putc( CB_CHR_CONTROL_A, fout );
				}
					
#endif
				if ( p[-1] == quoc )
					break;
				if ( p[-1] == '\n' )	/* bare \n terminates quotation */
				{
					--p;
					break;
				}
				if ( p[-1] == '\\' )
					for ( ;; )
					{
						if ( *p++ == '\n' )	/* escaped \n ignored */
						{
							++lineno[ifno];
							break;
						}
						if ( eob( --p ) )
							p = refill( p );
						else
						{
							++p;
							break;
						}
					}
				else if ( eob( --p ) )
					p = refill( p );
				else
					++p;	/* it was a different quote character */
			}
			break;
		case WARN:
			{
				int ii;
				int reallineno;

				dump();
				reallineno = 0;
				for ( ii = sizeof(int) / sizeof(char); --ii >= 0; )
				{
					if ( eob( p ) )
						p = refill( p );
					reallineno |= ( *p++ & 0xFF ) << ( ii * 8 );
				}
				lineno[ifno] = reallineno;
				putc('\n', fout);
				sayline( NOINCL );
				inp = outp = p;
				break;
			}
		case '\n':
		newline:
#ifdef BROWSER
			if ( cbcount > 0 )
				cbflushids();
#endif
			++lineno[ifno];
			if ( isslo )
			{
				state=LF;
				return( p );
			}
		prevlf:
			state = BEG;
			for ( ;; )
			{
				/*
				* ignore formfeeds and vertical tabs
				* which may be just before the SALT
				*/
				if ( *p == '\f' || *p == '\v' )
				{
					register char *s = p;

					while ( *++s == '\f' || *s == '\v' )
						;
					if ( *s == SALT )
					{
						/*
						* get the SALT to the front!
						*/
						*s = *p;
						*p = SALT;
					}
				}
				if ( *p++ == SALT )
					return( p );
				if ( eob( inp = --p ) )
					p = refill( p );
				else
					goto again;
			}
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			for ( ;; )
			{
				while ( isnum( *p++ ) )
					;
				if ( eob( --p ) )
					p = refill( p );
				else
					break;
			}
			break;
		case 'A': case 'B': case 'C': case 'D': case 'E':
		case 'F': case 'G': case 'H': case 'I': case 'J':
		case 'K': case 'L': case 'M': case 'N': case 'O':
		case 'P': case 'Q': case 'R': case 'S': case 'T':
		case 'U': case 'V': case 'W': case 'X': case 'Y':
		case 'Z': case '_':
		case 'a': case 'b': case 'c': case 'd': case 'e':
		case 'f': case 'g': case 'h': case 'i': case 'j':
		case 'k': case 'l': case 'm': case 'n': case 'o':
		case 'p': case 'q': case 'r': case 's': case 't':
		case 'u': case 'v': case 'w': case 'x': case 'y':
		case 'z':
#if scw1
#	define tmac1( c, bit )		if ( !xmac1( c, bit, & ) ) \
						goto nomac
#	define xmac1( c, bit, op )	( ( macbit + COFF )[c] op ( bit ) )
#else
#	define tmac1( c, bit )
#	define xmac1( c, bit, op )
#endif

#if scw2
#	define tmac2( c0, c1, cpos )	if ( !xmac2( c0, c1, cpos, & ) ) \
						goto nomac
#	define xmac2( c0, c1, cpos, op ) \
		( ( macbit + COFF )[ ( t21 + COFF )[c0] + \
		( t22 + COFF )[c1]] op ( t23 + COFF + cpos )[c0] )
#else
#	define tmac2( c0, c1, cpos )
#	define xmac2( c0, c1, cpos, op )
#endif

			if ( flslvl )
				goto nomac;
			for ( ;; )
			{
				c = p[-1];
				tmac1( c, b0 );
				i = *p++;
				if ( !isid( i ) )
					goto endid;
				tmac1( i, b1 );
				tmac2( c, i, 0 );
				c = *p++;
				if ( !isid( c ) )
					goto endid;
				tmac1( c, b2 );
				tmac2( i, c, 1 );
				i = *p++;
				if ( !isid( i ) )
					goto endid;
				tmac1( i, b3 );
				tmac2( c, i, 2 );
				c = *p++;
				if ( !isid( c ) )
					goto endid;
				tmac1( c, b4 );
				tmac2( i, c, 3 );
				i = *p++;
				if ( !isid( i ) )
					goto endid;
				tmac1( i, b5 );
				tmac2( c, i, 4 );
				c = *p++;
				if ( !isid( c ) )
					goto endid;
				tmac1( c, b6 );
				tmac2( i, c, 5 );
				i = *p++;
				if ( !isid( i ) )
					goto endid;
				tmac1( i, b7 );
				tmac2( c, i, 6 );
				tmac2( i, 0, 7 );
				while ( isid( *p++ ) )
					;
				if ( eob( --p ) )
				{
					refill( p );
					p = inp + 1;
					continue;
				}
				goto lokid;
			endid:
				if ( eob( --p ) )
				{
					refill( p );
					p = inp + 1;
					continue;
				}
				tmac2( p[-1], 0, -1 + ( p - inp ) );
			lokid:
				slookup( inp, p, 0 );
				if ( newp )
				{
					p = newp;
					goto again;
				}
				else
					break;
			nomac:
				while ( isid( *p++ ) )
					;
				if ( eob( --p ) )
				{
					p = refill( p );
					goto nomac;
				}
				else
					break;
			}
			break;
		} /* end of switch */
	
		if ( isslo )
			return( p );
	} /* end of infinite loop */
}

char *
skipbl( p )		/* get next non-blank token */
	register char *p;
{
	do
	{
		outp = inp = p;
		p = cotoken( p );
	}
	while ( ( toktyp + COFF )[*inp] == BLANK );
	return( p );
}

char *
unfill( p )
	register char *p;
{
	/* take <= PAGESIZ chars from right end of buffer and put
	/* them on instack.  slide rest of buffer to the right,
	/* update pointers, return new p.
	*/
	register char *np, *op;
	register int d;

	if ( mactop >= MAXFRE )
	{
		pperror( "%s: too much pushback", macnam );
		p = inp = pend;		 /* begin flushing pushback */
		dump();
		while ( mactop > inctop[ifno] )
		{
			p = refill( p );
			p = inp = pend;
			dump();
		}
	}
	if ( fretop > 0 )
		np = bufstack[--fretop];
	else
	{
		np = savch;
		savch += PAGESIZ;
		if ( savch >= sbf + SBSIZE )
		{
			allocsbf();
			np = savch;
			savch += PAGESIZ;
		}
		*savch++ = '\0';
	}
	instack[mactop] = np;
	op = pend - PAGESIZ;
	if ( op < p )
		op = p;
	for ( ;; )		/* out with old */
	{
		while ( *np++ = *op++ )
			;
		if ( eob( op ) )
			break;
	}
	endbuf[mactop++] = np;	/* mark end of saved text */
	np = pbuf + PAGESIZ;
	op = pend - PAGESIZ;
	pend = np;
	if ( op < p )
		op = p;
	while ( outp < op )		/* slide over new */
		*--np = *--op;
	if ( bob( np ) )
		pperror( "token too long" );
	d = np - outp;
	outp += d;
	inp += d;
	macdam += d;
#ifdef BROWSER
	if ( cbmacend != NULL )
		cbmacend += d;
#endif
	return( p + d );
}

char *
doincl( p )
	register char *p;
{
	int filok, inctype;
	register char *cp;
	char **dirp, *nfil;
	char filname[PAGESIZ];
#ifdef SUNPRO
	char translated[1000];
	char symbol[256];
	int fd;
#endif
#ifdef BROWSER
	struct stat status;
	char *namep;
#endif

	p = skipbl( p );
#ifdef BROWSER
	namep = inp;
#endif
	cp = filname;
	if ( *inp++ == '<' )	/* special <> syntax */
	{
		inctype = 1;
		++flslvl;	/* prevent macro expansion */
		for ( ;; )
		{
			outp = inp = p;
			p = cotoken( p );
			if ( *inp == '\n' )
			{
				--p;
				*cp = '\0';
				break;
			}
			if ( *inp == '>' )
			{
				*cp = '\0';
				break;
			}
# ifdef gimpel
			if ( *inp == '.' && !intss() )
				*inp = '#';
# endif
			while ( inp < p )
				*cp++ = *inp++;
		}
		--flslvl;	/* reenable macro expansion */
#ifdef BROWSER
		if ( cbflag && ( flslvl == 0 ) && ( cbmacend == NULL ) )
			cbputid( fout, CB_CHR_INCLUDE_SYSTEM,
				 xcopy( namep, inp + 1 ) );
#endif
	}
	else if ( inp[-1] == '"' )	/* regular "" syntax */
	{
		inctype = 0;
# ifdef gimpel
		while ( inp < p )
		{
			if ( *inp == '.' && !intss() )
				*inp = '#';
				*cp++ = *inp++;
		}
# else
		while ( inp < p )
			*cp++ = *inp++;
# endif
		if ( *--cp == '"' )
			*cp = '\0';
#ifdef BROWSER
		if ( cbflag && ( flslvl == 0 ) && ( cbmacend == NULL ) )
			cbputid( fout, CB_CHR_INCLUDE_LOCAL,
				xcopy( namep, inp ) );
#endif
	}
	else if ( inp[-1] == '\'' )	/* single quotes for quiche eaters */
	{
		inctype = 0;
# ifdef gimpel
		while ( inp < p )
		{
			if ( *inp == '.' && !intss() )
				*inp = '#';
				*cp++ = *inp++;
		}
# else
		while ( inp < p )
			*cp++ = *inp++;
# endif
		if ( *--cp == '\'' )
			*cp = '\0';
#ifdef BROWSER
		if ( cbflag && ( flslvl == 0 ) && ( cbmacend == NULL ) )
			cbputid( fout, CB_CHR_INCLUDE_LOCAL,
				xcopy( namep, inp ) );
#endif
	}
	else
	{
		/* This is not an error if we're skipping */
		if (!flslvl)
			pperror( "bad include syntax", 0 );
		inctype = 2;
	}
	/* flush current file to \n , then write \n */

	p = chkend( p, 1 );

	outp = inp = p;

	dump();
	if ( inctype == 2 || flslvl != 0 )
		return( p );
	/* look for included file */
	if ( ifno + 1 >= MAXNEST )
	{
		pperror( "Unreasonable include nesting", 0 );
		return( p );
	}
	if ( ( nfil = savch ) > sbf + SBSIZE - PAGESIZ )
	{
		allocsbf();
		nfil = savch;
	}

	/* check for #include "/usr/include/..." */
	if (strncmp(filname, "/usr/include/", 13) == 0)
	    ppwarn("#include of /usr/include/... may be non-portable");

	filok = 0;
	if (inctype >= nd)
	{
		pperror( "Can't find include file %s", filname );
		return(p);
	}
#ifdef BROWSER
	if ( cbcount > 0 )
		cbflushids();
#endif
#ifdef SUNPRO
	if ((fd= open_vroot(filname, O_RDONLY, 0, dirs + inctype, VROOT_DEFAULT)) >= 0)
		{		char *path;

			fins[ifno + 1] = fdopen(fd, "r");
			filok = 1;
			fin = fins[++ifno];
			get_vroot_path(NULL, &path, NULL);
			(void)strcpy(translated, path);
			if ( print_incs )
				fprintf( stderr, "%s\n", translated );
		}
#else
	for ( dirp = dirs + inctype; *dirp; ++dirp )
	{
		if (
# if gcos
			strchr( filname, '/' )
# else
			filname[0] == '/' 
# endif
				|| **dirp == '\0' )
		{
			strcpy( nfil, filname );
		}
		else
		{
			strcpy( nfil, *dirp );
# if unix || gcos || DMERT
			strcat( nfil, "/" );
# endif
#ifdef ibm
#	ifndef gimpel
			strcat( nfil, "." );
#	endif
#endif
			strcat( nfil, filname );
		}
		if ( NULL != ( fins[ifno + 1] = fopen( nfil, READ ) ) )
		{
			filok = 1;
			fin = fins[++ifno];
			if ( print_incs )
				fprintf( stderr, "%s\n", nfil );
			break;
		}
	}
#endif
	if ( filok == 0 )
		pperror( "Can't find include file %s", filname );
	else
	{
#ifdef SUNPRO
		fnames[ifno] = cp = copy(translated);
		report_dependency(cp);
#else
		fnames[ifno] = cp = nfil;
#endif
#ifdef BROWSER
		cbhashes[ifno] = 0;
		cblengths[ifno] = 0;
#endif
		lineno[ifno] = 1;
		realnames[ifno] = fnames[ifno];
		while ( *cp++)
			;
		savch = cp;
#ifdef SUNPRO
		dirnams[ifno] = trmdir( copy(translated  ) );
		add_dir_to_path(dirnams[ifno], &dirs, 0);
#else
		dirnams[ifno] = dirs[0] = trmdir( copy( nfil ) );
#endif
		sayline( PUSHINCL );
		if ( mflag )
			fprintf( mout, "%s: %s\n", infile, fnames[ifno] );
		if ( cxref )
			fprintf(outfp, "\"%s\"\n", fnames[ifno]);
#ifdef BROWSER
		if ( cbflag )
		{
			if ( cbmacend != NULL )
			{
				putc( CB_CHR_PREFIX, fout );
				putc( CB_CHR_MACRO_REF_END, fout );
				cbmacend = NULL;
			}
		}
#endif

		/* save current contents of buffer */
		while ( !eob( p ) )
			p = unfill( p );
		inctop[ifno] = mactop;
	}
	return( p );
}

equfrm( a, p1, p2 )
	register char *a, *p1, *p2;
{
	register char c;
	int flag;

	c = *p2;
	*p2 = '\0';
	flag = strcmp( a, p1 );
	*p2 = c;
	return( flag == SAME );
}

char *
dodef( p )		/* process '#define' */
	char *p;
{
	register char *pin, *psav, *cf;
	char **pf, **qf;
	int b, c, params;
	int ex_blank;	/* used to ignore extra blanks in token-string */
	int sav_passcom = passcom;	/* saved passcom, used to reset it */
	struct symtab *np;
	char *oldval, *oldsavch;
	char *formal[MAXFRM]; /* formal[n] is name of nth formal */
	char formtxt[PAGESIZ]; /* space for formal names */

	passcom=0;	/* delete comments in all but body of macro */

	if ( savch > sbf + SBSIZE - PAGESIZ )
		allocsbf();
	oldsavch = savch; /* to reclaim space if redefinition */
	++flslvl;	/* prevent macro expansion during 'define' */
	p = skipbl( p );
	pin = inp;
	if ( ( toktyp + COFF )[*pin] != IDENT )
	{
		if (*pin == '\n') --lineno[ifno];
		ppwarn( "illegal/missing macro name" );
		if (*pin == '\n') ++lineno[ifno];
		while ( *inp != '\n' )
			p = skipbl( p );
		--flslvl;		/* restore expansion */
		passcom = sav_passcom;	/* restore to "real" value */
		return( p );
	}
	np = slookup( pin, p, 1 );
	if ( oldval = np->value )	/* was previously defined */
		savch = oldsavch;
	if ( cxref )
		def(np->name, lineno[ifno]);
	b = 1;
	cf = pin;
	while ( cf < p )		/* update macbit */
	{
		c = *cf++;
		xmac1( c, b, |= );
		b = ( b + b ) & 0xFF;
		if ( cf != p )
			xmac2( c, *cf, -1 + ( cf - pin ), |= );
		else
			xmac2( c, 0, -1 + ( cf - pin ), |= );
	}
	params = 0;
	outp = inp = p;
	p = cotoken( p );
	pin = inp;
	if ( *pin == '(' )	/* with parameters; identify the formals */
	{
		if ( cxref )
			newf(np->name, lineno[ifno]);
#ifdef BROWSER
		if ( cbflag )
			cbputid( fout, CB_CHR_MACRO_DEF_W_FORMALS, np->name );
#endif
		cf = formtxt;
		pf = formal;
		for ( ;; )
		{
			p = skipbl( p );
			pin = inp;
			if ( *pin == '\n' )
			{
				--lineno[ifno];
				--p;
				pperror( "%s: missing )", np->name );
				break;
			}
			if ( *pin == ')' )
				break;
			if ( *pin == ',' )
				continue;
			if ( ( toktyp + COFF )[*pin] != IDENT )
			{
				c = *p;
				*p = '\0';
				pperror( "bad formal: %s", pin );
				*p = c;
			}
			else if ( pf >= &formal[MAXFRM] )
			{
				c = *p;
				*p = '\0';
				pperror( "too many formals: %s", pin );
				*p = c;
			}
			else
			{
				*pf++ = cf;
				while ( pin < p )
					*cf++ = *pin++;
				*cf++ = '\0';
				++params;
				if ( cxref )
					def(*(pf-1), lineno[ifno]);
#ifdef BROWSER
				if ( cbflag )
					cbputid( fout, CB_CHR_MACRO_FORMAL_DEF,
						 *(pf - 1) );
#endif
			}
		}
		if ( params == 0 )	/* #define foo() ... */
			--params;
	}
	else
	{
		if ( *pin == '\n' )
		{
			--lineno[ifno];
			--p;
		}
		else
			p = inp;    /* back up to scan non-blank next token */
#ifdef BROWSER
		if ( cbflag && !cbsuppress )
			cbputid( fout, CB_CHR_MACRO_DEF_WO_FORMALS, np->name );
#endif
	}
	/*
	* remember beginning of macro body, so that we can
	* warn if a redefinition is different from old value.
	*/
	oldsavch = psav = savch;
	passcom = 1;	/* make cotoken() return comments as tokens */
	ex_blank = 1;	/* must have some delimiter - might as well be blank */
	for ( ;; )	/* accumulate definition until linefeed */
	{
		outp = inp = p;
		p = cotoken( p );
		pin = inp;
		if ( *pin == '\\' && pin[1] == '\n' )	/* ignore escaped lf */
		{
			putc( '\n', fout );
			continue;
		}
		if ( *pin == '\n' )
			break;
#ifdef BROWSER
		if ( cbflag && !cbsuppress &&
		   ( ( *pin == '"' ) ||
		     ( ( *pin == '\'' ) && ( cbsuffix == 'p' ) ) ) )
			cbputid( fout, CB_CHR_MACRO_STRING_IN_DEF,
				 xcopy( pin, p ) );
#endif
		if ( ( toktyp + COFF )[*pin] == BLANK )	/* skip extra blanks */
		{
			if ( ex_blank )
				continue;
			*pin = ' ';	/* force it to be a "real" blank */
			ex_blank = 1;
		}
		else
			ex_blank = 0;

		if ( *pin == '/' && pin[1] == '*' )	/* skip comment */
		{					/* except for \n's */
			while ( pin < p )
				if ( *pin++ == '\n' )
					putc( '\n', fout );
			continue;
		}

		/*
		 *	skip // comment ivan Feb 1989 
		 */

		if ( eolcom && *pin == '/' && pin[1] == '/' )
		{
			while ( pin < p )
				if ( *pin++ == '\n' )
					putc( '\n', fout );
			continue;
		}


		if ( params )	/* mark the appearance of formals in the definiton */
		{
			if ( ( toktyp + COFF )[*pin] == IDENT )
			{
#ifdef BROWSER
				if ( cbflag )
					c = CB_CHR_MACRO_NON_FORMAL_REF;
#endif
				for ( qf = pf; --qf >= formal; )
				{
					if ( equfrm( *qf, pin, p ) )
					{
#ifndef NO_MACRO_FORMAL
						if ( cxref )
							ref(*qf, lineno[ifno]);
#endif
#ifdef BROWSER
						if ( cbflag )
						{
							c = CB_CHR_MACRO_FORMAL_REF;
							cbputid( fout, c, *qf );
						}
#endif
						*psav++ = qf - formal + 1;
						*psav++ = WARN;
						pin = p;
						break;
					}
				}
#ifdef BROWSER
				if ( cbflag &&
				     ( c == CB_CHR_MACRO_NON_FORMAL_REF ) )
					cbputid( fout, c, xcopy( pin, p ) );
#endif
			}
			/* inside quotation marks, too */
			else if ( *pin == '"' || *pin == '\''
# if gcos
					|| *pin == '`'
# endif
						)
			{
				char quoc = *pin;

				for ( *psav++ = *pin++; pin < p && *pin != quoc; )
				{
					while ( pin < p && !isid( *pin ) )
					{
						if ( *pin == '\n'
							&& pin[-1] == '\\' )
						{
							putc( '\n', fout );
							psav--;	/* no \ */
							pin++;	/* no \n */
						}
						else
							*psav++ = *pin++;
					}
					cf = pin;
					while ( cf < p && isid( *cf ) )
						++cf;
					for ( qf = pf; --qf >= formal; )
					{
						if ( equfrm( *qf, pin, cf ) )
						{
							*psav++ = qf - formal + 1;
							*psav++ = WARN;
							pin = cf;
							break;
						}
					}
					while ( pin < cf )
						*psav++ = *pin++;
				}
			}
		}
#ifdef BROWSER
		else if ( cbflag && !cbsuppress &&
			  isid( *pin ) && !isdigit( *pin ) )
			cbputid( fout, CB_CHR_MACRO_NON_FORMAL_REF,
				 xcopy( pin, p ) );
#endif
		while ( pin < p )
			if ( *pin == '\n' && pin[-1] == '\\' )
			{
				putc( '\n', fout );
				psav--;	/* no \ */
				pin++;	/* no \n */
			}
			else
				*psav++ = *pin++;
	}
	passcom = sav_passcom;	/* restore to "real" value */
	if ( psav[-1] == ' ' )	/* if token-string ended with a blank */
		psav--;		/* then it is unnecessary - throw away */
	*psav++ = params;
	*psav++ = '\0';
	if ( ( cf = oldval ) != NULL )		/* redefinition */
	{
		--cf;	/* skip no. of params, which may be zero */
		while ( *--cf )		/* go back to the beginning */
			;
		if ( 0 != strcmp( ++cf, oldsavch ) )	/* redefinition different from old */
		{
			--lineno[ifno];
			ppwarn( "%s redefined", np->name );
			++lineno[ifno];
			np->value = psav - 1;
		}
		else
			psav = oldsavch; /* identical redef.; reclaim space */
	}
	else
		np->value = psav - 1;
	--flslvl;
	inp = pin;
	savch = psav;
#ifdef BROWSER
	if ( cbflag && !cbsuppress )
	{
		putc( CB_CHR_PREFIX, fout );
		putc( CB_CHR_MACRO_DEF_END, fout );
	}
#endif
	return( p );
}

#define fasscan()	ptrtab = fastab + COFF
#define sloscan()	ptrtab = slotab + COFF
/* this macro manages the lookup of a macro name and produces
** a warning if the name is missing
*/
#define LOOKUP(skipblank, flag) \
	++flslvl; if (skipblank) p = skipbl(p);	\
	if ( ( toktyp + COFF )[*inp] != IDENT )	\
	{					\
		if (*inp == '\n') --lineno[ifno];\
		ppwarn( "illegal/missing macro name" );	\
		if (*inp == '\n') ++lineno[ifno];\
		while ( *inp != '\n' )		\
			p = skipbl( p );	\
		--flslvl;			\
		continue;			\
	}					\
	np = slookup(inp,p,flag); --flslvl

void
control( p )		/* find and handle preprocessor control lines */
	register char *p;
{
	register struct symtab *np;
	int save_passcom;
#ifdef BROWSER
	int cbline, cbflslvl = flslvl, cbkludge = 0;
#endif

	for ( ;; )
	{
		fasscan();
		p = cotoken( p );
		if ( *inp == '\n' )
			++inp;
		dump();
		sloscan();
		p = skipbl( p );
		*--inp = SALT;
		outp = inp;
		++flslvl;
		np = slookup( inp, p, 0 );
		--flslvl;
		if ( np == defloc )	/* define */
		{
			if ( flslvl == 0 )
			{
				p = dodef( p ); 
				continue;
			}
		}
		else if ( np == incloc )	/* include */
		{
			p = doincl( p );
			continue;
		}
		else if ( np == ifnloc )	/* ifndef */
		{
			LOOKUP(1/*skipbl*/, 0/*flag*/);	/* sets "np" */
			if ( flslvl == 0 && np == 0 )
				++trulvl;
			else
				++flslvl;
#ifdef BROWSER
			if ( cbflag && ( flslvl <= 1 ) )
				cbputid( fout,
					 ( flslvl == 0) ?
					     CB_CHR_IFNDEF_UNDEFD :
					     CB_CHR_IFNDEF_DEFD,
					 xcopy( inp, p ) );
#endif
			if ( trulvl + flslvl >= MAX_DEPTH )
				pperror( "#if, #ifdef, #ifndef nesting too deep" );
			else
				ifelstk[trulvl + flslvl] =
					flslvl ? 0 : TRUE_ELIF;
			if ( cxref )
				ref(xcopy(inp, p), lineno[ifno]);
			p = chkend( p, 1 );
		}
		else if ( np == ifdloc )	/* ifdef */
		{
			LOOKUP(1/*skipbl*/, 0/*flag*/);	/* sets "np" */
			if ( flslvl == 0 && np != 0 )
				++trulvl;
			else
				++flslvl;
#ifdef BROWSER
			if ( cbflag && ( flslvl <= 1 ) )
				cbputid( fout,
					 ( flslvl == 0) ?
					     CB_CHR_IFDEF_DEFD :
					     CB_CHR_IFDEF_UNDEFD,
					   xcopy( inp, p ) );
#endif
			if ( trulvl + flslvl >= MAX_DEPTH )
				pperror( "#if, #ifdef, #ifndef nesting too deep" );
			else
				ifelstk[trulvl + flslvl] =
					flslvl ? 0 : TRUE_ELIF;
			if ( cxref )
				ref(xcopy(inp, p), lineno[ifno]);
			p = chkend( p, 1 );
		}
		else if ( np == eifloc )	/* endif */
		{
			if ( flslvl )
			{
				if ( --flslvl == 0 )
					sayline( NOINCL );
			}
			else if ( trulvl )
				--trulvl;
			else
				pperror( "If-less endif", 0 );
			p = chkend( p, 1 );
		}
		else if ( np == elifloc )	/* elif */
		{
			if ( ifelstk[trulvl + flslvl] & SEEN_ELSE )
				pperror( "#elif following #else", 0 );
			if ( flslvl )
			{
				if ( --flslvl != 0 )
					++flslvl;
				else
				{
					newp = p;
					if ( cxref )
						xline = lineno[ifno];
					if ( ifelstk[trulvl + flslvl + 1] == 0 )
					{
#ifdef BROWSER
						if ( cbflag )
						{
							sayline( NOINCL );
							cbifsay = lineno[ifno];
							cbflslvl = 0;
							putc( CB_CHR_PREFIX, fout );
							putc( CB_CHR_END_INACTIVE, fout );
							putc( CB_CHR_PREFIX,
							      fout );
							putc( CB_CHR_IF,
							      fout );
						}
#endif BROWSER
						if ( yyparse() )
						{
							++trulvl;
							ifelstk[trulvl + flslvl] =
								TRUE_ELIF;
							sayline( NOINCL );
						}
						else
							++flslvl;
#ifdef BROWSER
						if ( cbflag )
						{
							putc( CB_CHR_PREFIX,
								fout );
							putc( ( flslvl == 0 ) ?
							    CB_CHR_IF_END_ACTIVE :
							    CB_CHR_IF_END_INACTIVE,
							    fout );
						}
#endif BROWSER
					}
					else
						++flslvl;
#ifdef BROWSER
					if ( lineno[ifno] != cbifsay + 1 )
					{
						/* Fix for non-cb mode too! */
						--lineno[ifno];
						sayline( NOINCL );
						++lineno[ifno];
					}
					cbifsay = cbifline = 0;
#endif BROWSER
					p = newp;
				}
			}
			else if ( trulvl )
			{
				++flslvl;
				--trulvl;
#ifdef BROWSER
				cbkludge++;
#endif BROWSER
			}
			else
				pperror( "If-less elif", 0 );
		}
		else if ( np == elsloc )	/* else */
		{
			if ( ifelstk[trulvl + flslvl] & SEEN_ELSE )
				pperror( "too many #else's", 0 );
			if ( flslvl )
			{
 				if ( --flslvl != 0 )
					++flslvl;
				else if ( ifelstk[trulvl + flslvl + 1] == 0 )
				{
					++trulvl;
					sayline( NOINCL );
				}
				else
					++flslvl;
			}
			else if ( trulvl )
			{
				++flslvl;
				--trulvl;
			}
			else
				pperror( "If-less else", 0 );
			ifelstk[trulvl + flslvl] |= SEEN_ELSE;
			p = chkend( p, 1 );
		}
		else if ( np == udfloc )	/* undefine */
		{
			++flslvl;
			p = skipbl( p );	/* inp -> start of undef id */
			--flslvl;
			if ( cxref )
				ref(xcopy(inp, p), lineno[ifno]);
			if ( flslvl == 0 )
			{
				LOOKUP(0/*skipbl*/, DROP); /* sets "np" */
#ifdef BROWSER
				if ( cbflag && ( flslvl == 0 ) )
					cbputid( fout, CB_CHR_UNDEF,
						 xcopy( inp, p ) );
#endif
				p = chkend( p, 1 );
			}
			else
				p = chkend( p, 2 );
		}
		else if ( np == ifloc )		/* if */
		{
#if tgp
			pperror( " IF not implemented, true assumed", 0 );
			if ( flslvl == 0 )
				++trulvl;
			else
				++flslvl;
#else
#	ifdef BROWSER
			int cbtemp;

			cbifsay = cbifline = lineno[ifno];
			cbtemp = 0;
			if ( cbflag && ( flslvl == 0 ) )
			{
				cbtemp = 1;
				putc( CB_CHR_PREFIX, fout );
				putc( CB_CHR_IF, fout );
			}
#	endif
			newp = p;
			if ( cxref )
				xline = lineno[ifno];
			if ( flslvl == 0 && yyparse() )
				++trulvl;
			else
				++flslvl;
#	ifdef BROWSER
			if ( cbtemp )
			{
				putc( CB_CHR_PREFIX, fout );
				putc( ( flslvl == 0 ) ? CB_CHR_IF_END_ACTIVE :
				      CB_CHR_IF_END_INACTIVE, fout );
			}
			if ( lineno[ifno] != cbifsay + 1 )
			{
				--lineno[ifno];
				sayline( NOINCL );
				++lineno[ifno];
			}
			cbifsay = cbifline = 0;
#	endif
			p = newp;
			if ( trulvl + flslvl >= MAX_DEPTH )
				pperror( "#if, #ifdef, #ifndef nesting too deep" );
			else
				ifelstk[trulvl + flslvl] =
					flslvl ? 0 : TRUE_ELIF;
#endif
		}
		else if ( np == lneloc )	/* line */
		{
			if ( flslvl == 0 && ignoreline == 0 )
			{
				register char *s;
				register int ln;

				outp = inp = p;
			do_line:
#ifdef BROWSER
				if ( cbflag )
				{
					putc( CB_CHR_PREFIX, fout );
					putc( CB_CHR_POUND_LINE, fout );
				}
#endif
				*--outp = '#';
				/*
				* make sure that the whole
				* directive has been read
				*/
				s = p;
				while ( *s && *s != '\n' )
					s++;
				if ( eob( s ) )
					(void)refill( s );
				/*
				* eat the line number
				*/
				s = p = inp;
				while ( ( toktyp + COFF )[*s] == BLANK )
					s++;
				ln = 0;
				while ( isdigit( *s ) )
					ln = ln * 10 + *s++ - '0';
				if ( ln )
					lineno[ifno] = ln - 1;
				else
					pperror( "bad number for #line" );
				/*
				* eat the optional "filename"
				*/
				while ( ( toktyp + COFF )[*s] == BLANK )
					s++;
				if ( *s != '\n' )
				{
						register char *t = savch;

						if ( *s != '"' ) 
							*t++ = *s;
						for ( ;; )
						{
							if ( *++s == '"' )
								break;
							else if ( *s == '\n' || *s == '\0' ) 
								break;
							*t++ = *s;
						}
						*t++ = '\0';
						if ( strcmp( savch, fnames[ifno] ) )
						{
							fnames[ifno] = savch;
							savch = t;
						}
				}
				/*
				* push it all along to be eventually printed
				*/
				save_passcom = passcom;
				passcom = 0;
				while ( *inp != '\n' )
					p = cotoken( p );
				passcom = save_passcom;
				continue;
			}
		}
		else if ( clsloc != 0 && np == clsloc )	/* class */
			exfail = CLASSCODE;		/* return value */
		else if ( np == pragloc  || np == idtloc ) /* pragma, ident */
		{
			register char *s;
			/* pass it through unfiltered */
			s = p;
#ifdef BROWSER
			if ( cbflag && ( np == idtloc) )
			{
				char * startp, * endp;
				++flslvl;
				if ( *p != '\n' )
					p = cotoken( p );
				startp = p;
				while ( (*p != '\n' ) && ( p[-1] != '\n' ) &&
					( ( *p != '/' ) || ( p[1] != '*' ) ) )
					p = cotoken( p );
				if ( p[-1] == '\n' )
					p--;
				endp = p;
				while ( ( *startp == ' ' ) ||
				        ( *startp == '\t' ) )
					startp++;
				while ( ( endp[-1] == ' ' ) ||
					( endp[-1] == '\t' ) )
					endp--;
				if ( endp > startp )
					cbputid( fout, CB_CHR_IDENT,
						 xcopy( startp, endp ) );
				--flslvl;
			}
			if ( cbflag )
			{
				putc( CB_CHR_PREFIX, fout );
				putc( CB_CHR_SAYLINE, fout );
			}
#endif
			while ( *s && *s != '\n' )
				s++;
			if ( eob( s ) )
				p = refill( s );
			save_passcom = passcom;
			passcom = 0;
			while ( *inp != '\n' )
				p = cotoken( p );
			passcom = save_passcom;
#ifdef BROWSER
			if (cbflag)
			{
				dump();
			}
#endif

		}
		else if ( *++inp == '\n' )	/* allows blank line after # */
			outp = inp;
		else if ( isdigit( *inp ) )	/* pass thru line directives */
		{
			outp = p = inp;
			goto do_line;
		}
		else
			pperror( "undefined control", 0 );
		/* flush to lf */
#ifdef BROWSER
		cbline = lineno[ifno];
#endif
		++flslvl;
		while ( *inp != '\n' )
		{
			outp = inp = p;
			p = cotoken( p );
		}
		--flslvl;
#ifdef BROWSER
		if ( cbflag && ( cbline + 1 != lineno[ifno] ) )
		{
			--lineno[ifno];
			sayline( NOINCL );
			++lineno[ifno];
		}
		if ( cbflag && ( cbflslvl != flslvl ) )
		{
			if ( ( cbflslvl == 1 ) && ( flslvl == 0 ) )
			{
				putc( CB_CHR_PREFIX, fout );
				putc( CB_CHR_END_INACTIVE, fout );
			}
			if ( ( cbflslvl == 0 ) && ( flslvl == 1 ) )
			{
				if ( cbkludge )
				{
					/* Make #elif look inactive: */
					lineno[ifno] -= 2;
					sayline( NOINCL );
					lineno[ifno] += 2;
					cbkludge = 0;
				}
				putc( CB_CHR_PREFIX, fout );
				putc( CB_CHR_START_INACTIVE, fout );
			}
			cbflslvl = flslvl;
		}
#endif
	}
}

struct symtab *
stsym( s )
	register char *s;
{
	char buf[PAGESIZ];
	register char *p;

	/* make definition look exactly like end of #define line */
	/* copy to avoid running off end of world when param list is at end */
	p = buf;
	while ( *p++ = *s++ )
		;
	p = buf;
	while ( isid( *p++ ) )		/* skip first identifier */
		;
	if ( *--p == '=' )
	{
		*p++ = ' '; 
		while ( *p++ )
			;
	}
	else
	{
		s = " 1";
		while ( *p++ = *s++ )
			;
	}
	pend = p;
	*--p = '\n';
	sloscan();
	dodef( buf );
	return( lastsym );
}

struct symtab *
ppsym( s )		/* kludge */
	char *s;
{
	register struct symtab *sp;

	cinit = SALT;
	*savch++ = SALT;
	sp = stsym( s );
	--sp->name;
	cinit = 0;
	return( sp );
}


int yy_errflag;		/* TRUE when pperror called by yyerror() */

/* VARARGS1 */
pperror( s, x, y )
	char *s;
{
	if ( fnames[ifno] && fnames[ifno][0] )
# if gcos
		fprintf( stderr, ERRFORM, exfail >= 0 ? 'F' : 'W',fnames[ifno]);
# else
		fprintf( stderr, "%s: ", fnames[ifno] );
# endif
	fprintf( stderr, "%d: ", lineno[ifno] );
	fprintf( stderr, s, x, y );
	if ( yy_errflag )
		fprintf( stderr, " (in preprocessor if)\n" );
	else
		fprintf( stderr, "\n" );
	if ( exfail < CLASSCODE - 1 )
		++exfail;
}

yyerror( s, a, b )
	char *s;
{
	yy_errflag = 1;
	pperror( s, a, b );
	yy_errflag = 0;
}

ppwarn( s, x )
	char *s;
{
	int fail = exfail;

	exfail = -1;
	pperror( s, x );
	exfail = fail;
}

/* getspace is used to get the next available space from the */
/* free list; if there is no free space, it will allocate    */
/* a new block of BKTINC elements                            */
struct symtab *
getspace()
{
	register struct symtab *bp;

	if ( nfree == 0 )
	{
		sfree = (struct symtab *)calloc( BKTINC, ELEMSIZ );
		if ( sfree == NULL )
		{
			pperror( "too many defines", 0 );
			exit( exfail ? (exfail ==
				CLASSCODE ? CLASSCODE : 2) : 0 );
		}
		nfree = BKTINC;
	}
	
	bp = sfree;
	sfree++;
	nfree--;
	return (bp);
}        

struct symtab *
lookup( namep, enterf )
	char *namep;
{
	register char *np, *snp;
	register int c, i;
	register struct symtab *sp;
	struct symtab *prev;

	/* namep had better not be too long (currently, <=ncps chars) */
	np = namep;
	i = cinit;
	while ( c = *np++ )
		i += i + c;
	c = i;	/* c=i for register usage on pdp11 */
	c %= symsiz;
	if ( c < 0 )
		c += symsiz;
	sp = stab[c];
	prev = sp;
	while ( sp )
	{
		snp = sp->name;
		np = namep;
		while ( *snp++ == *np )
			if ( *np++ == '\0' )
			{
				if ( enterf == DROP )
				{
					sp->name[0] = DROP;
					sp->value = 0;
				}
				/* symbol already in symbol table; return a */
				/* pointer to it */
				return( lastsym = sp );
			}
		prev = sp;
		sp = sp->next;
	}
	if ( enterf == 1 )
	{
		/* Since symbol is not in the symbol table */
		/* get a bucket for it and put the bucket  */
		/* in the symbol table                     */
		sp = getspace();
		sp->name = namep;
		sp->value = 0;
		sp->next = NULL;
		if ( prev )
			prev->next = sp;
		else
			stab[c] = sp;
	}
	return( lastsym = sp );
}

struct symtab *
slookup( p1, p2, enterf )
	register char *p1, *p2;
	int enterf;
{
	register char *p3;
	char c2, c3;
	struct symtab *np;

	c2 = *p2;	/* mark end of token */
	*p2 ='\0';
	if ( ( p2 - p1 ) > ncps )
		p3 = p1 + ncps;
	else
		p3 = p2;
	c3 = *p3;	/* truncate to ncps chars or less */
	*p3 ='\0';
	if ( enterf == 1 )
		p1 = copy( p1 );
	np = lookup( p1, enterf );
	*p3 = c3;
	*p2 = c2;
	if ( np && flslvl == 0 )
		newp = subst( p2, np );
	else
		newp = 0;
	return( np );
}

char *
subst( p, sp )
	register char *p;
	struct symtab *sp;
{
	static char match[] = "%s: argument mismatch";
	register char *ca, *vp;
	int params;
	int wasfast = 0;
	char *actual[MAXFRM]; /* actual[n] is text of nth actual */
	char acttxt[PAGESIZ]; /* space for actuals */
	int hadactuals;
#ifdef BROWSER
	char cbchr;
	int cbline, cbsayline;
#endif

	vp = sp->value;
	if ( ( p - macforw ) <= macdam )
	{
		if ( ++maclvl > symsiz && !rflag )
		{
			pperror( "%s: macro recursion", sp->name );
			return( p );
		}
	}
	else
		maclvl = 0;	/* level decreased */
	macforw = p;	/* new target for decrease in level */
	macdam = 0;
	macnam = sp->name;
	if ( cxref )
		ref(macnam, lineno[ifno]);
	dump();
	if ( sp == ulnloc )
	{
		vp = acttxt;
		*vp++ = '\0';
		sprintf( vp, "%d", lineno[ifno] );
		while ( *vp++ )
			;
	}
	else if ( sp == uflloc )
	{
		vp = acttxt;
		*vp++ = '\0';
		sprintf( vp, "\"%s\"", fnames[ifno] );
		while ( *vp++ )
			;
	}
#ifdef BROWSER
	cbinsubst = 1;
	cbchr = CB_CHR_MACRO_REF_WO_FORMALS_DEFD;
	cbsayline = cbline = lineno[ifno];
#endif
	if ( 0 != ( params = *--vp & 0xFF ) )	/*definition calls for params */
	{
		register char **pa;
		int dparams;		/* parameters in definition */

#ifdef BROWSER
		cbchr = CB_CHR_MACRO_REF_W_FORMALS_DEFD;
#endif
		ca = acttxt;
		pa = actual;
		if ( params == 0xFF )	/* #define foo() ... */
			params = 1;
		dparams = params;
		if ( !isslo )
		{
			sloscan();
			wasfast++;
		}
		++flslvl; /* no expansion during search for actuals */
		expanding_params= sp->name;	/* who's being expanded? */

		plvl = 0;
		maclin = lineno[ifno];
		macfil = fnames[ifno];
		hadactuals = 1;
		do
		{
			p = skipbl( p );
		}
		while ( *inp == '\n' );		/* skip \n too */

		expanding_params= 0;

		if ( *inp == '(' )
		{
			for ( plvl = 1; plvl != 0; )
			{
				*ca++ = '\0';
				for ( ;; )
				{
					outp = inp = p;
					p = cotoken( p );
					if ( *inp == '(' )
						++plvl;
					if ( *inp == ')' && --plvl == 0 )
					{
						if ( ca > acttxt+1 )
							--params;
						break;
					}
					if ( plvl == 1 && *inp == ',' )
					{
						--params;
						break;
					}
#ifdef BROWSER
					if ( cbflag && ( inp >= cbmacend ) &&
					   ( ( *inp == '"') ||
					     ( ( *inp == '\'' ) &&
					       ( cbsuffix == 'p' ) ) ||
					     ( isid( *inp ) &&
					       !isdigit( *inp ) ) ) )
					{
						char c;

						if (cbsayline != lineno[ifno] )
						{
							cbsayline = lineno[ifno];
							sayline( NOINCL );
						}
						if ( ( *inp == '"' ) ||
						      ( ( *inp == '\'' ) &&
							( cbsuffix == 'p' ) ) )
							c = CB_CHR_MACRO_STRING_ACTUAL;
						else
							c = CB_CHR_MACRO_ACTUAL_REF;
						cbputid( fout, c,
							 xcopy( inp, p ) );
					}
#endif
					while ( inp < p )
					{
						/*
						*  toss newlines in arguments
						*  to macros - keep problems
						*  to a minimum.
						*  if a backslash is just
						*  before the newline, assume
						*  it is in a string and
						*  leave it in.
						*/
						if ( *inp == '\n'
							&& inp[-1] != '\\' )
						{
							*inp = ' ';
						}
						*ca++ = *inp++;
					}
					if ( ca > &acttxt[PAGESIZ] ) {
						pperror( "%s: actuals too long", sp->name );
						ca = &acttxt[PAGESIZ];
					}
				}
				if ( pa >= &actual[MAXFRM] )
					ppwarn( match, sp->name );
				else
					*pa++ = ca;
			}
		}
		else {				/* should have seen (, because def
						** had args
						*/
			ppwarn( match, sp->name );
			p = inp;		/* back up to current token */
		}
		/* def with one arg and use with zero args is special ok case */
		if ( params < 0 || (params > 0 && dparams != 1) )
			ppwarn( match, sp->name );
		while ( --params >= 0 )
			*pa++ = "" + 1;	/* null string for missing actuals */
		--flslvl;
		if ( wasfast )
			fasscan();
	} else
		hadactuals = 0;
#ifdef BROWSER
	if ( cbflag )
	{
		if ( cbmacend == NULL )
		{
			if ( cbifsay != 0 )
			{
				sayline( NOINCL );
				cbputid( fout, cbchr, macnam );
			}
			else if ( cbline != lineno[ifno] )
			{
				cbsayline = lineno[ifno];
				lineno[ifno] = cbline;
				sayline( NOINCL );
				cbputid( fout, cbchr, macnam );
				lineno[ifno] = cbsayline;
				/* Embedded lf code does not always work */
				sayline( NOINCL );
			}
			else
				cbputid( fout, cbchr, macnam );
		}
		if (p > cbmacend )
			cbmacend = p;
	}
#endif
	for ( ;; )	/* push definition onto front of input stack */
	{
		while ( !iswarn( *--vp ) )
		{
			if ( bob( p ) )
			{
				outp = inp = p;
				p = unfill( p );
			}
			*--p = *vp;
		}
		if ( *vp == warnc )	/* insert actual param */
		{
			ca = actual[*--vp - 1];
			while ( *--ca )
			{
				if ( bob( p ) )
				{
					outp = inp = p;
					p = unfill( p );
				}
				*--p = *ca;
			}
		}
		else
			break;
	}
	if ( hadactuals && maclin != lineno[ifno] )	/* embedded linefeeds in macro call */
	{
		int i, j = lineno[ifno];

		for ( i = sizeof( int ) / sizeof( char ); --i >= 0; )
		{
			if ( bob( p ) )
			{
				outp = inp = p;
				p = unfill( p );
			}
			*--p = j;
			j >>= 8;
		}
		if ( bob( p ) )
		{
			outp = inp = p;
			p = unfill( p );
		}
		*--p = warnc;
	}
#ifdef BROWSER
	cbinsubst = 0;
#endif
	outp = inp = p;
	return( p );
}




char *
trmdir( s )
	register char *s;
{
	register char *p = s;

	while ( *p++ )
		;
	--p;
	while ( p > s && *--p != '/' )
		;
# if unix || DMERT
	if ( p == s )
		*p++ = '.';
# endif
	*p = '\0';
	return( s );
}

STATIC char *
copy( s )
	register char *s;
{
	register char *old, *new;

	old = new = savch;
	while ( *new++ = *s++ )
		;
	savch = new;
	return( old );
}

allocsbf()
{
	sbf = (char *)malloc(SBSIZE);
	if (sbf == NULL)
	{
		pperror( "out of buffer space" );
		exit(1);
	}
	savch = sbf;
	*savch++ = '\0';
}

yywrap()
{
	return( 1 );
}

main( argc, argv )
	char *argv[];
{
	register int i, c;
	register char *p;
	char *tf, *cp;
	int forclass = 0;		/* 1 if C-with-classes selected */
#ifdef SUNPRO
	char *p1, *p2;
	struct symtab *sp;
	char dashi[1024];
#endif
#ifdef BROWSER
	struct stat status;
#endif

	dashi[0] = '\0';
	/* initialize hash table */
	for (i = 0; i < symsiz; i++)
		stab[i] = NULL;

# if gcos
	if ( setjmp( env ) )
		return( exfail ? ( exfail == CLASSCODE ? CLASSCODE : 2 ) : 0 );
# endif
	allocsbf();
	if (port_flag)
		p = "_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	else
		p = "_$ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	i = 0;
	while ( c = *p++ )
	{
		( fastab + COFF )[c] |= IB | NB | SB;
		( toktyp + COFF )[c] = IDENT;
#if scw2
		/* 53 == 63-10; digits rarely appear in identifiers,
		/* and can never be the first char of an identifier.
		/* 11 == 53*53/sizeof(macbit) .
		*/
		++i;
		( t21 + COFF )[c] = ( 53 * i ) / 11;
		( t22 + COFF )[c] = i % 11;
#endif
	}
	p = "0123456789.";
	while ( c = *p++ )
	{
		( fastab + COFF )[c] |= NB | SB;
                ( toktyp + COFF )[c] = NUMBR;
	}
# if gcos
	p = "\n\"'`/\\";
# else
	p = "\n\"'/\\";
# endif
	while ( c = *p++ )
		( fastab + COFF )[c] |= SB;
# if gcos
	p = "\n\"'`\\";
# else
	p = "\n\"'\\";
# endif
	while ( c = *p++ )
		( fastab + COFF )[c] |= QB;
	p = "*\n";
	while ( c = *p++ )
		( fastab + COFF)[c] |= CB;
	( fastab + COFF )[warnc] |= WB | SB;
	( fastab + COFF )['\0'] |= CB | QB | SB | WB;
	for ( i = ALFSIZ; --i >= 0; )
		slotab[i] = fastab[i] | SB;
	p = " \t\v\f\r";	/* note no \n; */
	while ( c = *p++ )
		( toktyp + COFF )[c] = BLANK;
#if scw2
	for ( ( t23 + COFF )[i = ALFSIZ + 7 - COFF] = 1; --i >= -COFF; )
		if ( ( ( t23 + COFF )[i] = ( t23 + COFF + 1 )[i] << 1 ) == 0 )
			( t23 + COFF )[i] = 1;
#endif

# if unix || DMERT
	fnames[ifno = 0] = "";
#ifdef SUNPRO
	dirnams[0] = ".";
	add_dir_to_path(dirnams[0], &dirs, 0);
#else
	dirnams[0] = dirs[0] = ".";
#endif
	realnames[ifno] = "standard input";
# endif
# if ibm
	fnames[ifno = 0] = "";
	realnames[ifno] = "standard input";
# endif
# if gcos
	if ( inquire( stdin, _TTY ) )
		freopen( "*src", "rt", stdin );
# endif
# if gimpel || gcos
	fnames[ifno = 0] = (char *) inquire( stdin, _FILENAME );
	realnames[ifno] = fnames[ifno];
	dirnams[0] = dirs[0] = trmdir( copy( fnames[0] ) );
# endif
	for ( i = 1; i < argc; i++ )
	{
		switch ( argv[i][0] )
		{
		case '-':
# if gcos
					/* case-independent on GCOS */
			switch ( toupper( argv[i][1] ) )
# else
			switch( argv[i][1] )
# endif
			{
			case 'M':
				mflag++;
				pflag++;
#ifdef BROWSER
				if ( cbflag )
				{
					perror( "-M disables -cb" );
					cbflag = 0;
				}
#endif
				continue;
			case 'P':
				ignoreline++;
				pflag++;
#ifdef BROWSER
				if ( cbflag )
				{
					perror( "-P disables -cb" );
					cbflag = 0;
				}
#endif
				continue;
			case 'E':
				continue;
			case 'R':
				++rflag;
				continue;
			case 'C':
				passcom++;
				continue;
			case 'B':
				eolcom++;
				continue;
			case 'F':
				if ((outfp = fopen(argv[++i],"w")) == NULL)
				{
					fprintf(stderr, "Can't open ");
					perror(argv[i]);
					exit(1);
				}
				++cxref;
				continue;
			case 'D':
				/* This option is dealt with later. */
				continue;
			case 'U':
				/* This option is dealt with later. */
				continue;
			case 'n':
				if (strcmp(argv[i], "-noinclude") == 0) {
					default_include = 0;
				} else {
					pperror( "unknown flag %s", argv[i] );
				}
				continue;
			case 'u':
				if (strcmp(argv[i], "-undef") == 0) {
					predefining = 0;
				} else {
					pperror( "unknown flag %s", argv[i] );
				}
				continue;
			case 'I':
				if ( nd > MAXINC - 4 )
					pperror( "excessive -I file (%s) ignored", argv[i] );
				else
#ifdef SUNPRO
					add_dir_to_path(argv[i] + 2, &dirs, nd++);
					strcat(dashi, argv[i]);
					strcat(dashi, " ");
#else
					dirs[nd++] = argv[i] + 2;
#endif
				continue;
			case 'Y':
				dfltdir = argv[i] + 2;
				continue;
			case '\0':
				continue;
			case 'T':
				ncps = 8;	/* backward name compatabilty */
				continue;
			case 'H':		/* print included filenames */
				print_incs++;
				continue;
#ifdef BROWSER
			case 'c':
				if ( strncmp( argv[i], "-cb" , 3 ) == 0 )
				{
					if ( pflag )
						perror( "-P and - M disable -cb" );
					else
					{
						( fastab + COFF )[CB_CHR_PREFIX] |= QB;
						++cbflag;
					}
					cbsuffix = argv[i][strlen( argv[i] ) - 1];
				}
				else
					pperror( "unknown flag %s", argv[i] );
				continue;
#endif
			case 'W':		/* enable C with classes */
				forclass = 1;
				continue;
			case 'p':
				port_flag++;	/* reject #endif FOO, $ in identifiers */
				ncps = 8;	/* restrict names to 8 characters */
				continue;
			default: 
				pperror( "unknown flag %s", argv[i] );
				continue;
			}
		default:
			if ( fin == stdin )
			{
#ifdef SUNPRO
				int fd;
				fd = open_vroot(argv[i], O_RDONLY, 0, NULL, VROOT_DEFAULT);
				if ((fd < 0) || ( NULL == ( fin = fdopen( fd, READ ) ) ) )
#else
				if ( NULL == ( fin = fopen( argv[i], READ ) ) )
#endif
				{
					fprintf( stderr, "cpp: ");
					perror( argv[i] );
					exit(2);
				}
				fnames[ifno] = copy( argv[i] );
				realnames[ifno] = fnames[ifno];
				infile = copy( argv[i] );
				if ( cxref )
					fprintf(outfp,"\"%s\"\n", fnames[ifno]);
#ifdef BROWSER
				cbsuffix = argv[i][strlen( argv[i] ) - 1];
#endif
#ifdef SUNPRO
				if (dashi[0] != '\0')
					dashi[strlen(dashi)-1] = '\0';
				report_search_path(dashi);
				dirnams[ifno] = trmdir( argv[i] );
				add_dir_to_path(dirnams[ifno], &dirs, 0);
#else
				dirs[0] = dirnams[ifno] = trmdir( argv[i] );
#endif
# ifndef gcos
/* too dangerous to have file name in same syntactic position
   be input or output file depending on file redirections,
   so force output to stdout, willy-nilly
	[i don't see what the problem is.  jfr]
*/
			}
			else if ( fout == stdout )
			{
				if ( NULL == ( fout = fopen( argv[i], WRITE ) ) )
				{
					fprintf( stderr, "cpp: Can't create ");
					perror( argv[i] );
					exit(2);
				}
				else
					fclose( stdout );
# endif
			}
			else
				pperror( "extraneous name %s", argv[i] );
		}
	}

	if ( mflag )
	{
		if ( infile == NULL )
		{
			fprintf( stderr,
				"cpp: no input file specified with -M flag\n" );
			exit(8);
		}
		tf = strrchr( infile, '.' );
		if ( tf == NULL )
		{
			fprintf( stderr, "cpp: missing component name on %s\n",
				infile );
			exit(8);
		}
		tf[1] = 'o';
		tf = strrchr( infile, '/' );
		if ( tf != NULL )
			infile = tf + 1;
		mout = fout;
		if ( NULL == ( fout = fopen( "/dev/null", "w" ) ) )
		{
			perror( "cpp: Can't open /dev/null" );
			exit(8);
		}
	}

	fins[ifno] = fin;
	exfail = 0;
	/* after user -I files here are the standard include libraries */
	/* use user-supplied default first */
	if (dfltdir != (char *) 0) {
#	    ifdef SUNPRO
		add_dir_to_path(dfltdir, &dirs, nd++);
		scan_vroot_first();
#	    else
		dirs[nd++] = dfltdir;
#	    endif
	} else {
		if (default_include) {
#		    if unix
#			ifdef SUNPRO
			    add_dir_to_path("/usr/include", &dirs, nd++);
			    scan_vroot_first();
#			else
			    dirs[nd++] = "/usr/include";
#			endif
#		    endif
#		    if gcos
			dirs[nd++] = "cc/include";
#		    endif
#		    if ibm
#			ifndef gimpel
			    dirs[nd++] = "BTL$CLIB";
#			endif
#		    endif
#		    ifdef gimpel
			dirs[nd++] = intss() ?  "SYS3.C." : "";
#		    endif
#		    ifdef compool
			dirs[nd++] = "/compool";
#		    endif
		} /* default_include */
	}

#	ifdef SUNPRO
	    /* add_dir_to_path appends a null dirname */
#	else
	    dirs[nd++] = 0;
#	endif

#	ifdef BROWSER
	cbsuppress = 1;
#	endif
	defloc = ppsym( "define" );
	udfloc = ppsym( "undef" );
	incloc = ppsym( "include" );
	elsloc = ppsym( "else" );
	eifloc = ppsym( "endif" );
	elifloc = ppsym( "elif" );
	ifdloc = ppsym( "ifdef" );
	ifnloc = ppsym( "ifndef" );
	ifloc = ppsym( "if" );
	lneloc = ppsym( "line" );
	if (forclass)			/* only when C with classes enabled */
	    clsloc = ppsym( "class" );
	idtloc = ppsym( "ident" );
	pragloc = ppsym( "pragma" );
	for ( i = sizeof( macbit ) / sizeof( macbit[0] ); --i >= 0; )
		macbit[i] = 0;
	if (predefining) {
#	    if unix
		(void) stsym( "unix" );
#	    endif
#	    if sun
		(void) stsym( "sun" );
#           endif
#	    if mc68000
		(void) stsym( "mc68000" );
#	    endif
	    /* processors */
#	    if mc68010
		(void) stsym( "mc68010" );
#	    elif mc68020
		(void) stsym( "mc68020" );
#	    elif sparc
		(void) stsym( "sparc" );
#	    elif i386
		(void) stsym( "i386" );
#	    endif

	}
#	ifdef sun
	    /*
	     * varargs hack: defining __BUILTIN_VA_ARG_INCR enables
	     * the compiler to treat the va_arg() macro as a compile
	     * time intrinsic.  [assumes that 4.1 ccom and cpp are
	     * installed together]
	     */
	    (void) stsym( "__BUILTIN_VA_ARG_INCR" );
#	endif

	tf = fnames[ifno];		/* do the -D and -U options now */
	fnames[ifno] = "command line";
	lineno[ifno] = 1;
	for ( i = 1; i < argc; i++ )
	{
		if ( argv[i][0] == '-' && argv[i][1] == 'D' )
		{
			cp = &argv[i][2];
			/* ignore plain "-D" (no argument) */
			if ( *cp )
				stsym( cp );
		}
	}
	for ( i = 1; i < argc; i++ )
	{
		if ( argv[i][0] == '-' && argv[i][1] == 'U' )
		{
			cp = &argv[i][2];
			if ( p = strchr( cp, '=' ) )
				*p = '\0';
			/*
			* Truncate to ncps characters if needed.
			*/
			if ( strlen( cp ) > ncps )
				(cp)[ncps] = '\0';
			lookup( cp, DROP );
		}
	}

	ulnloc = stsym( "__LINE__" );
	uflloc = stsym( "__FILE__" );
	fnames[ifno] = tf;
#ifdef BROWSER
	cbsuppress = 0;
#endif
	pbeg = buffer + ncps;
	pbuf = pbeg + PAGESIZ;
	pend = pbuf + PAGESIZ;

	trulvl = 0;
	flslvl = 0;
	lineno[0] = 1;
	sayline( NOINCL );
	if ( mflag )
		fprintf( mout, "%s: %s\n", infile, fnames[ifno] );
	outp = inp = pend;
	if ( cxref )
		ready = 1;
	control( pend );
	if ( fout && ferror( fout ) )
	{
		perror( "cpp: Can't write output file" );
		if ( exfail < CLASSCODE - 1 )
			++exfail;
	}
	exit( exfail ? ( exfail == CLASSCODE ? CLASSCODE : 2 ) : 0 );
}
 
ref( name, line )
	char *name;
	int line;
{
#ifdef FLEXNAMES
	fprintf(outfp, "R%s\t%05d\n", name, line);
#else
	fprintf(outfp, "R%.8s\t%05d\n", name, line);
#endif
}

def( name, line )
	char *name;
	int line;
{
	if (ready)
#ifdef FLEXNAMES
		fprintf(outfp, "D%s\t%05d\n", name, line);
#else
		fprintf(outfp, "D%.8s\t%05d\n", name, line);
#endif
}

newf( name, line )
	char *name;
	int line;
{
#ifdef FLEXNAMES
	fprintf(outfp, "F%s\t%05d\n", name, line);
#else
	fprintf(outfp, "F%.8s\t%05d\n", name, line);
#endif
}

char *
xcopy( ptr1, ptr2 )
	register char *ptr1, *ptr2;
{
#ifdef BROWSER
	/* The code browser needs an xcopy that never truncates. */
	static char name[NCPS + 1];
	static char *buffer = name;
	static int size = NCPS;
	int len;

	len = ptr2 - ptr1;
	if ( len > size )
	{
		size = len;
		buffer = malloc( size + 1 );
	}
	bcopy(ptr1, buffer, len);
	buffer[len] = '\0';
	return buffer;
#else
	static char name[NCPS+1];
	char *saveptr, ch;
	register char *ptr3 = name;

	/* locate end of name; save character there */
	if ((ptr2 - ptr1) > ncps)
	{
		saveptr = ptr1 + ncps;
		ch = *saveptr;
		*saveptr = '\0';
	}
	else {
		ch = *ptr2;
		*ptr2 = '\0';
		saveptr = ptr2;
	}
	while (*ptr3++ = *ptr1++) ;	/* copy name */
	*saveptr = ch;	/* replace character */
	return( name );
#endif
}

/* Scan over the end of a directive, making sure there's no
** residual junk.
**
** This code is a little messier than you might expect.  To avoid bumping
** the line number when we hit the newline, print the message the first time
** we find we've got too many tokens.
*/

static char *
chkend( p, ntoken )
char * p;
int ntoken;				/* number of tokens at which to warn */
{
    int extratokens = 0;
    int save_passcom = passcom;

    if (eolcom) passcom = 1;		/* ignore comments in this context */
    else passcom=0;
    ++flslvl;				/* don't expand anything */

    /* if at newline, force rescan to do end-of-line stuff */
    if (*inp == '\n')
	p = inp;

    for (;;) {
	p = skipbl(p);		/* get next token */
	if (*inp == '\n')
	    break;			/* reached end */
	if (port_flag && ++extratokens == ntoken)	/* hit number of extras; flag here */
	    ppwarn("extra tokens (ignored) after directive");
    }

    /* restore comment flag */
    passcom = save_passcom;
    --flslvl;				/* back to previous expansion level */

    return( p );
}

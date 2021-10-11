#ifndef lint
static	char sccsid[] = "@(#)scan.c 1.1 94/10/31 SMI"; /* from S5R2 1.4 */
#endif

#if !defined(BROWSER) && !defined(LINT) && !defined(CXREF)
#include "+++ -DBROWSER should be left ON! +++"
#endif

# include <ctype.h>
# include "cpass1.h"
# include "messages.h"
# ifndef LINT
# include <a.out.h>
# include <stab.h>
# endif

/* 
 * Guy's changes to merge with system V lint:
 *
 * Here's the updated "scan.c".  As before, the bulk of the changes are
 * message-related.  Some "lint" cleanup was done (eliminating unused
 * variables, casting one call to "xgetchar()" to "void").  Some S5 "lint"
 * additions were made (support for truncating names to 8 characters when
 * the "-p" flag is passed to the S5 version, stuff to eliminate bogus
 * "statement not reached" messages).  Also, if "-p" is passed to the S5
 * version, "$" is disallowed as a character in variable names.
 * 
 * Code to support "cxref" was also added.
 * 
 * Explicit checks for "malloc" failing were added, as per the comments
 * for "pftn.c".  I removed some #ifdef FLEXNAMES, as per previous
 * comments.
 * 
 * It prints "illegal octal constant" even for octal digits 8 and 9;
 * however, it is only a warning if 8 or 9 are used, and an error
 * otherwise.  The 4.0 "cpp" doesn't distinguish between the two cases,
 * issuing a warning for both with the same error.  If you want it to
 * issue the "not a standard octal digit" message, it's a trivial change;
 * I don't think we should encourage using 8 and 9, though.
 */

# ifdef BROWSER
# include <cb_cpp_protocol.h>
# include <cb_enter.h>
# include <cb_sun_c_tags.h>
# include <cb_ccom.h>
# endif

int asm_esc = 0; /* asm escaped used in file */

/*
 * When we have a syntax error, we'd like to faithfully report
 * the token near which the error occured. This can only be done
 * if we squarrel away the token as we read it.
 */
#ifdef BROWSER
char *tokebufr = NULL;
int tokebuflen = 0;
#else
char tokebufr[200];
#endif
int  tokebufptr = -1;
#define xreset() tokebufptr = -1;

#ifdef BROWSER
char *cb_program_name = "ccom";
int cbchr;
int cbtemp;

int
cbgetc()
{
	register chr;

	if (cb_flag) {
		do {
			if (cb_ccom_cpp_protocol()) {
				return CB_CHR_PREFIX;
			}
		} while ((chr = getchar()) == CB_CHR_PREFIX);
	} else {
		chr = CB_CHR_PREFIX;
	}
	return chr;
}

int
cbgrowtokebufr()
{
	char	*p = malloc((tokebuflen+100)*2);

	tokebuflen = (tokebuflen+99)*2;
	if (tokebufr != NULL) {
		(void)strncpy(p, tokebufr, tokebufptr);
		(void)free(tokebufr);
	}
	tokebufr = p;
}

# define cbgetchar() (((cbchr = getchar()) == CB_CHR_PREFIX)?cbgetc():cbchr)
# define xgetchar() (cbtemp=cbgetchar(),(++tokebufptr >= tokebuflen)?cbgrowtokebufr():0, tokebufr[tokebufptr] = cbtemp)
# define xungetc(c,filedes) ((((filedes==stdin)&&(tokebufptr>=0))?tokebufptr--:0),ungetc(c,filedes))

/*
 *	cb_ccom_get_char()
 */
char
cb_ccom_get_char()
{
	return getchar();
}

#else

#define xgetchar() ((++tokebufptr < sizeof tokebufr)?(tokebufr[tokebufptr] = getchar()): getchar())
#define xungetc(c,filedes) (((filedes==stdin)?tokebufptr--:0),ungetc(c,filedes))

#endif

/*
 * When we encounter a quoted string, we stash the characters therein in
 * a buffer, which is then examined during processing of a semantic action.
 * We have no idea whatsoever of the maximum size of strings, so must
 * dynamically allocate the string space. The largest amount of space we'll
 * need is no larger that the largest single string in the program.
 */
char *stringval= NULL;
int   stringmax= 0;
int   stringsize;
char  *newstring();
#define STRINGINIT 512
#define stashstring(c) {if(i<stringmax)*++nextstringchar=(c); else nextstringchar = newstring(c);}
#define mask(nbits)	((1 << (nbits)) - 1)

int	lxsizeof = 0;		/* counts nested uses of "sizeof", so that */
				/* sizeof works within initializers */

	/* lexical actions */

# define A_ERR 0		/* illegal character */
# define A_LET 1		/* saw a letter */
# define A_DIG 2		/* saw a digit */
# define A_1C 3			/* return a single character */
# define A_STR 4		/* string */
# define A_CC 5			/* character constant */
# define A_BCD 6		/* GCOS BCD constant */
# define A_SL 7			/* saw a / */
# define A_DOT 8		/* saw a . */
# define A_PL 9		/* + */
# define A_MI 10		/* - */
# define A_EQ 11		/* = */
# define A_NOT 12		/* ! */
# define A_LT 13		/* < */
# define A_GT 14		/* > */
# define A_AND 16		/* & */
# define A_OR 17		/* | */
# define A_WS 18		/* whitespace (not \n) */
# define A_NL 19		/* \n */

	/* character classes */

# define LEXLET 01
# define LEXDIG 02
# define LEXOCT 04
# define LEXHEX 010
# define LEXWS 020
# define LEXDOT 040

	/* reserved word actions */

# define AR_TY 0		/* type word */
# define AR_RW 1		/* simple reserved word */
# define AR_CL 2		/* storage class word */
# define AR_S 3		/* struct */
# define AR_U 4		/* union */
# define AR_E 5		/* enum */
# define AR_A 6		/* asm */

	/* text buffer */
#define	LXTSZ	BUFSIZ
char yytext[LXTSZ];
char * lxgcp;

#ifndef LINT
extern int gdebug;	/* from code.c; =1 if emitting symbolic debug info */
extern int twopass;	/* from code.c; =1 if generating intermediate  file */
extern int oldway;	/* from code.c; =1 if generating old sdb symbols */
extern int innerlist;	/* from code.c; =1 to list source lines in output */
extern int lastloc;	/* from code.c; identifies currently active csect */
extern void fileargs();	/* from code.c; interpret file arguments  */
#if TARGET == I386 && !defined(LINT)
extern void bg_file();
#endif
extern int ir_debug;	/* from ir_debug.c; IR debugging */
extern void listline(); /* from code.c; list source lines in output */
extern char *ir_optim_level(/*arg*/);	/* set optimization level */
extern void do_pragma(); /* from local.c; semantics of #pragma */
#endif
int truncate_flag = 0;		/* force all names <= 8 chars. */

extern unsigned int offsz;
extern unsigned int caloff();

# if TARGET == I386 && !defined(LINT)
extern int zflag, proflag;
extern int Rflag;
# endif

	/* ARGSUSED */
mainp1( argc, argv )
	int argc;
	char *argv[];
{
	register i;
	register char *cp;
	extern int idebug, bdebug, tdebug, edebug, ddebug, xdebug, gdebug;
	char *release = "4.0 Sun C compiler";

	offsz = caloff();
	for( i=1; i<argc; ++i ){
#ifdef BROWSER
		if ((*(cp=argv[i]) == '-') &&
			(*(cp+1) == 'c') &&
			(*(cp+2) == 'b')) {
			cb_enter_start(&lineno,
				       CB_CURRENT_LANGUAGE,
				       CB_CURRENT_MAJOR_VERSION,
				       CB_CURRENT_MINOR_VERSION);
			continue;
		}
#endif
		if( *(cp=argv[i]) == '-' && *++cp == 'X' ){
			while( *++cp ){
				switch( *cp ){

				case 'b':
					++bdebug;
					break;
				case 'd':
					++ddebug;
					break;
				case 'e':
					++edebug;
					break;
#ifndef LINT
				case 'G':
					oldway = 1;
					/* fall through */

				case 'g':
					++gdebug;
					/* explicitly disable optimization */
					optimize = 0;
					break;
				case 'I':
					++ir_debug;
					break;
				case 'l':
					++innerlist;
					break;
				case 'k':
				case 'K':
					++picflag;
					if (*cp == 'k')
						++smallpic;
					break;
				case 'O':
					++twopass;
					cp = ir_optim_level(cp);
					break;
#endif !LINT
				case 'i':
					++idebug;
					break;
				case 'r':
					fprintf( stderr, "Release: %s\n",
						release );
					break;
				case 't':
					++tdebug;
					break;
				case 'T':
					++truncate_flag;
					break;
				case 'x':
					++xdebug;
					break;
# if TARGET == I386 && !defined(LINT)
				case 'R':
					Rflag = 1;
					break;
				case 'P':
					++proflag;
					break;
				case 'z':
					++zflag;
					break;
# endif
				} /*switch*/
			} /*while*/
		} else if( strcmp(argv[i], "-") == 0) {
			/* "-" is dummy filename for stdin or stdout,
			   depending on order */
			fileargs("");
		} else if( argv[i][0] !=  '-' ) {
			/* argument is filename */
			fileargs(argv[i]);
		} /*else*/
	} /*for*/

# ifdef ONEPASS
	p2init( argc, argv );
# endif

	for( i=0; i<SYMTSZ; ++i ) stab[i] = NULL;
# ifndef LINT
# if TARGET != I386
	if (gdebug)
		ginit();
# endif
# endif
	lxinit();

# if TARGET == I386 && !defined(LINT)
	bg_file();
# endif

	tinit();
	mkdope();

#ifndef BROWSER
	lineno = 1;
#endif

	/* dimension table initialization */

	dimtab = (int*)malloc(DIMTABSZ*sizeof(dimtab[0]));
	if (dimtab == NULL)
		fatal( "not enough room for dimension table" );
	bzero(dimtab, (MAXTYPE+1)*sizeof(dimtab[0]));
	dimtab[UNDEF] = 0;
	dimtab[CHAR] = SZCHAR;
	dimtab[INT] = SZINT;
	dimtab[FLOAT] = SZFLOAT;
	dimtab[DOUBLE] = SZDOUBLE;
	dimtab[LONG] = SZLONG;
	dimtab[SHORT] = SZSHORT;
	dimtab[UCHAR] = SZCHAR;
	dimtab[USHORT] = SZSHORT;
	dimtab[UNSIGNED] = SZINT;
	dimtab[ULONG] = SZLONG;
	dimtab[TERROR] = SZINT;
	/* curdim starts past any of the above built-in types */
	curdim = MAXTYPE+1;
	reached = 1;
	reachflg = 0;

	yyparse();
	yyaccpt();

	ejobcode( nerrors ? 1 : 0 );
#ifdef BROWSER
	if ( cb_flag ) {
		cb_enter_pop_file();
		cb_ccom_maybe_write = NULL;
		}
#endif
	return(nerrors?1:0);

}

# ifdef ibm

# define CSMASK 0377
# define CSSZ 256

# else

# define CSMASK 0177
# define CSSZ 128

# endif

short lxmask[CSSZ+1];

#define lxalpha(c) (((unsigned)c<CSSZ) && (lxmask[c+1]&LEXLET))
#define lxdigit(c) (((unsigned)c<CSSZ) && (lxmask[c+1]&LEXDIG))
#define lxoctal(c) (((unsigned)c<CSSZ) && (lxmask[c+1]&LEXOCT))
#define lxhex(c)   (((unsigned)c<CSSZ) && (lxmask[c+1]&LEXHEX))
#define lxspace(c) (((unsigned)c<CSSZ) && (lxmask[c+1]&LEXWS))
	/* note lxspace('\n') == 0 */

lxenter( s, m ) register char *s; register short m; {
	/* enter a mask into lxmask */
	register c;

	while( c= *s++ ) lxmask[c+1] |= m;

	}


# define lxget(c,m) (lxgcp=yytext,lxmore(c,m))

lxmore( c, m )  register c, m; {
	register char *cp;

	*(cp = lxgcp) = c;
	while( c=xgetchar(), lxmask[c+1]&m ){
		if( cp < &yytext[LXTSZ-1] ){
			*++cp = c;
			}
		}
	xungetc(c,stdin);
	*(lxgcp = cp+1) = '\0';
	}

struct lxdope {
	short lxch;	/* the character */
	short lxact;	/* the action to be performed */
	short lxtok;	/* the token number to be returned */
	short lxval;	/* the value to be returned */
	} lxdope[] = {

	'@',	A_ERR,	0,	0,	/* illegal characters go here... */
	'_',	A_LET,	0,	0,	/* letters point here */
	'0',	A_DIG,	0,	0,	/* digits point here */
	' ',	A_WS,	0,	0,	/* whitespace goes here */
	'\n',	A_NL,	0,	0,
	'"',	A_STR,	0,	0,	/* character string */
	'\'',	A_CC,	0,	0,	/* character constant */
	'`',	A_BCD,	0,	0,	/* GCOS BCD constant */
	'(',	A_1C,	LP,	0,
	')',	A_1C,	RP,	0,
	'{',	A_1C,	LC,	0,
	'}',	A_1C,	RC,	0,
	'[',	A_1C,	LB,	0,
	']',	A_1C,	RB,	0,
	'*',	A_1C,	MUL,	MUL,
	'?',	A_1C,	QUEST,	0,
	':',	A_1C,	COLON,	0,
	'+',	A_PL,	PLUS,	PLUS,
	'-',	A_MI,	MINUS,	MINUS,
	'/',	A_SL,	DIVOP,	DIV,
	'%',	A_1C,	DIVOP,	MOD,
	'&',	A_AND,	AND,	AND,
	'|',	A_OR,	OR,	OR,
	'^',	A_1C,	ER,	ER,
	'!',	A_NOT,	UNOP,	NOT,
	'~',	A_1C,	UNOP,	COMPL,
	',',	A_1C,	CM,	CM,
	';',	A_1C,	SM,	0,
	'.',	A_DOT,	STROP,	DOT,
	'<',	A_LT,	RELOP,	LT,
	'>',	A_GT,	RELOP,	GT,
	'=',	A_EQ,	ASSIGN,	ASSIGN,
	-1,	A_1C,	0,	0,
	};

struct lxdope *lxcp[CSSZ+1];

lxinit(){
	register struct lxdope *p;
	register i;
	register char *cp;
	/* set up character classes */

	if (pflag)
		lxenter( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_", LEXLET );
	else
		lxenter( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_$", LEXLET );
	lxenter( "0123456789", LEXDIG );
	lxenter( "0123456789abcdefABCDEF", LEXHEX );
		/* \013 should become \v someday; \013 is OK for ASCII and EBCDIC */
	lxenter( " \t\r\b\f\013", LEXWS );
	lxenter( "01234567", LEXOCT );
	lxmask['.'+1] |= LEXDOT;

	/* make lxcp point to appropriate lxdope entry for each character */

	/* initialize error entries */

	for( i= 0; i<=CSSZ; ++i ) lxcp[i] = lxdope;

	/* make unique entries */

	for( p=lxdope; ; ++p ) {
		lxcp[p->lxch+1] = p;
		if( p->lxch < 0 ) break;
		}

	/* handle letters, digits, and whitespace */
	/* by convention, first, second, and third places */

	if (pflag)
		cp = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	else
		cp = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ$";
	while( *cp ) lxcp[*cp++ + 1] = &lxdope[1];
	cp = "123456789";
	while( *cp ) lxcp[*cp++ + 1] = &lxdope[2];
	cp = "\t\b\r\f\013";
	while( *cp ) lxcp[*cp++ + 1] = &lxdope[3];

	/* first line might have title */
#ifdef BROWSER
	lineno = 1;
	lxtitle(TRUE);
#else
	lxtitle();
#endif
	}

lxstr(lxmatch)
    register lxmatch;
{
	/* match a string or character constant, up to lxmatch */

	register c;
	register val;
	register i;
	register char *nextstringchar = stringval-1;

#ifdef BROWSER
	if (cb_flag)
		xreset();
#endif
	i=0;
	while( (c=xgetchar()) != lxmatch ){
		switch( c ) {

		case EOF:
			/* "unexpected EOF" */
			UERROR( MESSAGE( 113 ) );
			break;

		case '\n':
			/* "newline in string or char constant" */
			UERROR( MESSAGE( 78 ) );
			++lineno;
			break;

		case '\\':
			switch( c = xgetchar() ){

			case '\n':
				++lineno;
				continue;

			default:
				val = c;
				goto mkcc;

			case 'n':
				val = '\n';
				goto mkcc;

			case 'r':
				val = '\r';
				goto mkcc;

			case 'b':
				val = '\b';
				goto mkcc;

			case 't':
				val = '\t';
				goto mkcc;

			case 'f':
				val = '\f';
				goto mkcc;

			case 'v':
				val = '\013';
				goto mkcc;

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				val = c-'0';
				c=xgetchar();  /* try for 2 */
				if( lxoctal(c) ) {
					val = (val<<3) | (c-'0');
					c = xgetchar();  /* try for 3 */
					if( lxoctal(c) ) {
						val = (val<<3) | (c-'0');
						}
					else xungetc( c ,stdin);
					}
				else xungetc( c ,stdin);

				goto mkcc1;

				}
		default:
			val =c;
		mkcc:
			val = CCTRANS(val);
		mkcc1:
			if( lxmatch == '\'' ){
				val = CHARCAST(val);  /* it is, after all, a "character" constant */
				makecc( val, i );
			} else { 
				/* stash the byte into the string */
				stashstring( val );
			}
			++i;
			continue;
		}
		break;
	}
	/* end of string or  char constant */

	if( lxmatch == '"' ){
		stringsize = i;
	} else { /* end the character constant */
		/* "empty character constant" */
		if( i == 0 ) UERROR( MESSAGE( 36 ) );
		if( i>(SZINT/SZCHAR) || ( (pflag||hflag)&&i>1) )
			/* "too many characters in character constant" */
			UERROR( MESSAGE( 107 ) );
	}
#ifdef BROWSER
	if (cb_flag) {
		if (tokebufptr > cb_ccom_string_length-10) {
			cb_ccom_string_length = (tokebufptr+100)*2;
			cb_ccom_string = malloc(cb_ccom_string_length);
		}
		cb_ccom_string[0] = lxmatch;
		bcopy(tokebufr, cb_ccom_string + 1, tokebufptr + 1);
		cb_ccom_string[tokebufptr + 2] = '\0';
	}
#endif
}

char
*newstring( c )
    char c;
{
    /*
     * our string buffer has run out of room. Attempt to realloc it.
     */
    int oldstringmax = stringmax;
    if (stringmax == 0 ){
	/* initial allocation */
	stringval = (char *)malloc( STRINGINIT );
	stringmax = STRINGINIT;
    } else {
	stringmax *= 2;
	stringval = (char *)realloc(stringval, stringmax);
    }
    if (stringval == NULL  ){
	cerror("Out of string space");
    }
    stringval[oldstringmax] = c;
    return &stringval[oldstringmax];
}

lxstrlen()
{
    return stringsize;
}

lxgetstr( ct )
    register ct;
{
    /*
     * retrieve value of string constant stashed by lxstr()
     */
    register char *c = stringval;
    register int i;
    register int s;
    if( strflg ) {
	if( ct==0 || stringsize<=ct ) 
	    s = stringsize;
	else {
	    /* "string initializer too long" */
	    WERROR( MESSAGE( 81 ) );
	    s = ct-1;
	}
	for (i=0; i<s; i++)
	    putbyte( *c++ & 0377 );
	if( ct==0 || s<ct ) putbyte( 0 );  /* the null at the end */
    } else {
	strcode(c, stringsize);
	dimtab[curdim] = stringsize+1;  /* in case of later sizeof ... */
    }
}

lxcom(){
	register c;
	/* saw a /*: process a comment */

	for(;;){

		switch( c = xgetchar() ){

		case EOF:
			/* "unexpected EOF"  */
			UERROR( MESSAGE( 113 ) );
			return;

		case '\n':
			++lineno;

		default:
			continue;

		case '*':
			if( (c = xgetchar()) == '/' ) return;
			else xungetc( c ,stdin);
			continue;

# ifdef LINT
		case 'V':
			lxget( c, LEXLET|LEXDIG );
			{
				extern int vaflag;
				int i;
				i = yytext[7]?yytext[7]-'0':0;
				yytext[7] = '\0';
				if( strcmp( yytext, "VARARGS" ) ) continue;
				vaflag = i;
				continue;
				}
		case 'L':
			lxget( c, LEXLET );
			if( strcmp( yytext, "LINTLIBRARY" ) ) continue;
			{
				extern int libflag, vflag;
				libflag = 1;
				vflag = 0;
				}
			continue;

		case 'A':
			lxget( c, LEXLET );
			if( strcmp( yytext, "ARGSUSED" ) ) continue;
			{
				extern int argflag, vflag;
				argflag = 1;
				vflag = 0;
				}
			continue;

		case 'N':
			lxget( c, LEXLET );
			if( strcmp( yytext, "NOTREACHED" ) ) continue;
			reached = 0;
			{
			 /* 6/18/80 */
				extern int reachflg;
				reachflg = 1;
			}
			continue;
# endif
			}
		}
	}

yylex(){
#ifdef BROWSER
	int line;
	int hidden;
#endif
	for(;;){

		register lxchar;
		register unsigned x;
		register struct lxdope *p;
		register struct symtab *sp;
		int id;

		xreset();
		lxchar = xgetchar();
#ifdef BROWSER
		line = lineno;
		hidden = cb_hidden;
#endif
		x = lxchar+1;
		if (x >= CSSZ+1)
			goto errcase;
		switch( (p=lxcp[x])->lxact ){

		onechar:
			xungetc( lxchar ,stdin);

		case A_1C:
			/* eat up a single character, and return an opcode */

			yylval.intval = p->lxval;
			return( p->lxtok );

		errcase:
		case A_ERR:
			if (isprint(lxchar)) {
			    /* "illegal character: '%c'" */
			    UERROR( MESSAGE( 64 ), lxchar);
			} else {
			    /* "illegal character: %03o (octal)" */
			    UERROR( MESSAGE( 51 ), lxchar );
			}
			continue;

		case A_LET:
			/* collect an identifier, check for reserved word, and return */
			lxget( lxchar, LEXLET|LEXDIG );
			if( (lxchar=lxres()) > 0 ) return( lxchar ); /* reserved word */
			if( lxchar== 0 ) continue;
			if ( truncate_flag )
				yytext[8] = '\0';	/* truncate it */
			id = lookup( hash( yytext ),
				/* tag name for struct/union/enum */
				(stwart&TAGNAME)? STAG:
				/* member name for struct/union */
				(stwart&(INSTRUCT|INUNION|FUNNYNAME))?SMOS:0 );
			sp = STP(id);
			if( sp != NULL && sp->sclass == TYPEDEF && !stwart ){
				stwart = instruct;
#ifdef BROWSER
				yylval.type.nodep = mkty( sp->stype, sp->dimoff, sp->sizoff );
				yylval.type.typdef = sp;
				if (cb_hidden == 0) {
					yylval.type.lineno = lineno;
				} else {
					yylval.type.lineno = -lineno;
				}
#else
				yylval.nodep = mkty( sp->stype, sp->dimoff, sp->sizoff );
#endif
				return( TYPE );
				}
			stwart = (stwart&SEENAME) ? instruct : 0;
#ifdef BROWSER
			yylval.name.id = id;
			if (hidden == 0) {
				yylval.name.lineno = line;
			} else {
				yylval.name.lineno = -line;
			}
#else
			yylval.intval = id;
#endif
			return( NAME );

		case A_DIG:
			/* collect a digit string, then look at last one... */
			lastcon = 0;
			lxget( lxchar, LEXDIG );
			switch( lxchar=xgetchar() ){

			case 'x':
			case 'X':
				/* "illegal hex constant"  */
				if( yytext[0] != '0' && !yytext[1] ) UERROR( MESSAGE( 59 ) );
				lxmore( lxchar, LEXHEX );
				/* convert the value */
				{
					register char *cp;
					for( cp = yytext+2; *cp; ++cp ){
						/* this code won't work for all wild character sets,
						   but seems ok for ascii and ebcdic */
						lastcon <<= 4;
						if( lxdigit( *cp ) ) lastcon += *cp-'0';
						else if( isupper( *cp ) ) lastcon += *cp - 'A'+ 10;
						else lastcon += *cp - 'a'+ 10;
						}
					}

			hexlong:
				yylval.intval = 0;
				/* 
				 * If the constant does not fit within an
				 * int, then consider it a long.
				 * Note that if the host of this compiler has
				 * a long size that is <= SZINT, then the
				 * expression "mask(SZINT)" has no meaning.
				 * Thus we avoid asking that question if
				 * possible.
				 */
				if( (SZLONG>SZINT) && (lastcon & ~mask(SZINT))) 
					yylval.intval = 1;
				goto islong;

			case '.':
				lxmore( lxchar, LEXDIG );

			getfp:
				if( (lxchar=xgetchar()) == 'e' || lxchar == 'E' ){ /* exponent */

			case 'e':
			case 'E':
					if( (lxchar=xgetchar()) == '+' || lxchar == '-' ){
						*lxgcp++ = 'e';
						}
					else {
						xungetc(lxchar,stdin);
						lxchar = 'e';
						}
					lxmore( lxchar, LEXDIG );
					/* now have the whole thing... */
					}
				else {  /* no exponent */
					xungetc( lxchar ,stdin);
					}
				return( isitfloat( yytext ) );

			default:
				xungetc( lxchar ,stdin);
				if( yytext[0] == '0' ){
					/* convert in octal */
					register char c, *cp;
					for( cp = yytext+1; (c=*cp)!=0; ++cp ){
						lastcon <<= 3;
						if (!lxoctal(c)) {
							/*
							 * K&R allow '8' and '9' in octal
							 * constants; they have the values
							 * 010 and 011, respectively.
							 * This goes away in ANSI C.
							 */
							/* illegal octal constant */
							if (c == '8' || c == '9')
								WERROR( MESSAGE( 124 ) );
							else
								UERROR( MESSAGE( 124 ) );
							break;
							}
						lastcon += c - '0';
						}
					goto hexlong;
					}
				else {
					/* convert in decimal */
					register char *cp;
					for( cp = yytext; *cp; ++cp ){
						lastcon = lastcon * 10 + *cp - '0';
						}
					}

				/* 
				 * Decide if it is long or not (decimal case).
				 * If it is positive, and fits in (SZINT - 1)
				 * bits, or negative and fits in (SZINT - 1)
				 * bits plus an extended sign, it is int; 
				 * otherwise long.  If there is an l or L 
				 * following, then it is always a long.
				 */
				{	CONSZ v;
					v = lastcon & ~mask(SZINT - 1);
					if( v == 0 || v == ~mask(SZINT - 1) ) 
						yylval.intval = 0;
					else yylval.intval = 1;
					}


			islong:
				/* finally, look for trailing L or l */
				if( (lxchar = xgetchar()) == 'L' || lxchar == 'l' ) yylval.intval = 1;
				else xungetc( lxchar ,stdin);

				return( ICON );
				}

		case A_DOT:
			/* look for a dot: if followed by a digit, floating point */
			lxchar = xgetchar();
			if( lxmask[lxchar+1] & LEXDIG ){
				xungetc(lxchar,stdin);
				lxget( '.', LEXDIG );
				goto getfp;
				}
			stwart = FUNNYNAME;
			goto onechar;

		case A_STR:
			/* string constant */
#ifdef BROWSER
			yylval.intval = lineno;
#endif
			lxstr('"');
			return( STRING );

		case A_CC:
			/* character constant */
			lastcon = 0;
			lxstr('\'');
			yylval.intval = 0;
			return( ICON );

		case A_BCD:
			{
				register i;
				int j;
				for( i=0; i<LXTSZ; ++i ){
					if( ( j = xgetchar() ) == '`' ) break;
					if( j == '\n' ){
						/* "newline in BCD constant" */
						UERROR( MESSAGE( 77 ) );
						break;
						}
					yytext[i] = j;
					}
				yytext[i] = '\0';
				/* "BCD constant exceeds 6 characters" */
				if( i>6 ) UERROR( MESSAGE( 10 ) );
# ifdef gcos
				else strtob( yytext, &lastcon, i );
				lastcon >>= 6*(6-i);
# else
				/* "gcos BCD constant illegal" */
				UERROR( MESSAGE( 48 ) );
# endif
				yylval.intval = 0;  /* not long */
				return( ICON );
				}

		case A_SL:
			/* / */
			if( (lxchar=xgetchar()) != '*' ) goto onechar;
			lxcom();
		case A_WS:
#ifdef BROWSER
			line = lineno;
			hidden = cb_hidden;
#endif
			continue;

		case A_NL:
			++lineno;
#ifdef BROWSER
			if ( !cb_flag )
				lxtitle(FALSE);
			line = lineno;
			hidden = cb_hidden;
#else
			lxtitle();
#endif
			continue;

		case A_NOT:
			/* ! */
			if( (lxchar=xgetchar()) != '=' ) goto onechar;
			yylval.intval = NE;
			return( EQUOP );

		case A_MI:
			/* - */
			if( (lxchar=xgetchar()) == '-' ){
				yylval.intval = DECR;
				return( INCOP );
				}
			if( lxchar != '>' ) goto onechar;
			stwart = FUNNYNAME;
			yylval.intval=STREF;
			return( STROP );

		case A_PL:
			/* + */
			if( (lxchar=xgetchar()) != '+' ) goto onechar;
			yylval.intval = INCR;
			return( INCOP );

		case A_AND:
			/* & */
			if( (lxchar=xgetchar()) != '&' ) goto onechar;
			return( yylval.intval = ANDAND );

		case A_OR:
			/* | */
			if( (lxchar=xgetchar()) != '|' ) goto onechar;
			return( yylval.intval = OROR );

		case A_LT:
			/* < */
			if( (lxchar=xgetchar()) == '<' ){
				yylval.intval = LS;
				return( SHIFTOP );
				}
			if( lxchar != '=' ) goto onechar;
			yylval.intval = LE;
			return( RELOP );

		case A_GT:
			/* > */
			if( (lxchar=xgetchar()) == '>' ){
				yylval.intval = RS;
				return(SHIFTOP );
				}
			if( lxchar != '=' ) goto onechar;
			yylval.intval = GE;
			return( RELOP );

		case A_EQ:
			/* = */
			switch( lxchar = xgetchar() ){

			case '=':
				yylval.intval = EQ;
				return( EQUOP );

			default:
				goto onechar;

				}

		default:
			cerror( "yylex error, character %03o (octal)", lxchar );

			}

		/* ordinarily, repeat here... */
		cerror( "out of switch in yylex" );

		}

	}

struct lxrdope {
	/* dope for reserved, in alphabetical order */

	char *lxrch;	/* name of reserved word */
	short lxract;	/* reserved word action */
	short lxrval;	/* value to be returned */
	} lxrdope[] = {

	"asm",		AR_A,	0,
	"auto",		AR_CL,	AUTO,
	"break",	AR_RW,	BREAK,
	"char",		AR_TY,	CHAR,
	"case",		AR_RW,	CASE,
	"continue",	AR_RW,	CONTINUE,
	"double",	AR_TY,	DOUBLE,
	"default",	AR_RW,	DEFAULT,
	"do",		AR_RW,	DO,
	"extern",	AR_CL,	EXTERN,
	"else",		AR_RW,	ELSE,
	"enum",		AR_E,	ENUM,
	"for",		AR_RW,	FOR,
	"float",	AR_TY,	FLOAT,
	"fortran",	AR_CL,	FORTRAN,
	"goto",		AR_RW,	GOTO,
	"if",		AR_RW,	IF,
	"int",		AR_TY,	INT,
	"long",		AR_TY,	LONG,
	"return",	AR_RW,	RETURN,
	"register",	AR_CL,	REGISTER,
	"switch",	AR_RW,	SWITCH,
	"struct",	AR_S,	0,
	"sizeof",	AR_RW,	SIZEOF,
	"short",	AR_TY,	SHORT,
	"static",	AR_CL,	STATIC,
	"typedef",	AR_CL,	TYPEDEF,
	"unsigned",	AR_TY,	UNSIGNED,
	"union",	AR_U,	0,
	"void",		AR_TY,	TVOID,
	"while",	AR_RW,	WHILE,
	"",		0,	0,	/* to stop the search */
	};

lxres() {
	/* check to see of yytext is reserved; if so,
	/* do the appropriate action and return */
	/* otherwise, return -1 */

	register c, ch;
	register struct lxrdope *p;

	ch = yytext[0];

	if( !islower(ch) ) return( -1 );

	switch( ch ){

	case 'a':
		c=0; break;
	case 'b':
		c=2; break;
	case 'c':
		c=3; break;
	case 'd':
		c=6; break;
	case 'e':
		c=9; break;
	case 'f':
		c=12; break;
	case 'g':
		c=15; break;
	case 'i':
		c=16; break;
	case 'l':
		c=18; break;
	case 'r':
		c=19; break;
	case 's':
		c=21; break;
	case 't':
		c=26; break;
	case 'u':
		c=27; break;
	case 'v':
		c=29; break;
	case 'w':
		c=30; break;

	default:
		return( -1 );
		}

	for( p= lxrdope+c; p->lxrch[0] == ch; ++p ){
		if( !strcmp( yytext, p->lxrch ) ){ /* match */
			switch( p->lxract ){

			case AR_TY:
				/* type word */
				stwart = instruct;
#ifdef BROWSER
				yylval.type.nodep = mkty( (TWORD)p->lxrval, 0, p->lxrval );
				yylval.type.typdef = NULL;
				if (cb_hidden == 0) {
					yylval.type.lineno = lineno;
				} else {
					yylval.type.lineno = -lineno;
				}
#else
				yylval.nodep = mkty( (TWORD)p->lxrval, 0, p->lxrval );
#endif
				return( TYPE );

			case AR_RW:
				/* ordinary reserved word */
				if (p->lxrval == SIZEOF) {
					lxsizeof++;
#ifdef BROWSER
					if (cb_hidden == 0) {
						yylval.intval = lineno;
					} else {
						yylval.intval = -lineno;
					}
					return( p->lxrval );
#endif
				}
				return( yylval.intval = p->lxrval );

			case AR_CL:
				/* class word */
				yylval.intval = p->lxrval;
				return( CLASS );

			case AR_S:
				/* struct */
				stwart = INSTRUCT|SEENAME|TAGNAME;
#ifdef BROWSER
				yylval.intval2.i1 = INSTRUCT;
				if (cb_hidden == 0) {
					yylval.intval2.i2 = lineno;
				} else {
					yylval.intval2.i2 = -lineno;
				}
#else
				yylval.intval = INSTRUCT;
#endif
				return( STRUCT );

			case AR_U:
				/* union */
				stwart = INUNION|SEENAME|TAGNAME;
#ifdef BROWSER
				yylval.intval2.i1 = INUNION;
				if (cb_hidden == 0) {
					yylval.intval2.i2 = lineno;
				} else {
					yylval.intval2.i2 = -lineno;
				}
#else
				yylval.intval = INUNION;
#endif
				return( STRUCT );

			case AR_E:
				/* enums */
				stwart = SEENAME|TAGNAME;
#ifdef BROWSER
				if (cb_hidden == 0) {
					yylval.intval2.i2 = lineno;
				} else {
					yylval.intval2.i2 = -lineno;
				}
				return( yylval.intval2.i1 = ENUM );
#else
				return( yylval.intval = ENUM );
#endif

			case AR_A:
				/* asm */
				asm_esc = 1; /* warn the world! */
				return( ASM );

			default:
				cerror( "bad AR_?? action" );
				}
			}
		}
	return( -1 );
	}

#ifndef LINT
extern int	labelno;

#define PUSHINCL	'1'		/* Entering a header file */
#define POPINCL		'2'		/* Exited a header file */
#endif

/*
 * skip to the end of a directive line, and increment lineno.
 * Called only for lines that begin with # and do not specify
 * a new value for lineno.
 */
void
lxnewline() {
	register c;

	c = xgetchar();
	while (c != EOF && c != '\n') {
		c = xgetchar();
	}
	lineno++;
}

/*
 * Recognize #pragma directives, of which the following forms are
 * meaningful:
 *	'#' 'pragma' id
 *	'#' 'pragma' id '(' [id|number] {',' [id|number] } ')'
 *	'#' 'pragma' id string
 *	'#' 'ident'  string
 * Other forms are ignored.
 */

#define lxidchar(c) (lxalpha(c) || c == '_')

typedef enum { KW_PRAGMA, KW_IDENT } Keywords;

static void
lxpragma(keyword)
	Keywords keyword;
{
	register c;
	register NODE *arg, *arglist;
	register long val;
	int pragma;

	if (keyword == KW_PRAGMA) {
		/* look for identifier */
		lxget( ' ', LEXWS );
		c = xgetchar();
		if (!lxidchar(c)) {
			werror("identifier expected");
			xungetc(c, stdin);
			lxnewline();
			return;
		}
		lxget(c, LEXLET|LEXDIG);
#ifdef BROWSER
		if ( cb_flag )
		  cb_enter_symbol(yytext,
					   lineno,
					   cb_sun_c_ref_pragma_name);
#endif
	}
	pragma = lookup_pragma(yytext);
	if (!pragma) {
		lxnewline();
		return;
	}
	/* look for optional argument list */
	arglist = NULL;
	lxget( ' ', LEXWS );
	c = xgetchar();
	if (c == '(') {
		/* arg list */
		do {
			/* look for name or number */
			arg = NULL;
			lxget( ' ', LEXWS );
			c = xgetchar();
			if(lxidchar(c)) {
				/* name */
				lxget(c, LEXLET|LEXDIG);
#ifdef BROWSER
				if ( cb_flag )
					cb_enter_symbol(yytext,
							lineno,
							cb_sun_c_ref_pragma_arg);
#endif
				arg = block(NAME, NIL, lookup(hash(yytext),0),
					UNDEF, NIL, NIL);
			} else if (lxdigit(c)) {
				/* number */
				val = (c - '0');
				for( c=xgetchar(); lxdigit(c); c=xgetchar() ){
					val = val*10+ c - '0';
				}
				xungetc( c, stdin );
				arg = bcon(val);
			} else {
				werror("illegal argument to #pragma");
				xungetc( c, stdin );
				lxnewline();
				return;
			}
			if (arglist == NIL) {
				arglist = arg;
			} else {
				arglist = block(CM, arglist, arg,
					UNDEF, NIL, NIL);
			}
			/* look for ',' to indicate more args */
			lxget( ' ', LEXWS );
			c = xgetchar();
		} while (c == ',');
		/* arg list ends with ')' */
		if (c != ')') {
			werror("')' expected");
			xungetc(c, stdin);
		}
	} else if (c == '"') {
		/* string */
		lxstr('"');
		if (stringsize < stringmax)
			stringval[stringsize] = 0;
		else
			(void)newstring(0);
		arglist = block(STRING, stringval, NULL, UNDEF, NIL, NIL);
	} else {
		/* c != '(' && c != '"' */
		xungetc(c, stdin);
	}
	do_pragma(pragma, arglist);
	lxnewline();
}

#ifdef BROWSER
lxtitle(has_timestamp){
#else
lxtitle(){
#endif
	/* called after a newline; set linenumber and file name */

	register c, val;
	register char *cp;
#ifndef LINT
	register char *cq;
#endif
	char	 titlbuf[BUFSIZ];
#ifdef BROWSER
	static cb_first_push_done = 0;
#endif

	for(;;){  /* might be several such lines in a row */
		if( (c=xgetchar()) != '#' ){
			if( c != EOF ) xungetc(c,stdin);
#ifndef LINT
			if ( lastloc != PROG) return;
			if (innerlist) listline();
#endif
			return;
			}

		lxget( ' ', LEXWS );
		c = xgetchar();
		if (c != 'p' && c != 'i') {
			xungetc(c, stdin);
			}
		else {
			lxget(c, LEXLET);
			if (strcmp(yytext,"pragma") == 0) {
				lxpragma(KW_PRAGMA);
				}
			else if (strcmp(yytext,"ident") == 0) {
				lxpragma(KW_IDENT);
				}
			else {
				/* "unknown preprocessor directive" */
				UERROR( MESSAGE( 143 ) );
				lxnewline();
				}
			continue;
			}
		val = 0;
		for( c=xgetchar(); lxdigit(c); c=xgetchar() ){
			val = val*10+ c - '0';
			}
		xungetc( c, stdin );
		lineno = val;
		lxget( ' ', LEXWS );
		if( (c=xgetchar()) != '\n' ){
			for( cp=titlbuf; !lxspace(c) && c!='\n' ; c=xgetchar(),++cp ){
				*cp = c;
				}
			*cp = '\0';
			ftitle = hash(titlbuf);

# if TARGET == I386 && !defined(LINT)
			bg_file();
# endif
			if (c != '\n') {
				while ((c = xgetchar()), lxspace(c))
					;
#ifdef BROWSER
				if (cb_flag && has_timestamp) {
					if (!cb_first_push_done) {
						cb_first_push_done++;
						cb_enter_push_file(ftitle, 1);
					}
				}
#endif
				if (c != '\n')
					(void) xgetchar();
				}
#ifdef CXREF
			fprintf(outfp, "%s\n", ftitle);
#endif
#ifndef LINT
			if (ititle[0] == '\0') {
				cp = ftitle;
				cq = ititle;
				while ( *cp )  
					*cq++ = *cp++;
				*cq = '\0';
				*--cq = '\0';
				pstab(ititle+1, N_SO);
				if (gdebug) {
# if TARGET == I386 && !defined(LINT)
					printf("0,0,.LL%d\n", labelno);
# else
					printf("0,0,LL%d\n", labelno);
# endif
					insert_filename(ititle+1);
					}
				*cq = '"';
# if TARGET == I386 && !defined(LINT)
				printf(".LL%d:\n", labelno++);
# else
				printf("LL%d:\n", labelno++);
#endif
				}
			if (c == PUSHINCL) {
#ifdef BROWSER
				if ( cb_flag && has_timestamp )
					cb_enter_push_file(ftitle, 1);
#endif
				stab_startheader();
				} 
			else if (c == POPINCL) {
#ifdef BROWSER
				if ( cb_flag && has_timestamp ) {
					cb_enter_pop_file();
					}
#endif
				stab_endheader();
				}
#endif
			}
		}
	}

		

#define NSAVETAB	4096
char	*savetab;
int	saveleft;

char *
savestr( cp )			/* place string into permanent string storage */
	register char *cp;
{
	register int len = strlen( cp ) + 1;

	if ( len > saveleft )
	{
		saveleft = NSAVETAB;
		if ( len > saveleft )
			saveleft = len;
		if ( ( savetab = (char *) malloc( saveleft ) ) == 0 )
			cerror( "out of memory [savestr()]" );
	}
	strncpy( savetab, cp, len );
	cp = savetab;
	savetab += len;
	saveleft -= len;
	return ( cp );
}

#ifdef LINT
#	define LNCHNAM	8	/* length of symbols to check for pflag */
#endif

/*
 * The definition for the segmented hash tables.
 */
#define MAXHASH		20
#define HASHINC		1013
struct ht
{
	char	**ht_low;
	char	**ht_high;
	int	ht_used;
} htab[MAXHASH];

char *
hash( s )	/* look for s in seg. hash tables.  Not found, make new entry */
	char *s;
{
	register char **h;
	register int i;
	register char *cp;
	struct ht *htp;
	int sh;
#ifdef LINT
	char *found = 0;	/* set once LNCHNAM chars. matched for name */
#endif

	/*
	 * The hash function is a modular hash of
	 * the sum of the characters with the sum
	 * doubled before each successive character
	 * is added.
	 * Hash on the correct number of characters.  Lint needs to be able
	 * to limit this so that it can note length of names for portablility
	 * concerns.
	 */
	cp = s;
	i = 0;
#ifdef LINT
	while ( *cp && ( ( cp - s ) < LNCHNAM ) )
#else
	while ( *cp )
#endif
	{
		i = ( i << 1 ) + *cp++;
	}
	sh = ( i & 077777 ) % HASHINC;
	cp = s;
	/*
	 * There are as many as MAXHASH active
	 * hash tables at any given point in time.
	 * The search starts with the first table
	 * and continues through the active tables
	 * as necessary.
	 */
	for ( htp = htab; htp < &htab[MAXHASH]; htp++ )
	{
		if ( htp->ht_low == 0 )
		{
			register char **hp = (char **) calloc(
				sizeof (char **), HASHINC );

			if ( hp == 0 )
				cerror( "out of memory [hash()]" );
			htp->ht_low = hp;
			htp->ht_high = hp + HASHINC;
		}
		h = htp->ht_low + sh;
		/*
		 * quadratic rehash increment
		 * starts at 1 and incremented
		 * by two each rehash.
		 */
		i = 1;
		do
		{
			if ( *h == 0 )
			{
				if ( htp->ht_used > ( HASHINC * 3 ) / 4 )
					break;
				htp->ht_used++;
				*h = savestr( cp );
#ifdef LINT
				if ( pflag && found && *found != '"' )
				{
					/*
					* If pflag set, then warn of greater
					* than LNCHNAM character names which
					* differ past the LNCHNAM'th character.
					*
					* the test for a quote keeps #include
					* names from getting an error.
					*/
					/*
					* "`%s' may be indistinguishable from
					*  `%s' due to internal name truncation
					*/
					WERROR( MESSAGE( 128 ), *h, found );
				}
#endif
				return ( *h );
			}
#ifdef LINT
			if ( pflag )
			{
				if ( **h == *cp &&
					strncmp( *h, cp, LNCHNAM ) == 0 )
				{
					/*
					* We have matched on LNCHNAM chars.
					* Now, look for the ``total'' name.
					*/
					found = *h;
					if ( strcmp( *h, cp ) == 0 )
					{
						/*
						* This entry really is
						* the name we want.
						*/
						return ( *h );
					}
				}
			}
			else	/* No pflag - use entire name length */
			{
				if ( **h == *cp && strcmp( *h, cp ) == 0 )
					return ( *h );
			}
#else
			if ( **h == *cp && strcmp( *h, cp ) == 0 )
				return ( *h );
#endif
			h += i;
			i += 2;
			if ( h >= htp->ht_high )
				h -= HASHINC;
		} while ( i < HASHINC );
	}
	cerror( "out of hash tables" );
	/*NOTREACHED*/
}


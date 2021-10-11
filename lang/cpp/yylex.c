/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)yylex.c 1.1 94/10/31 SMI"; /* from S5R3.1 1.11 */
#endif

#include "y.tab.h"
#ifdef FLEXNAMES
#	define NCPS	128
#else
#	define NCPS	8
#endif

#ifdef BROWSER
# include <stdio.h>
# include "cb_cpp_protocol.h"
# define NOINCL ""
extern int cbflag;
extern int cbifsay;
extern char *cbmacend;
extern FILE *fout;
extern int lineno[];
extern int ifno;
#endif

extern int ncps;	/* actual number of chars. */
extern int xline;
extern int yylval;
extern int cxref;

#define isid(a)  ((fastab+COFF)[a]&IB)
#define IB 1
/*	#if '\377' < 0		it would be nice if this worked properly!!!!! */
#if pdp11 | vax | mc68000 | sparc | i386
#define COFF 128
#else
#define COFF 0
#endif

yylex()
{
	static int ifdef = 0;
	static char *op2[] = {"||",  "&&" , ">>", "<<", ">=", "<=", "!=", "=="};
	static int  val2[] = {OROR, ANDAND,  RS,   LS,   GE,   LE,   NE,   EQ};
	static char *opc = "b\bt\tn\nf\fr\r\\\\";
	extern char fastab[];
	extern char *outp, *inp, *newp;
	extern int flslvl;
	register char savc, *s;
	char *skipbl();
	int val;
	register char **p2;
	struct symtab
	{
		char *name;
		char *value;
		struct symtab  *next;  /* pointer to next chain entry */
	} *sp, *lookup();

	for ( ;; )
	{
#ifdef BROWSER
		if ( ( cbmacend != NULL ) && ( outp >= cbmacend ) )
		{
			putc( CB_CHR_PREFIX, fout );
			putc( CB_CHR_MACRO_REF_END, fout );
			cbmacend = NULL;
		}
#endif
		newp = skipbl( newp );
		if ( *inp == '\n' ) 		/* end of #if */
			return( stop );
		savc = *newp;
		*newp = '\0';
		if ( *inp == '/' && inp[1] == '*' )
		{
			/* found a comment with -C option, still toss here */
			*newp = savc;
			outp = inp = newp;
			continue;
		}
		for ( p2 = op2 + 8; --p2 >= op2; )	/* check 2-char ops */
			if ( strcmp( *p2, inp ) == 0 )
			{
				val = val2[ p2 - op2 ];
				goto ret;
			}
		s = "+-*/%<>&^|?:!~(),";		/* check 1-char ops */
		while ( *s )
			if ( *s++ == *inp )
			{
				val= *--s;
				goto ret;
			}
		if ( *inp<='9' && *inp>='0' )		/* a number */
		{
			if ( *inp == '0' )
				yylval= ( inp[1] == 'x' || inp[1] == 'X' ) ?
					tobinary( inp + 2, 16 ) :
					tobinary( inp + 1, 8 );
			else
				yylval = tobinary( inp, 10 );
			val = number;
		}
		else if ( isid( *inp ) )
		{
			if ( strcmp( inp, "defined" ) == 0 )
			{
				ifdef = 1;
				++flslvl;
				val = DEFINED;
			}
			else
			{
				if ( ifdef != 0 )
				{
					register char *p;
					register int savech;

					/* make sure names <= ncps chars */
					if ( ( newp - inp ) > ncps )
						p = inp + ncps;
					else
						p = newp;
					savech = *p;
					*p = '\0';
					sp = lookup( inp, -1 );
					*p = savech;
					ifdef = 0;
					--flslvl;
				}
				else
					sp = lookup( inp, -1 );
				if ( cxref )
					ref(inp, xline);
#ifdef BROWSER
				if ( cbflag )
				{
					register char *p;
					register int savech;

					if ( ( newp - inp ) > ncps )
						p = inp + ncps;
					else
						p = newp;
					savech = *p;
					*p = '\0';
					if ( lineno[ifno] != cbifsay )
					{
						sayline( NOINCL );
						cbifsay = lineno[ifno];
					}
					if ( inp > cbmacend )
					{
						cbputid( fout,
						    ( sp == 0 ) ?
						    CB_CHR_MACRO_REF_WO_FORMALS_UNDEFD :
						    CB_CHR_MACRO_REF_WO_FORMALS_DEFD,
						    inp );
						putc( CB_CHR_PREFIX, fout );
						putc( CB_CHR_MACRO_REF_END,
						      fout );
					}
					*p = savech;
				}
#endif
				yylval = ( sp == 0 ) ? 0 : 1;
				val = number;
			}
		}
		else if ( *inp == '\'' )	/* character constant */
		{
			val = number;
			if ( inp[1] == '\\' )	/* escaped */
			{
				char c;

				if ( newp[-1] == '\'' )
					newp[-1] = '\0';
				s = opc;
				while ( *s )
					if ( *s++ != inp[2] )
						++s;
					else
					{
						yylval = *s;
						goto ret;
					}
				if ( inp[2] <= '9' && inp[2] >= '0' )
					yylval = c = tobinary( inp + 2, 8 );
				else
					yylval = inp[2];
			}
			else
				yylval = inp[1];
		}
		else if ( strcmp( "\\\n", inp ) == 0 )
		{
			*newp = savc;
			continue;
		}
		else
		{
			*newp = savc;
			pperror( "Illegal character %c in preprocessor if",
				*inp );
			continue;
		}
	ret:
		/* check for non-ident after defined (note need the paren!) */
		if ( ifdef && val != '(' && val != DEFINED )
		{
			pperror( "\"defined\" modifying non-identifier \"%s\" in preprocessor if", inp );
			ifdef = 0;
			flslvl--;
		}
		*newp = savc;
		outp = inp = newp;
		return( val );
	}
}

tobinary( st, b )
	char *st;
{
	int n, c, t;
	char *s;
	int warned = 0;

	n = 0;
	s = st;
	while ( c = *s++ )
	{
		switch( c )
		{
		case '8': case '9':
			if (b <= 8 && !warned) {
				ppwarn("Illegal octal number %s", st);
				warned = 1;
			}
		case '0': case '1': case '2': case '3': case '4': 
		case '5': case '6': case '7':
			t = c - '0';
			break;
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': 
			t = (c - 'a') + 10;
			if ( b > 10 )
				break;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': 
			t = (c - 'A') + 10;
			if ( b > 10 )
				break;
		default:
			t = -1;
			if ( c == 'l' || c == 'L' )
				if ( *s == '\0' )
					break;
			pperror( "Illegal number %s", st );
		}
		if ( t < 0 )
			break;
		n = n * b + t;
	}
	return( n );
}

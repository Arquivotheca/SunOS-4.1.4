#ifndef lint
static	char sccsid[] = "@(#)yyerror.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "cpass1.h"
#include <ctype.h>

/*
 * printnames for yacc tokens
 */

extern int tokebufptr;

#ifdef BROWSER
extern char *tokebufr;
#else
extern char tokebufr[];
#endif

#define MAXTOKELENGTH 20 /* don't care past here */

struct pname {
    int	   pval;
    char * name;
    enum { NOPRINT, VALPRINT, FVALPRINT, NAMEPRINT, STRINGPRINT,
	   ASGPRINT,RELPRINT, EQLPRINT, DIVPRINT, INCRPRINT, UNIPRINT,
	   STRPRINT,SHIFTPRINT, TYPEPRINT, CLASSPRINT, STRUCTPRINT,
	   WORDPRINT, SYMPRINT } howprint;
} pname[] = {
    0,		"<nothing>",NOPRINT,
    ERROR,	"<nothing>",NOPRINT,
    NAME,	"<name>",	NAMEPRINT,
    STRING,	"<string>",	STRINGPRINT,
    ICON,	"<constant>",VALPRINT,
    FCON,	"<fconstant>", FVALPRINT,
    PLUS,	"+",	SYMPRINT,
    ASG PLUS,	"+=",	SYMPRINT,
    MINUS,	"-",	SYMPRINT,
    ASG MINUS,	"-=",	SYMPRINT,
    UNARY MINUS,"-",	SYMPRINT,
    MUL,	"*",	SYMPRINT,
    ASG MUL,	"*=",	SYMPRINT,
    UNARY MUL,	"*",	SYMPRINT,
    AND,	"&",	SYMPRINT,
    ASG AND,	"&=",	SYMPRINT,
    UNARY AND,	"&",	SYMPRINT,
    OR,		"|",	SYMPRINT,
    ASG OR,	"|=",	SYMPRINT,
    ER,		"^",	SYMPRINT,
    ASG ER,	"^=",	SYMPRINT,
    QUEST,	"?",	SYMPRINT,
    COLON,	":",	SYMPRINT,
    ANDAND,	"&&",	SYMPRINT,
    OROR,	"||",	SYMPRINT,
    25,		"=op",	ASGPRINT,
    26,		"relop",RELPRINT,
    27,		"==",	EQLPRINT,
    28,		"/",	DIVPRINT,
    29,		"<<",	SYMPRINT,
    30,		"++",	INCRPRINT,
    31,		"!",	UNIPRINT,
    32,		"->",	STRPRINT,
    TYPE,	"type",	TYPEPRINT,
    CLASS,	"extern",CLASSPRINT,
    STRUCT,	"struct",STRUCTPRINT,
    RETURN,	"return",WORDPRINT,
    GOTO,	"goto",	WORDPRINT,
    IF,		"if",	WORDPRINT,
    ELSE,	"else",	WORDPRINT,
    SWITCH,	"switch",WORDPRINT,
    BREAK,	"break",WORDPRINT,
    CONTINUE,	"continue",WORDPRINT,
    WHILE,	"while",WORDPRINT,
    DO,		"do",	WORDPRINT,
    FOR,	"for",	WORDPRINT,
    DEFAULT,	"default",WORDPRINT,
    CASE,	"case",	WORDPRINT,
    SIZEOF,	"sizeof",WORDPRINT,
    ENUM,	"enum",	WORDPRINT,
    LP,		"(",	SYMPRINT,
    RP,		")",	SYMPRINT,
    LC,		"{",	SYMPRINT,
    RC,		"}",	SYMPRINT,
    LB,		"[",	SYMPRINT,
    RB,		"]",	SYMPRINT,
    CM,		",",	SYMPRINT,
    SM,		";",	SYMPRINT,
    ASSIGN,	"=",	SYMPRINT,
    ASM, 	"asm",	WORDPRINT,
    0,		0,	NOPRINT,
};

static struct pname *
plookup( n ) 
    register n;
{
    register struct pname *p;
    p = pname;
    while ( p->name != 0 ){
	if ( p->pval == n ) return p;
	p++;
    }
    return 0;
}

ccerror( s, c ) char *s; int c;
{ /* error printing routine in parser */
    static char mungbuffer[300];
    static char word[] = "word", symbol[] = "symbol";
    char *printname;
    char *noun;
    char *q = "\"";
    struct pname *p;
    extern char yytext[];
    int i,mt;
    char token[MAXTOKELENGTH+10], *thischar;

    p = plookup( c );
    if (p == 0 || p->howprint==NOPRINT){
	uerror( s );
    } else {
	printname = p->name;
	switch(p->howprint){
	case WORDPRINT:
	    noun = word;
	    break;
	case TYPEPRINT:
	    /* this is hard */
	    noun = "type word";
	    printname = yytext;
	    break;
	case CLASSPRINT:
	    noun = word;
	    switch(yylval.intval){
	    case AUTO:
		printname = "auto"; break;
	    case EXTERN:
		printname = "extern"; break;
	    case FORTRAN:
		printname = "fortran"; break;
	    case REGISTER:
		printname = "register"; break;
	    case STATIC:
		printname = "static"; break;
	    case TYPEDEF:
		printname = "typedef"; break;
	    }
	    break;
	case STRUCTPRINT:
	    noun = word;
	    if (yylval.intval==INUNION)
		printname = "union";
	    break;
	case SYMPRINT:
	    noun = symbol;
	    q = "";
	    break;
	case RELPRINT:
	    noun = symbol;
	    q = "";
	    switch ( yylval.intval ){
	    case LS: printname = "<"; break;
	    case LE: printname = "<="; break;
	    case GT: printname = ">"; break;
	    case GE: printname = ">="; break;
	    }
	    break;
	case EQLPRINT:
	    noun = symbol;
	    q = "";
	    if ( yylval.intval == NE )
		printname = "!=";
	    break;
	case DIVPRINT:
	    noun = symbol;
	    q = "";
	    if ( yylval.intval == MOD )
		printname = "%";
	    break;
	case SHIFTPRINT:
	    noun = symbol;
	    q = "";
	    if ( yylval.intval == RS)
		printname = ">>" ;
	    break;
	case INCRPRINT:
	    noun = symbol;
	    q = "";
	    if ( yylval.intval == DECR )
		printname = "--";
	    break;
	case UNIPRINT:
	    noun = symbol;
	    q = "";
	    if ( yylval.intval == COMPL )
		printname = "~";
	    break;
	case STRPRINT:
	    noun = symbol;
	    q = "";
	    if ( yylval.intval == DOT )
		printname = ".";
	    break;
	case ASGPRINT:
	    noun = symbol;
	    q = "";
	    switch ( yylval.intval ){
	    case ASG PLUS: printname = "+="; break;
	    case ASG MINUS: printname = "-="; break;
	    case ASG MUL: printname = "*="; break;
	    case ASG DIV: printname = "/="; break;
	    case ASG MOD: printname = "%="; break;
	    case ASG AND: printname = "&="; break;
	    case ASG OR: printname = "|="; break;
	    case ASG ER: printname = "^="; break;
	    case ASG LS: printname = "<<="; break;
	    case ASG RS: printname = ">>="; break;
	    }
	    break;
	case NAMEPRINT:
	    noun = "variable name";
	    printname = yytext;
	    break;
	case STRINGPRINT:
	    noun = "string";
	    goto gettoken;
	case VALPRINT:
	case FVALPRINT:
	    noun = "constant";
	gettoken:
	    thischar = tokebufr;
	    q = "";
	    if (tokebufptr >= MAXTOKELENGTH)
		mt = MAXTOKELENGTH;
	    else
		mt = tokebufptr;
	    for (i=0; i<=mt; i++){
		if (isascii(*thischar) && isprint(*thischar))
		    token[i] = *thischar;
		else
		    token[i] = '?';
		thischar ++;
	    }
	    token[i] = '\0';
	    if (mt < tokebufptr)
		strcat( token, "...");
	    printname = token;
	    break;
	default:
	    noun = "";
	    break;
	}
	sprintf( mungbuffer, "%s at or near %s %s%s%s", s, noun, q, printname, q);
	uerror( "%s", mungbuffer );
    }
}

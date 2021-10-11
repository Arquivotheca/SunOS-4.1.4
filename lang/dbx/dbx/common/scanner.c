#ifndef lint
static	char sccsid[] = "@(#)scanner.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Debugger scanner.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <ctype.h>
#include "defs.h"
#include "scanner.h"
#include "machine.h"
#include "library.h"
#include "dbxmain.h"
#include "display.h"
#include "keywords.h"
#include "tree.h"
#include "symbols.h"
#include "names.h"
#include "pipeout.h"
#include "commands.h"
#include "expand_name.h"

#ifndef public
typedef int Token;
#endif

typedef enum { WHITE, ALPHA, NUM, OTHER } Charclass;

private Charclass class[256 + 1];
private Charclass *lexclass = class + 1;

#undef isdigit
#undef isalnum
#undef ishexdigit
#define isdigit(c) (lexclass[c] == NUM)
#define isalnum(c) (lexclass[c] == ALPHA or lexclass[c] == NUM)
#define ishexdigit(c) ( \
    isdigit(c) or (c >= 'a' and c <= 'f') or (c >= 'A' and c <= 'F') \
)
#define isoctaldigit(c) (c >= '0' and c <= '7')

#define MAXLINESIZE 10240

private File in;
private Char linebuf[MAXLINESIZE];
private Char *curchar;
private Boolean aliasing = true;		/* Look for aliases */
private Boolean tag;				/* Look for a tag name */
private Boolean qualified=false;	/* Looking for a qualifiable name */
private Boolean search_str;			/* Look for a search string */
private int	search_delim;			/* Search string delimiter */
private Boolean case_sense = true;		/* Case sensitivity */

#define MAXINCLDEPTH 10

private struct {
    File savefile;
    Filename savefn;
    int savelineno;
} inclinfo[MAXINCLDEPTH];

private unsigned int curinclindex;

private Token getident();
private Token getnum();
private Token getstring();
private Token getcharcon();
private Boolean eofinput();
private char *charcon();
private Boolean expand_metachars();

private enterlexclass(cls, s)
Charclass cls;
String s;
{
    register char *p;

    for (p = s; *p != '\0'; p++) {
	lexclass[*p] = cls;
    }
}

public scanner_init()
{
    register Integer i;

    for (i = 0; i < 257; i++) {
	class[i] = OTHER;
    }
    enterlexclass(WHITE, " \t");
    enterlexclass(ALPHA, "abcdefghijklmnopqrstuvwxyz");
    enterlexclass(ALPHA, "ABCDEFGHIJKLMNOPQRSTUVWXYZ_$");
    enterlexclass(NUM, "0123456789");
    in = stdin;
    errfilename = nil;
    errlineno = 0;
    curchar = linebuf;
    linebuf[0] = '\0';
}

private String refill_line( prompt, loop )
char *prompt;
Boolean loop;
{
  String line;

    do {
	if (isterm(in)) {
	    printf("%s", get_prompt());
	    fflush(stdout);
	}
	line = fgets(linebuf, MAXLINESIZE, in);
    } while( loop  &&  line == nil  &&  not eofinput( ) );
    if( line == nil )
	linebuf[ 0 ] = 0;
    return line;
}

/*
 * Read a single token.
 *
 * Input is line buffered.
 *
 * There are two "modes" of operation:  one as in a compiler,
 * and one for reading shell-like syntax.
 */

private Boolean shellmode;
private Boolean adb_mode;
private Boolean filenamemode;


public Token yylex()
{
    register int c;
    register char *p;
    register Token t;
    String line;

 backslash_seen:
    p = curchar;
    if (*p == '\0') {
	line = refill_line( get_prompt(), true );
	if (line != nil) {
	    p = linebuf;
	    while (lexclass[*p] == WHITE) {
		p++;
	    }
	    set_shellmode(false);
	}
    } else {
	if (search_str) {
	    find_search();
	    return(STRING);
	}
	while (lexclass[*p] == WHITE) {
	    p++;
	}
    }
    curchar = p;
    c = *p;
    if (lexclass[c] == ALPHA) {
	t = getident();
    } else if (lexclass[c] == NUM or (c == '.' and lexclass[p[1]] == NUM)) {
	if (shellmode) {
	    t = getident();
	} else {
	    t = getnum();
	}
    } else {
	++curchar;
	if (shellmode and (c != '\0') and (index(";\n<>", c) == nil)) {
	    if (c == '"' || c == '\'') {
		(void) getstring(c);
		yylval.y_name = identname(yylval.y_string, false);
		t = NAME;
	    } else {
	        --curchar;
	        t = getident();
	    }
	} else switch (c) {
	    case '#':
		gobble();

		/* FALL THRU */

	    case '\\':
		(void) refill_line( get_prompt(), false );
		curchar = p = linebuf;	/* Don't copy the \n at all. */
		goto backslash_seen;

	    case '\n':
		t = '\n';
		if (errlineno != 0) {
		    errlineno++;
		}
		break;

	    case '"':
		t = getstring(c);
		break;

	    case '\'':
		t = getcharcon();
		break;

	    case '.':
		if (isdigit(*curchar)) {
		    --curchar;
		    t = getnum();
		} else {
		    t = '.';
		}
		break;

	    case '<':
		if (not shellmode and *curchar == '<') {
		    ++curchar;
		    t = LSHIFT;
		} else {
		    t = '<';
		}
		break;

	    case '>':
		if (*curchar == '>') {
		    ++curchar;
		    t = RSHIFT;
		} else {
		    t = '>';
		}
		break;

	    case '-':
		if (*curchar == '>') {
		    ++curchar;
		    t = ARROW;
		} else {
		    t = '-';
		}
		break;

	    case '&':
		if (*curchar == '&') {
		    ++curchar;
		    t = ANDAND;
		} else {
		    t = '&';
		}
		break;

	    case '|':
		if (*curchar != '|') {
		    t = '|';
		} else {
		    ++curchar;
		    t = OROR;
		}
		break;
	    
	    case '%':
		t = CMOD;
		break;

	    case '\0':
		t = 0;
		break;

	    default:
		t = c;
		break;
	}
    }
#   ifdef LEXDEBUG
	if (lexdebug) {
	    printf("yylex returns ");
	    print_token(t);
	    printf("\n");
	}
#   endif
    return t;
} /* end of yylex */


/*
 * Look for a search string.
 * Take all the characters until a slash or newline.
 */
private find_search()
{
   char buf[256];
   char *to;
   char *from;

   to = buf;
   from = curchar;
   while (*from != '\n' and *from != search_delim) {
	if (*from == '\\') {
	    if (from[1] == search_delim) {
	        from++;
	    }
	}
	*to++ = *from++;
    }
    curchar = from;
    *to = '\0';
    yylval.y_string = strdup(buf);
}

/*
 * Parser error handling.
 */

public yyerror(s)
String s;
{
    register Char *p, *tokenbegin, *tokenend;
    register Integer len;
    static int once_only = 0;

    set_tag(false); 
    if (streq(s, "syntax error")) {
	beginerrmsg();
	tokenend = curchar - 1;
	tokenbegin = tokenend;
	while (lexclass[*tokenbegin] != WHITE and tokenbegin > &linebuf[0]) {
	    --tokenbegin;
	}
	len = tokenend - tokenbegin + 1;
	p = tokenbegin;
	if (p > &linebuf[0]) {
	    while (lexclass[*p] == WHITE and p > &linebuf[0]) {
		--p;
	    }
	}
	if (p == &linebuf[0]) {
	    printf("unrecognized command/syntax \"%.*s\"\n",
	      len, tokenbegin);
	    if (once_only == 0) {
		once_only++;
		printf("(Type 'help' for help)");
	    }
	} else {
	    printf("syntax error");
	    if (len != 0) {
		printf(" on \"%.*s\"", len, tokenbegin);
	    }
	}
	enderrmsg();
    } else {
	error(s);
    }
}

/*
 * Eat the current line.
 */

public gobble()
{
    curchar = linebuf;
    linebuf[0] = '\0';
}

/*
 * Enable/disable adb mode. It doesn't do anything fancy, just turns
 * helps with case insensitivity in 'getident()'
 */
public	set_adb(value)
Boolean	value;
{
    adb_mode = value;
}


/*
 * Enable/disable aliasing.  Aliasing is enabled whereever
 * a dbx command may be found.
 */
public	set_alias(value)
Boolean	value;
{
    aliasing = value;
}

/*
 * Indicate whether we are looking for at tag name or not.
 * If looking for a tag name, then do not check a token for
 * being a keyword or typename.
 */
public set_tag(value)
Boolean value;
{
    tag = value;
}

/* indicate that we're looking for a qualifiable name, 
 * typically a field name. No typenames or keywords allowed
 */
public	set_qualified(value)
Boolean	value;
{
    qualified = value;
}

/*
 * Tell the scanner to look for a search string for the search (/)
 * command. 
 */
public 	search(value, delim)
Boolean	value;
int	delim;
{
    search_str = value;
    search_delim = delim;
}

/*
 * Scan an identifier and check to see if it's a keyword.
 */

private Token getident()
{
    char buf[MAXLINESIZE];
    register Char *q;
    register Token t;
    Token key;
    Symbol s;

    collect_ident( buf );	/* Get all the characters of the ident */

    if (case_sense == false && ! adb_mode) {
	q = buf;
	while (*q) {
	    if (isupper(*q)) {
		*q = tolower(*q);
	    }
	    q++;
	}
    }
    yylval.y_name = identname(buf, false);
    t = NAME;
    if (not shellmode) {
	/*
	 * Have a name, find out if it is an alias,
	 * a keyword, or a typedef.  If none of the above, then 
	 * it is a name.
	 */
	if (aliasing and find_alias(yylval.y_name, buf)) {
	    /*
	     * Found an alias.  Insert the new string into
	     * the input buffer to be scanned.
	     */
	    input_insert(buf);
	    aliasing = false;
	    t = yylex();
	} else if (!tag and !qualified and (key = findkeyword(yylval.y_name)) != 0) {
	    t = key;
	} else if (!tag and !qualified and istype(yylval.y_name)) {
	    t = TYPENAME;
	}
    }
    aliasing = false;
		/* qualification only applies to the next token - now that
		** we've seen it - reset
		*/
	set_qualified(false); 
    return t;
}


/*
 * collect an identifier's characters and deal with shell mode
 * escapes and possibly strings.
 */
private collect_ident ( buffer ) char *buffer; {
    register Char *p, *q;
    Boolean endofident;

    p = curchar;
    q = buffer;

    if (!shellmode) {
	if (filenamemode) {
		do {
		    *q++ = *p++;
		} while (isalnum(*p) or (*p == '.') or (*p == '_') or
				(*p == '-'));
	} else {
		do {
		    *q++ = *p++;
		} while (isalnum(*p));
	}
	curchar = p;
	*q = '\0';
	return;
    }

    /*
     * We're in shell mode:  a NAME can start as an identifier
     * and turn into a quoted string; might have backslashes too.
     */
    for( endofident = false;
	!endofident;
	 endofident = (Boolean)( index(" \t\n<>;", *p) != nil ) )
    {
	switch( *p ) {
	 case '\'':
	 case '"':
	    /*
	     * This "identifier" has a string inside it.  (Input was,
	     * e.g., <a"b c">, so we have a string, not an ident.)  But
	     * because we're in shellmode, we'll still call it a NAME,
	     * and we will want to expand it with its quotes left intact,
	     * including escaped ones!  First, let's get it working
	     * without keeping the quotes intact and see if that works.
	     */
	    curchar = p+1;
	    interpretstring( *p, q );
	    q = index( q, 0 );
	    p = curchar;
	    break;
	
	 case '\\':	/* a backslash not inside a string */
	    if( p[1] == '\n' ||  p[1] == 0 ) {
		/* Got backslash-newline.  Get a new line. */
		(void) refill_line( get_prompt(), false );
		curchar = p = linebuf;	/* Don't copy the \n at all. */
	    } else {
		*q++ = *p++;		/* Copy char but don't check it */
	    }
	    break;

	 default:
	    *q++ = *p++;
	    break;
	} /* end switch */

    } /* end for */

    curchar = p;
    *q = '\0';

    { Namelist namelist;

	namelist = expand_name(buffer);
	if (namelist != NONAMES && namelist->names[0]) {
	    strcpy(buffer, namelist->names[0]);
	    if (namelist->count > 1) {
		input_namelist(namelist);
	    }
	    free_namelist(namelist);
        }
    }

} /* end collect_ident */


/*
 * Scan a number.
 */
private Token getnum()
{
    char buf[256];
    register Char *p, *q;
    register Token t;
    Integer base;

    p = curchar;
    q = buf;
    if (*p == '0') {
	if (*(p+1) == 'x') {
	    p += 2;
	    base = 16;
	} else {
	    base = 8;
	}
    } else {
	base = 10;
    }
    if (base == 16) {
	do {
	    *q++ = *p++;
	} while (ishexdigit(*p));
    } else {
	while (isdigit(*p)) {
	    *q++ = *p++;
	}
    }
    if (*p == '.') {
	do {
	    *q++ = *p++;
	} while (isdigit(*p));
	if (*p == 'e' or *p == 'E') {
	    p++;
	    if (*p == '+' or *p == '-' or isdigit(*p)) {
		*q++ = 'e';
		do {
		    *q++ = *p++;
		} while (isdigit(*p));
	    }
	}
	*q = '\0';
	yylval.y_real = atof(buf);
	t = REAL;
    } else {
	*q = '\0';
	switch (base) {
	    case 10:
		yylval.y_int = atol(buf);
		break;

	    case 8:
		yylval.y_int = octal(buf);
		break;

	    case 16:
		yylval.y_int = hex(buf);
		break;

	    default:
		badcaseval(base);
	}
	t = INUM;
    }
    curchar = p;
    return t;
}

/*
 * Convert a string of octal digits to an integer.
 */

private int octal(s)
String s;
{
    register Char *p;
    register Integer n;

    n = 0;
    for (p = s; *p != '\0'; p++) {
	n = 8*n + (*p - '0');
    }
    return n;
}

/*
 * Convert a string of hexadecimal digits to an integer.
 */

private int hex(s)
String s;
{
    register Char *p;
    register Integer n;

    n = 0;
    for (p = s; *p != '\0'; p++) {
	n *= 16;
	if (*p >= 'a' and *p <= 'f') {
	    n += (*p - 'a' + 10);
	} else if (*p >= 'A' and *p <= 'F') {
	    n += (*p - 'A' + 10);
	} else {
	    n += (*p - '0');
	}
    }
    return n;
}





/*
 * Scan a string.
 */

private Token getstring(terminator)
int	terminator;
{
    char buf[MAXLINESIZE];

    interpretstring( terminator, buf );
    yylval.y_string = strdup(buf);
    return(STRING);
}


/*
 * Scan a string.
 */

private Token interpretstring(terminator, bufptr)
int	terminator;
char   *bufptr;
{
    register Char *p, *q;
    Boolean endofstring;

    p = curchar;
    q = bufptr;
    endofstring = false;
    while (not endofstring) {
	if (*p == '\n' or *p == '\0') {
	    error("non-terminated string");
	    endofstring = true;
	} else if (*p == terminator) {
	    p++;
	    endofstring = true;
	} else {
	    p = charcon(p, q);
	    q++;
	}
    }
    curchar = p;
    *q = '\0';
}


/*
 * Get a character constant (one in single quotes).
 */
private Token getcharcon()
{
    char *p;
    char c;
    int val;

    p = curchar;
    p = charcon(p, &c);
    val = c;
    if (*p != '\'') {
	warning("More than one character in a character constant");
	while (*p != '\'') { 
	    p = charcon(p, &c);
	    val = (val << 8) + c;
	}
    }
    p++;
    curchar = p;
    yylval.y_int = val;
    return(INUM);
}

/*
 * Get the value of a character constant.
 * Watch for special escape sequences like \n or \015
 * Returns pointer to char following end of the char con.
 * fixed bug 1005495 Feb 8 1998, DT, getconchar, to read octal
 * escape sequences properly
 */
private char *charcon(from, to)
char *from;
char *to;
{
	int c;
	int val;
	int i;

	switch(c = *from++) {
	case '\n':
		error("newline in string or char constant");
		break;

	case '\\':
		switch(c = *from++) {
		case 'n':
			val = '\n';
			break;

		case 'r':
			val = '\r';
			break;

		case 'b':
			val = '\b';
			break;

		case 't':
			val = '\t';
			break;

		case 'f':
			val = '\f';
			break;

		case 'v':
			val = '\013';
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		/* Feb 8 1988 DT fixed escape sequences re bug # 1005495 */
			val = c - '0';
			c = *from;  		/* try for 2 */
			if (isoctaldigit(c)) {
				val = (val << 3) | (c - '0');
				c = *++from;  	/* try for 3 */
				if (isoctaldigit(c)) {
				        from = from++;
					val = (val << 3) | (c - '0');
				}
			}
			break;

		default:
			val = c;
			break;
		}
		break;
	default:
		val = c;
		break;
	}
	*to = val;
	return(from);
}

/*
 * Input file management routines.
 */

public setinput(filename)
Filename filename;
{
    File f;
    jmp_buf save_env;
    struct stat sbuf;
    extern int yychar;
    extern short yyerrflag;

    f = fopen(filename, "r");
    if (f == nil) {
	error("can't open %s", filename);
    } else if( fstat( fileno(f), &sbuf ) < 0 ) {	/* cannot stat */
	error( "stat() failure on \"%s\"", filename);
    } else if( ((sbuf.st_mode & S_IFMT) == S_IFDIR ) ) {
	error("\"%s\" is a directory", filename);
    } else {
	/* All is well. */
	if (curinclindex >= MAXINCLDEPTH) {
	    error("unreasonable input nesting on \"%s\"", filename);
	}
	inclinfo[curinclindex].savefile = in;
	inclinfo[curinclindex].savefn = errfilename;
	inclinfo[curinclindex].savelineno = errlineno;
	curinclindex++;
	in = f;
	errfilename = filename;
	errlineno = 1;
	copy_jmpbuf(env, save_env);
	setjmp(env);
	yyparse();
	copy_jmpbuf(save_env, env);
	yychar = -1;
	yyerrflag = 0;
	fclose(in);
	--curinclindex;
	in = inclinfo[curinclindex].savefile;
	errfilename = inclinfo[curinclindex].savefn;
	errlineno = inclinfo[curinclindex].savelineno;
	curchar = linebuf;
	linebuf[0] = '\0';
    }
}

private Boolean eofinput()
{
    register Boolean b;

    if (isterm(in)) {
        putchar('\n');
        clearerr(in);
        b = false;
    } else {
        b = true;
    }
    return b;
}

/*
 * Return whether we are currently reading from standard input.
 */

public Boolean isstdin()
{
    return (Boolean) (in == stdin);
}

/*
 * Send the current line to the shell.
 */

public shellline()
{
    register char *p;

    p = curchar;
    while (*p != '\0' and (*p == '\n' or lexclass[*p] == WHITE)) {
	++p;
    }
    shell(p);
    if (*p == '\0' and isterm(in)) {
	putchar('\n');
    }
    erecover();
}

/*
 * Set the value of the shellmode flag.  If on, look for filenames.
 */
public set_shellmode(val)
Boolean	val;
{
    shellmode = val;
}

/*
 * Set the value of the filenamemode flag.  If on, look for filenames.
 */
public set_filenamemode(val)
Boolean	val;
{
    filenamemode = val;
}

#ifdef LEXDEBUG
/*
 * Print out a token for debugging.
 */

public print_token(t)
Token t;
{
    if (t == '\n') {
	printf("char '\\n'");
    } else if (t == EOF) {
	printf("EOF");
    } else if (t < 256) {
	printf("char '%c'", t);
    } else {
	printf("\"%s\"", keywdstring(t));
    }
}
#endif

/*
 * Insert a string in the input stream.
 * Save what is left of the current line,
 * copy in the new string and concatenate the 
 * saved string to the new one.
 */
private input_insert(str)
char *str;
{
    char buf[MAXLINESIZE];

    strcpy(buf, curchar);
    strcpy(linebuf, str);
    strcat(linebuf, buf);
    curchar = linebuf;
}

/*
 * We have expanded some meta characters and have some
 * extra tokens to put back in the input stream.
 * Collect all the tokens into one string and then
 * insert that string back into the stream.
 */
private input_namelist(namelist)
Namelist namelist;
{
    int i;
    char buf[MAXLINESIZE];
    char *to;
    char *from;

    to = buf;
    for (i = 1; i < namelist->count; i++) {
	from = namelist->names[i];
	while (*from) {
		*to++ = *from++;
	}
	*to++ = ' ';
    }
    to--;
    *to = '\0';
    input_insert(buf);
}

/*
 * For alias processing, return the rest of the command line.
 * The command line is terminated by a newline or an unescaped semicolon.
 */
public get_restofline(to)
char *to;
{
    char *from;
    int	quote;

    quote = 0;
    from = curchar; 
    while (*from != '\0') {
	if (*from == '\n') {
	    break;
	}
	if (*from == ';' and not quote) {
	    break;
	}
	if (*from == '\'' or *from == '"') {
	    if (*from == quote) {
		quote = 0;		/* End of string */
	    } else if (quote == 0) {
	        quote = *from;		/* Beginning of string */
	    }
	} else if (*from == '\\') {
	    *to++ = *from++;
	}
	*to++ = *from++;
    }
    *to = '\0';
    curchar = from;
}

/*
 * Set the case sensitivity.
 */
public set_case(val)
Boolean val;
{
    case_sense = val;
}

/*
 * Return the case sensitivity
 */
public Boolean get_case()
{
    return(case_sense);
}

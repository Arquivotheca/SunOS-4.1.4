#ifndef lint
static	char sccsid[] = "@(#)alias_expand.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 *  Expand command based on alias string
 */
#include <ctype.h>
#include "defs.h"

#define BACKSLASH 92
#define SINGLEQUOTE 39
#define DOUBLEQUOTE 34

#define MAXARGS 20

#define scan() *string++
#define unscan() --string
#define peek() *string
#define skip() string++
#define parse_int(n) scan_int(&string, n)
#define substitute(n, m) sub_args(&cp, cmd, args, argv, n, m)
#define copystr(a, b) { while (*(a)++ = *(b)++); (a)--;}


expand_alias(cmd, args, string, line)
    char *cmd;
    char *args;
    char *string;
    char *line;
{
    char *argv[MAXARGS][2], *cp = line, c;
    int n, m;
    Boolean bang;

    bang = false;
    argv[0][0] = 0;			/* Clear arg list */
    while (c = scan()) {
	switch (c) {
	case '!':			/* replacement keys off '!'  */
	    bang = true;
	    if (':' == peek()) {
		skip();			/* handle !:n, etc.  Compatability */
	    }
	    switch (c = scan()) {
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		unscan();
		parse_int(&n);		/* !n so far  */
		m = n;
		if ('*' == peek()) {
		    m = -1;
		    skip();		/* !n*  */
		} else if ('-' == peek()) {
		    skip();
		    if (isdigit(peek())) {
			parse_int(&m);	/* !n-m  */
		    } else {
			m = -2;		/* !n-  */
		    }
		}
		substitute(n, m);
		break;

	    case '-':			/* !-m or !- (= !0-m or !0-$) */
		m = -1;
		parse_int(&m);
		substitute(0, m);
		break;

	    case '^':			/* !^ (= !1)  */
		substitute(1, 1);
		break;

	    case '$':			/* !$ (= !last)  */
		substitute(-1, -1);
		break;

	    case '*':			/* !* (= !1-$)  */
		substitute(1, -1);
		break;

	    default:			/* no match, straight insert  */
		*cp++ = '!';
	        *cp++ = c;
	    }
	    break;

	case BACKSLASH:		/* quote next char  */
	    if (peek()) *cp++ = scan();
	    break;

	default:
	    *cp++ = c;
	    break;
	}
    }
    if (not bang) {
	substitute(1, -1);
    }
    *cp = 0;
}

/* Scan positive integer */
scan_int(sp, ip)
    char **sp;
    int *ip;
{
    char nbuf[20], *cp = nbuf;

    while (isdigit(**sp)) *cp++ = *(*sp)++;
    if (cp != nbuf) {
	*cp = 0;
	*ip = atoi(nbuf);
    }
}

/* Substitute alias args with command args */
sub_args(linep, cmd, string, argv, n, m)
char **linep, *cmd, *string, *argv[MAXARGS][2];
int n, m;
{
    static int arg_cnt;
    int direction;

#ifdef debug
    printf("sub args %d, %d\n", n, m);
#endif
    if (n == 0) {
	copystr(*linep, cmd);
	if (m == 0) return;
	*(*linep)++ = ' ';
	n = 1;
    }
    if (n == 1 && m == -1) {
	copystr(*linep, string);
	return;
    }
    if (argv[0][0] == 0) {
	arg_cnt = parse_args(cmd, string, argv);
    }
    if (m < 0) {
	m = arg_cnt + m > 0 ? arg_cnt + m : 0;
    }
    if (n < 0) {
	n = arg_cnt + n > 0 ? arg_cnt + n : 0;
    }
    m += direction = n-m <= 0 ? 1 : -1;
    for (; n-m && n < arg_cnt; n += direction) {
	strncpy(*linep, argv[n][0], (int)argv[n][1]);
	*linep += (int)argv[n][1];
	*(*linep)++ = ' ';
    }
    --(*linep);
}

parse_args(cmd, string, argv)
char *cmd, *string, *argv[MAXARGS][2];
{
    char c;
    int i = 1, n = 0, whitespace = 1;

    if (argv[0][0] != 0) {
	return(0);
    }
    argv[0][0] = cmd;
    argv[0][1] = (char *)strlen(cmd);
    while (c = scan()) {
	switch (c) {
	case ' ':
	case '\t':
	    if (!whitespace) {
		argv[i++][1] = (char *)n;
		whitespace = 1;
	    }
	    break;

	case BACKSLASH:			/* quote next char  */
	    if (!peek()) break;
	    skip();
	    /* Fall thru */

	default:
	    if (whitespace) {
		argv[i][0] = string-1;
		whitespace = n = 0;
	    }
	    n++;
	}
    }
    if (!whitespace) {
	argv[i++][1] = (char *)n;
    }
    argv[i][0] = 0;
    return(i);
}

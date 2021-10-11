#ifndef lint
static	char sccsid[] = "@(#)input.c 1.1 94/10/31 SMI";
#endif
/*
 * adb - input routines
 */

#include "adb.h"
#include <stdio.h>

char	line[BUFSIZ];
int	infile;
int	eof;
char	lastc = '\n';

eol(c)
	char c;
{

	return (c == '\n' || c == ';');
}

rdc()
{

	do
		(void) readchar();
	while (lastc == ' ' || lastc == '\t');
	return (lastc);
}

readchar()
{

	if (eof) {
		lastc = 0;
		return (0);
	}
	if (lp == 0) {
		lp = line;
		do {
			eof = read(infile, lp, 1) <= 0;
			if (interrupted)
				error((char *)0);
		} while (eof == 0 && *lp++ != '\n');
		*lp = 0;
		lp = line;
	}
	if (lastc = nextc)
		nextc = 0;
	else
		if (lastc = *lp)
			lp++;
	return (lastc);
}

nextchar()
{

	if (eol(rdc())) {
		lp--;
		return (0);
	}
	return (lastc);
}

quotchar()
{

	if (readchar() == '\\')
		return (readchar());
	if (lastc=='\'')
		return (0);
	return (lastc);
}

getformat(deformat)
	char *deformat;
{
	register char *fptr = deformat;
	register int quote = 0;

	while (quote ? readchar() != '\n' : !eol(readchar()))
		if ((*fptr++ = lastc)=='"')
			quote = ~quote;
	lp--;
	if (fptr != deformat)
		*fptr++ = '\0';
}

#define	MAXIFD	5
struct {
	int	fd;
	int	r9;
} istack[MAXIFD];
int	ifiledepth;

iclose(stack, err)
	int stack, err;
{

	if (err) {
		if (infile) {
			(void) close(infile);
			infile = 0;
		}
		while (--ifiledepth >= 0)
			if (istack[ifiledepth].fd)
				(void) close(istack[ifiledepth].fd);
		ifiledepth = 0;
		return;
	}
	switch (stack) {

	case 0:
		if (infile) {
			(void) close(infile);
			infile = 0;
		}
		break;
	
	case 1:
		if (ifiledepth >= MAXIFD)
			error("$<< nesting too deep");
		istack[ifiledepth].fd = infile;
		istack[ifiledepth].r9 = var[9];
		ifiledepth++;
		infile = 0;
		break;

	case -1:
		if (infile) {
			(void) close(infile);
			infile = 0;
		}
		if (ifiledepth > 0) {
			infile = istack[--ifiledepth].fd;
			var[9] = istack[ifiledepth].r9;
		}
	}
}

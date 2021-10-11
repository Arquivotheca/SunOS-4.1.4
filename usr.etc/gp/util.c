#ifndef lint
static char sccsid[] = "@(#)util.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1987 by Sun Microsystems,  Inc.
 */

#include <stdio.h>
#include <varargs.h>

char *Progname;

char *
basename(path)
	char *path;
{
	register char *p = path, c;

	while (c = *p++)
		if (c == '/')
			path = p;

	return path;
}

char *
sexpand(str, size)
	char *str;
	int size;
{
	char *p, *malloc(), *strcpy();

	if (p = malloc((unsigned) (strlen(str) + size + 1)))
		return strcpy(p, str);

	error("out of memory");
	/*NOTREACHED*/
}

/*VARARGS*/
error(va_alist)
va_dcl
{
	va_list ap;

	va_start(ap);
	_warning(ap);
	va_end(ap);

	exit(1);
}

/*VARARGS*/
warning(va_alist)
va_dcl
{
	va_list ap;

	va_start(ap);
	_warning(ap);
	va_end(ap);
}

/*VARARGS*/
static
_warning(ap)
	va_list ap;
{
	char *fmt;

	if (fmt = va_arg(ap, char *))
		(void) fprintf(stderr, "%s: ", Progname);
	else
		fmt = va_arg(ap, char *);

	(void) vfprintf(stderr, fmt, ap);
	(void) fprintf(stderr, "\n");
}

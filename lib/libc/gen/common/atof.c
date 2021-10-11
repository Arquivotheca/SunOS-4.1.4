#if !defined(lint) && defined(SCCSIDS)
static char     sccsid[] = "@(#)atof.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc. 
 */

#include <stdio.h>

extern double 
strtod();

double
atof(cp)
	char           *cp;
{
	return strtod(cp, (char **) NULL);
}

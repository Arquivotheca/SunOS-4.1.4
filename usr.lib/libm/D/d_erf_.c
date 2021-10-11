
#ifndef lint
static	char sccsid[] = "@(#)d_erf_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_erf_(x)
double *x;
{
	return erf(*x);
}

double d_erfc_(x)
double *x;
{
	return erfc(*x);
}

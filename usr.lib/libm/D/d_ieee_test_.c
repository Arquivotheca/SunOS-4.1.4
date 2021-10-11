
#ifndef lint
static	char sccsid[] = "@(#)d_ieee_test_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_logb_(x)
double *x;
{
	return logb(*x);
}

double d_scalb_(x,fn)
double *x,*fn;
{
	return scalb(*x,*fn);
}

double d_significand_(x)
double *x;
{
	return significand(*x);
}

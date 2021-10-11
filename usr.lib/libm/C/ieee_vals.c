
#ifndef lint
static char     sccsid[] = "@(#)ieee_vals.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

#include "libm.h"

double
min_subnormal()
{
	return fmin;
}

double
max_subnormal()
{
	return fmaxs;
}

double
min_normal()
{
	return fminn;
}

double
max_normal()
{
	return fmax;
}

double
infinity()
{
	return Inf;
}

double
quiet_nan(n)
	long            n;
{
	return NaN;
}

double
signaling_nan(n)
	long            n;
{
	return sNaN;
}

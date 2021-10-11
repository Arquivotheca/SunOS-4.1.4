#ifndef lint
static	char sccsid[] = "@(#)r_nint.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE
r_nint(x)
float *x;
{
	return r_anint_(x);
}

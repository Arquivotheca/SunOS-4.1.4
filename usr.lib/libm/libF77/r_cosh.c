#ifndef lint
static	char sccsid[] = "@(#)r_cosh.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE
r_cosh(x)
float *x;
{
	return r_cosh_(x);
}

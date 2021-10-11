#ifndef lint
static	char sccsid[] = "@(#)r_atan.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE
r_atan(x)
float *x;
{
	return r_atan_(x);
}

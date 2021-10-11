#ifndef lint
static	char sccsid[] = "@(#)r_acos.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE
r_acos(x)
float *x;
{
	return r_acos_(x);
}

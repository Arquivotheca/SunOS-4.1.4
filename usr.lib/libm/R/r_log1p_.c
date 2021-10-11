
#ifndef lint
static	char sccsid[] = "@(#)r_log1p_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE r_log1p_(x)
float *x;
{
	float w;
	w = (float) log1p((double)*x);
	RETURNFLOAT(w);
}

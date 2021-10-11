
#ifndef lint
static	char sccsid[] = "@(#)r_atanh_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE r_atanh_(x)
float *x;
{
	float w;
	w = (float) atanh((double)*x);
	RETURNFLOAT(w);
}


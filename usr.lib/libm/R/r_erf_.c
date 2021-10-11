
#ifndef lint
static	char sccsid[] = "@(#)r_erf_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE r_erf_(x)
float *x;
{
	float w;
	w = (float) erf((double)*x);
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE r_erfc_(x)
float *x;
{
	float w;
	w = (float) erfc((double)*x);
	RETURNFLOAT(w);
}


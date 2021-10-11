
#ifndef lint
static	char sccsid[] = "@(#)r_remainder_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE r_remainder_(x,y)
float *x, *y;
{
	float w;
	w = (float) remainder((double)*x,(double)*y);
	RETURNFLOAT(w);
}

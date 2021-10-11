
#ifndef lint
static	char sccsid[] = "@(#)r_fabs_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE r_fabs_(x)
float *x;
{
	float w; 
	*(int *)(&w)  = (*(int *) x)&0x7fffffff;
	RETURNFLOAT(w);
}


#ifndef lint
static	char sccsid[] = "@(#)r_compound_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* r_compound_(r,n)
 *
 * Returns (1+r)**n; undefined for r <= -1.
 */

#include <math.h>
FLOATFUNCTIONTYPE r_compound_(r,n)
float *r,*n;
{
	float w;
	w = (float)  compound((double)*r,(double)*n);
	RETURNFLOAT(w);
}

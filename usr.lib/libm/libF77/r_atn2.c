#ifndef lint
static	char sccsid[] = "@(#)r_atn2.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE
r_atn2(x,y)
float *x, *y;
{
	return r_atan2_(x, y);
}

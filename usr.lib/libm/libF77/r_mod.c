#ifndef lint
static	char sccsid[] = "@(#)r_mod.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE
r_mod(x,y)
float *x, *y;
{
	return r_fmod_(x, y);
}

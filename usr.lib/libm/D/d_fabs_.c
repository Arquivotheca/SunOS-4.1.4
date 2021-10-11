
#ifndef lint
static	char sccsid[] = "@(#)d_fabs_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_fabs_(x)
double *x;
{
	return fabs(*x);
}

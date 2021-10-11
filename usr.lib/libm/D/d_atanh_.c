
#ifndef lint
static	char sccsid[] = "@(#)d_atanh_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_atanh_(x)
double *x;
{
	return atanh(*x);
}


#ifndef lint
static	char sccsid[] = "@(#)d_acosh_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_acosh_(x)
double *x;
{
	return acosh(*x);
}

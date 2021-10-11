
#ifndef lint
static	char sccsid[] = "@(#)d_log10_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_log10_(x)
double *x;
{
	return log10(*x);
}

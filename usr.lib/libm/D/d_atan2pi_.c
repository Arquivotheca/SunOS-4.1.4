
#ifndef lint
static	char sccsid[] = "@(#)d_atan2pi_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_atan2pi_(y,x)
double *y,*x;
{
	return atan2pi(*y,*x);
}

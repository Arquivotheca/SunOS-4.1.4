
#ifndef lint
static	char sccsid[] = "@(#)d_atan2_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_atan2_(y,x)
double *y,*x;
{
	return atan2(*y,*x);
}

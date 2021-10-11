
#ifndef lint
static	char sccsid[] = "@(#)d_acospi_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_acospi_(x)
double *x;
{
	return acospi(*x);
}

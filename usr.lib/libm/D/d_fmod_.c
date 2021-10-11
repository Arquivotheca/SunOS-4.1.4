
#ifndef lint
static	char sccsid[] = "@(#)d_fmod_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_fmod_(x,y)
double *x,*y;
{
	return fmod(*x,*y);
}

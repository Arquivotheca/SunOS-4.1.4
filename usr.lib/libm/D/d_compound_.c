
#ifndef lint
static	char sccsid[] = "@(#)d_compound_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* d_compound_(r,n)
 *
 * Returns (1+r)**n; undefined for r <= -1.
 */

#include <math.h>
double d_compound_(r,n)
double *r,*n;
{
	return compound(*r,*n);
}


#ifndef lint
static	char sccsid[] = "@(#)d_annuity_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* d_annuity_(r,n)
 *
 * Returns (1-(1+r)**-n)/r (return n when r==0); undefined for r <= -1.
 */

#include <math.h>
double d_annuity_(r,n)
double *r,*n;
{
	return annuity(*r,*n);
}

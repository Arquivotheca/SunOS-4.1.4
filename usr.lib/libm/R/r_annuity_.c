
#ifndef lint
static	char sccsid[] = "@(#)r_annuity_.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* r_annuity_(r,n)
 *
 * Returns (1-(1+r)**-n)/r (return n when r==0); undefined for r <= -1.
 */

#include <math.h>
FLOATFUNCTIONTYPE r_annuity_(r,n)
float *r,*n;
{
	float w;
	w = (float) annuity((double)*r,(double)*n);
	RETURNFLOAT(w);
}

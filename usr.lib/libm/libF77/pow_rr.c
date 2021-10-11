#ifndef lint
static	char sccsid[] = "@(#)pow_rr.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE 
pow_rr(ap, bp)
float *ap, *bp;
{
return r_pow_(ap, bp);
}

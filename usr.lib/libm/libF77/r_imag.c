#ifndef lint
static	char sccsid[] = "@(#)r_imag.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "complex.h"

FLOATFUNCTIONTYPE
r_imag(z)
complex *z;
{
	RETURNFLOAT(z->imag);
}

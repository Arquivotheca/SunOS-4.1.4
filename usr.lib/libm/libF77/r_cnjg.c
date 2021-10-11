#ifndef lint
 static	char sccsid[] = "@(#)r_cnjg.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

#include "complex.h"

r_cnjg(r, z)
complex *r, *z;
{
r->real = z->real;
r->imag = - z->imag;
}

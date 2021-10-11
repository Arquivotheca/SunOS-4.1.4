#ifndef lint
 static	char sccsid[] = "@(#)c_cos.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

#include "complex.h"

c_cos(r, z)
complex *r, *z;
{
double sin(), cos(), sinh(), cosh();

r->real = cos(z->real) * cosh(z->imag);
r->imag = - sin(z->real) * sinh(z->imag);
}

#ifndef lint
 static	char sccsid[] = "@(#)z_exp.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

#include "complex.h"

z_exp(r, z)
dcomplex *r, *z;
{
double expx;
double exp(), cos(), sin();

expx = exp(z->dreal);
r->dreal = expx * cos(z->dimag);
r->dimag = expx * sin(z->dimag);
}

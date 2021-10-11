#ifndef lint
 static	char sccsid[] = "@(#)z_log.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

#include "complex.h"

z_log(r, z)
dcomplex *r, *z;
{
double log(), hypot(), atan2();

r->dimag = atan2(z->dimag, z->dreal);
r->dreal = log( hypot( z->dreal, z->dimag ) );
}

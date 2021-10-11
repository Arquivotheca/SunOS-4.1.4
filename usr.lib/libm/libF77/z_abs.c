#ifndef lint
 static	char sccsid[] = "@(#)z_abs.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

#include "complex.h"

double z_abs(z)
dcomplex *z;
{
double hypot();

return( hypot( z->dreal, z->dimag ) );
}

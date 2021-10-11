#ifndef lint
 static	char sccsid[] = "@(#)d_imag.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

#include "complex.h"

double d_imag(z)
dcomplex *z;
{
return(z->dimag);
}

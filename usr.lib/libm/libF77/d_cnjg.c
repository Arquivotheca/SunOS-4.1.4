#ifndef lint
 static	char sccsid[] = "@(#)d_cnjg.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

#include "complex.h"

d_cnjg(r, z)
dcomplex *r, *z;
{
r->dreal = z->dreal;
r->dimag = - z->dimag;
}

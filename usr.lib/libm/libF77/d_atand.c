#ifndef lint
 static	char sccsid[] = "@(#)d_atand.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 *  VMS Fortran compatibility function:
 *  arc tangent in degrees.
 */

#include <math.h>
static double pitod = 57.2957795130823208767981548141; /* 180/pi */

double d_atand(x)
double *x;
{
	return pitod*atan(*x);
}

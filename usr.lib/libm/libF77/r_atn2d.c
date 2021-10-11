#ifndef lint
 static	char sccsid[] = "@(#)r_atn2d.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif

/*
 *  VMS Fortran compatibility function:
 *  two-argument arc tangent in degrees.
 */

#include <math.h>
static float pitod = 57.2957795130823208767981548141; /* 180/pi */

FLOATFUNCTIONTYPE r_atn2d(y,x)
float *y,*x;
{
	FLOATFUNCTIONTYPE w;
	float z;
	w = r_atan2_(y,x);
	z = *((float *)&w) * pitod;
	RETURNFLOAT(z);
}


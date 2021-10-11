
#ifndef lint
static	char sccsid[] = "@(#)fabs.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

double fabs(x)
double x;
{	
double	one	= 1.0;

	long *px = (long *) &x;

	if ((* (int *) &one) != 0) { px[0] &= 0x7fffffff; }  /* not a i386 */
	else { px[1] &=  0x7fffffff; }			     /* is a i386  */

	return x;
}

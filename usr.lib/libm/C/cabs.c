
#ifndef lint
static	char sccsid[] = "@(#)cabs.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

double cabs(z)
struct { double x, y;} z;
{
	double hypot();
	return(hypot(z.x,z.y));
}

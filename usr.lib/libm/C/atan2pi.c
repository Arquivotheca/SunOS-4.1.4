#ifndef lint
static	char sccsid[] = "@(#)atan2pi.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* atan2pi(x)
 *
 *	atan2pi(x) = atan2(x)/pi
 *
 */

static double invpi = 0.3183098861837906715377675;

double atan2pi(y,x)
double y,x;
{
	double atan2(); return atan2(y,x)*invpi;
}

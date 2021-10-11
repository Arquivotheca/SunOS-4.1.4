#ifndef lint
static	char sccsid[] = "@(#)acospi.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* acospi(x)
 *
 *	acospi(x) = acos(x)/pi
 *
 */

static double invpi = 0.3183098861837906715377675;

double acospi(x)
double x;
{
	double acos(); return acos(x)*invpi;
}

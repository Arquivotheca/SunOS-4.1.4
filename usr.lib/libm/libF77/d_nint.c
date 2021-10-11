#ifndef lint
 static	char sccsid[] = "@(#)d_nint.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_nint(x)
double *x;
{
double anint(); return anint(*x);
}

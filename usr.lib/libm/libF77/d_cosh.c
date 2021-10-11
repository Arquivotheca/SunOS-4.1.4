#ifndef lint
 static	char sccsid[] = "@(#)d_cosh.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_cosh(x)
double *x;
{
double cosh();
return( cosh(*x) );
}

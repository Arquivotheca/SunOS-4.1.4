#ifndef lint
 static	char sccsid[] = "@(#)d_asin.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_asin(x)
double *x;
{
double asin();
return( asin(*x) );
}

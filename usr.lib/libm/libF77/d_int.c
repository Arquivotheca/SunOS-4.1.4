#ifndef lint
 static	char sccsid[] = "@(#)d_int.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_int(x)
double *x;
{
double floor();

return( (*x >= 0) ? floor(*x) : -floor(- *x) );
}

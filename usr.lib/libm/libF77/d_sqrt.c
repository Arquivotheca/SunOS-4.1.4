#ifndef lint
 static	char sccsid[] = "@(#)d_sqrt.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_sqrt(x)
double *x;
{
double sqrt();
return( sqrt(*x) );
}

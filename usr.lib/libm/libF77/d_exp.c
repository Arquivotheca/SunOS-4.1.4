#ifndef lint
 static	char sccsid[] = "@(#)d_exp.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_exp(x)
double *x;
{
double exp();
return( exp(*x) );
}

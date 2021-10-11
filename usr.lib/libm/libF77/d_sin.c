#ifndef lint
 static	char sccsid[] = "@(#)d_sin.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_sin(x)
double *x;
{
double sin();
return( sin(*x) );
}

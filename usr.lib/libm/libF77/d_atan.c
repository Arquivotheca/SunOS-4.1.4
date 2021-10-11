#ifndef lint
 static	char sccsid[] = "@(#)d_atan.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_atan(x)
double *x;
{
double atan();
return( atan(*x) );
}

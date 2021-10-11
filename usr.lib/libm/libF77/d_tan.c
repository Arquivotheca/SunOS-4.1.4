#ifndef lint
 static	char sccsid[] = "@(#)d_tan.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_tan(x)
double *x;
{
double tan();
return( tan(*x) );
}

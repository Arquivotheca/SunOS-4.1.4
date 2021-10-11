#ifndef lint
 static	char sccsid[] = "@(#)d_log.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_log(x)
double *x;
{
double log();
return( log(*x) );
}

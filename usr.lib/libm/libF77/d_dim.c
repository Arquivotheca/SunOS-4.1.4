#ifndef lint
 static	char sccsid[] = "@(#)d_dim.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_dim(a,b)
double *a, *b;
{
return( *a > *b ? *a - *b : 0);
}

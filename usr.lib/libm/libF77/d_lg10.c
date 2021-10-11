#ifndef lint
 static	char sccsid[] = "@(#)d_lg10.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

#define log10e 0.43429448190325182765

double d_lg10(x)
double *x;
{
double log();

return( log10e * log(*x) );
}

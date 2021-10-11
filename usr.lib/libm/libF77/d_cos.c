#ifndef lint
 static	char sccsid[] = "@(#)d_cos.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_cos(x)
double *x;
{
double cos();
return( cos(*x) );
}

#ifndef lint
 static	char sccsid[] = "@(#)d_tanh.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_tanh(x)
double *x;
{
double tanh();
return( tanh(*x) );
}

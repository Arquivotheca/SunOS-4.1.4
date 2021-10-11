#ifndef lint
 static	char sccsid[] = "@(#)d_sinh.c 1.1 94/10/31 SMI"; /* from UCB 1.1" */
#endif
/*
 */

double d_sinh(x)
double *x;
{
double sinh();
return( sinh(*x) );
}

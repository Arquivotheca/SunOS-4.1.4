#ifndef lint
static	char sccsid[] = "@(#)d_mod.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

double d_mod(x,y)
double *x, *y;
{
double fmod();
return fmod(*x,*y);
}

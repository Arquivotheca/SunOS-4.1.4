#ifndef lint
static	char sccsid[] = "@(#)i_dnnt.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

int i_dnnt(x)
double *x;
{
int nint();

return( nint(*x) ) ;
}

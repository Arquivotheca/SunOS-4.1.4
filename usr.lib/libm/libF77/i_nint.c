#ifndef lint
static	char sccsid[] = "@(#)i_nint.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

int i_nint(x)
float *x;
{
int ir_nint_();

return( ir_nint_(x) ) ;
}

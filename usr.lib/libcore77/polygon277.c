/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
#ifndef lint
static char sccsid[] = "@(#)polygon277.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int polygon_abs_2();
int polygon_rel_2();

int polygonabs2_(xlist, ylist, n)
float *xlist, *ylist;
int *n;
	{
	return(polygon_abs_2(xlist, ylist, *n));
	}

int polygonrel2_(xlist, ylist, n)
float *xlist, *ylist;
int *n;
	{
	return(polygon_rel_2(xlist, ylist, *n));
	}

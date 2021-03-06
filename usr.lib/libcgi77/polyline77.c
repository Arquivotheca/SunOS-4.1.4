#ifndef lint
static char	sccsid[] = "@(#)polyline77.c 1.1 94/10/31 Copyr 1985-9 Sun Micro";
#endif

/*
 * Copyright (c) 1985, 1986, 1987, 1988, 1989 by Sun Microsystems, Inc. 
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
/*
 * CGI Polyline functions
 */

/*
polyline
disjoint_polyline
polymarker
rectangle
*/

#include "cgidefs.h"		/* defines types used in this file  */
#include "cf77.h"		/* fortran wrapper macros used in this file  */

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfpolyline 						    */
/*                                                                          */
/****************************************************************************/

int     cfpolyline_ (xcoors, ycoors, n)
int	*xcoors;
int	*ycoors;
int	*n;
{
    int err;
    Ccoorlist coorlist;

    if ((err = ALLOC_COORLIST(&coorlist, xcoors, ycoors, *n)) == NO_ERROR)
    {
	err = polyline(&coorlist);
    }
    FREE_COORLIST(&coorlist);
    return(err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfdpolyline 						    */
/*                                                                          */
/****************************************************************************/

int     cfdpolyline_ (xcoors, ycoors, n)
int	*xcoors;
int	*ycoors;
int 	*n;
{
    int err;
    Ccoorlist coorlist;

    if ((err = ALLOC_COORLIST(&coorlist, xcoors, ycoors, *n)) == NO_ERROR)
    {
	err = disjoint_polyline(&coorlist);
    }
    FREE_COORLIST(&coorlist);
    return(err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfpolymarker 					    */
/*                                                                          */
/****************************************************************************/

int     cfpolymarker_ (xcoors, ycoors, n)
int	*xcoors;
int	*ycoors;
int	*n;
{
    int err;
    Ccoorlist coorlist;

    if ((err = ALLOC_COORLIST(&coorlist, xcoors, ycoors, *n)) == NO_ERROR)
    {
	err = polymarker(&coorlist);
    }
    FREE_COORLIST(&coorlist);
    return(err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfrectangle 		    			            */
/*                                                                          */
/****************************************************************************/

int     cfrectangle_ (xbot,ybot,xtop,ytop)
int 	*xbot,*ybot,*xtop,*ytop;
{
	Ccoor rbc, ltc;		/* corners defining rectangle */

	ASSIGN_COOR(&rbc, *xbot, *ybot);
	ASSIGN_COOR(&ltc, *xtop, *ytop);
        return (rectangle(&rbc,&ltc));
}

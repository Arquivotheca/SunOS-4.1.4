#ifndef lint
static  char    sccsid[] =  "@(#)cg8_vector.c	1.1 94/10/31 SMI";
#endif

/* Copyright 1989 Sun Microsystems, Inc. */

#include <pixrect/pixrect.h>
#include <pixrect/cg8var.h>
#include <pixrect/mem32_var.h>
#include <pixrect/pr_planegroups.h> /* PIX_ALL_PLANES       */

cg8_vector(pr, x0, y0, x1, y1, op, color)
Pixrect            *pr;
int                 x0,
		    y0,
		    x1,
		    y1,
		    op,
		    color;
{
    struct cg8_data     *cg8d;             /* Pointer to cg8 private data. */
    int                 current_state;     /* Current state of pip in test loop. */
    Pixrect             mem32_pr;
    struct mprp32_data  mem32_pr_data;
    int                 original_state;    /* State of pip at entry: = 1 if pip on, 0 if pip off. */
    int                 rc;                /* Return code from vector routine. */
    int                 test;              /* Index: number of times around test loop. */

    cg8d = cg8_d(pr);
    if ( cg8d->flags & CG8_STOP_PIP )
    {
	ioctl(cg8d->fd, PIPIO_G_PIP_ON_OFF_SUSPEND, &original_state);
	if ( original_state ) 
	{
	    for (test = 0; test < 44; test++)
	    {
		ioctl(cg8d->fd, PIPIO_G_PIP_ON_OFF, &current_state);
		if ( current_state == 0 ) break;
		usleep(1000);
	    }
	}
    }
 
    if ( (pr->pr_depth == 1) || (cg8d->flags & CG8_EIGHT_BIT_PRESENT) )
    {
	CG8_PR_TO_MEM(pr, mem32_pr);
	rc = mem_vector(pr, x0, y0, x1, y1, op, color);
    }
    else
    {
	CG8_PR_TO_MEM32(pr, mem32_pr, mem32_pr_data);
	rc = mem32_vector(pr, x0, y0, x1, y1, op, color);
    }

    if ( cg8d->flags & CG8_STOP_PIP ) ioctl(cg8d->fd, PIPIO_G_PIP_ON_OFF_RESUME, &original_state );
    return rc;
}

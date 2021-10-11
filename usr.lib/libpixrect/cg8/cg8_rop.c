#ifndef lint
static  char    sccsid[] = "@(#)cg8_rop.c	1.1 94/10/31 SMI";
#endif

/* Copyright 1989 Sun Microsystems, Inc. */

#include <pixrect/pixrect.h>
#include <pixrect/cg8var.h>                /* CG8*, cg8_d(pr)         */
#include <pixrect/pr_planegroups.h>
#include <pixrect/mem32_var.h>                /* mem_ops                 */

cg8_rop(dpr, dx, dy, w, h, op, spr, sx, sy)
Pixrect            *dpr,
		   *spr;
int                 dx,
		    dy,
		    w,
		    h,
		    op,
		    sx,
		    sy;
{
    struct cg8_data     *cg8d;             /* Pointer to cg8 private data. */
    int                 current_state;     /* State of pip in wait loop: = 1 if pip on, 0 if pip off. */
    Pixrect             dmempr;            /* Storage for destination memory pixrect. */
    struct mprp32_data  dst_mem32_pr_data; /* Private data area for 32 bit destination memory pr. */
    int                 original_state;    /* State of pip at entry: = 1 if pip on, 0 if pip off. */
    int                 rop_rc;            /* Return code from rop routine. */
    Pixrect             smempr;            /* Storage for source memory pixrect. */
    struct mprp32_data  src_mem32_pr_data; /* Private data area for 32 bit source memory pr. */
    int                 test;              /* Index: number of times around test loop. */

#ifdef        KERNEL
#define        ioctl        cgeightioctl

    if (dpr->pr_depth > 1 || (spr && spr->pr_depth > 1))
    {
	printf("kernel: cg8_rop error: attempt at 32 bit rop\n");
	return 0;
    }

    CG8_PR_TO_MEM(dpr, dmempr);
    CG8_PR_TO_MEM(spr, smempr);
    return pr_rop(dpr, dx, dy, w, h, op, spr, sx, sy);

#else

    /*  Let mem_rop handle the actual move. The pip must be shut down
     *  if the access is being made to the 8-bit, true color or video enable
     *  memories (as indicated by the CG8_STOP_PIP flag.) If the pip was on
     *  it is necessary to wait for it to actually turn off (which 
     *  happens at the end of the field.
     */
    cg8d = (dpr->pr_ops != &mem_ops ) ? cg8_d(dpr) : cg8_d(spr);
    if ( cg8d->flags & CG8_STOP_PIP )
    {
	ioctl(cg8d->fd, PIPIO_G_PIP_ON_OFF_SUSPEND, &original_state); 
	if ( original_state ) 
	{
	    for ( test = 0; test < 44; test++ )
	    {
		ioctl(cg8d->fd, PIPIO_G_PIP_ON_OFF, &current_state);
		if ( current_state == 0 ) break;
		usleep(500);
	    }
	}
    }
    if ( (dpr->pr_depth == 1) || (cg8d->flags & CG8_EIGHT_BIT_PRESENT) )
    {
	CG8_PR_TO_MEM(dpr, dmempr);
	CG8_PR_TO_MEM(spr, smempr);
    }
    else        /* Use emulation if necessary. */
    {
	CG8_PR_TO_MEM32(spr, smempr, src_mem32_pr_data);
	CG8_PR_TO_MEM32(dpr, dmempr, dst_mem32_pr_data);
    }

    rop_rc = pr_rop(dpr, dx, dy, w, h, op, spr, sx, sy);
    if ( cg8d->flags & CG8_STOP_PIP ) ioctl(cg8d->fd, PIPIO_G_PIP_ON_OFF_RESUME, &original_state );
    return rop_rc;
#endif                                        /* KERNEL */
}

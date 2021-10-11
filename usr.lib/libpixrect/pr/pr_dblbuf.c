#ifndef lint
static char sccsid[] = "@(#)pr_dblbuf.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986-1989 by Sun Microsystems, Inc.
 */

/* pixrect double buffering routines. */

#ifdef KERNEL
#include "cgtwo.h"
#include "gpone.h"
#else
#define	NCGTWO	1
#define	NGPONE	1
#endif

#include <sys/varargs.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>

#include <pixrect/pixrect.h>
#include <pixrect/pr_dblbuf.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>
#include <pixrect/gp1var.h>
#include <sundev/cg9reg.h>
#include <pixrect/cg9var.h>

/* handy macro for identifying cg2/gp1 pixrect */
#ifdef KERNEL
#define	IS_CG2(pr) ((pr)->pr_ops->pro_putcolormap == cg2_putcolormap)
#else
#define IS_GP(pr) ((pr)->pr_ops->pro_rop == gp1_rop)
#define	IS_CG2(pr) ((pr)->pr_ops->pro_putcolormap == cg2_putcolormap \
		    || (IS_GP(pr) && \
			gp1_d((pr))->fbtype == FBTYPE_SUN2COLOR))
#endif

/*ARGSUSED*/
pr_dbl_get(pr, attribute)
	Pixrect *pr;
	int attribute;
{
#if NCGTWO > 0
    if (IS_CG2(pr))
    {
	register struct cg2pr *prd;
	union {
	    struct dblbufreg reg;
	    short word;
	} dbl;

	prd = cg2_d(pr);
	if (!(prd->flags & CG2D_NOZOOM))
		return 0;

	GP1_PRD_SYNC(prd, return PIX_ERR);

	/* Insure that we read the correct 16 bits of the register by
	 * reading the whole register as a short.  A structure to structure
	 * copy fails through the sparc compiler. */

	dbl.word = prd->cgpr_va->misc.nozoom.dblbuf.word;

	switch (attribute) {
	case PR_DBL_DISPLAY:
		return dbl.reg.display_b ? PR_DBL_B : PR_DBL_A;

	case PR_DBL_READ:
		return dbl.reg.read_b ? PR_DBL_B : PR_DBL_A;

	case PR_DBL_WRITE: {
		static int result[4] = {
			PR_DBL_BOTH, PR_DBL_B, PR_DBL_A, PR_DBL_NONE
		};

		return result[dbl.reg.nowrite_a | dbl.reg.nowrite_b << 1];
	}

	case PR_DBL_AVAIL:
		return prd->flags & CG2D_DBLBUF ? PR_DBL_EXISTS : 0;

	case PR_DBL_AVAIL_PG:
		return prd->flags & CG2D_DBLBUF ? PR_DBL_EXISTS : 0;

	case PR_DBL_DEPTH:
		return pr->pr_depth;
	}

	return PIX_ERR;
    }
#endif NCGTWO > 0
    {
	struct fbdblinfo dblinfo;

	/* lint, bzero, and structures just don't get along sometimes */

	dblinfo.dbl_display = 0;
	dblinfo.dbl_read = 0;
	dblinfo.dbl_write = 0;
	dblinfo.dbl_devstate = 0;
	dblinfo.dbl_wid = 0;

	if (pr_ioctl(pr, FBIODBLGINFO, &dblinfo) < 0)
	    return 0;
	switch (attribute) {
	case PR_DBL_DISPLAY:
	    return dblinfo.dbl_display & FBDBL_B ? PR_DBL_B : PR_DBL_A;

	case PR_DBL_READ:
	    return dblinfo.dbl_read & FBDBL_B ? PR_DBL_B : PR_DBL_A;

	case PR_DBL_WRITE:
	    {
		static int      results[4] = 
			{PR_DBL_NONE, PR_DBL_A, PR_DBL_B, PR_DBL_BOTH};
		if (dblinfo.dbl_write & FBDBL_NONE)
		    return PR_DBL_NONE;
		return results[dblinfo.dbl_write &
			       (FBDBL_A | FBDBL_B)];
	    }

	case PR_DBL_AVAIL:
	    return dblinfo.dbl_devstate & FBDBL_AVAIL ? PR_DBL_EXISTS : 0;

	case PR_DBL_AVAIL_PG:
	    return dblinfo.dbl_devstate & FBDBL_AVAIL_PG ? PR_DBL_EXISTS : 0;

	case PR_DBL_DEPTH:
	    return dblinfo.dbl_devstate & FBDBL_AVAIL ? dblinfo.dbl_depth : 0;

	case PR_DBL_WID:
	    return dblinfo.dbl_wid;
	}
    }
    return PIX_ERR;
}

/* pr_dbl_set(pr, attr, value...) */
/*ARGSUSED*/
/*VARARGS1*/
#ifdef lint	/* lint really sucks sometimes */
pr_dbl_set(pr, va_alist)
	Pixrect *pr;
	va_dcl
#else
pr_dbl_set(va_alist)
	va_dcl
#endif
{
    va_list valist;

#ifndef lint
    Pixrect *pr;
#endif

    int attribute;

    va_start(valist);

#ifndef lint
    pr = va_arg(valist, Pixrect *);
#endif

    if (!pr) return PIX_ERR;

#if NCGTWO > 0
    if (IS_CG2(pr))
    {
	union {
	    struct dblbufreg reg;
	    short word;
	} dbl;
	register struct cg2pr *prd;
	int wait = 0;	/* wait for retrace flag */

#ifndef KERNEL
	int fd;
#endif

	prd = cg2_d(pr);
	if (!(prd->flags & CG2D_NOZOOM))
		return PIX_ERR;

	GP1_PRD_SYNC(prd, return PIX_ERR);

	/* Insure that we read the correct 16 bits of the register by
	 * reading the whole register as a short.  A structure to structure
	 * copy fails through the sparc compiler. */

	dbl.word = prd->cgpr_va->misc.nozoom.dblbuf.word;

	while (attribute = va_arg(valist, int)) {
		switch (attribute) {
		case PR_DBL_DISPLAY:
			wait = 1;
			/* fall through to other DISPLAY attribute. */
		case PR_DBL_DISPLAY_DONTBLOCK:
			switch (va_arg(valist, int)) {
			case PR_DBL_A:
				dbl.reg.display_b = 0;
				break;
			case PR_DBL_B:
				dbl.reg.display_b = 1;
				break;
			default:
				return PIX_ERR;
			}
			dbl.reg.wait = 1;
			break;

		case PR_DBL_READ:
			switch (va_arg(valist, int)) {
			case PR_DBL_A:
				dbl.reg.read_b = 0;
				break;
			case PR_DBL_B:
				dbl.reg.read_b = 1;
				break;
			default:
				return PIX_ERR;
			}
			break;

		case PR_DBL_WRITE:
			switch (va_arg(valist, int)) {
			case PR_DBL_A:
				dbl.reg.nowrite_b = 1;
				dbl.reg.nowrite_a = 0;
				break;
			case PR_DBL_B:
				dbl.reg.nowrite_b = 0;
				dbl.reg.nowrite_a = 1;
				break;
			case PR_DBL_BOTH:
				dbl.reg.nowrite_b = 0;
				dbl.reg.nowrite_a = 0;
				break;
			default:
				return PIX_ERR;
			}
			break;

		default:
			return PIX_ERR;
		}
	}
	va_end(valist);

	/* Insure that we write the correct 16 bits of the register by
	 * writing the whole register as a short.  A structure to structure
	 * copy fails through the sparc compiler. */

	prd->cgpr_va->misc.nozoom.dblbuf.word = dbl.word;

	if (wait)
#ifdef KERNEL
		cgtwo_wait(prd->ioctl_fd);
#else  KERNEL
		if (ioctl(prd->ioctl_fd, FBIOVERTICAL, 0) < 0) 
			return PIX_ERR;
#endif KERNEL

	return 0;
    }
#endif NCGTWO > 0

    {
	struct fbdblinfo dbl;

	bzero((char *) &dbl, sizeof (dbl));
	while (attribute = va_arg(valist, int)) {
	    switch (attribute) {
	    case PR_DBL_DISPLAY_DONTBLOCK:
		dbl.dbl_devstate = FBDBL_DONT_BLOCK;
		/* fall through to other DISPLAY attribute. */
	    case PR_DBL_DISPLAY:
		switch (va_arg(valist, int)) {
		case PR_DBL_A:
		    dbl.dbl_display = FBDBL_A;
		    break;
		case PR_DBL_B:
		    dbl.dbl_display = FBDBL_B;
		    break;
		default:
		    return PIX_ERR;
		}
		break;

	    case PR_DBL_READ:
		switch (va_arg(valist, int)) {
		case PR_DBL_A:
		    dbl.dbl_read = FBDBL_A;
		    break;
		case PR_DBL_B:
		    dbl.dbl_read = FBDBL_B;
		    break;
		default:
		    return PIX_ERR;
		}
		break;

	    case PR_DBL_WRITE:
		switch (va_arg(valist, int)) {
		case PR_DBL_A:
		    dbl.dbl_write = FBDBL_A;
		    break;
		case PR_DBL_B:
		    dbl.dbl_write = FBDBL_B;
		    break;
		case PR_DBL_BOTH:
		    dbl.dbl_write = FBDBL_A | FBDBL_B;
		    break;
		case PR_DBL_NONE:
		    dbl.dbl_write = FBDBL_NONE;
		    break;
		default:
		    return PIX_ERR;
		}
		break;

	    default:
		return PIX_ERR;
	    }
	}
	va_end(valist);

	if (pr_ioctl(pr, FBIODBLSINFO, &dbl) < 0)
	    return PIX_ERR;
	return 0;
    }

}

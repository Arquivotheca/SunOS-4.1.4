#ifndef lint
static char sccsid[] = "@(#)cg2_colormap.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1983, 1987 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>


cg2_putcolormap(pr, index, count, red, green, blue)
	Pixrect *pr;
	int index, count;
	u_char *red, *green, *blue;
{
	register struct cg2fb *fb;

	if ((unsigned) index >= 256 || 
		index + (unsigned) count > 256)
		return PIX_ERR;

	{
		register struct cg2pr *prd;

		prd = cg2_d(pr);
		GP1_PRD_SYNC(prd, /* nothing */);
		fb = prd->cgpr_va;
	}

	/* make sure shadow colormap is accessible */
	fb->status.reg.update_cmap = 0;

	{
		register u_char *in;
		register u_short *out;
		register int offset;

		offset = (256 - count) << 1;

		out = &fb->redmap[index];
		in = red;

		PR_LOOP(count, *out++ = *in++);

		PTR_INCR(u_short *, out, offset);
		in = green;

		PR_LOOP(count, *out++ = *in++);

		PTR_INCR(u_short *, out, offset);
		in = blue;

		PR_LOOP(count, *out++ = *in++);
	}

	/* copy shadow colormap to active colormap */
	fb->status.reg.update_cmap = 1;

	return 0;
}

cg2_putattributes(pr, planes)
	Pixrect *pr;
	int *planes;
{

	if (planes)
		cg2_d(pr)->cgpr_planes = *planes & 0xff;

	return 0;
}

#ifndef	KERNEL

cg2_getcolormap(pr, index, count, red, green, blue)
	Pixrect *pr;
	int index, count;
	u_char *red, *green, *blue;
{
	register struct cg2fb *fb;

	if ((unsigned) index >= 256 || 
		index + (unsigned) count > 256)
		return PIX_ERR;

	{
		register struct cg2pr *prd;

		prd = cg2_d(pr);
		GP1_PRD_SYNC(prd, /* nothing */);
		fb = prd->cgpr_va;
	}

	/* make sure shadow colormap is accessible */
	fb->status.reg.update_cmap = 0;

	{
		register u_short *in;
		register u_char *out;
		register int offset;

		offset = (256 - count) << 1;

		in = &fb->redmap[index];
		out = red;

		PR_LOOP(count, *out++ = *in++);

		PTR_INCR(u_short *, in, offset);
		out = green;

		PR_LOOP(count, *out++ = *in++);

		PTR_INCR(u_short *, in, offset);
		out = blue;

		PR_LOOP(count, *out++ = *in++);
	}

	/* we might have aborted a colormap update */
	fb->status.reg.update_cmap = 1;

	return 0;
}

cg2_getattributes(pr, planes)
	Pixrect *pr;
	int *planes;
{

	if (planes)
		*planes = cg2_d(pr)->cgpr_planes;

	return 0;
}

#endif !KERNEL

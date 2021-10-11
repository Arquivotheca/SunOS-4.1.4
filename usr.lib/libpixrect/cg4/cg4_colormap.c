#ifndef lint
static char sccsid[] = "@(#)cg4_colormap.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

/*
 * cg4 colormap and attribute functions
 */ 

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/pr_util.h>
#include <pixrect/cg4var.h>

#ifndef	KERNEL
cg4_putcolormap(pr, index, count, red, green, blue)
	Pixrect *pr;
	int index, count;
	unsigned char *red, *green, *blue;
{
	register struct cg4_data *cgd = cg4_d(pr);

	if (pr->pr_depth == 8 ||
		cgd->flags & CG4_OVERLAY_CMAP &&
		cgd->fb[cgd->active].group == PIXPG_OVERLAY) {

		struct fbcmap cmap;

		cmap.index = index  & PIX_ALL_PLANES;;
		if (pr->pr_depth != 8)
			cmap.index |= PIX_GROUP(PIXPG_OVERLAY);
		cmap.count = count;
		cmap.red   = red;
		cmap.green = green;
		cmap.blue  = blue;
		return ioctl(cgd->fd, FBIOPUTCMAP, &cmap);
	}
	else {
		int retval;

		retval = mem_putcolormap(pr, index, count, red, green, blue);
		cgd->fb[cgd->active].mprp.mpr.md_flags =
			cgd->mprp.mpr.md_flags;

		return retval;
	}
}

cg4_getcolormap(pr, index, count, red, green, blue)
	register Pixrect *pr;
	int index, count;
	unsigned char *red, *green, *blue;
{
	register struct cg4_data *cgd = cg4_d(pr);

	if (pr->pr_depth == 8 ||
		cgd->flags & CG4_OVERLAY_CMAP &&
		cgd->fb[cgd->active].group == PIXPG_OVERLAY) {

		struct fbcmap cmap;

		cmap.index = index & PIX_ALL_PLANES;
		if (pr->pr_depth != 8)
			cmap.index |= PIX_GROUP(PIXPG_OVERLAY);
		cmap.count = count;
		cmap.red   = red;
		cmap.green = green;
		cmap.blue  = blue;
		return ioctl(cgd->fd, FBIOGETCMAP, &cmap);
	}
	else
		return mem_getcolormap(pr, index, count, red, green, blue);
}
#endif !KERNEL


cg4_putattributes(pr, planesp)
	Pixrect *pr;
	int *planesp;
{
	register int planes;
	register struct cg4_data *cgd;
	register int group;

	if (planesp == 0)
		return 0;

	planes = *planesp;
        /*
         * for those call putattributes with ~0.  This can be better
         * handled if the check for PIX_DONT_SET_PLANE is inverted.   
         */
        if (planes == ~0)
                planes &= ~PIX_DONT_SET_PLANES; 
	cgd = cg4_d(pr);

	/*
	 * If user is trying to set the group, look for the frame
	 * buffer which implements the desired group and make it
	 * the active frame buffer.
	 */
	group = PIX_ATTRGROUP(planes);
	if (group == PIX_GROUP_MASK)
		planes &= PIX_ALL_PLANES;
	if (group != PIXPG_CURRENT || group == PIXPG_INVALID) {
		register int active;
		register struct cg4fb *fbp;
		int	cmsize;

		for (active = 0, fbp = cgd->fb; 
			active < CG4_NFBS; active++, fbp++)
			if (group == fbp->group) {
				cgd->planes = PIX_GROUP(group);
				cgd->active = active;

				if (!(planes & PIX_DONT_SET_PLANES))
					fbp->mprp.planes = 
						planes & PIX_ALL_PLANES;

				cgd->mprp = fbp->mprp;
				cgd->planes |= fbp->mprp.planes;
				pr->pr_depth = fbp->depth;
				cmsize = (pr->pr_depth==8) ? 256 : 2;
				(void) cg4_ioctl(pr, FBIOSCMSIZE, (caddr_t)&cmsize);
				return 0;
			}
	}

	/* set planes for current group */
	if (!(planes & PIX_DONT_SET_PLANES)) {
		planes &= PIX_ALL_PLANES;
		cgd->planes = (cgd->planes & ~PIX_ALL_PLANES) | planes;
		cgd->mprp.planes = planes;
		cgd->fb[cgd->active].mprp.planes = planes;
	}

	return 0;
}

#ifndef	KERNEL
cg4_getattributes(pr, planesp)
	Pixrect *pr;
	int *planesp;
{
	if (planesp != 0)
		*planesp = cg4_d(pr)->planes & PIX_ALL_PLANES;

	return 0;
}
#endif !KERNEL

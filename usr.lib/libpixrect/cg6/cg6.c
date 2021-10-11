#ifndef lint
static char sccsid[] = "@(#)cg6.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1988-1989, Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_make.h>
#include <pixrect/cg6var.h>
#include <sundev/cg6reg.h>

/* device data list head */
static struct pr_devdata *cg6devdata;

Pixrect *
cg6_make(fd, size, depth)
	int fd;
	struct pr_size size;
	int depth;
{
	register Pixrect *pr;
	struct pr_devdata *dd;
	int linebytes = mpr_linebytes(size.x, CG6_DEPTH);
	register struct cg6pr *cg6pr;

	/* allocate pixrect and map frame buffer */
	if (!(pr = pr_makefromfd(fd, size, depth, &cg6devdata, &dd,
		MMAPSIZE((size.y * linebytes)),
		sizeof (struct cg6pr), CG6_VBASE)))
		return pr;

	pr->pr_ops = &cg6_ops;

	cg6pr = cg6_d(pr);
	cg6pr->mprp.mpr.md_image = CG6VA_TO_DFB(dd->va);
	cg6pr->mprp.mpr.md_linebytes = linebytes;
	cg6pr->mprp.mpr.md_offset.x = 0;
	cg6pr->mprp.mpr.md_offset.y = 0;
	cg6pr->mprp.mpr.md_primary = 1;
	cg6pr->mprp.mpr.md_flags = MP_DISPLAY | MP_PLANEMASK;
	cg6pr->mprp.planes = 255;
	cg6pr->fd = dd->fd;
	cg6pr->cg6_size = size;
	cg6pr->cg6_fbc = CG6VA_TO_FBC(dd->va);
	cg6pr->cg6_tec = CG6VA_TO_TEC(dd->va);

#if FBC_REV0
	/* use alternate vector function for FBC0 */
	if (fbc_rev0(cg6pr->cg6_fbc))
		pr->pr_ops->pro_vector = cg6_vector0;
#endif FBC_REV0

	cg6_reset_uregs(cg6pr->cg6_fbc, cg6pr->cg6_tec,
		size.x, size.y);
	
	return pr;
}

cg6_destroy(pr)
	Pixrect *pr;
{
	if (pr) {
		register struct cg6pr *cgd;

		if ((cgd = cg6_d(pr)) != 0) {
			if (cgd->mprp.mpr.md_primary)
				pr_unmakefromfd(cgd->fd, &cg6devdata);
			free(cgd);
		}
		free(pr);
	}
	return 0;
}

/* reset FBC registers */
cg6_reset_uregs(fbc, tec, w, h)
	struct fbc *fbc;
	struct tec *tec;
	int w, h;
{
	union {
		struct l_fbc_misc misc;
		struct l_fbc_rasterop rop;
		u_int	val;
	} reg;

	/* Wait at the beginning here, before reading/setting any of the 
	   registers.  An application could get a bus error here if the 
	   waitidle was not at the beginning. */
	cg6_waitidle(fbc);

	/* raster offset */
	fbc->l_fbc_rasteroffx = 0;
	fbc->l_fbc_rasteroffy = 0;

	/* background color */
	fbc->l_fbc_bcolor = 0;

	/* read and set the misc reg */
	reg.misc = fbc->l_fbc_misc;
	reg.misc.l_fbc_misc_blit = L_FBC_MISC_BLIT_NOSRC;
	reg.misc.l_fbc_misc_data = L_FBC_MISC_DATA_COLOR1;
	reg.misc.l_fbc_misc_draw = L_FBC_MISC_DRAW_RENDER;
	fbc->l_fbc_misc = reg.misc;

	/* read and set the rasterop reg */
	reg.rop = fbc->l_fbc_rasterop;
	reg.rop.l_fbc_rasterop_pixel = L_FBC_RASTEROP_PIXEL_MASK;
	reg.rop.l_fbc_rasterop_plane = L_FBC_RASTEROP_PLANE_MASK;
	reg.rop.l_fbc_rasterop_polyg = L_FBC_RASTEROP_POLYG_NONOVERLAP;
	reg.rop.l_fbc_rasterop_attr = L_FBC_RASTEROP_ATTR_SUPP;
	reg.rop.l_fbc_rasterop_rast = L_FBC_RASTEROP_RAST_BOOL;
	reg.rop.l_fbc_rasterop_rop00 = 0x6;	/* default ROP is XOR */
	reg.rop.l_fbc_rasterop_rop01 = 0x6;
	reg.rop.l_fbc_rasterop_rop10 = 0x6;
	reg.rop.l_fbc_rasterop_rop11 = 0x6;

	fbc->l_fbc_rasterop = reg.rop;

	/* (one-time) clear Z (and others too) bits in FBC CLIP CHECK */
	
	fbc->l_fbc_clipcheck = 0;

	fbc->l_fbc_pixelmask = ~0;
	fbc->l_fbc_planemask = ~0;

        bzero(&tec->l_tec_clip, sizeof (struct l_tec_clip)); 
	cg6_clip(fbc, 0, 0, w - 1, h - 1);
}

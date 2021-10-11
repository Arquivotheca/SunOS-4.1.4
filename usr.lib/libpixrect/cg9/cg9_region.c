#ifndef lint
static char sccsid[] = "@(#)cg9_region.c	1.1 94/10/31 SMI";
#endif

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <cg9/cg9var.h>

Pixrect  *  cg9_region (pr, x, y, w, h)	       
	Pixrect	*pr;
	int	x,
		y,
		w,
		h;
{
	struct pr_subregion	src_reg;
	static struct pr_prpos	nullpos = {  0, {0,0}  };
	register struct cg9fb	*fbp;
	register Pixrect	*newpr;
	register struct	cg9_data
				*cg9d;

	if ((newpr = alloctype (Pixrect)) == 0)
		return 0;

	if ((cg9d = alloctype (struct cg9_data)) == 0)  {
		free (newpr);
		return 0;
	}

	src_reg.pr = pr;
	src_reg.pos.x = x;
	src_reg.pos.y = y;
	src_reg.size.x = w;
	src_reg.size.y = h;

	pr_clip (&src_reg, &nullpos);

	*newpr = *pr;				/* Copy old pixrect  */
	newpr->pr_size = src_reg.size;		/* Set new pixrect size  */
	newpr->pr_data = (caddr_t) cg9d;	/* Set pixrect data ptr.  */
	*cg9d = *cg9_d (pr);			/* Copy old pixrect data  */
	cg9d->flags &= ~(CG9_PRIMARY);		/* New pr. is secondary pr.  */

	cg9d->mprp.mpr.md_offset.x += src_reg.pos.x;
	cg9d->mprp.mpr.md_offset.y += src_reg.pos.y;

	for (fbp = cg9d->fb;  fbp < &cg9d->fb[CG9_NFBS];  fbp++)
		fbp->mprp.mpr.md_offset = cg9d->mprp.mpr.md_offset;

	return  newpr;

}


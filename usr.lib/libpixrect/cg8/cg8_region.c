#ifndef lint
static char sccsid[] = "@(#)cg8_region.c	1.1 94/10/31 SMI";
#endif

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <cg8/cg8var.h>

Pixrect  *  cg8_region (pr, x, y, w, h)	       
	Pixrect	*pr;
	int	x,
		y,
		w,
		h;
{
	struct pr_subregion	src_reg;
	static struct pr_prpos	nullpos = {  0, {0,0}  };
	register struct cg4fb	*fbp;
	register Pixrect	*newpr;
	register struct	cg8_data
				*cg8d;

	if ((newpr = alloctype (Pixrect)) == 0)
		return 0;

	if ((cg8d = alloctype (struct cg8_data)) == 0)  {
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
	newpr->pr_data = (caddr_t) cg8d;	/* Set pixrect data ptr.  */
	*cg8d = *cg8_d (pr);			/* Copy old pixrect data  */
	cg8d->flags &= ~(CG8_PRIMARY);		/* New pr. is secondary pr.  */

	cg8d->mprp.mpr.md_offset.x += src_reg.pos.x;
	cg8d->mprp.mpr.md_offset.y += src_reg.pos.y;

	for (fbp = cg8d->fb;  fbp < &cg8d->fb[cg8d->num_fbs];  fbp++)
		fbp->mprp.mpr.md_offset = cg8d->mprp.mpr.md_offset;

	return  newpr;

}


#ifndef lint
static char sccsid[] = "@(#)cg12_region.c	1.1 94/10/31 SMI";
#endif

#include <cg12/cg12_var.h>
#include <pixrect/pr_util.h>

Pixrect            *
cg12_region(pr, x, y, w, h)
Pixrect            *pr;
int                 x, y, w, h;
{
    struct pr_subregion src_reg;
    static struct pr_prpos nullpos = {0, {0, 0}};
    struct cg12fb      *fbp;
    Pixrect            *newpr;
    struct cg12_data   *cg12d;

    if ((newpr = alloctype(Pixrect)) == 0)
	return 0;

    if ((cg12d = alloctype(struct cg12_data)) == 0)
    {
	free(newpr);
	return 0;
    }

    src_reg.pr = pr;
    src_reg.pos.x = x;
    src_reg.pos.y = y;
    src_reg.size.x = w;
    src_reg.size.y = h;

    pr_clip(&src_reg, &nullpos);

    *newpr = *pr;			/* Copy old pixrect  */
    newpr->pr_size = src_reg.size;	/* Set new pixrect size  */
    newpr->pr_data = (caddr_t) cg12d;	/* Set pixrect data ptr.  */
    *cg12d = *cg12_d(pr);		/* Copy old pixrect data  */
    cg12d->cg12_flags &= ~(CG12_PRIMARY);	/* New pr. is secondary pr.  */

    cg12d->mprp.mpr.md_offset.x += src_reg.pos.x;
    cg12d->mprp.mpr.md_offset.y += src_reg.pos.y;

    cg12d->cgpr_offset = cg12d->mprp.mpr.md_offset;

    for (fbp = cg12d->fb; fbp < &cg12d->fb[CG12_NFBS]; fbp++)
	fbp->mprp.mpr.md_offset = cg12d->mprp.mpr.md_offset;

    return newpr;
}

#ifndef lint
static char sccsid[] = "@(#)gp1_region.c	1.1  94/10/31  SMI";
#endif

/*
 * Copyright 1989 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>
#include <pixrect/cg9var.h>
#include <pixrect/gp1cmds.h>
#include <pixrect/gp1reg.h>
#include <pixrect/gp1var.h>

extern char *malloc();
static Pixrect cgpixrect;

Pixrect *
gp1_region(pr, x, y, w, h)
	Pixrect *pr;
	int x, y, w, h;
{
    struct gp1pr *srcgppr = gp1_d(pr);
    Pixrect *newpr;
    Pixrect *(*gp1_makeregion)();
    Pixrect *gp1_cg2_region(), *gp1_cg9_region();
    
    if (srcgppr->fbtype == FBTYPE_SUN2COLOR)
	gp1_makeregion = gp1_cg2_region;
    else if (srcgppr->fbtype == FBTYPE_SUNROP_COLOR)
	gp1_makeregion = gp1_cg9_region;
    newpr = (*gp1_makeregion)(pr, x, y, w, h);    
    return newpr;
}


static Pixrect *
gp1_cg2_region(pr, x, y, w, h)
	Pixrect *pr;
	int x, y, w, h;
{
    extern Pixrect *cg2_region();
    Pixrect *cg2_pixrect, tmppr;
    struct gp1pr *gppr;

    tmppr = *pr;
    cg2_d(&tmppr) = &gp1_d(pr)->cgpr.cg2pr;
    if ((gppr = (struct gp1pr *) malloc(sizeof(struct gp1pr))) == NULL ||
	(cg2_pixrect = cg2_region(&tmppr, x, y, w, h)) == NULL)
	return NULL;


    *gppr = *gp1_d(pr);
    gppr->cgpr.cg2pr = *cg2_d(cg2_pixrect);
    GP1_COPY_TOTAL(cg2_d(cg2_pixrect), gppr);
    gppr->fbtype = gp1_d(pr)->fbtype;
    (void) free(cg2_pixrect->pr_data);
    cg2_pixrect->pr_data = (char *) gppr;
    return cg2_pixrect;
}

static Pixrect *
gp1_cg9_region(pr, x, y, w, h)
	Pixrect *pr;
	int x, y, w, h;
{
    extern Pixrect *cg9_region();
    Pixrect *cg9_pixrect, *tmppr = &cgpixrect;
    struct gp1pr *gppr;

    *tmppr = *pr;
    cg9_d(tmppr) = &gp1_d(pr)->cgpr.cg9pr;
    if ((gppr = (struct gp1pr *) malloc(sizeof(struct gp1pr))) == NULL ||
	(cg9_pixrect = cg9_region(tmppr, x, y, w, h)) == NULL)
	return NULL;

    *gppr = *gp1_d(pr);
    gppr->cgpr.cg9pr = *cg9_d(cg9_pixrect);
    gppr->cgpr_offset = gppr->cgpr.cg9pr.mprp.mpr.md_offset;
    gppr->cgpr_fd = -1;			/* secondary pixrect */
    (void) free(cg9_pixrect->pr_data);
    cg9_pixrect->pr_data = (char *) gppr;
    return cg9_pixrect;
}

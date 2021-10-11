#ifndef lint
static	char sccsid[] = "@(#)cg1_getput.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Sun1 Color get and put single pixel
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/cg1reg.h>
#include <pixrect/cg1var.h>

cg1_get(src)
	struct pr_prpos src;
{
	register u_char *by;
	struct cg1fb *fb = cg1_fbfrompr(src.pr);
	u_char pumpprimer;
	struct cg1pr *sbd;

 	if (src.pos.x < 0 || src.pos.x >= src.pr->pr_size.x || 
	    src.pos.y < 0 || src.pos.y >= src.pr->pr_size.y)
		return (-1);
	sbd = cg1_d(src.pr);
	cg1_touch(*cg1_xsrc(fb, src.pos.x + sbd->cgpr_offset.x));
	by = cg1_ysrc(fb, src.pos.y + sbd->cgpr_offset.y);
	pumpprimer = *by;
	return ( (int)((sbd->cgpr_planes) & *by) );
}

cg1_put(dst, val)
	struct pr_prpos dst;
	int val;
{
	struct cg1fb *fb = cg1_fbfrompr(dst.pr);
	struct cg1pr *dbd;

 	if (dst.pos.x < 0 || dst.pos.x >= dst.pr->pr_size.x || 
	    dst.pos.y < 0 || dst.pos.y >= dst.pr->pr_size.y)
		return (-1);
	dbd = cg1_d(dst.pr);
	cg1_setfunction(fb, (CG_NOTMASK & CG_DEST)|(CG_MASK & CG_SRC));
	cg1_setmask(fb, dbd->cgpr_planes);
	cg1_touch(*cg1_xdst(fb, dst.pos.x + dbd->cgpr_offset.x));
	*cg1_ydst(fb, dst.pos.y + dbd->cgpr_offset.y) = (u_char)val;
	return (0);
}

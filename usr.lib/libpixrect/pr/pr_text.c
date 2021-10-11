#ifndef lint
static char sccsid[] = "@(#)pr_text.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

/*
 * Unstructured pixrect text functions
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>

pr_text(pr, x, y, op, pf, str)
	Pixrect *pr;
	int x, y, op;
	Pixfont *pf;
	char *str;
{
	struct pr_prpos prpos;

	prpos.pr = pr;
	prpos.pos.x = x;
	prpos.pos.y = y;

	return pf_text(prpos, op, pf, str);
}

pr_ttext(pr, x, y, op, pf, str)
	Pixrect *pr;
	int x, y, op;
	Pixfont *pf;
	char *str;
{
	struct pr_prpos prpos;

	prpos.pr = pr;
	prpos.pos.x = x;
	prpos.pos.y = y;

	return pf_ttext(prpos, op, pf, str);
}


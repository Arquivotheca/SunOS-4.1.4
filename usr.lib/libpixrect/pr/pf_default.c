#ifndef lint
static	char sccsid[] = "@(#)pf_default.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Default font (memory resident).
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>

char	*getenv();

PIXFONT *
pf_default()
{
	register PIXFONT *pf;
	char	*fontname;
	extern	PIXFONT *pf_bltindef();

	fontname = getenv("DEFAULT_FONT");
	if (fontname != 0) {
		pf = pf_open(fontname);
		if (pf != 0)
			return (pf);
	}
	pf = pf_bltindef();
	return (pf);
}

#ifndef lint
static	char sccsid[] = "@(#)demo_time.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/bw1reg.h>
#include <pixrect/bw1var.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>

main()
{
	register struct pixrect *src = mem_create(100, 100, 1);
	register struct pixrect *screen = bw1_create(1024, 800, 1);
	register int i;

	for (i = 0; i < 25000; i++)
		pr_rop(screen, 0, 0, 10, 20, PIX_DONTCLIP|PIX_SRC,
		    src, 0, 0);
}

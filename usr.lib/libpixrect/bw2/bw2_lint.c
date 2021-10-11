/* 	bw2_lint.c 1.1 of 10/31/94
*	@(#)bw2_lint.c	1.1
*/
#ifndef LINT_INCLUDE
#define LINT_INCLUDE
#include <pixrect/pixrect_hs.h>
#include <stdio.h>
#endif

/* bw2 specific functions */

Pixrect *
bw2_make(fd, size, depth)
	int fd, depth;
	struct pr_size size;
	{ return (Pixrect *) 0; }

bw2_destroy(pr) 
	Pixrect *pr;
	{ return 0; }


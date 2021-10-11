/*	@(#)auto_alloc.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __auto_alloc__
#define __auto_alloc__

extern void	do_auto_alloc();
extern void	visit_operands(/*tp, fn*/);
extern int	nleaves_deleted;
extern int	nsegs_deleted;

#endif

/*	@(#)reg_alloc.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __reg_alloc__
#define __reg_alloc__

extern int	reg_share_wordsize;
extern int	n_dreg, n_freg, max_freg;

#if TARGET == MC68000 || TARGET == I386
extern int	n_areg;
#endif

extern void	do_register_alloc();

#endif

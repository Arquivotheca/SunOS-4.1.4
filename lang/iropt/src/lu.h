/*	@(#)lu.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __lu__
#define __lu__

extern void	set_unroll_option(/*count*/);
extern BOOLEAN	loop_unroll_enabled();
extern void	do_loop_unrolling(/*loop*/);
extern LIST	*connect_block(/*list, block*/);

#endif

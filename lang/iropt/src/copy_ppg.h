
/*	@(#)copy_ppg.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __copy_ppg__
#define __copy_ppg__

extern SET_PTR	copy_kill, copy_gen, copy_in, copy_out;
extern int	copy_set_wordsize;

extern void	do_local_ppg();
extern void	do_global_ppg();

#endif

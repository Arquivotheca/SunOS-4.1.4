/*	@(#)live.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __live__
#define __live__

void	compute_live_after_blocks(/*reachdefs, live_after_blocks*/);
void	compute_live_after_site(/*live_after_blocks, live_after_site, site*/);
void	show_live_after_block(/*bp*/);
void	show_live_after_site(/*site*/);
double	density(/*set_ptr*/);

#endif

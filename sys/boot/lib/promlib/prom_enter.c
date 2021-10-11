/*	@(#)prom_enter.c 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

void
prom_enter_mon()
{
	(void)montrap(OBP_ENTER_MON);
}

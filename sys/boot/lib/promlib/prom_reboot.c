/*	@(#)prom_reboot.c 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

void
prom_reboot(s)
	char *s;
{
	OBP_BOOT(s);
	/* NOTREACHED */
}

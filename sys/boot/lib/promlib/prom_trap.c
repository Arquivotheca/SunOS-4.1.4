/*	@(#)prom_trap.c 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

/*
 * Not for the kernel.  Dummy replacement for kernel montrap.
 */
int
montrap(funcptr)
	int (*funcptr)();
{
	return (*funcptr)();
}


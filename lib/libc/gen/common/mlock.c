/* @(#)mlock.c 1.1 94/10/31 SMI */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/mman.h>

/*
 * Function to lock address range in memory.
 */

/*LINTLIBRARY*/
mlock(addr, len)
	caddr_t addr;
	u_int len;
{

	return (mctl(addr, len, MC_LOCK, 0));
}

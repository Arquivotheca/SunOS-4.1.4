/* @(#)msync.c 1.1 94/10/31 SMI */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/mman.h>

/*
 * Function to synchronize address range with backing store.
 */

/*LINTLIBRARY*/
msync(addr, len, flags)
	caddr_t addr;
	u_int len;
	int flags;
{

	return (mctl(addr, len, MC_SYNC, flags));
}

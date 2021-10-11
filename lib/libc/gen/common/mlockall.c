/* @(#)mlockall.c 1.1 94/10/31 SMI */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/mman.h>

/*
 * Function to lock address space in memory.
 */

/*LINTLIBRARY*/
mlockall(flags)
	int flags;
{

	return (mctl(0, 0, MC_LOCKAS, flags));
}

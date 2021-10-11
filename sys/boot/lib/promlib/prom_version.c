/*	@(#)prom_version.c 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

int
prom_getversion()
{
	return (obp_romvec_version);
}

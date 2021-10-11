/*	@(#)prom_bootargs.c 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

char *
prom_bootargs()
{
	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		return ((char *)0);

	default:
		return (OBP_V2_BOOTARGS);
	}
}

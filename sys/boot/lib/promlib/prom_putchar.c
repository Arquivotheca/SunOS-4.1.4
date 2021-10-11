/*	@(#)prom_putchar.c 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

void
prom_putchar(c)
	char c;
{
	while (prom_mayput(c) == -1)
		;
}

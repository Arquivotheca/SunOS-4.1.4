/*
 * @(#)prom_putchar.c 1.1 94/10/31 Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include "promcommon.h"

#ifdef OPENPROMS
void
prom_putchar(c)
	char c;
{
	while (prom_mayput(c) == -1)
		;
}
#endif OPENPROM

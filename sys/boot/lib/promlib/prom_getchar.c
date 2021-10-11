/*	@(#)prom_getchar.c 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

u_char
prom_getchar()
{
	int c;
	while ((c = prom_mayget()) == -1)
		;
	return ((u_char)c);
}

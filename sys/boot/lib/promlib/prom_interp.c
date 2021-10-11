/*	@(#)prom_interp.c 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

void
prom_interpret(forthstring)
	char	*forthstring;
{
	OBP_INTERPRET(forthstring);
}

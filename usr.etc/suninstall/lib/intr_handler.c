/*      @(#)intr_handler.c 1.1 94/10/31 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "signal.h"

void
intr_handler()
{
	int id;

	id = getgid();
	(void) killpg(id,SIGINT);
	stop(1);
}

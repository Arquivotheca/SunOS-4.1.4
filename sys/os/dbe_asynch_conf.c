/*	@(#)dbe_asynch_conf.c 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/dbe_asynch.h>

#include "id.h"
#if NID == 0
/*ARGSUSED*/	
int (*id_get_strategy(chrmajor))()
{
	return (int (*)()) 0;
}
#endif NID


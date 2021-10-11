#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)nint_r_death.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_r_death.c - Implement the notify_remove_destroy_func interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/nint.h>

extern Notify_error	
notify_remove_destroy_func(nclient, func)
	Notify_client nclient;
	Notify_func func;
{
	return(nint_remove_func(nclient, func, NTFY_DESTROY, NTFY_DATA_NULL,
	    NTFY_IGNORE_DATA));
}


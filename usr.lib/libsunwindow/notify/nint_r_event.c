#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)nint_r_event.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_r_event.c - Implement the notify_remove_event_func interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/nint.h>

extern Notify_error
notify_remove_event_func(nclient, func, when)
	Notify_client nclient;
	Notify_func func;
	Notify_event_type when;
{
	NTFY_TYPE type;

	/* Check arguments */
	if (ndet_check_when(when, &type))
		return(notify_errno);
	return(nint_remove_func(nclient, func, type, NTFY_DATA_NULL,
	    NTFY_IGNORE_DATA));
}


#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndis_g_sched.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif
#endif
 
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndis_g_sched.c - Implement the notify_get_sheduler_func.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndis.h>

extern Notify_func
notify_get_scheduler_func()
{
	return(ndis_scheduler);
}


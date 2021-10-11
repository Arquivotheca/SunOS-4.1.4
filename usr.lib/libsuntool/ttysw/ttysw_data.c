#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)ttysw_data.c 1.1 94/10/31";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * ttysw_data.c: fix for shared libraries in SunOS4.0.  Code was 
 *		 isolated from ttyansi.c and csr_change.c
 */

#include <sys/time.h>
#include <suntool/ttyansi.h>

int tty_cursor = BLOCKCURSOR | LIGHTCURSOR;

struct timeval ttysw_bell_tv = {0, 100000}; /* 1/10 second */

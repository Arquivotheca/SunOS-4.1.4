/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)erasechar.c 1.1 94/10/31 SMI"; /* from S5R3.1 1.1.1.4 */
#endif

/*
 * Routines to deal with setting and resetting modes in the tty driver.
 * See also setupterm.c in the termlib part.
 */
#include "curses_inc.h"

char
erasechar()
{
#ifdef SYSV
    return (SHELLTTY.c_cc[VERASE]);
#else
    return (SHELLTTY.sg_erase);
#endif
}

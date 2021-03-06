/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)V3.m_move.c 1.1 94/10/31 SMI"; /* from S5R3.1 1.1 */
#endif

#include	"curses_inc.h"
extern	int	_outchar();

#ifdef	_VR3_COMPAT_CODE
m_move(y, x)
int	y, x;
{
    return (wmove(stdscr, y, x));
}
#endif	/* _VR3_COMPAT_CODE */

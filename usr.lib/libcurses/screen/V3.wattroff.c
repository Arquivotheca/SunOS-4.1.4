/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)V3.wattroff.c 1.1 94/10/31 SMI"; /* from S5R3.1 1.1 */
#endif

#include	"curses_inc.h"
extern	int	_outchar();

#ifdef	_VR3_COMPAT_CODE
#undef	wattroff
wattroff(win, attrs)
WINDOW		*win;
_ochtype	attrs;
{
    win->_attrs &= (~_FROM_OCHTYPE(attrs) | win->_bkgd) & A_ATTRIBUTES;
    return (OK);
}
#endif	/* _VR3_COMPAT_CODE */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)_mvwinchnst.c 1.1 94/10/31 SMI"; /* from S5R3.1 1.1 */
#endif

#define	NOMACROS

#include	"curses_inc.h"

mvwinchnstr(win, y, x, s, n)
WINDOW  *win;
int	y, x, n;
chtype	*s;
{
    return (wmove(win, y, x)==ERR?ERR:winchnstr(win, s, n));
}

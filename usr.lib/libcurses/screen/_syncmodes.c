#ifndef lint
static	char sccsid[] = "@(#)_syncmodes.c 1.1 94/10/31 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

_syncmodes()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_syncmodes().\n");
#endif
	_sethl();
	_setmode();
	_setwind();
}

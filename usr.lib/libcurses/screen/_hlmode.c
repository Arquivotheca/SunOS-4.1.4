#ifndef lint
static	char sccsid[] = "@(#)_hlmode.c 1.1 94/10/31 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

_hlmode (on)
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_hlmode(%o).\n", on);
#endif
	SP->virt_gr = on;
}

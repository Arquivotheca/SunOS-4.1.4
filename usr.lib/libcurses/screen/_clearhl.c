#ifndef lint
static	char sccsid[] = "@(#)_clearhl.c 1.1 94/10/31 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

_clearhl ()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_clearhl().\n");
#endif
	if (SP->phys_gr) {
		register oldes = SP->virt_gr;
		SP->virt_gr = 0;
		_sethl ();
		SP->virt_gr = oldes;
	}
}

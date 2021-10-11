#ifndef lint
static	char sccsid[] = "@(#)_insmode.c 1.1 94/10/31 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

/*
 * Set the virtual insert/replacement mode to new.
 */
_insmode (new)
int new;
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_insmode(%d).\n", new);
#endif
	SP->virt_irm = new;
}

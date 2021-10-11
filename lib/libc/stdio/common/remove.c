#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)remove.c 1.1 94/10/31 SMI";
#endif

/*LINTLIBRARY*/
#include <stdio.h>

#undef remove

int
remove(fname)
register char *fname;
{
	return (unlink(fname));
}

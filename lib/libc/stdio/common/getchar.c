#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)getchar.c 1.1 94/10/31 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
/*
 * A subroutine version of the macro getchar.
 */
#include <stdio.h>
#undef getchar

int
getchar()
{
	return(getc(stdin));
}

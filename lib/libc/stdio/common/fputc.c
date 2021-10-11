#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)fputc.c 1.1 94/10/31 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>

int
fputc(c, fp)
int	c;
register FILE *fp;
{
	return(putc(c, fp));
}

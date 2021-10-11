#ifndef lint
static	char sccsid[] = "@(#)linmod.c 1.1 94/10/31 SMI"; /* from UCB 4.1 6/27/83 */
#endif

#include <stdio.h>
linemod(s)
char *s;
{
	int i;
	putc('f',stdout);
	for(i=0;s[i];)putc(s[i++],stdout);
	putc('\n',stdout);
}

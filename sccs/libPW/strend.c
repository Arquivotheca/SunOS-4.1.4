#ifndef lint
static	char sccsid[] = "@(#)strend.c 1.1 94/10/31 SMI"; /* from System III 3.1 */
#endif

char *strend(p)
register char *p;
{
	while (*p++)
		;
	return(--p);
}

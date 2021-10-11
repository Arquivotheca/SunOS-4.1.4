#ifndef lint
static	char sccsid[] = "@(#)memset.c 1.1 94/10/31 SMI"; /* from S5R2 1.1 */
#endif

/*LINTLIBRARY*/
/*
 * Set an array of n chars starting at sp to the character c.
 * Return sp.
 */
char *
memset(sp, c, n)
register char *sp, c;
register int n;
{
	register char *sp0 = sp;

	while (--n >= 0)
		*sp++ = c;
	return (sp0);
}

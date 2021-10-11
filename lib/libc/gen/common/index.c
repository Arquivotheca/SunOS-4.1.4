#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)index.c 1.1 94/10/31 SMI"; /* from UCB 4.1 80/12/21 */
#endif

/*
 * Return the ptr in sp at which the character c appears;
 * NULL if not found
 */

#define	NULL	0

char *
index(sp, c)
	register char *sp, c;
{

	do {
		if (*sp == c)
			return (sp);
	} while (*sp++);
	return (NULL);
}

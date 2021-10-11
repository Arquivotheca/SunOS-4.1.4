#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)strchr.c 1.1 94/10/31 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
/*
 * Return the ptr in sp at which the character c appears;
 * NULL if not found
 */

#define	NULL	0

char *
strchr(sp, c)
register char *sp, c;
{
	do {
		if(*sp == c)
			return(sp);
	} while(*sp++);
	return(NULL);
}

#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)strpbrk.c 1.1 94/10/31 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
/*
 * Return ptr to first occurance of any character from `brkset'
 * in the character string `string'; NULL if none exists.
 */

#define	NULL	(char *) 0

char *
strpbrk(string, brkset)
register char *string, *brkset;
{
	register char *p;

	do {
		for(p=brkset; *p != '\0' && *p != *string; ++p)
			;
		if(*p != '\0')
			return(string);
	}
	while(*string++);
	return(NULL);
}

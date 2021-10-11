/*	@(#)dbe_subr.c 1.1 94/10/31 SMI	*/

#include <sys/param.h>
#include <sys/vm.h>

/*
 * In kernel version of getenv. Returns pointer to value of 
 * environment variable matching argument or NULL if not found.
 *
 * XXX - the code makes certain assumptions about the format of data
 *	 in below USRSTACK.
 */

static char *knvmatch(/* s1, s2 */);

char *
kgetenv(name)
	register char *name;
{
	register char *v, **p = (char **) USRSTACK;

	while (fuword(--p) == NULL);
	while (fuword(--p) != NULL);
	while (fuword(--p) == NULL);
	while (fuword(--p) != NULL);
	p++;

	while (fuword(p) != NULL)
		if ((v = knvmatch(name, fuword(p++))) != NULL)
			return(v);
	return(NULL);
}

/*
 * s1 is either name, or name=value
 * s2 is name=value
 * if names match, return value of s2, else NULL
 * used for environment searching: see getenv
 */

static char *
knvmatch(s1, s2)
	register char *s1, *s2;
{

	while(*s1 == fubyte(s2++))
		if(*s1++ == '=')
			return(s2);
	if(*s1 == '\0' && fubyte (s2-1) == '=')
		return(s2);
	return(NULL);
}

char *
nvmatch(s1, s2)
	register char *s1, *s2;
{

	while(*s1 == *s2++)
		if(*s1++ == '=')
			return(s2);
	if(*s1 == '\0' && s2[-1] == '=')
		return(s2);
	return(NULL);
}


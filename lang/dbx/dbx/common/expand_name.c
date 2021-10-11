#ifndef lint
static	char sccsid[] = "@(#)expand_name.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 *  expand_name:  Take a file name, possibly with shell meta-characters
 *	in it, and expand it by using "sh -c echo filename"
 *	Return the resulting file names as a dynamic struct namelist.
 *
 *  free_namelist:  Free a dynamic struct namelist allocated by expand_name.
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdio.h>
#include <sgtty.h>
#include "expand_name.h"

#define MAXCHAR 255
#define NOSTR   ((char *) 0)

char		*getenv();
char		*index();
char		*malloc();
struct namelist *make_0list();
struct namelist *make_1list();
struct namelist *makelist();

static char   Default_Shell[] = "/bin/sh";

struct namelist *
expand_name(name)
	char name[];
{
	char		 xnames[MAX_ARG_CHARS];
	char		 cmdbuf[BUFSIZ];
	register int	 pid, length;
	register char	 *Shell;
	int		 status, pivec[2];

	if (!anyof(name, "~{[*?$`'\"\\"))
		return(make_1list(name));
	if (pipe(pivec) < 0) {
		perror("Pipe failed");
		return(NONAMES);
	}
	sprintf(cmdbuf, "echo %s", name);
	if ((pid = vfork()) == 0) {
		Shell = getenv("SHELL");
		if (Shell == NOSTR)
			Shell = Default_Shell;
		close(pivec[0]);
		close(1);
		dup(pivec[1]);
		close(pivec[1]);
		close(2);
		execl(Shell, Shell, "-c", cmdbuf, 0);
		perror("Exec failed");
		_exit(1);
	}
	if (pid == -1) {
		perror("Fork failed");
		close(pivec[0]);
		close(pivec[1]);
		return(NONAMES);
	}
	close(pivec[1]);
	for (status=1, length = 0; length < MAX_ARG_CHARS;) {
		status = read(pivec[0], xnames+length, MAX_ARG_CHARS-length);
		if (status < 0) {
			perror("Read failed");
			return(NONAMES);
		}
		if (status == 0)
			break;
		length += status;
	}
	close(pivec[0]);
	while (wait(&status) != pid);
		;
	status &= 0377;
	if (status != 0 && status != SIGPIPE) {
		fprintf(stderr, "\"Echo\" failed\n");
		return(NONAMES);
	}
	if (length == 0)  {
		return(make_0list());
	}
	if (length == MAX_ARG_CHARS) {
		fprintf(stderr, "Buffer overflow (> %d)  expanding \"%s\"\n",
				MAX_ARG_CHARS, name);
		return(NONAMES);
	}
	xnames[length] = '\0';
	while (length > 0 && xnames[length-1] == '\n') {
		xnames[--length] = '\0';
	}
	return(makelist(length+1, xnames));
}


anyof(s1, s2)
        register char *s1, *s2;
{
        register int c;
	char	     table[MAXCHAR+1];

	for (c=0; c <= MAXCHAR; c++)
		table[c] = '\0';
	while (c = *s2++)
		table[c] = '\177';
        while (c = *s1++)
                if (table[c])
                        return(1);
        return(0);
}


/*
 * Return a pointer to a dynamically allocated namelist
 *
 * First, the 2 commonest special cases:
 */
 

struct namelist *
make_0list()
{
	struct namelist *result;

	result = (struct namelist *)
			malloc(sizeof(int) + sizeof(char *)); 
	if (result == NONAMES) {
		fprintf(stderr, "malloc failed in expand_name\n");
	} else  {
		result->count = 0;
		result->names[0] = NOSTR;
	}
	return(result);
}


struct namelist *
make_1list(str)
        char *str;
{
        struct namelist *result;

	result = (struct namelist *)
			malloc(sizeof(int) + 2*sizeof(char *) + strlen(str));
	if (result == NONAMES) {
		fprintf(stderr, "malloc failed in expand_name\n");
	} else  {
		result->count = 1;
		result->names[0] = (char *)(&result->names[2]);
		result->names[1] = NOSTR;
		strcpy(result->names[0], str);
	}
	return(result);
}


struct namelist *
makelist(len, str)
	register int	 len;
	register char	*str;
{
	register char	*cp;
	register int	 count, i;
	struct namelist	*result;

	if (str[0] == '\0')  {
		return(NONAMES);
	}
	for (count = 1, cp = str; cp && *cp ;) {
		cp = index(cp, ' ');
		if (cp)  {
			count += 1;
			*cp++ = '\0';
		}
	}
	result = (struct namelist *)
			malloc(sizeof(int) + (count+1)*sizeof(char *) + len);
	if (result == NONAMES)  {
		fprintf(stderr, "malloc failed in expand_name\n");
	} else {
		result->count = count;
		cp  = (char *)(&result->names[count+1]);
		for (i=len; i--;)
			cp[i] = str[i];
		for (i=0; i<count; i++)  {
			result->names[i] = cp;
			while (*cp++)
				;
		}
		result->names[i] = NOSTR;
	}
	return(result);
}


void
free_namelist(ptr)
	struct namelist	*ptr;
{
	if (ptr)
		free(ptr);
}

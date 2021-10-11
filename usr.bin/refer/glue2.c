#ifndef lint
static char sccsid[] = "@(#)glue2.c 1.1 94/10/31 SMI";	/* from UCB 4.1 5/6/83 */
#endif

#include <sys/param.h>

char refdir[MAXPATHLEN];

savedir()
{
	if (refdir[0]==0)
		if (getwd(refdir) == NULL)
			err("can't get current directory");
}

restodir()
{
	chdir(refdir);
}

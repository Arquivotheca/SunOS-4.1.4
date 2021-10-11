#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)telldir.c 1.1 94/10/31 SMI";
#endif

#include <sys/param.h>
#include <dirent.h>

/*
 * return a pointer into a directory
 */
long
telldir(dirp)
	register DIR *dirp;
{
        return(dirp->dd_off);
}

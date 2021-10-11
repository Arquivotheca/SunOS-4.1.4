#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)closedir.c 1.1 94/10/31 SMI";
#endif

#include <sys/param.h>
#include <dirent.h>

/*
 * close a directory.
 */
int
closedir(dirp)
	register DIR *dirp;
{
	int fd;
	extern void free();
	extern int close();

	fd = dirp->dd_fd;
	dirp->dd_fd = -1;
	dirp->dd_loc = 0;
	free(dirp->dd_buf);
	free((char *)dirp);
	return (close(fd));
}

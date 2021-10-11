#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)setgid.c 1.1 94/10/31 SMI";
#endif

#include <errno.h>

/*
 * SVID-compatible setgid.
 */
setgid(gid)
	int gid;
{

	if (gid < 0) {
		errno = EINVAL;
		return (-1);
	}
	if (geteuid() == 0)
		return (setregid(gid, gid));
	else
		return (setregid(-1, gid));
}

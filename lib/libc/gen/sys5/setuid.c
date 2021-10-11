#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)setuid.c 1.1 94/10/31 SMI";
#endif

#include <errno.h>
#include <sys/param.h>

/*
 * SVID-compatible setuid.
 */
setuid(uid)
	int uid;
{

	if (uid < 0  ||  uid > MAXUID) {
		errno = EINVAL;
		return (-1);
	}
	if (geteuid() == 0)
		return (setreuid(uid, uid));
	else
		return (setreuid(-1, uid));
}

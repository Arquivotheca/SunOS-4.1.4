#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)gtty.c 1.1 94/10/31 SMI"; /* from UCB 4.1 83/07/04 */
#endif

/*
 * Writearound to old gtty system call.
 */

#include <sgtty.h>

gtty(fd, ap)
	struct sgttyb *ap;
{

	return(ioctl(fd, TIOCGETP, ap));
}

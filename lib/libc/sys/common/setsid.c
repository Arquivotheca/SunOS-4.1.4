#if	!defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)setsid.c 1.1 94/10/31 SMI";
#endif

#include <sys/session.h>
#include <sys/syscall.h>

#define _SETSID		syscall(SYS_setsid, SESS_NEW)

setsid()
{
	return (_SETSID);
}

/* 
 *  For 4.0 compatibility
 */
setspgldr()
{
	return (_SETSID);
}


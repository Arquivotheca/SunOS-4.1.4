#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)utimes.c 1.1 94/10/31 SMI";
#endif

# include "../common/chkpath.h"

utimes(f, t)
    char           *f;
    struct timeval *t;
{
    CHK(f);
    return syscall(SYS_utimes, f, t);
}

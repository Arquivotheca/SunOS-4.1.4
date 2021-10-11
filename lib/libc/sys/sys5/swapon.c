#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)swapon.c 1.1 94/10/31 SMI";
#endif

# include "../common/chkpath.h"

swapon(s)
    char           *s;
{
    CHK(s);
    return syscall(SYS_swapon, s);
}

#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)unmount.c 1.1 94/10/31 SMI";
#endif

# include "../common/chkpath.h"

unmount(s)
    char           *s;
{
    CHK(s);
    return syscall(SYS_unmount, s);
}

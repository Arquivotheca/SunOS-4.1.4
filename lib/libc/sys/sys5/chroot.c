#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)chroot.c 1.1 94/10/31 SMI";
#endif

# include "../common/chkpath.h"

chroot(d)
    char           *d;
{
    CHK(d);
    return syscall(SYS_chroot, d);
}

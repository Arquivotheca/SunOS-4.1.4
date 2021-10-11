#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)symlink.c 1.1 94/10/31 SMI";
#endif

# include "../common/chkpath.h"

symlink(t, f)
    char           *t, *f;
{
    CHK(t);
    CHK(f);
    return syscall(SYS_symlink, t, f);
}

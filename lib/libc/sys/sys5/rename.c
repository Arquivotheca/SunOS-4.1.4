#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)rename.c 1.1 94/10/31 SMI";
#endif

# include "../common/chkpath.h"

rename(f, t)
    char           *f, *t;
{
    CHK(f);
    CHK(t);
    return syscall(SYS_rename, f, t);
}

#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)lstat.c 1.1 94/10/31 SMI";
#endif

# include "../common/chkpath.h"

lstat(s, b)
    char           *s;
    struct stat    *b;
{
    CHK(s);
    return syscall(SYS_lstat, s, b);
}

#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)chdir.c 1.1 94/10/31 SMI";
#endif

# include "../common/chkpath.h"

chdir(s)
    char           *s;
{
    CHK(s);
    return syscall(SYS_chdir, s);
}

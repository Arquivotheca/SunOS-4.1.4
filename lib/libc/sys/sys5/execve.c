#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)execve.c 1.1 94/10/31 SMI";
#endif

# include "../common/chkpath.h"

execve(s, v, e)
    char           *s, **v, **e;
{
    CHK(s);
    return syscall(SYS_execve, s, v, e);
}

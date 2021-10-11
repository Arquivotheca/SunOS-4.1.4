#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)creat.c 1.1 94/10/31 SMI";
#endif

# include "../common/chkpath.h"

creat(s, m)
    char           *s;
{
    CHK(s);
    return syscall(SYS_creat, s, m);
}

#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)chown.c 1.1 94/10/31 SMI";
#endif

# include "../common/chkpath.h"

chown(s, u, g)
    char           *s;
{
    CHK(s);
    return syscall(SYS_chown, s, u, g);
}

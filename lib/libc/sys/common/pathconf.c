#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)pathconf.c 1.1 94/10/31 SMI";
#endif

#include "../common/chkpath.h"

pathconf(p, what)
    char* p;
{
    CHK(p);
    return syscall(SYS_pathconf, p, what);
}

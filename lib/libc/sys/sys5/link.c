#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)link.c 1.1 94/10/31 SMI";
#endif

# include "../common/chkpath.h"

link(a, b)
    char           *a;
    char           *b;
{
    CHK(a);
    CHK(b);
    return syscall(SYS_link, a, b);
}

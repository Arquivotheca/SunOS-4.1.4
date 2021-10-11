#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)getc.c 1.1 94/10/31 SMI";
#endif

/*LINTLIBRARY*/
#include <stdio.h>

#undef getc
#define	__getc__(p)		(--(p)->_cnt>=0? ((int)*(p)->_ptr++):_filbuf(p))

int
getc(fp)
register FILE *fp;
{
	return (__getc__(fp));
}

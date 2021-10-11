#ifndef lint
static	char sccsid[] = "@(#)putsi.c 1.1 94/10/31 SMI"; /* from UCB 4.1 6/27/83 */
#endif

#include <stdio.h>
putsi(a){
	putc((char)a,stdout);
	putc((char)(a>>8),stdout);
}

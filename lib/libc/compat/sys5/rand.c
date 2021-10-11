#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)rand.c 1.1 94/10/31 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/

static long randx=1;

void
srand(x)
unsigned x;
{
	randx = x;
}

int
rand()
{
	return(((randx = randx * 1103515245L + 12345)>>16) & 0x7fff);
}

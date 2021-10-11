#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)rand.c 1.1 94/10/31 SMI"; /* from UCB 4.1 80/12/21 */
#endif

static	long	randx = 1;

srand(x)
unsigned x;
{
	randx = x;
}

rand()
{
	return((randx = randx * 1103515245 + 12345) & 0x7fffffff);
}

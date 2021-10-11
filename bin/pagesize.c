#ifndef lint
static	char sccsid[] = "@(#)pagesize.c 1.1 94/10/31 SMI"; /* from UCB 4.1 82/11/07 */
#endif
/*
 * getpagesize
 */

main()
{

	printf("%d\n", getpagesize());
	exit(0);
	/* NOTREACHED */
}

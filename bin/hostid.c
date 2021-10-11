#ifndef lint
static	char sccsid[] = "@(#)hostid.c 1.1 94/10/31 SMI"; /* from UCB 4.1 82/11/07 */
#endif

/*
 * hostid
 */
main(argc, argv)
	int argc;
	char **argv;
{
	printf("%08x\n", gethostid());
	exit(0);
	/* NOTREACHED */
}

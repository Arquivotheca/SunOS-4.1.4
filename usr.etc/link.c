#ifndef lint
static	char sccsid[] = "@(#)link.c 1.1 94/10/31 SMI"; /* from S5R2 1.1 */
#endif

main(argc, argv) char *argv[]; {
	if(argc!=3) {
		write(2, "Usage: link from to\n", 25);
		exit(1);
	}
	exit((link(argv[1], argv[2])==0)? 0: 2);
	/* NOTREACHED */
}

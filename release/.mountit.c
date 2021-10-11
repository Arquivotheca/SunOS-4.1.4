#ifndef lint
static	char sccsid[] = "@(#).mountit.c 1.1 94/10/31 SMI";
#endif

char *name = "mount";

/* ARGSUSED */
main(argc, argv)
int	argc;
char	*argv[];
{
	(void) setuid(0);
	execvp(name, argv);
	perror("exec failed");
	exit(1);
}

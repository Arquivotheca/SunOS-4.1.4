#ifndef lint
static	char sccsid[] = "@(#)unmount.c 1.1 94/10/31 SMI";
#endif

char *name = "umount";

/* ARGSUSED */
main(argc, argv)
int	argc;
char	*argv[];
{
	int	uid;

	uid = geteuid();
	(void) setuid(uid);

	execvp(name, argv);
	perror("exec failed");
	exit(1);
}

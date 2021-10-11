#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)seteuid.c 1.1 94/10/31 SMI"; /* from UCB 4.1 83/06/30 */
#endif

seteuid(euid)
	int euid;
{

	return (setreuid(-1, euid));
}

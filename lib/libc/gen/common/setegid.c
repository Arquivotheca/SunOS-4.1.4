#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)setegid.c 1.1 94/10/31 SMI"; /* from UCB 4.1 83/06/30 */
#endif

setegid(egid)
	int egid;
{

	return (setregid(-1, egid));
}

#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)setruid.c 1.1 94/10/31 SMI"; /* from UCB 4.1 83/06/30 */
#endif

setruid(ruid)
	int ruid;
{

	return (setreuid(ruid, -1));
}

#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)pause.c 1.1 94/10/31 SMI"; /* from UCB 4.1 83/06/09 */
#endif

/*
 * Backwards compatible pause.
 */
pause()
{

	sigpause(sigblock(0));
	return (-1);
}

#ifndef lint
static	char sccsid[] = "@(#)userexit.c 1.1 94/10/31 SMI"; /* from System III 3.1 */
#endif

/*
	Default userexit routine for fatal and setsig.
	User supplied userexit routines can be used for logging.
*/

userexit(code)
{
	return(code);
}

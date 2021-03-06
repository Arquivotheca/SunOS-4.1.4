#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)vprintf.c 1.1 94/10/31 SMI"; /* from S5R2 1.1 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>
#include <varargs.h>
#include <errno.h>

extern int _doprnt();

/*VARARGS1*/
int
vprintf(format, ap)
char *format;
va_list ap;
{
	register int count;

	if (!(stdout->_flag & _IOWRT)) {
		/* if no write flag */
		if (stdout->_flag & _IORW) {
			/* if ok, cause read-write */
			stdout->_flag |= _IOWRT;
		} else {
			/* else error */
#ifdef POSIX
			errno = EBADF;
#endif
			return EOF;
		}
	}
	count = _doprnt(format, ap, stdout);
	return(ferror(stdout)? EOF: count);
}

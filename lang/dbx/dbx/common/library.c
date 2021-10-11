#ifndef lint
static	char sccsid[] = "@(#)library.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * General purpose routines.
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include "defs.h"
#include "library.h"

String cmdname;			/* name of command for error messages */
Filename errfilename;		/* current file associated with error */
short errlineno;		/* line number associated with error */

Boolean nocerror;

/*
 * Definitions for doing memory allocation.
 */


/*
 * String definitions.
 */

extern String strcpy(), index(), rindex();
extern int strlen();

typedef struct {
    INTFUNC *func;
} ERRINFO;



public pwait(pid, statusp)
int	pid;
union	wait	*statusp;
{
	while (wait(statusp) != pid)
		;
}

/*
 * Invoke a shell on a command line.
 */

#define DEF_SHELL	"csh"

public shell(s)
String s;
{
    extern String getenv();
    String sh;

    if ((sh = getenv("SHELL")) == nil) {
	sh = DEF_SHELL;
    }
    if (s != nil and *s != '\0') {
	system(s);
    } else {
	system(sh);
    }
}

/*
 * System call error handler.
 *
 * The syserr routine is called when a system call is about to
 * set the c-bit to report an error.  Certain errors are caught
 * and cause the process to print a message and immediately exit.
 */

extern int sys_nerr;
extern char *sys_errlist[];
 
/*
 * Before calling syserr, the integer errno is set to contain the
 * number of the error.  The routine "_mycerror" is a dummy which
 * is used to force the loader to get my version of cerror rather
 * than the usual one.
 */

extern int errno;
extern _mycerror();

/*
 * Default error handling.
 */

private ERRINFO errinfo[] ={
/* no error */		ERR_IGNORE,
/* EPERM */		ERR_IGNORE,
/* ENOENT */		ERR_IGNORE,
/* ESRCH */		ERR_IGNORE,
/* EINTR */		ERR_CATCH,
/* EIO */		ERR_CATCH,
/* ENXIO */		ERR_CATCH,
/* E2BIG */		ERR_CATCH,
/* ENOEXEC */		ERR_CATCH,
/* EBADF */		ERR_IGNORE,
/* ECHILD */		ERR_CATCH,
/* EAGAIN */		ERR_CATCH,
/* ENOMEM */		ERR_CATCH,
/* EACCES */		ERR_IGNORE,
#ifdef sparc
/* EFAULT */		ERR_IGNORE, /* to work around no sigstack*/
#else
/* EFAULT */		ERR_CATCH,
#endif
/* ENOTBLK */		ERR_CATCH,
/* EBUSY */		ERR_CATCH,
/* EEXIST */		ERR_CATCH,
/* EXDEV */		ERR_CATCH,
/* ENODEV */		ERR_CATCH,
/* ENOTDIR */		ERR_CATCH,
/* EISDIR */		ERR_CATCH,
#ifdef sparc
/* EINVAL */		ERR_IGNORE,  /* "" failing ioctl in __fillbuf()/isatty() */
#else
/* EINVAL */		ERR_CATCH,
#endif
/* ENFILE */		ERR_CATCH,
/* EMFILE */		ERR_CATCH,
/* ENOTTY */		ERR_IGNORE,
/* ETXTBSY */		ERR_CATCH,
/* EFBIG */		ERR_CATCH,
/* ENOSPC */		ERR_CATCH,
/* ESPIPE */		ERR_CATCH,
/* EROFS */		ERR_CATCH,
/* EMLINK */		ERR_CATCH,
/* EPIPE */		ERR_CATCH,
/* EDOM */		ERR_CATCH,
/* ERANGE */		ERR_CATCH,
/* EWOULDBLOCK */	ERR_CATCH,
/* EINPROGRESS */	ERR_CATCH,
/* EALREADY */		ERR_CATCH,
/* ENOTSOCK */		ERR_CATCH,
/* EDESTADDRREQ */	ERR_CATCH,
/* EMSGSIZE */		ERR_CATCH,
/* EPROTOTYPE */	ERR_CATCH,
/* ENOPROTOOPT */	ERR_CATCH,
/* EPROTONOSUPPORT */	ERR_CATCH,
/* ESOCKTNOSUPPORT */	ERR_CATCH,
/* EOPNOTSUPP */	ERR_CATCH,
/* EPFNOSUPPORT */	ERR_CATCH,
/* EAFNOSUPPORT */	ERR_CATCH,
/* EADDRINUSE */	ERR_CATCH,
/* EADDRNOTAVAIL */	ERR_CATCH,
/* ENETDOWN */		ERR_CATCH,
/* ENETUNREACH */	ERR_CATCH,
/* ENETRESET */		ERR_CATCH,
/* ECONNABORTED */	ERR_CATCH,
/* ECONNRESET */	ERR_CATCH,
/* ENOBUFS */		ERR_CATCH,
/* EISCONN */		ERR_CATCH,
/* ENOTCONN */		ERR_CATCH,
/* ESHUTDOWN */		ERR_CATCH,
/* ETOOMANYREFS */	ERR_CATCH,
/* ETIMEDOUT */		ERR_CATCH,
/* ECONNREFUSED */	ERR_CATCH,
/* ELOOP */		ERR_CATCH,
/* ENAMETOOLONG */	ERR_CATCH,
/* EHOSTDOWN */		ERR_CATCH,
/* EHOSTUNREACH */	ERR_CATCH,
/* ENOTEMPTY */		ERR_CATCH,
/* EPROCLIM */		ERR_CATCH,
/* EUSERS */		ERR_CATCH,
/* EDQUOT */		ERR_CATCH,
/* ESTALE */		ERR_IGNORE,
/* EREMOTE */		ERR_CATCH,
/* EIDRM */		ERR_CATCH,
/* ENOMSG */		ERR_CATCH,
/* EDEADLK */		ERR_CATCH,
/* ENOLCK */		ERR_CATCH,
};

#define	NERRS		(sizeof errinfo / sizeof errinfo[0])

public syserr()
{
    INTFUNC *errfunc;

    if (nocerror) {
	return;
    }
    if (errno >= NERRS)
	errfunc = ERR_CATCH;
    else
	errfunc = errinfo[errno].func;
    if (errfunc == ERR_CATCH) {
	if (errno < sys_nerr) {
	    fatal(sys_errlist[errno]);
	} else {
	    fatal("errno %d", errno);
	}
    } else if (errfunc != ERR_IGNORE) {
	(*errfunc)();
    }
}

/*
 * Catcherrs only purpose is to get this module loaded and make
 * sure my cerror is loaded (only applicable when this is in a library).
 */

public catcherrs()
{
    _mycerror();
}

/*
 * Change the action on receipt of an error.
 */

public INTFUNC *onsyserr(n, f)
int n;
INTFUNC *f;
{
    INTFUNC *retf;

    retf = errinfo[n].func;
    errinfo[n].func = f;
    return(retf);
}
/*
 * Standard error handling routines.
 */

private short nerrs;
private short nwarnings;

/*
 * Main driver of error message reporting.
 * If this is a fatal error and this is a slave of dbxtool
 * then write the message to stderr.
 */

/* VARARGS2 */
private errmsg(errname, shouldquit, s, a, b, c, d, e, f, g, h, i, j, k, l, m)
String errname;
Boolean shouldquit;
String s;
{
    File fp;

    if (shouldquit and istool()) {
	fp = fopen("/dev/console", "w");
    } else {
	fp = stdout;
    }
    fflush(fp);
    if (shouldquit and cmdname != nil) {
	fprintf(fp, "%s: ", cmdname);
    }
    if (errfilename != nil) {
	fprintf(fp, "%s: ", errfilename);
    }
    if (errlineno > 0) {
	fprintf(fp, "%d: ", errlineno);
    }
    if (errname != nil) {
	fprintf(fp, "%s: ", errname);
    }
    fprintf(fp, s, a, b, c, d, e, f, g, h, i, j, k, l, m);
    fprintf(fp, "\n");
    if (shouldquit) {
	fflush(fp);
	quit(1);
    }
}

/*
 * For when printf isn't sufficient for printing the error message ...
 */

public beginerrmsg()
{
    fflush(stdout);
    if (errfilename != nil) {
	printf("%s: ", errfilename);
    }
    if (errlineno > 0) {
	printf("%d: ", errlineno);
    }
}

public enderrmsg()
{
    printf("\n");
    erecover();
}

/*
 * The messages are listed in increasing order of seriousness.
 *
 * First are warnings.
 */

/* VARARGS1 */
public warning(s, a, b, c, d, e, f, g, h, i, j, k, l, m)
String s;
{
    nwarnings++;
    errmsg("warning", false, s, a, b, c, d, e, f, g, h, i, j, k, l, m);
}

/*
 * Errors are a little worse, they mean something is wrong,
 * but not so bad that processing can't continue.
 *
 * The routine "erecover" is called to recover from the error,
 * a default routine is provided that does nothing.
 */

/* VARARGS1 */
public error(s, a, b, c, d, e, f, g, h, i, j, k, l, m)
String s;
{
    extern erecover();

    nerrs++;
    errmsg(nil, false, s, a, b, c, d, e, f, g, h, i, j, k, l, m);
    erecover();
}

/*
 * Non-recoverable user error.
 */

/* VARARGS1 */
public fatal(s, a, b, c, d, e, f, g, h, i, j, k, l, m)
String s;
{
    errmsg("fatal error", true, s, a, b, c, d, e, f, g, h, i, j, k, l, m);
}

/*
 * Panics indicate an internal program error.
 */

/* VARARGS1 */
public panic(s, a, b, c, d, e, f, g, h, i, j, k, l, m)
String s;
{
    errmsg("internal error", true, s, a, b, c, d, e, f, g, h, i, j, k, l, m);
}

short numerrors()
{
    short r;

    r = nerrs;
    nerrs = 0;
    return r;
}

short numwarnings()
{
    short r;

    r = nwarnings;
    nwarnings = 0;
    return r;
}

/*
 * Recover from an error.
 *
 * This is the default routine which we aren't using since we have our own.
 *
public erecover()
{
}
 *
 */

/*
 * Default way to quit from a program is just to exit.
 *
public quit(r)
int r;
{
    exit(r);
}
 *
 */

/*
 * Compare n-byte areas pointed to by s1 and s2
 * if n is 0 then compare up until one has a null byte.
 */

public int cmp(s1, s2, n)
register char *s1, *s2;
register unsigned int n;
{
    if (s1 == nil || s2 == nil) {
	panic("cmp: nil pointer");
    }
    if (n == 0) {
	while (*s1 == *s2++) {
	    if (*s1++ == '\0') {
		return(0);
	    }
	}
	return(*s1 - *(s2-1));
    } else {
	for (; n != 0; n--) {
	    if (*s1++ != *s2++) {
		return(*(s1-1) - *(s2-1));
	    }
	}
	return(0);
    }
}

/*
 * Move n bytes from src to dest.
 * If n is 0 move until a null is found.
 */

public mov(src, dest, n)
register char *src, *dest;
register unsigned int n;
{
    if (src == nil)
	panic("mov: nil source");
    if (dest == nil)
	panic("mov: nil destination");
    if (n != 0) {
	for (; n != 0; n--) {
	    *dest++ = *src++;
	}
    } else {
	while ((*dest++ = *src++) != '\0');
    }
}

public char *strdup(string)
	char *string;
{	char *new_string;

	new_string = malloc(strlen(string) + 1);
	if (new_string == 0)
		fatal("No memory available. Out of swap space?");
	(void)strcpy(new_string, string);
	return new_string;
}

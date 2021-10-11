
/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static  char sccsid[] = "@(#)rsh.c 1.1 94/10/31 SMI"; /* from UCB 5.4 8/28/85 */
#endif not lint

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
/* just for FIONBIO ... */
#include <sys/filio.h>

#include <netinet/in.h>

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <pwd.h>
#include <netdb.h>
#include <locale.h>

#ifdef SYSV
#define	rindex		strrchr
#define	index		strchr
#define	bcopy(a,b,c)	memcpy((b),(a),(c))
#define	bzero(s,n)	memset((s), 0, (n))
#define signal(s,f)	sigset((s), (f))
#endif /* SYSV */

#ifndef SYSV
#define	gettext(s)	(s)
#define textdomain(d)	((char *) 0)
#endif /* SYSV */

int	error();
char	*index(), *rindex(), *malloc(), *getpass(), *strcpy();
struct passwd	*getpwuid();

int	errno;
int	options;
int	rfd2;
int	sendsig();


#ifdef SYSV
#ifndef sigmask
#define sigmask(m)      (1 << ((m)-1))
#endif

#define set2mask(setp) ((setp)->__sigbits[0])
#define mask2set(mask, setp) \
	((mask) == -1 ? sigfillset(setp) : (((setp)->__sigbits[0]) = (mask)))
	

static sigsetmask(mask)
	int mask;
{
	sigset_t oset;
	sigset_t nset;

	(void) sigprocmask(0, (sigset_t *)0, &nset);
	mask2set(mask, &nset);
	(void) sigprocmask(SIG_SETMASK, &nset, &oset);
	return set2mask(&oset);
}

static sigblock(mask)
	int mask;
{
	sigset_t oset;
	sigset_t nset;

	(void) sigprocmask(0, (sigset_t *)0, &nset);
	mask2set(mask, &nset);
	(void) sigprocmask(SIG_BLOCK, &nset, &oset);
	return set2mask(&oset);
}

#endif

#define	mask(s)	(1 << ((s) - 1))

/*
 * rsh - remote shell
 */
/* VARARGS */
main(argc, argv0)
	int argc;
	char **argv0;
{
	pid_t pid;
	int rem;
	char *host, *cp, **ap, buf[BUFSIZ], *args, **argv = argv0, *user = 0;
	register int cc;
	int asrsh = 0;
	struct passwd *pwd;
	int readfrom_rem;
	int readfrom_rfd2;
	int one = 1;
	struct servent *sp;
	int omask;
	int nflag = 0;

	(void) setlocale(LC_ALL, "");

#if !defined(TEXT_DOMAIN)
#define TEXT_DOMAIN "SYS_TEST"
#endif
	(void) textdomain(TEXT_DOMAIN);

	host = rindex(argv[0], '/');
	if (host)
		host++;
	else
		host = argv[0];
	argv++, --argc;
	if (!strcmp(host, "rsh")) {
		if (argc == 0)
			goto usage;
		if (*argv[0] != '-') {
			host = *argv++, --argc;
			asrsh = 1;
		} else
			host = 0;
	}
another:
	if (argc > 0 && !strcmp(*argv, "-l")) {
		argv++, argc--;
		if (argc > 0)
			user = *argv++, argc--;
		goto another;
	}
	if (argc > 0 && !strcmp(*argv, "-n")) {
		argv++, argc--;
		nflag++;
		(void) close(0);
		(void) open("/dev/null", 0);
		goto another;
	}
	if (argc > 0 && !strcmp(*argv, "-d")) {
		argv++, argc--;
		options |= SO_DEBUG;
		goto another;
	}
	/*
	 * Ignore the -L, -w, -e and -8 flags to allow aliases with rlogin
	 * to work
	 *
	 * There must be a better way to do this! -jmb
	 */
	if (argc > 0 && !strncmp(*argv, "-L", 2)) {
		argv++, argc--;
		goto another;
	}
	if (argc > 0 && !strncmp(*argv, "-w", 2)) {
		argv++, argc--;
		goto another;
	}
	if (argc > 0 && !strncmp(*argv, "-e", 2)) {
		argv++, argc--;
		goto another;
	}
	if (argc > 0 && !strncmp(*argv, "-8", 2)) {
		argv++, argc--;
		goto another;
	}
	if (host == 0) {
		if (argc == 0)
			goto usage;
 		host = *argv++, --argc;
		asrsh = 1;
	}
	if (argv[0] == 0) {
		if (asrsh)
			*argv0 = "rlogin";
		execv("/usr/bin/rlogin", argv0);
		execv("/usr/ucb/rlogin", argv0);
		fprintf(stderr, gettext("No local rlogin program found\n"));
		exit(1);
	}
	pwd = getpwuid(getuid());
	if (pwd == 0) {
		fprintf(stderr, gettext("who are you?\n"));
		exit(1);
	}
	cc = 0;
	for (ap = argv; *ap; ap++)
		cc += strlen(*ap) + 1;
	cp = args = malloc(cc);
	for (ap = argv; *ap; ap++) {
		(void) strcpy(cp, *ap);
		while (*cp)
			cp++;
		if (ap[1])
			*cp++ = ' ';
	}
	sp = getservbyname("shell", "tcp");
	if (sp == 0) {
		fprintf(stderr, gettext("rsh: shell/tcp: unknown service\n"));
		exit(1);
	}
        rem = rcmd(&host, sp->s_port, pwd->pw_name,
	    user ? user : pwd->pw_name, args, &rfd2);
        if (rem < 0)
                exit(1);
	if (rfd2 < 0) {
		fprintf(stderr, gettext("rsh: can't establish stderr\n"));
		exit(2);
	}
	if (options & SO_DEBUG) {
		if (setsockopt(rem, SOL_SOCKET, SO_DEBUG, (char *) &one,
			       sizeof (one)) < 0)
			perror("rsh: setsockopt (stdin)");
		if (setsockopt(rfd2, SOL_SOCKET, SO_DEBUG, (char *) &one,
			       sizeof (one)) < 0)
			perror("rsh: setsockopt (stderr)");
	}
	if ( setsockopt(rem, SOL_SOCKET, SO_KEEPALIVE,(char *) &one, 
			sizeof (one)) < 0)
		perror("rsh: setsockopt (stdin)");
	if ( setsockopt(rfd2, SOL_SOCKET, SO_KEEPALIVE,(char *) &one, 
			sizeof (one)) < 0)
        	perror("rsh: setsockopt (stdin)");
	(void) setuid(getuid());
	omask = sigblock(mask(SIGINT)|mask(SIGQUIT)|mask(SIGTERM));
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, (void (*)())sendsig);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, (void (*)())sendsig);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, (void (*)())sendsig);
	ioctl(rfd2, FIONBIO, &one);
	ioctl(rem, FIONBIO, &one);
	if (nflag) {
		(void) shutdown(rem, 1);
	} else {
		pid = fork();
		if (pid < 0) {
			perror("rsh: fork");
			exit(1);
		}
		if (pid == 0) {
			fd_set remset;
			char *bp; 
			int  wc;
			(void) close(rfd2);
		reread:
			errno = 0;
			cc = read(0, buf, sizeof buf);
			if (cc <= 0)
				goto done;
			bp = buf;
		rewrite:
			FD_ZERO (&remset);
			FD_SET (rem, &remset);
			if (select(rem + 1, (fd_set *) 0, &remset, (fd_set *) 0,
			    (struct timeval *) 0) < 0) {
				if (errno != EINTR) {
					perror("rsh: select");
					exit(1);
				}
				goto rewrite;
			}
			if (!FD_ISSET (rem, &remset))
				goto rewrite;
			wc = write(rem, bp, cc);
			if (wc < 0) {
				if (errno == EWOULDBLOCK)
					goto rewrite;
				goto done;
			}
			cc -= wc; bp += wc;
			if (cc == 0)
				goto reread;
			goto rewrite;
		done:
			(void) shutdown(rem, 1);
			exit(0);
		}
	}

#define MAX(a,b)	(((a) > (b)) ? (a) : (b))

	sigsetmask(omask);
	readfrom_rem = 1;
	readfrom_rfd2 = 1;
	do {
		fd_set readyset;

		FD_ZERO (&readyset);
		if (readfrom_rem)
			FD_SET (rem, &readyset);
		if (readfrom_rfd2)
			FD_SET (rfd2, &readyset);
		if (select(MAX (rem, rfd2) + 1, &readyset, (fd_set *) 0,
			   (fd_set *) 0, (struct timeval *)0) < 0) {
			if (errno != EINTR) {
				perror("rsh: select");
				exit(1);
			}
			continue;
		}
		if (FD_ISSET (rfd2, &readyset)) {
			errno = 0;
			cc = read(rfd2, buf, sizeof buf);
			if (cc <= 0) {
				if (errno != EWOULDBLOCK)
					readfrom_rfd2 = 0;
			} else
				(void) write(2, buf, cc);
		}
		if (FD_ISSET (rem, &readyset)) {
			errno = 0;
			cc = read(rem, buf, sizeof buf);
			if (cc <= 0) {
				if (errno != EWOULDBLOCK)
					readfrom_rem = 0;
			} else
				(void) write(1, buf, cc);
		}
        } while (readfrom_rem | readfrom_rfd2);
        (void) kill(pid, SIGKILL);
	exit(0);
usage:
	fprintf(stderr,
	    gettext("usage: rsh [ -l login ] [ -n ] host command\n"));
	exit(1);
	/* NOTREACHED */
}

sendsig(signo)
	char signo;
{

	(void) write(rfd2, &signo, 1);
}


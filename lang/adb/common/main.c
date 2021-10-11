#ifndef lint
static	char sccsid[] = "@(#)main.c 1.1 94/10/31 SMI";
#endif

/*
 * adb - main command loop and error/interrupt handling
 */

#include "adb.h"
#include "fpascii.h"
#include <stdio.h>
#include <kvm.h>

#ifndef cross
#  include <setjmp.h>
#else cross
/* When creating the cross-debugger, we must get the HOST's setjmp.h */
#  define mc68000
#  include "/usr/include/setjmp.h"
#  undef mc68000
#endif cross
jmp_buf exitbuffer;
#define setexit() setjmp(exitbuffer)
#define reset()   longjmp(exitbuffer,1)

int	infile;
int	maxpos;
void	onintr();
int	exitflg;
int	debugging = 0;

int	eof;
char	*Ipath = "/usr/lib/adb";

char	*swapfil = NULL;
kvm_t	*kvmd;

main(argc, argv)
	register char **argv;
	int argc;
{
	extern char *optarg;		/* getopt() variables */
	extern int optind;
	int c;

# ifdef vax
	mkioptab();
# else  !vax
#   ifndef sparc
	mc68881 = MC68881Present();	/* 68881 present? */
#	ifndef KADB
	    fpa_disasm = fpa_avail = is_fpa_avail();
#	endif !KADB
#   endif !sparc
# endif !vax

another:
	while ((c = getopt(argc, argv, "wkDI:")) != EOF) {
		switch (c) {
		case 'w':
			wtflag = 2;		/* suitable for open() */
			break;
		case 'k':
			kernel = 1;
			break;
		case 'D':
			debugging++;
			break;
		case 'I':
			Ipath = optarg;
			break;
		default:
			printf( "usage: adb [-w] [-k] [-Idir] [objfile [corfil [swapfil]]]\n");
			exit(2);
		}
	}
	argc -= optind;
	argv += optind;
	if (argc > 0)
		symfil = argv[0];
	if (argc > 1)
		corfil = argv[1];
	if (argc > 2) {
		if (kernel)
			swapfil = argv[2];
		else {
			printf("swapfil only valid if -k\n");
			exit(2);
		}
	}
	xargc = argc + 1;

	kopen();		/* init libkvm code, if kernel */
	setsym(); setcor(); setvar(); setty();

    	adb_pgrpid = getpgrp(0);
	if ((sigint = signal(SIGINT, SIG_IGN)) != SIG_IGN) {
		sigint = onintr;
		signal(SIGINT, onintr);
	}
	sigqit = signal(SIGQUIT, SIG_IGN);
	setexit();
	adbtty();
	if (executing)
		delbp();
	executing = 0;
	for (;;) {
		killbuf();
		if (errflg) {
			printf("%s\n", errflg);
			errflg = 0;
			exitflg = 1;
		}
		if (interrupted) {
			interrupted = 0;
			prints("\nadb\n");
		}
		lp = 0; (void) rdc(); lp--;
		if (eof) {
			if (infile) {
				iclose(-1, 0);
				eof = 0;
				reset();
				/*NOTREACHED*/
			} else
				done();
		} else
			exitflg = 0;
		(void) command((char *)0, lastcom);
		if (lp && lastc!='\n')
			error("newline expected");
	}
}

done()
{

	endpcs();
	exit(exitflg);
}

chkerr()
{

	if (errflg || interrupted)
		error(errflg);
}

kopen()
{

	if (kvmd != NULL)
		(void) kvm_close(kvmd);
	if (kernel &&
	    ((kvmd = kvm_open(symfil, corfil, swapfil, wtflag, NULL)) ==
	    NULL) &&
	    ((kvmd = kvm_open(symfil, corfil, swapfil, 0, "Cannot adb -k")) ==
	    NULL)) {
		exit(2);
	}
}

static
doreset()
{

	iclose(0, 1);
	oclose();
	reset();
	/*NOTREACHED*/
}

error(n)
	char *n;
{

	errflg = n;
	doreset();
	/*NOTREACHED*/
}

static void
onintr(s)
{

	(void) signal(s, onintr);
	interrupted = 1;
	lastcom = 0;
	doreset();
	/*NOTREACHED*/
}

shell()
{
	int rc, status, unixpid;
	void (*oldsig)();
	char *argp = lp;
	char *getenv(), *shell = getenv("SHELL");
#ifdef VFORK
	char oldstlp;
#endif

	if (shell == 0)
		shell = "/bin/sh";
	while (lastc != '\n')
		(void) rdc();
#ifndef VFORK
	if ((unixpid=fork())==0)
#else
	oldstlp = *lp;
	if ((unixpid=vfork())==0)
#endif
	{
		signal(SIGINT, sigint);
		signal(SIGQUIT, sigqit);
		*lp=0;
		execl(shell, "sh", "-c", argp, 0);
		_exit(16);
	}
#ifdef VFORK
	lp = oldstlp;
#endif
	if (unixpid == -1)
		error("try again");
	oldsig = signal(SIGINT, SIG_IGN);
	while ((rc = wait(&status)) != unixpid && rc != -1)
		continue;
	signal(SIGINT, oldsig);
	setty();
	prints("!"); lp--;
}

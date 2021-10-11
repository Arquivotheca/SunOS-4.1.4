#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)cmdline.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Ttysw initialization, destruction and error procedures
 */

/* Overwrite existing argv with rest of arg list past this point */
/* Assumes NULL terminates argv */
cmdline_scrunch(argcptr, argv)
	int *argcptr;
	register char **argv;
{
	while (*argv) {
		*argv = *(argv+1);
		argv++;
	}
	/*(*argcptr)--;*/
	*argcptr = *argcptr-1;
}


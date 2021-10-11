/*      @(#)init.c 1.1 94/10/31 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "curses.h"
#include "signal.h"

init()
{
	void intr_handler();

	/*
	 * initialize screen
	 */
	savetty();
	initscr();
	clear();
	refresh();
	/*
	 * trap some signal's...
	 */
	(void) signal(SIGINT,intr_handler);
	(void) signal(SIGQUIT,SIG_IGN);
	(void) signal(SIGHUP,SIG_IGN);
        (void) signal(SIGTSTP,SIG_IGN);
	/*
	 * set some terminal modes..
	 */
	nl();			/* turn on CR-LF remap	*/
	crmode();		/* set cbreak...	*/
	echo();			/* turn on echoing     */
	leaveok(stdscr,FALSE);
	scrollok(stdscr,FALSE);
}

aborting(val)
register val;
{
	(void) signal(SIGINT,SIG_IGN);
	stop(val);
}

stop(val)
register val;
{
	clear();
	refresh();
	endwin();
	resetty();
	exit(val);
}


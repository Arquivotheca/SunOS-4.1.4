#ifndef lint
static	char sccsid[] = "@(#)ttycontrol.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#include <sgtty.h>

static int tty = -1;

struct ttystate {
	struct sgttyb	sgttyb;
	struct tchars	tchars;
	long		lword;
	struct ltchars	ltchars;
};

static struct ttystate mystate, substate;

static
savettystate(t)
	struct ttystate *t;
{

	if (tty < 0)
		return;
	ioctl(tty, TIOCGETP, &(t->sgttyb));
	ioctl(tty, TIOCGETC, &(t->tchars));
	ioctl(tty, TIOCLGET, &(t->lword));
	ioctl(tty, TIOCGLTC, &(t->ltchars));
}

static
putttystate(c, t)
	register struct ttystate *c, *t;
{
#define NEQ(a) (c->a != t->a)

	if (tty < 0)
		return;
	if (NEQ(sgttyb.sg_ispeed) || NEQ(sgttyb.sg_ospeed) ||
	    NEQ(sgttyb.sg_erase) || NEQ(sgttyb.sg_kill) ||
	    NEQ(sgttyb.sg_flags))
		ioctl(tty, TIOCSETN, &(t->sgttyb));
	if (NEQ(tchars.t_intrc) || NEQ(tchars.t_quitc) || NEQ(tchars.t_startc)||
	    NEQ(tchars.t_stopc) || NEQ(tchars.t_eofc) || NEQ(tchars.t_brkc))
		ioctl(tty, TIOCSETC, &(t->tchars));
	if (NEQ(lword))
		ioctl(tty, TIOCLSET, &(t->lword));
	if (NEQ(ltchars.t_suspc) || NEQ(ltchars.t_dsuspc) ||
	    NEQ(ltchars.t_rprntc)|| NEQ(ltchars.t_flushc) ||
	    NEQ(ltchars.t_werasc) || NEQ(ltchars.t_lnextc))
		ioctl(tty, TIOCSLTC, &(t->ltchars));
#undef NEQ
}

/*
 * find the tty. Save current state as ``mystate''.
 */
setty()
{
	register int i;

	if (tty < 0) {
		for (i = 0; i <= 2; i++)
			if (isatty(i)) {
				break;
			}
		if (i <= 2)
			tty = dup(i);
		else
			return -1;
	}
	if (tty < 0)
		return -1; /* dup failed - no open handle */
	savettystate(&mystate);
	return 0;
}

/*
 * we're firing up a new sub-process. it gets a clean tty
 */
newsubtty()
{

	substate = mystate;
}

/* close the tty fd in the sub-process before the exec */
closettyfd()
{
	if(tty >= 0) (void) close(tty);
	tty = -1;
}

/*
 * we're about to run the subprocess. give it its own tty state
 */
subtty()
{

	putttystate(&mystate, &substate);
}

/*
 * control is returning to adb. save user's tty state and reinstitute my own
 */
adbtty()
{

	savettystate(&substate);
	putttystate(&substate, &mystate);
}

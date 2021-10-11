#ifndef lint
static	char sccsid[] = "@(#)tty_ptyconf.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Pseudo-terminal driver.
 *
 * Configuration dependent variables
 */

#include "pty.h"
#include <sys/param.h>
#include <sys/termios.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/tty.h>
#include <sys/ptyvar.h>

#if NPTY == 1
#undef NPTY
#define	NPTY	48		/* crude XXX */
#endif

int	npty = NPTY;

struct	pty pty_softc[NPTY];

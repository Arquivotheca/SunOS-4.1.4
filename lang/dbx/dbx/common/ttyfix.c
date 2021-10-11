#ifndef lint
static	char sccsid[] = "@(#)ttyfix.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Routines to save & restore the terminal modes
 */

#include "defs.h"
#include <fcntl.h>
#include <signal.h>
#include "ttyfix.h"

#ifndef public	/* Export this stuff */

#include <sys/termios.h>

typedef struct {
	int was_saved;
	struct termios	term_modes;
	int		fd_flags;
} Ttyinfo;

#endif

private Ttyinfo ttyinfo;
private int	tty_fd;			/* Controlling tty file descriptor */


/*
 * Save the state of a tty.
 */
public savetty(t)
Ttyinfo *t;
{
    if( t == 0 ) {
	t = &ttyinfo;
    }
    if (tty_fd == 0) {
	tty_fd = open("/dev/tty", 0);
    }
    if (tty_fd != -1) {
	ioctl( tty_fd, TCGETS, &( t->term_modes ) );
	t->fd_flags = fcntl( 0, F_GETFL, 0 );
	t->was_saved = 1;
    }
}

/*
 * Restore the state of a tty.
 */
public restoretty(t)
Ttyinfo *t;
{
    int set_how, was_canon, new_canon;
    struct termios was_modes;
	int mask;

	if( t == 0 ) {
	    t = &ttyinfo;
	}
    if (tty_fd != -1 && t->was_saved) {
	ioctl( tty_fd, TCGETS, &was_modes );

	/*
	 * If it *was* in "raw" (any non-canonical) mode,
	 * and we're changing it to a canonical (non-"raw") mode,
	 * then flush any typeahead when we change the mode.
	 */
	was_canon = (was_modes.c_lflag) & ICANON ;
	new_canon = (t->term_modes.c_lflag) & ICANON ;
	if( was_canon == 0  &&  new_canon != 0 ) {
	    set_how = TCSETSF;		/* flush when mode changes */
	} else {
	    set_how = TCSETS;
	}

	mask = sigblock(sigmask(SIGTTOU));
	ioctl( tty_fd, set_how, &( t->term_modes ) );
	fcntl( 0, F_SETFL, t->fd_flags );
	sigsetmask(mask);
    }
}
#ifdef DEBUG
dump_termios(fd, t)
	struct termios *t;
{
int i;

	printf( "fd=%d %x  %x %x %x %x \n", fd,
		t->c_iflag, t->c_oflag, t->c_cflag, t->c_lflag, t->c_line
	);
	for(i=0;i<NCCS;i++) printf("%x ", t->c_cc[i]);
	printf("\n");
	fflush(stdout);
}
#endif

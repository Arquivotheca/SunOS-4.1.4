#ifndef lint
static	char sccsid[] = "@(#)conf.c	1.1 94/10/31	SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Configuration table for standalone I/O system.
 *
 * This table lists all the supported drivers.  It is searched by open()
 * to parse the device specification.
 */

#include <stand/saio.h>

extern struct boottab xddriver;
extern struct boottab xydriver;
extern struct boottab sddriver;
extern struct boottab stdriver;
extern struct boottab mtdriver;
extern struct boottab xtdriver;
extern struct boottab ipidriver;

/*
 * The device table 
 */
struct boottab *(devsw[]) = {
	&ipidriver,
	&xddriver,
	&xydriver,
	&sddriver,
	&stdriver,
	&mtdriver,
	&xtdriver,
	(struct boottab *)0,
};

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)move.c 1.1 94/10/31 SMI"; /* from UCB 5.1 5/7/85 */
#endif not lint

#include "hp7221.h"

move(xi,yi)
int xi,yi;
{
	currentx = scaleX(xi);
	currenty = scaleY(yi);
	putchar( 'p' );
	putMBP( currentx, currenty );
}

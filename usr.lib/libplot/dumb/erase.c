/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)erase.c 1.1 94/10/31 SMI"; /* from UCB 5.1 5/7/85 */
#endif not lint

#include "dumb.h"

erase()
{
	register int i, j;

	for(i=0;i<COLS;i++)
		for(j=0;j<LINES;j++)
			screenmat[i][j] = ' ';
}

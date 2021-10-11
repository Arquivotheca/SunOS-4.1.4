#ifndef lint
static char sccsid[] = "@(#)des.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include "des_crypt.h"

#define DES_CBC	0
#define DES_ECB 1


int ifd=0, ofd=1;		/* input and output descriptors */
char *cmdname;			/* our command name */


struct des_info {
	char *key;			/* encryption key */
	char *ivec;			/* initialization vector: CBC mode only */
	unsigned flags;		/* direction, device flags */
	unsigned mode;		/* des mode: ecb or cbc */
} g_des;

		
main(argc, argv)
	char **argv;
{
	char *key = NULL, keybuf[8], *getpass();
	unsigned mode = DES_CBC;	
	unsigned flags = DES_HW;
	int dirset = 0; 	/* set if dir set */
	int fflg = 0;	/* suppress warning if H/W DES not available */
	unsigned err;

	exit(0);
	/* NOTREACHED */
}


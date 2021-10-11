#ifndef lint
static  char sccsid[] = "@(#)cerror.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * modified version of cerror
 *
 * The idea is that every time an error occurs in a system call
 * I want a special function "syserr" called.  This function will
 * either print a message and exit or do nothing depending on
 * defaults and use of "onsyserr".
 *
 * So why in hell did it have to be in assembly language!?
 */
int errno ;

cerror ( ) {
	syserr( errno );
	return -1;
}

_mycerror ( ) { }

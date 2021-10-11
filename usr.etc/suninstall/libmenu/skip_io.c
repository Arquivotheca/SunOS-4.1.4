#ifndef lint
static	char	sccsid[] = "@(#)skip_io.c 1.1 94/10/31";
#endif

/*
 *	Name:		skip_io.c
 *
 *	Description:	Return a value that causes I/O for an object to
 *		be skipped.
 *
 *	Call syntax:	return(menu_skip_io());
 */

#include "menu.h"
#include "menu_impl.h"


int
menu_skip_io()
{
	return(MENU_SKIP_IO);
} /* end menu_skip_io() */

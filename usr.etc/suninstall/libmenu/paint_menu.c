#ifndef lint
static	char	sccsid[] = "@(#)paint_menu.c 1.1 94/10/31";
#endif

/*
 *	Name:		paint_menu.c
 *
 *	Description:	Paint the menu on the screen in an optimal way.
 *
 *	Call syntax:	paint_menu();
 */

#include <curses.h>




void
paint_menu()
{
	refresh();
} /* end paint_menu() */

#ifndef lint
static	char sccsid[] = "@(#)menu.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <suntool/sunview.h>
#include <suntool/ttysw.h>

#undef ord
#include "defs.h"
#include "pipeout.h"

#include "typedefs.h"
#include "dbxlib.h"
#include "selection.h"

#define DBXTOOL_MENU_NROWS	10

typedef	struct	Dbxmenu	*Dbxmenu;

struct	Dbxmenu	{
	char	*text;			/* Text of menu */
	Strfunc	sel_inter;		/* Selection interpretation routine */
	Menu_item handle;		/* Handle per item */
	Dbxmenu	next;			/* Linked list */
};

/*
 * The default menu.
 */
private struct 	Dbxmenu	def_menus[] = {
	{ "display",	sel_expand },
	{ "undisplay",	sel_expand },
	{ "file",	sel_expand },
	{ "func",	sel_expand },
	{ "status",	sel_ignore },
	{ "cont at",	sel_lineno },
	{ "make",	sel_ignore },
	{ "kill",	sel_ignore },
	{ "list",	sel_expand },
	{ "help",	sel_ignore },
	{ nil,		nil },
};

private Dbxmenu menu_hdr;		/* List of menu items */
private Dbxmenu	*menu_bpatch = &menu_hdr;	/* Place to a new menu item */
private	int	nmenu_items;		/* # of items in menu */
private Menu	menu;			/* The menu itself */

private int	menu_cmd();

/*
 * Initialize the menu to its default values.
 */
public menu_default()
{
	Dbxmenu	menup;

	menu = menu_create(
		MENU_NROWS, DBXTOOL_MENU_NROWS,
		0);
	menu_hdr = def_menus;
	nmenu_items++;
	menu_bpatch = &def_menus[0].next;
	for (menup = &def_menus[1]; menup->text != nil; menup++) {
		menup->handle = menu_create_item(
			MENU_ACTION_ITEM, menup->text, menu_cmd,
			MENU_CLIENT_DATA, menup,
			0);
		menu_set(menu, 
			MENU_APPEND_ITEM, menup->handle,
			0);
		*menu_bpatch = menup;
		menu_bpatch = &menup->next;
		nmenu_items++;
	}
	menu_hdr = &def_menus[0];

	/*
	 * Dynamically allocate the strings for the default
	 * buttons so the deallocator doesn't need to know 
	 * the difference.
	 */
	for (menup = def_menus; menup->text != nil; menup++) {
		menup->text = strdup(menup->text);
	}
}

/*
 * Return a handle on the menu
 */
public	Menu get_menu()
{
	return(menu);
}

/*
 * Notify proc for menu items.
 */
private menu_cmd(menu, menu_item)
Menu	menu;
Menu_item menu_item;
{
	Dbxmenu	dbxmenu;

	dbxmenu = (Dbxmenu) menu_get(menu_item, MENU_CLIENT_DATA);
	interpret_selection(dbxmenu->sel_inter, dbxmenu->text);
}

/*
 * Add an item to the menu.
 */
public dbx_new_menu(seltype, str)
Seltype	seltype;
char	*str;
{
	Dbxmenu dbxmenu;

	dbxmenu = new(Dbxmenu);
	dbxmenu->sel_inter = sel_func(seltype);
	dbxmenu->text = strdup(str);
	dbxmenu->next = nil;
	dbxmenu->handle = menu_create_item(
		MENU_ACTION_ITEM, dbxmenu->text, menu_cmd,
		MENU_CLIENT_DATA, dbxmenu,
		0);
	menu_set(menu, 
		MENU_APPEND_ITEM, dbxmenu->handle,
		0);
	*menu_bpatch = dbxmenu;
	menu_bpatch = &dbxmenu->next;
}

/*
 * Remove an item from the menu.
 */
public dbx_unmenu(str)
char	*str;
{
	Dbxmenu	dbxmenu;
	Dbxmenu *bpatch;

	bpatch = &menu_hdr;
	for (dbxmenu = menu_hdr; dbxmenu != nil; dbxmenu = dbxmenu->next) {
		if (streq(str, dbxmenu->text)) {
			*bpatch = dbxmenu->next;
			menu_set(menu,
			    MENU_REMOVE_ITEM, dbxmenu->handle,
			    0);
			menu_destroy(dbxmenu->handle);
			dispose(dbxmenu->text);
			dispose(dbxmenu);
			break;
		}
		bpatch = &dbxmenu->next;
	}
}

#ifndef lint
static	char	sccsid[] = "@(#)on_form.c 1.1 94/10/31";
#endif

/*
 *	Name:		on_form.c
 *
 *	Description:	Turn on (activate) a form.
 *
 *	Call syntax:	on_form(form_p);
 *
 *	Parameters:	form *		form_p;
 */

#include <curses.h>
#include "menu.h"


void
on_form(form_p)
	form *		form_p;
{
	form_field *	field_p;		/* ptr to a field */
	menu_file *	file_p;			/* ptr to file object */
	form_noecho *	noecho_p;		/* ptr to a noecho object */
	form_radio *	radio_p;		/* ptr to a radio */
	menu_string *	string_p;		/* ptr to a string */
	int		x_coord, y_coord;	/* saved coordinates */
	form_yesno *	yesno_p;		/* ptr to a yes/no question */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	for (string_p = form_p->f_mstrings; string_p;
	     string_p = string_p->ms_next) {
		on_menu_string(string_p);
	}
	for (field_p = form_p->f_fields; field_p;
	     field_p = field_p->ff_next) {
		on_form_field(field_p);
	}
	for (file_p = form_p->f_files; file_p;
	     file_p = file_p->mf_next) {
		on_menu_file(file_p);
	}
	for (noecho_p = form_p->f_noechos; noecho_p;
	     noecho_p = noecho_p->fne_next) {
		on_form_noecho(noecho_p);
	}
	for (radio_p = form_p->f_radios; radio_p;
	     radio_p = radio_p->fr_next) {
		on_form_radio(radio_p);
	}
	for (yesno_p = form_p->f_yesnos; yesno_p;
	     yesno_p = yesno_p->fyn_next) {
		on_form_yesno(yesno_p);
	}

	display_form(form_p);

	move(y_coord, x_coord);			/* put cursor back */
} /* end on_form() */

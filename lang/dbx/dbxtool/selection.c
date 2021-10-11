#ifndef lint
static	char sccsid[] = "@(#)selection.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/selection_svc.h>
#include <suntool/selection_attributes.h>
#include <suntool/ttysw.h>
#include <suntool/textsw.h>
#include <ctype.h>

#include "defs.h"
#include "pipeout.h"

#include "typedefs.h"
#include "pipe.h"
#include "cmd.h"

private	char	prompt[] = "(dbxtool) ";
private char	selbuf[256];

typedef	Seln_request *Selreq;

/*
 * Ignore the selection entirely.
 * ARGSUSED
 */
public	char	*sel_ignore()
{
	return("");
}

/*
 * Use the selection text as is, regardless of where it came from.
 */
public	char	*sel_literal()
{
	
	Seln_holder holder;
	Selreq	selreq;
	Attr_avlist avlist;

        holder = seln_inquire(SELN_PRIMARY);
	selreq = seln_ask(&holder, 
		SELN_REQ_CONTENTS_ASCII,	0,
		0);
	avlist = (Attr_avlist) &selreq->data[0];
	if ((Seln_attribute) *avlist == SELN_REQ_CONTENTS_ASCII) {
		avlist++;
		strcpy(selbuf, (char *) avlist);
		return(selbuf);
	}
	selection_error("literal");
	return(nil);
}

/*
 * Use the line number of the selected text.
 * Create a string of the form
 *	"filename":lineno
 */
public	char	*sel_lineno()
{
	Seln_holder holder;
	Selreq	selreq;
	Attr_avlist avlist;
	char	*filename;
	char	*fullcurfile;
	int	lineno;

        holder = seln_inquire(SELN_PRIMARY);
	selreq = seln_ask(&holder, 
		SELN_REQ_FILE_NAME,	0,
		SELN_REQ_FAKE_LEVEL,	SELN_LEVEL_LINE,
		SELN_REQ_FIRST_UNIT,	0,
		0);
	avlist = (Attr_avlist) &selreq->data[0];
	if ((Seln_attribute) *avlist != SELN_REQ_FILE_NAME) {
		selection_error("lineno");
		return(nil);
	}
	filename = (char *) &avlist[1];

	avlist = attr_skip((Attr_attribute) *avlist,  &avlist[1]);
	if ((Seln_attribute) *avlist != SELN_REQ_FAKE_LEVEL) {
		selection_error("lineno");
		return(nil);
	}

	avlist = attr_skip((Attr_attribute) *avlist,  &avlist[1]);
	if ((Seln_attribute) *avlist != SELN_REQ_FIRST_UNIT) {
		selection_error("lineno");
		return(nil);
	}
	lineno = 1 + (int) avlist[1];
	fullcurfile = get_curfile();
	if (fullcurfile != nil and streq(fullcurfile, filename)) {
		filename = get_simple_curfile();
	}
	sprintf(selbuf, "\"%s\":%d", filename, lineno);
	return(selbuf);
}

/*
 * Expand the text.
 * Check the end points (first and last character).
 * If an end point is a character, number or underscore,
 * then expand it to include as many characters, numbers or underscores
 * as possible.
 *
 * Get the actual selection and its character position and get
 * the line containing the beginning of the selection and its character
 * position.  From that, do the expansion.
 */
public	char	*sel_expand()
{
	Seln_holder holder;
	Selreq	selreq;
	Attr_avlist avlist;
	char	*start;
	char	*cp;
	char	*sel_line;		/* The full line */
	int	sel_begtext;		/* Char pos of beginning of selection*/
	int	sel_endtext;		/* Char pos of end of selection */
	int	sel_begline;		/* Char pos to beginning of sel_line */
	int	ix;

        holder = seln_inquire(SELN_PRIMARY);
	selreq = seln_ask(&holder, 
		SELN_REQ_CONTENTS_ASCII,	0,
		SELN_REQ_FIRST,			0,
		SELN_REQ_LAST,			0,
		SELN_REQ_FAKE_LEVEL,		SELN_LEVEL_LINE,
		SELN_REQ_CONTENTS_ASCII,	0,
		SELN_REQ_FIRST,			0,
		0);
	/*
	 * Check each of the requests one by one to see if they
	 * were fulfilled.  After the first CONTENTS_ASCII request,
	 * if one fails, return the selected text.
	 */
	avlist = (Attr_avlist) &selreq->data[0];
	if ((Seln_attribute) *avlist != SELN_REQ_CONTENTS_ASCII) {
		selection_error("expand");
		return(nil);
	}
	strcpy(selbuf, (char *) &avlist[1]);

	avlist = attr_skip((Attr_attribute) *avlist,  &avlist[1]);
	if ((Seln_attribute) *avlist != SELN_REQ_FIRST) {
		expand_error();
		return(selbuf);
	}
	sel_begtext = (int) avlist[1];

	avlist = attr_skip((Attr_attribute) *avlist,  &avlist[1]);
	if ((Seln_attribute) *avlist != SELN_REQ_LAST) {
		expand_error();
		return(selbuf);
	}
	sel_endtext = (int) avlist[1];
	if (sel_begtext > sel_endtext) {
		/* Selection is empty or malformed */
		expand_error();
		return(selbuf);
	}

	avlist = attr_skip((Attr_attribute) *avlist,  &avlist[1]);
	if ((Seln_attribute) *avlist != SELN_REQ_FAKE_LEVEL) {
		expand_error();
		return(selbuf);
	}

	avlist = attr_skip((Attr_attribute) *avlist,  &avlist[1]);
	if ((Seln_attribute) *avlist != SELN_REQ_CONTENTS_ASCII) {
		expand_error();
		return(selbuf);
	}
	sel_line = (char *) &avlist[1];

	avlist = attr_skip((Attr_attribute) *avlist,  &avlist[1]);
	if ((Seln_attribute) *avlist != SELN_REQ_FIRST) {
		expand_error();
		return(selbuf);
	}
	sel_begline = (int) avlist[1];

	/*
	 * Find the beginning of the token.
	 */
	ix = sel_begtext - sel_begline;
	cp = &sel_line[ix];
	if (isalnum(*cp) || *cp == '_') {
		while (isalnum(*cp) || *cp == '_') {
			cp--;
			if (cp < sel_line) {
				break;
			}
		}
		cp++;
	}
	start = cp;

	/*
	 * Find the end of the token.
	 */
	cp = &sel_line[ix + sel_endtext - sel_begtext];
	if (isalnum(*cp) || *cp == '_') {
		while (isalnum(*cp) || *cp == '_') {
			cp++;
		}
	} else {
		cp++;
	}
	*cp = '\0';
	strcpy(selbuf, start);
	return(selbuf);
}

/*
 * Use the entire line as the selection.
 * In this case see if the line begins with a prompt and, if so,
 * ignore return the line without the prompt.
 */
public char	*sel_command()
{
	Seln_holder holder;
	Selreq	selreq;
	Attr_avlist avlist;
	char	*sel_line;

        holder = seln_inquire(SELN_PRIMARY);
	selreq = seln_ask(&holder, 
		SELN_REQ_FAKE_LEVEL,		SELN_LEVEL_LINE,
		SELN_REQ_CONTENTS_ASCII,	0,
		0);
	avlist = (Attr_avlist) &selreq->data[0];
	if ((Seln_attribute) *avlist != SELN_REQ_FAKE_LEVEL) {
		selection_error("command");
		return(nil);
	}

	avlist = &avlist[2];
	if ((Seln_attribute) *avlist != SELN_REQ_CONTENTS_ASCII) {
		selection_error("command");
		return(nil);
	}
	sel_line = (char *) &avlist[1];
	if (strncmp(sel_line, prompt, strlen(prompt)) == 0) {
		strcpy(selbuf, &sel_line[strlen(prompt)]);
		return(selbuf);
	}
	selection_error("command");
	return(nil);
} 

/*
 * Convert a selection type into a function name.
 */
public Strfunc	sel_func(seltype)
Seltype	seltype;
{
	Strfunc	func;

	switch(seltype) {
	case SEL_COMMAND:
		func = sel_command;
		break;
	case SEL_EXPAND:
		func = sel_expand;
		break;
	case SEL_LINENO:
		func = sel_lineno;
		break;
	case SEL_IGNORE:
		func = sel_ignore;
		break;
	case SEL_LITERAL:
		func = sel_literal;
		break;
	default:
		func = nil;
		break;
	}
	return(func);
}

/*
 * Could not expand the selection for some reason.
 * Print a warning to that effect.
 */
private expand_error()
{
	Cmdsw	cmd;
	char	*warning = "Warning: could not expand the selection - using it literally\n";

	cmd = get_cmd_sw_h();
	ttysw_output(cmd, warning, strlen(warning));
}

/*
 * Could not get the selection for some reason.
 * Print a warning to that effect.
 */
private selection_error(str)
char	*str;
{
	Cmdsw	cmd;
	char	buf[256];

	sprintf(buf, "Could not complete the `%s' selection\n", str); 
	cmd = get_cmd_sw_h();
	ttysw_output(cmd, buf, strlen(buf));
}

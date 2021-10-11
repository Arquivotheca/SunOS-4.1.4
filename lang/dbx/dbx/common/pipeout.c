#ifndef lint
static	char sccsid[] = "@(#)pipeout.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <signal.h>

#include "defs.h"
#include "pipeout.h"
#include "lists.h"
#include "symbols.h"
#include "scanner.h"
#include "source.h"
#include "process.h"
#include "machine.h"
#include "dbxmain.h"

private Intermed rdtype();

#ifndef public
#define SIGINT_MASK	(1 << (SIGINT - 1))

enum	seltype	{
	SEL_COMMAND = 1,		/* Redo a command line */
	SEL_EXPAND,			/* Expand to largest syntactic sym */
	SEL_IGNORE,			/* Ignore the selection */
	SEL_LINENO,			/* Use line num with src sw */
	SEL_LITERAL			/* Use the literal selection */
};
typedef enum	seltype	Seltype;

/*
 * This is a version number to check for incompatibilities between
 * dbx and dbxtool.  It should be changed (incremented) any time
 * the interface between dbx and dbxtool changes.
 */
#define DBXTOOL_VERSION	3

enum 	intermed {
	I_BADOP,			/* Bad operator */
	I_INITDONE,			/* Done with initialization */
	I_STOPPED, 			/* Debuggee is stopped */
	I_STRING, 			/* Literal string */
	I_INT, 				/* Integer value */
	I_QUIT,				/* Processed the quit command */
	I_PRINTLINES,			/* Print lines from the curent file */
	I_BRKSET,			/* Breakpoint has been set */
	I_BRKDEL,			/* Breakpoint has been deleted */
	I_RESUME,			/* Execution is ready to resume */
	I_USE,				/* Give directory 'use' list */
	I_REINIT,			/* Re-initialize for another prog */
	I_KILL,				/* The debugee has been thrown away */
	I_CHDIR,			/* Change directories */
	I_EMPHASIZE,			/* Display and hilight a line */
	I_TRACE,			/* Display a trace line */
	I_DISPLAY,			/* File containing display data */
	I_VERSION,			/* Version number */
	I_TOOLENV,			/* Print the tool environment */
	I_WIDTH,			/* Width of the tool */
	I_SRCLINES,			/* # of lines in src subwindow */
	I_CMDLINES,			/* # of lines in cmd subwindow */
	I_DISPLINES,			/* # of lines in disp subwindow */
	I_FONT,				/* Change the font */
	I_TOPMARGIN,			/* Margin from top of src subwindow */
	I_BOTMARGIN,			/* Margin from bot of src subwindow */
	I_UNBUTTON,			/* Remove a button */
	I_BUTTON,			/* Add a button */
	I_CALLSTACK,			/* Put a hollow arrow on this line */
	I_NEWINITDONE,			/* Done with initialization */
	I_MENU,				/* Add a menu item */
	I_UNMENU,			/* Remove a menu item */
	I_FIRST = I_INITDONE,
	I_LAST = I_UNMENU,
};
typedef	enum	intermed Intermed;
#endif

private char	*seltype_str[] = {
	nil,
	"command",
	"expand",
	"ignore",
	"lineno",
	"literal",
};

private	char	*inter_type[] = {
	"I_BADTYPE",
	"I_INITDONE",
	"I_STOPPED",
	"I_STRING",
	"I_INT",
	"I_QUIT",
	"I_PRINTLINES",
	"I_BRKSET",
	"I_BRKDEL",
	"I_RESUME",
	"I_USE",
	"I_REINIT",
	"I_KILL",
	"I_CHDIR",
	"I_EMPHASIZE",
	"I_TRACE",
	"I_DISPLAY",
	"I_VERSION",
	"I_TOOLENV",
	"I_WIDTH",
	"I_SRCLINES",
	"I_CMDLINES",
	"I_DISPLINES",
	"I_FONT",
	"I_TOPMARGIN",
	"I_BOTMARGIN",
	"I_UNBUTTON",
	"I_BUTTON",
	"I_CALLSTACK",
	"I_MENU",
	"I_UNMENU",
};

private int 	version = DBXTOOL_VERSION;
public	int	pipeout;		/* Fd to write tool operators */
public	Boolean	istool();

/*
 * Write a string down the pipe to the interface program.
 * A string is a binary integer length followed by that many
 * characters.
 * VARARGS
 */
public	dbx_string(str, a, b, c, d, e, f, g, h)
char	*str;
{
	char	buf[512];
	int	len;
	int	mask;

	if (not istool()) {
		return;
	}
	mask = sigblock(SIGINT_MASK);
 	if (str == nil) {
		buf[0] = '\0';
	} else {
		sprintf(buf, str, a, b, c, d, e, f, g, h);
	}
	if (debugpipe) {
		fprintf(stderr, "\t%s\n", buf);
	}
	len = strlen(buf);
	dbx_type(I_STRING);
	write(pipeout, (char *) &len, sizeof(len));
	write(pipeout, buf, len);
	sigsetmask(mask);
}

/*
 * Send a type word down the pipe.
 */
public	dbx_type(type)
Intermed type;
{
	if (not istool()) {
		return;
	}
	if (debugpipe) {
		fprintf(stderr, "pipeout: %s\n", inter_type[ord(type)]);
	}
	write(pipeout, (char *) &type, sizeof(type));
}

/*
 * Send an integer value down the pipe.
 */
public	dbx_int(val)
int	val;
{
	if (not istool()) {
		return;
	}
	if (debugpipe) {
		fprintf(stderr, "\t%d\n", val);
	}
	dbx_type(I_INT);
	write(pipeout, (char *) &val, sizeof(val));
}

/*
 * Tell the interface to display certain lines from the current file.
 */
public	dbx_printlines(l1, l2)
Lineno	l1;
Lineno	l2;
{
	int	mask;

	if (not istool()) {
		return;
	}
	mask = sigblock(SIGINT_MASK);
	dbx_type(I_PRINTLINES);
	dbx_string(cursource);
	dbx_int((int) l1);
	dbx_int((int) l2);
	sigsetmask(mask);
}

/*
 * Tell the interface that we are done with initialization.
 */
public	dbx_initdone()
{
	int	mask;

	if (not istool()) {
		return;
	}
	mask = sigblock(SIGINT_MASK);
	dbx_type(I_NEWINITDONE);
	dbx_filefuncline(cursource, curfunc, cursrcline);
	dbx_filefuncline(brksource, brkfunc, brkline);
	sigsetmask(mask);
}

/*
 * Pass a file, func, lineno group down the pipe.
 */
private dbx_filefuncline(file, func, lineno)
char	*file;
Symbol	func;
int	lineno;
{
	dbx_string(file);
	if (func != nil) {
		dbx_string(idnt(func->name));
	} else {
		dbx_string("");
	}
	dbx_int(lineno);
}

/*
 * Tell the interface that we are quiting and give it 
 * and exit status.
 */
public	dbx_quit(val)
int	val;
{
	dbx_typeint(I_QUIT, val);
}

/*
 * Tell the interface about the setting or deleting of a breakpoint.
 */
public	dbx_breakpoint(file, lineno, flag)
char	*file;
int	lineno;
Boolean	flag;
{
	int	mask;

	if (not istool()) {
		return;
	}
	mask = sigblock(SIGINT_MASK);
	if (flag == true) {
		dbx_type(I_BRKSET);
	} else {
		dbx_type(I_BRKDEL);
	}
	dbx_string(file);
	dbx_int(lineno);
	sigsetmask(mask);
}

/*
 * Tell the interface that the debuggee is about to resume execution.
 * This gives dbxtool an opportunity to take the arrow down.
 */
public	dbx_resume()
{
	dbx_type(I_RESUME);
}

/*
 * Send out a type followed by an int.
 */
public	dbx_typeint(type, n)
Intermed type;
int	n;
{
	int	mask;

	if (not istool()) {
		return;
	}
	mask = sigblock(SIGINT_MASK);
	dbx_type(type);
	dbx_int(n);
	sigsetmask(mask);
}

/*
 * Set a new button.
 */
public	dbx_button(seltype, str)
Seltype	seltype;
char	*str;
{
	dbx_panel(I_BUTTON, seltype, str);
}

/*
 * Set a new menu item.
 */
public	dbx_menu(seltype, str)
Seltype	seltype;
char	*str;
{
	dbx_panel(I_MENU, seltype, str);
}

/*
 * Set a panel item in dbxtool's buttons subwindow.
 */
private	dbx_panel(type, seltype, str)
Intermed type;
Seltype	seltype;
char	*str;
{
	int	mask;

	if (not istool()) {
		return;
	}
	mask = sigblock(SIGINT_MASK);
	dbx_type(type);
	dbx_int((int) seltype);
	dbx_string(str);
	sigsetmask(mask);
}

/*
 * Remove an existing button
 */
public	dbx_unbutton(str)
char	*str;
{
	dbx_unpanel(I_UNBUTTON, str);
}

/*
 * Remove an existing menu item
 */
public	dbx_unmenu(str)
char	*str;
{
	dbx_unpanel(I_UNMENU, str);
}

private	dbx_unpanel(type, str)
Intermed type;
char	*str;
{
	int	mask;

	if (not istool()) {
		return;
	}
	mask = sigblock(SIGINT_MASK);
	dbx_type(type);
	dbx_string(str);
	sigsetmask(mask);
}

/*
 * Tell the interface we have stopped.  Give it the current
 * line number, function and file.  Also give it the line number,
 * function and file of the stoping point.
 */
public	dbx_stopped()
{
	int	mask;

	check_outofdate(cursource);
	if (not istool()) {
		return;
	}
	mask = sigblock(SIGINT_MASK);
	dbx_type(I_STOPPED);
	dbx_filefuncline(cursource, curfunc, cursrcline);
	dbx_filefuncline(brksource, brkfunc, brkline);
	sigsetmask(mask);
}

/*
 * Make an addition to the display list.
 */
public	dbx_display(file)
char	*file;
{
	int	mask;

	if (not istool()) {
		return;
	}
	mask = sigblock(SIGINT_MASK);
	dbx_type(I_DISPLAY);
	dbx_string(file);
	sigsetmask(mask);
}

/*
 * Start debugging another program or re-initialize the current
 * because it changed.
 */
public	dbx_reinit()
{
	dbx_type(I_REINIT);
}

/*
 * Kill the debugee and throw the process away.
 * This is useful to clear the screen of the remains of
 * a window program.
 */
public	dbx_kill()
{
	dbx_type(I_KILL);
}

/*
 * Change directories
 */
public dbx_chdir(dir)
char	*dir;
{
	int	mask;

	if (not istool()) {
		return;
	}
	mask = sigblock(SIGINT_MASK);
	dbx_type(I_CHDIR);
	dbx_string(dir);
	sigsetmask(mask);
}

/*
 * Give the directory 'use' list
 */
public	dbx_use(l)
List	l;
{
	int	mask;
	String	dir;

	if (not istool()) {
		return;
	}
	mask = sigblock(SIGINT_MASK);
	dbx_type(I_USE);
	dbx_int(l->nitems);
	foreach (String, dir, sourcepath)
		dbx_string(dir);
	endfor
	sigsetmask(mask);
}

/*
 * Move the given line number to the 'topmargin' location and
 * hilight for a second to emphasize it.
 */
public dbx_emphasize(lineno)
Lineno	lineno;
{
	dbx_curline(I_EMPHASIZE, lineno);
}

/*
 * Change the current line (and possibly) file so that
 * a hollow arrow will be shown.
 */
public dbx_callstack(lineno)
int	lineno;
{
	dbx_curline(I_CALLSTACK, lineno);
}

/*
 * Pass up an operator with the current file and line number as
 * arguments.
 */
private dbx_curline(type, lineno)
Intermed type;
int	lineno;
{
	int	mask;

	if (not istool()) {
		return;
	}
	mask = sigblock(SIGINT_MASK);
	dbx_type(type);
	dbx_string(cursource);
	dbx_int(lineno);
	sigsetmask(mask);
}

/*
 * Display a trace line.
 */
public dbx_trace()
{
	int	mask;

	if (not istool()) {
		return;
	}
	mask = sigblock(SIGINT_MASK);
	dbx_type(I_TRACE);
	dbx_string(cursource);
	dbx_string(idnt(curfunc->name));
	dbx_int(cursrcline);
	sigsetmask(mask);
}

/*
 * Send out the version number.
 */
public dbx_version()
{
	dbx_typeint(I_VERSION, version);
}

/*
 * Print out the name of a selection type.
 */
public pr_seltype(seltype)
Seltype seltype;
{
	if (ord(seltype) >= ord(SEL_COMMAND) and 
	    ord(seltype) <= ord(SEL_LITERAL)) {
		printf("%s", seltype_str[ord(seltype)]);
	} else {
		printf("bad seltype");
	}
}

/*
 * Are we running in dbxtool?
 */
Boolean	istool()
{
	return((Boolean) (pipeout != 0));
}

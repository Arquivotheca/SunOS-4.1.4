#ifndef lint
static	char sccsid[] = "@(#)dbxenv.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <signal.h>

#include "defs.h"

#ifdef mc68000
#  include "fpa_inst.h"
#endif mc68000

#include "dbxenv.h"
#include "lists.h"
#include "pipeout.h"
#include "source.h"
#include "tree.h"
#include "eval.h"
#include "object.h"
#include "commands.h"

extern 	char	**environ;

/*
 * Default argument passed to make
 */
private	char	*makeargs = "CC=cc -g";

private char	*get_makeargs();

/*
 * Print the values of the dbx environment variables.
 */
public dbx_prdbxenv()
{
	int	fpabase;

	dbxenv();
	printf("case\t\t%s\n", get_case()? "sensitive": "insensitive");

#ifdef mc68000
	dbxenv();
	printf("fpaasm\t\t%s\n", get_fpaasm()? "on": "off");
	dbxenv();
	fpabase = get_fpabase();
	if (fpabase == -1) {
		printf("fpabase\t\toff\n");
	} else {
		printf("fpabase\t\ta%d\n", fpabase);
	}
#endif mc68000

	dbxenv();
	printf("makeargs\t\"%s\"\n", get_makeargs());
	dbxenv();
	printf("speed\t\t%4.2f\n", get_speed());
	dbxenv();
	printf("stringlen\t%d\n", get_stringlen());
}

/*
 * If the output is redirected, begin the line with "dbxenv"
 * so that it can be read back in as a command.
 */
private	dbxenv()
{
	if (isredirected()) {
		printf("dbxenv ");
	}
}

/*
 * Check that the args to the dbxenv command are appropriate.
 */
public check_dbxenv(p)
Node p;
{
    char *str;

    switch (p->value.dbxenv.var_id) {
    case CASE:
	str = idnt(p->value.dbxenv.value->value.name);
	if (not streq(str, "insensitive") and
	  not streq(str, "sensitive")) {
	    error("Operand to `dbxenv case' must be `insensitive' or `sensitive'");
	}
	break;

#ifdef mc68000
    case FPAASM:
	str = idnt(p->value.dbxenv.value->value.name);
	if (not streq(str, "on") and not streq(str, "off")) {
	    error("Operand to `dbxenv fpaasm' must be `on' or `off'");
	}
	break;

    case FPABASE:
	str = idnt(p->value.dbxenv.value->value.name);
	if (not streq("off", str) and
	  (str[0] != 'a' or str[1] < '0' or str[1] > '7' or str[2] != '\0')) {
	    error("Operand to `dbxenv fpabase' must be `off' or one of `a0-a7'");
	}
	break;
#endif mc68000
    }
}

/*
 * Evaluate a dbxenv command.
 */
public eval_dbxenv(p)
Node p;
{
    Node n;
    Boolean bool;
    char *str;
    int fpabase;

    n = p->value.dbxenv.value;
    switch (p->value.dbxenv.var_id) {
    case CASE:
	bool = (Boolean) (streq("sensitive", idnt(n->value.name)));
	set_case(bool);
	break;

    case STRINGLEN:
	set_stringlen(n->value.lcon);
	break;

    case SPEED: {
	    double workaround = n->value.fcon;
	    set_speed( workaround );
	}
	break;

#ifdef mc68000
    case FPAASM:
	bool = (Boolean) (streq("on", idnt(n->value.name)));
	set_fpaasm(bool);
	break;

    case FPABASE:
	str = idnt(n->value.name);
	if (streq("off", str)) {
	    fpabase = -1;
	} else {
	    fpabase = str[1] - '0';
	}
	set_fpabase(fpabase);
	break;
#endif mc68000

    case MAKEARGS:
	str = n->value.scon;
	set_makeargs(str);
	break;
    }
}

/*
 * Print a dbxenv command.
 */
public print_dbxenv_cmd(cmd)
Node cmd;
{
    Node p;

    p = cmd->value.dbxenv.value;
    switch (cmd->value.dbxenv.var_id) {
    case CASE:
	printf("case %s", idnt(p->value.name));
	break;

    case STRINGLEN:
	printf("stringlen %d", p->value.lcon);
	break;

    case SPEED:
	printf("speed %4.2f", p->value.fcon);
	break;

#ifdef mc68000
    case FPAASM:
	printf("fpaasm %s", idnt(p->value.name));
	break;

    case FPABASE:
	printf("fpabase %s", idnt(p->value.name));
	break;
#endif mc68000

    case MAKEARGS:
	printf("makeargs \"%s\"", p->value.scon);
	break;
    }
}

/*
 * Print the values of the toolenv variables
 */
public dbx_prtoolenv()
{
	dbx_type(I_TOOLENV);
}

/*
 * Set the bottom margin in the src subwindow
 */
public dbx_botmargin(n)
int	n;
{
	dbx_typeint(I_BOTMARGIN, n);
}

/*
 * Set the number of lines in the cmd subwindow
 */
public dbx_cmdlines(n)
int	n;
{
	dbx_typeint(I_CMDLINES, n);
}

/*
 * Set the number of lines in the display subwindow
 */
public dbx_displines(n)
int	n;
{
	dbx_typeint(I_DISPLINES, n);
}

/*
 * Set the font used in dbxtool
 */
public dbx_font(file)
char	*file;
{
	int	mask;

	if (not istool()) {
		return;
	}
	mask = sigblock(SIGINT_MASK);
	dbx_type(I_FONT);
	dbx_string(file);
	sigsetmask(mask);
}

/*
 * Set the number of lines in the src subwindow
 */
public dbx_srclines(n)
int	n;
{
	dbx_typeint(I_SRCLINES, n);
}

/*
 * Set the top margin in the src subwindow
 */
public dbx_topmargin(n)
int	n;
{
	dbx_typeint(I_TOPMARGIN, n);
}

/*
 * Set the width of dbxtool
 */
public dbx_width(n)
int	n;
{
	dbx_typeint(I_WIDTH, n);
}

/*
 * Process the use command.
 */
public use(p)
List	p;
{
	String dir;

	if (list_size(p) == 0) {
	    	pr_use();
	} else {
	    	foreach (String, dir, sourcepath)
			list_delete(list_curitem(sourcepath), sourcepath);
	    	endfor
	    	sourcepath = p;
		prevsource = nil;
		dbx_use(p);
	}
}

/*
 * Print the list of 'use' directories.
 */
public pr_use()
{
	String dir;

	if (isredirected()) {
		printf("use ");
	}
	foreach (String, dir, sourcepath)
		printf("%s ", dir);
	endfor
	printf("\n");
}

private char	strlist[5000];		/* Buffer to accumulate strings into */
private	char	*strp;			/* Pointer within strlist[] */

/*
 * Initialize the string list
 */
public init_strlist()
{
	strlist[0] = '\0';
	strp = strlist;
}

/*
 * Add a string to the string list
 * If the string list is empty, simply copy the string.
 * Otherwise, add a blank.
 */
public	char	*add_strlist(str)
char	*str;
{
	int n = strlen (str);

	if (strp != strlist) {
		*strp++ = ' ';
		*strp = '\0';
	}
	if((strp + n ) >= (strlist + sizeof(strlist)) ) {
		error("command too long");
	}
	strcpy(strp, str);
	strp += n;
	return(strlist);
}


/*
 * Process the 'make' command.
 * ARGSUSED.
 */
public	make(p)
Node	p;
{
	char	cmd[100];
	char	*slash;
	char	c;

	if (objname == nil) {
		error("No program to make");
	}
	slash = rindex(objname, '/');
	if (slash == nil) {
		sprintf(cmd, "make \"%s\" %s", get_makeargs(), objname);
	} else {
		c = *slash;
		*slash = '\0';
		sprintf(cmd, "(cd %s; make \"%s\" %s)", objname, get_makeargs(),
		    &slash[1]);
		*slash = c;
	}
	printf("%s\n", cmd);
	fflush(stdout);
	system(cmd);
}

/*
 * Set the make arguments.
 */
private set_makeargs(str)
char	*str;
{
	makeargs = str;
}

/*
 * Return the make arguments.
 */
private char *get_makeargs()
{
	return(makeargs);
}

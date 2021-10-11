#ident "@(#)cb_stubs.c 1.1 94/10/31 SMI"

#ifndef lint
#ifdef INCLUDE_COPYRIGHT
static char _copyright_notice_[] =
"Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved. \
Sun considers its source code as an unpublished, proprietary \
trade secret, and it is available only under strict license \
provisions.  This copyright notice is placed here only to protect \
Sun in the event the source is deemed a published work.  Dissassembly, \
decompilation, or other means of reducing the object code to human \
readable form is prohibited by the license agreement under which \
this code is provided to the user or company in possession of this \
copy. \
RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the \
Government is subject to restrictions as set forth in subparagraph \
(c)(1)(ii) of the Rights in Technical Data and Computer Software \
clause at DFARS 52.227-7013 and in similar clauses in the FAR and \
NASA FAR Supplement.";
#endif

#ifdef INCLUDE_COPYRIGHT_REFERENCE
static char _copyright_notice_reference_[] =
"Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved. \
See copyright notice in cb_copyright.o in the libsb.a library.";
#endif
#endif


/*
 * This file defines stubs for a few functions that are used to
 * communicate between the cblib stuff and the browser.
 */

#ifndef cb_libc_h_INCLUDED
#include "cb_libc.h"
#endif

#ifndef cb_misc_h_INCLUDED
#include "cb_misc.h"
#endif

#ifndef cb_string_h_INCLUDED
#include "cb_string.h"
#endif

#ifndef cb_stubs_h_INCLUDED
#include "cb_stubs.h"
#endif


int		cb_abort_query;

/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	int		qr_starting_update();
extern	int		qr_state_file_locked();
extern	int		qr_message_no_database();
extern	int		cb_message();

/*
 *	qr_starting_update()
 *		This function should be called when a quickref update
 *		operation starts.
 */
int
qr_starting_update()
{
}

/*
 *	qr_state_file_locked()
 *		This is called whenever the state file appears to be locked.
 */
int
qr_state_file_locked()
{
}

/*
 *	qr_message_no_database()
 *		This routine is called whenever there is not database
 *		available.
 */
int
qr_message_no_database()
{
	cb_fatal("No SunSourceBrowser database in this directory.\n");
	exit(1);
	/* NOTREACHED */
}

/*
 *	cb_message(format, args, ...)
 *		Used for informative messages.
 *		The browser has a private definition of this routine.
 */
/*VARARGS0*/
int
cb_message(va_alist)
	va_dcl
{
	va_list		args;		/* Argument list */
	char		*format;	/* Format string */

	(void)fflush(stdout);
	va_start(args);
	format = va_arg(args, char *);
	(void)vfprintf(stderr, format, args);
	va_end(args);
	(void)fflush(stderr);
}

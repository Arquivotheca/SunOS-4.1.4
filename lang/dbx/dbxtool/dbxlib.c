#ifndef lint
static	char sccsid[] = "@(#)dbxlib.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <suntool/sunview.h>
#include <suntool/textsw.h>

#include "defs.h"
#include "src.h"

/*
 * List a range of lines for the 'list' command.
 * ARGSUSED
 */
public	dbx_printlines(file, l1)
char	*file;
int	l1;
{
	Textsw	srctext = get_srctext();

	if (file[0] == '\0') {
		return;
	}
	if (file_in_src(file)) {
		text_display(srctext, l1, 0);
	} else {
		dbx_load_src_file(file, l1);
	}
}

/*
 * Dbx has terminated, cleanup
 */
public	dbx_cleanup()
{
}

/*
 * malloc checking version of strdup. See also dbx/common/library.c
 * either this, or #include <strings.h> to get lib version of strdup
 */
public char *strdup(string)
	char *string;
{	char *new_string;

	new_string = malloc(strlen(string) + 1);
	if (new_string == 0) { 
	    fprintf(stderr, "No memory available. Out of swap space?");
	    fflush(stderr);
	    exit(1);
	}
	(void)strcpy(new_string, string);
	return new_string;
}

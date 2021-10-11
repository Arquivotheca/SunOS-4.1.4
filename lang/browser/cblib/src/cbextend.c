#ident "@(#)cbextend.c 1.1 94/10/31 SMI"

#ifndef lint
#define INCLUDE_COPYRIGHT
#ifdef INCLUDE_COPYRIGHT
static char _copyright_notice_[] =
"Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved.\
Sun considers its source code as an unpublished, proprietary\
trade secret, and it is available only under strict license\
provisions.  This copyright notice is placed here only to protect\
Sun in the event the source is deemed a published work.  Dissassembly,\
decompilation, or other means of reducing the object code to human\
readable form is prohibited by the license agreement under which\
this code is provided to the user or company in possession of this\
copy.\
RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the\
Government is subject to restrictions as set forth in subparagraph\
(c)(1)(ii) of the Rights in Technical Data and Computer Software\
clause at DFARS 52.227-7013 and in similar clauses in the FAR and\
NASA FAR Supplement.";
#endif
#endif

#include <vroot.h>

#ifndef CB_BOOT
#ifndef cb_enter_h_INCLUDED
#include "cb_enter.h"
#endif
#endif

#ifndef cb_extend_h_INCLUDED
#include "cb_extend.h"
#endif

#ifndef cb_libc_h_INCLUDED
#include "cb_libc.h"
#endif

#ifndef cb_misc_h_INCLUDED
#include "cb_misc.h"
#endif

extern	FILE		*yyin;
extern	char		*sprintf();

/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	void		main();

char		*cb_program_name = "sbextend";

void
main(argc, argv)
	int	argc;
	char	*argv[];
{
	char		command[1024];
	int		file_count = 0;
	char		*write_h = ".";

	if (argc == 1) {
		(void)printf("sbextend [-Dsym] [-Usym] [-Idir] src-file dest-dir\n");
		exit(0);
	}
	(int)access_vroot("/lib/cpp", X_OK, VROOT_DEFAULT, VROOT_DEFAULT);
	(void)strcpy(command, get_vroot_path(NULL, NULL, NULL));
#ifndef CB_BOOT
	(void)strcat(command, " -cb");
#endif
	for (argv++, argc--; argc > 0; argv++, argc--) {
		if (argv[0][0] == '-') {
			(void)strcat(command, " ");
			(void)strcat(command, argv[0]);
			continue;
		}
		switch (file_count++) {
		case 0:
			(void)strcat(command, " ");
			(void)strcat(command, argv[0]);
			break;
		case 1:
			write_h = argv[0];
			break;
		default:
			cb_fatal("Too many source files\n");
		}
	}
	(void)strcat(command, " -I/usr/include/sbextend");
	yyin = popen(command, "r");
	cb_ex_init();
	if (yyparse()) {
		(void)printf("Parse failed\n");
		exit(1);
	} else {
		cb_ex_write_files(write_h);
#ifndef CB_BOOT
		cb_enter_pop_file();
#endif
		exit(0);
	}
}

/*ARGSUSED*/
cb_callback_write_stab(cb_file)
	char	*cb_file;
{
}

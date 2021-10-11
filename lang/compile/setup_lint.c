#ifndef lint
static	char sccsid[] = "@(#)setup_lint.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "driver.h"
#include <sys/file.h>

/*
 *	setup_cat_for_lint
 *		Since lint2 only takes one in arg we cat all the .ln files
 *		together first
 */
char *
setup_cat_for_lint()
{
	register ListP		p;

	for (p= infile_ln; p != NULL; p= p->next)
		append_list(&program.cat.infile, p->value);
	return program.cat.outfile= temp_file_name(&program.cat, &suffix.ln, 0);
}	/* setup_cat_for_lint */

	
/* This is a special program to determine what software release
 * we are going to be using. It is called from do_infiles.
 * The sw release is not known at the time it is first encountered
 * (parsing the command line), so we will get it know. This
 * function replaces the DUMMY_FLAG with the proper directory locations
 * for the specified sw release
 */
char *
special_lint_l(file)
	char	*file;

{
	char	*lint_path;
	char	path[MAXPATHLEN];
	char	*temp;

        switch (target_sw_release[R_PATHS].value->value)
	{
		case SW_REL_3X:
		case SW_REL_40:
			if (is_on(sys5_flag))
			{
				/* system 5 and release 4.0 or 3.x */
				lint_path = "/usr/5lib";
			}
			else
			{
				/* bsd and release 4.0 or 3.x */
				lint_path = "/usr/lib";
			}
			break;

		case SW_REL_41:
			if (is_on(sys5_flag))
			{
				/* system 5 and release 4.1 */
				lint_path = "/usr/5lib";
			}		
			else
			{
				/* bsd and release 4.1 */
				lint_path = "/usr/lib";
			}
			break;
		default:
			fatal("Invalid sw release specified for lint library");

	}

	temp = rindex(file,'/');
	temp++;
        (void)sprintf(path, "%s/lint/%s", lint_path, temp);

	return make_string(path);
}



/*
 *	lint_doit
 */
void
lint_doit()
{
	temp_dir= "/usr/tmp";
	(void)dup2(fileno(stdout), fileno(stderr));

	if (make_lint_lib_fd)
	{
		/* Open the outfile if creating a lint library */
		if (is_on(dryrun))
			make_lint_lib_fd= 1;
		else
			make_lint_lib_fd= open(outfile,
						O_WRONLY|O_CREAT|O_TRUNC, 0666);
		if (make_lint_lib_fd == -1)
			fatal("Cannot open %s for write", outfile);
	}

	if (requested_suffix == NULL)
	{
		set_requested_suffix(&suffix.i);
	}
	else
	{
		if (requested_suffix == &suffix.ln)
			set_named_int(product, &lint1_file);
	}

	do_infiles();

	/* Make sure the lint2 run sees -lc.
	 * This must come *after* the call to do_infiles(), so that all user
	 * function definitions are seen before the library ones -- in case
	 * the user used a function of the same name as one of the library
	 * routines.
	 */
	if (is_off(ignore_lc) && (product.value != &lint1_file))
	{
		collect_ln(lint_lib("-lc"), &suffix.ln);
	}

	if (make_lint_lib_fd)
	{
		(void)close(make_lint_lib_fd);
	}
	else
	{
		if ((product.value != &lint1_file) && (infile_ln != NULL))
		{
			requested_suffix= &suffix.none;
			if (source_infile_count > 1)
				(void)printf("Lint pass2:\n");
			clear_program_options();
			exit_status|= run_steps("", lint_steps, FALSE, "");
		}
	}
}	/* lint_doit */

#ifndef lint
static	char sccsid[] = "@(#)setup_f77.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "driver.h"

/*
 *	compile_f
 */
int
compile_f(source)
	char			*source;
{
	int			status;

#ifdef BROWSER
	global_flag.code_browser = global_flag.code_browser_seen;
#endif
	clear_program_options();

	if ( (requested_suffix == &suffix.s) ||
	     (target_arch.value->value == ARCH_SUN386) )
	{
		set_flag(do_cat);
	}
	if (is_on(statement_count))
	{
		setup_tcov_file(source);
	}

	/* if this is version 1.2 and sw release 4.0
	 * then look for the assembler and c2 elsewhere
	 */
	if (check_release_version_driver(SW_REL_40,DRIVER_REL_12,DRIVER_F) )
	{
		program.as.path = "/usr/lib/f77/as";
		program.asS.path = "/usr/lib/f77/as";
		program.c2.path = "/usr/lib/f77/c2";
	}

	/* if the pic flag was specified on the command line,  we need 
	 * to turn it off, it is only valid for 1.2 and 4.0 OS 
	 */
	else
	{
		if (is_on(PIC_code))
		{
			reset_flag(PIC_code);
			warning("-PIC option ignored");
		}
		if (is_on(pic_code))
		{
			reset_flag(pic_code);
			warning("-pic option ignored");
		}
	}

	status= run_steps(source,
			  ((optimizer_level > OPTIM_C2) ||
			   is_on(statement_count))
				? f_iropt_steps : f_no_iropt_steps,
			  TRUE,
			  "f");
	reset_flag(do_cat);
#ifdef BROWSER
	reset_flag(code_browser);
#endif
	return status;
}	/* compile_f */

/*
 *	compile_F
 */
int
compile_F(source)
	char			*source;
{
	int			status;

	set_flag(do_cpp);
	status= compile_f(source);
	reset_flag(do_cpp);
	return status;
}	/* compile_F */

/*
 *	compile_r
 */
int
compile_r(source)
	char			*source;
{
	int			status;

	set_flag(do_ratfor);
	status= compile_f(source);
	reset_flag(do_ratfor);
	return status;
}	/* compile_r */

/*
 *	setup_f1
 */
char *
setup_f1()
{
	if (!program.f1.has_been_initialized)
	{
		program.f1.has_been_initialized = TRUE;
		f1_to_mf1_copy();
		if (profile.touched)
		{
                        append_list(&program.f1.permanent_options, "-p");
		}
		if (is_on(checkC) || is_on(checkH) || is_on(checkV))
		{
			append_list(&program.f1.permanent_options, "-V");
		}
		switch (target_arch.value->value)
		{
		case ARCH_SUN2:
		case ARCH_SUN3X:
		case ARCH_SUN3:
			append_list(&program.f1.permanent_options,
					float_mode.value->name);
			append_list(&program.f1.permanent_options,
					get_processor_flag(target_arch.value));
			break;
		default:
			/* do nothing. */
			break;
		}
	}
	return NULL;
}	/* setup_f1 */

/*
 *	setup_f77pass1
 */
/*ARGSUSED*/
char *
setup_f77pass1(source, file)
	char			*source;
	char			*file;
{
	char			buffer[LOCAL_STRING_LENGTH];

	if (!program.f77pass1.has_been_initialized)
	{
		program.f77pass1.has_been_initialized = TRUE;
		if (optimizer_level > OPTIM_C2)
			append_list(&program.f77pass1.permanent_options, "-O");
		if (is_on(statement_count))
			append_list(&program.f77pass1.permanent_options, "-a");
		if (is_on(onetrip))
			append_list(&program.f77pass1.permanent_options, "-1");
		if (float_mode.value == &fsky)
			append_list(&program.f77pass1.permanent_options, "-F");
		if (debugger.value == &dbx)
			append_list(&program.f77pass1.permanent_options, "-g");
		if ( is_on(handle_misalignment) )
			append_list(&program.f77pass1.permanent_options, "-m");
		if ( (target_sw_release[R_PASSES].value->value >= SW_REL_41 )
					||
			/* if 4.0 OS and 1.2 fortran */
		check_release_version_driver(SW_REL_40,DRIVER_REL_12,DRIVER_F) )
		{
			if ( is_on(doubleword_aligned_doubles) )
			{	
				append_list(&program.f77pass1.permanent_options, "-f");
			}
		}

		if ( (optimizer_level < OPTIM_IROPT_P) &&
		     is_off(statement_count) )
		{

			strcpy(buffer, "-P");

                	if (is_on(pic_code))
				strcat(buffer, " -k");
			if (is_on(PIC_code))
				strcat(buffer, " -K");

			if (profile.touched)
			{
				if ( (target_sw_release[R_PASSES].value->value
					== SW_REL_3X) &&
				     (target_arch.value->value != ARCH_SUN4C) &&
				     (target_arch.value->value != ARCH_SUN4)
				   )
				{
					/* In release 3.x (for Sun-2 and Sun-3),
					 * -p was passed directly to the
					 * f77pass1 front-end.
					 */
					append_list(&program.f77pass1.permanent_options,
							"-p");
				}
				else
				{
					/* In release 4.x (and in 3.x on Sun4),
					 * -p is passed to the f77pass1
					 * back-end via "-P -p".
					 */
					strcat(buffer, " -p");
				}
			}

			switch (target_arch.value->value)
			{
			case ARCH_SUN2:
			case ARCH_SUN3X:
			case ARCH_SUN3:
				strcat(buffer, " ");
				strcat(buffer,
					get_processor_flag(target_arch.value));
				strcat(buffer, " ");
				strcat(buffer, float_mode.value->name);
				break;
			case ARCH_SUN4C:
			case ARCH_SUN4:
				if ( (target_sw_release[R_PASSES].value->value
					>= SW_REL_41 )
						|| 
				/* if 4.0 OS and 1.2 fortran */
				check_release_version_driver(SW_REL_40,DRIVER_REL_12,DRIVER_F) )
							      
				{
					if ( is_on(doubleword_aligned_doubles) )
					{
						strcat(buffer, " -d");
					}
				}
				if ( is_on(handle_misalignment) )
				{
					strcat(buffer, " -m");
				}
				break;
			case ARCH_FOREIGN:
				/* foreign arch'ture: let them set their own. */
				break;
			default:
				/* nothing to do. */
				break;
			}

			if ( !STR_EQUAL(buffer, "-P") )
			{
				/* buffer contains more than just "-P". */
				append_list(&program.f77pass1.permanent_options,
					make_string(buffer));
			}
			else
				if (target_arch.value->value == ARCH_SUN386)
				{
					append_list(&program.f77pass1.permanent_options, make_string(buffer));
				}	
		}
	}

	/* When the original source file had a ".F" suffix, pass an extra
	 * argument to f77pass1:  "-Efile.F".
	 * [This was done in f77's old dedicated driver (SunOS 3.0 and earlier).
	 *  It disappeared when /lib/compile was created, and was missing in
	 *  SunOS releases 3.2 thru 4.0beta2, and Sun FORTRAN releases up thru
	 *  at least 1.05].
	 */
	if (target_arch.value->value != ARCH_SUN386)
	{
		if (STR_EQUAL("F", get_file_suffix(source)))
		{
			strcpy(buffer, "-E");
			strcat(buffer, source);
			append_list(&program.f77pass1.options, make_string(buffer));
		}
	}

#ifdef BROWSER
	if (is_on(code_browser))
	{
		append_list(&program.f77pass1.options, "-cb");
	}
#endif

	append_list(&program.f77pass1.infile, file);

	/* Since f77pass1 produces three outfiles we need some extra temps */
	iropt_files[0]= temp_file_name(&program.f77pass1, &suffix.s, 's');
	iropt_files[1]= temp_file_name(&program.f77pass1, &suffix.s, 'i');
	iropt_files[2]= temp_file_name(&program.f77pass1, &suffix.s, 'd');
	append_list(&program.f77pass1.infile, iropt_files[0]);
	append_list(&program.f77pass1.infile, iropt_files[1]);
	append_list(&program.f77pass1.infile, iropt_files[2]);

	return iropt_files[1];
}	/* setup_f77pass1 */


/*
 *	setup_cat_for_f77
 *		Used for "-S -O3 foo.f" when we need to collect several
 *		assembly sources.
 */
char *
setup_cat_for_f77(source, file)
	char			*source;
	char			*file;
{
        if ((requested_suffix != &suffix.s) && (target_arch.value == &arch_sun386)) 
	{
		append_list(&program.cat.infile, iropt_files[0]);
		append_list(&program.cat.infile, file);
		append_list(&program.cat.infile, iropt_files[2]);
		return program.cat.outfile= to_as_file = temp_file_name(&program.as, &suffix.s,0);
	}
	else
	{
		append_list(&program.cat.infile, iropt_files[0]);
		append_list(&program.cat.infile, file);
		append_list(&program.cat.infile, iropt_files[2]);
	}
	if (target_arch.value->value == ARCH_SUN386) 
	{
		if (optim_after_cat)
		{
			return program.cat.outfile = temp_file_name(&program.as, &suffix.s,0);
		}
		else
		{
			return program.cat.outfile = outfile_name(source, &suffix.s, (ProgramP)NULL);
		}
	}
	else
	{
		return program.cat.outfile = outfile_name(source, &suffix.s, (ProgramP)NULL);
	}



}	/* setup_cat_for_f77 */

static int as_use_c2_output = FALSE;
/*
 *      setup_c2_for_f77
 *          For the 386 if c2 is called then we must let the
 *          assembler know that to_as_file should not be used, but the
 *          output from c2 should be.
*/
char *
setup_c2_for_f77(source, file)
char                    *source;
char                    *file;
{
	as_use_c2_output = TRUE;
	return NULL;
}


/*
 *	setup_as_for_f77
 *		Used when "-O3 foo.f" means that as should be fed a number of
 *		assembly source files.
 *
 *		Also used for sun386 .. whose "as" does not take more than
 *		one input file.
 *
 */
char *
setup_as_for_f77(source, file)
	char			*source;
	char			*file;

{
	
	(void)setup_as();
	if (target_arch.value->value == ARCH_SUN386)
	{
		if (as_use_c2_output)
			append_list(&program.as.infile, file);
		else  
			append_list(&program.as.infile, to_as_file);
		as_use_c2_output = FALSE;

	}
	else
	{
		append_list(&program.as.infile, iropt_files[0]);
		append_list(&program.as.infile, file);
		append_list(&program.as.infile, iropt_files[2]);
	}
	program.as.outfile= outfile_name(source, &suffix.o, (ProgramP)NULL);
	if ((source_infile_count == 1) && (product.value == &executable))
		append_list(&files_to_unlink, make_string(program.as.outfile));
	return program.as.outfile;
}	/* setup_as_for_f77 */

/*
 *	setup_asS_for_f77
 *		Used when "-O3 foo.f" means that "as" should be fed a number of
 *		assembly source files.	(SPARC version)
 */
char *
setup_asS_for_f77(source, file)
	char			*source;
	char			*file;
{
	setup_asS();
	append_list(&program.asS.infile, iropt_files[0]);
	append_list(&program.asS.infile, file);
	append_list(&program.asS.infile, iropt_files[2]);
	program.asS.outfile= outfile_name(source, &suffix.s, (ProgramP)NULL);
	return program.asS.outfile;
}	/* setup_asS_for_f77 */

/*
 *	setup_ld_for_f77
 */
char *
setup_ld_for_f77()
{
	register ListP	p;
	char		buffer[LOCAL_STRING_LENGTH];
#ifdef sun386
	static char	*minus_u_options[] = { "-u", "MAIN_", NULL };
#else
	static char	*minus_u_options[] = { "-u", "_MAIN_", NULL };
#endif
	static char	*libraries[] =
				{ "-lF77",   "-lI77",   "-lU77",   "-lm",
					"-lc",   NULL };
	static char	*profiled_libraries[] =
				{ "-lF77_p", "-lI77_p", "-lU77_p", "-lm_p",
					"-lc_p", NULL };

#ifdef sun386
	append_list(&program.ld.permanent_options, "-Bstatic");
#endif  sun386

	setup_ld( (driver.value == &f77) ? minus_u_options : (char **)NULL,
		  libraries, profiled_libraries );

	return "";
}

/*
 *	f77_doit
 */
void
f77_doit()
{
	set_requested_suffix(&suffix.f);
	do_infiles();
	if (!exit_status && (product.value == &executable) &&
	    (infile_o != NULL))
	{
		requested_suffix= &suffix.none;
		if (source_infile_count > 1)
		{
			(void)printf("Linking:\n");
		}
		clear_program_options();
		exit_status|= run_steps("", link_steps, FALSE, "");
		if (exit_status != 0)
		{
			do_not_unlink_ld_infiles();
		}
	}
}	/* f77_doit */

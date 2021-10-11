#ifndef lint
static	char sccsid[] = "@(#)setup_pc.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "driver.h"

/*
 *	compile_pi
 */
int
compile_pi(source)
	char			*source;
{
	int			status;

#ifdef BROWSER
	global_flag.code_browser = global_flag.code_browser_seen;
#endif
	clear_program_options();

	if (target_sw_release[R_PASSES].value->value == SW_REL_3X)
	{
		return run_steps(source, p_rel3x_steps, TRUE, "p");
	}
	else
	{
		if ( (requested_suffix == &suffix.s) ||
			(target_arch.value->value == ARCH_SUN386) )
		{
			set_flag(do_cat);
		}

		if (optimizer_level > OPTIM_C2 || is_on(statement_count))
		{
			status= run_steps(source, p_iropt_steps, TRUE, "p");
		}
		else	status= run_steps(source, p_no_iropt_steps, TRUE, "p");

		reset_flag(do_cat);
#ifdef BROWSER
		reset_flag(code_browser);
#endif
		return status;
	}
}	/* compile_pi */


#ifdef PASCAL_105
Bool			special_pascal_105_case = FALSE;
#endif /*PASCAL_105*/

/*
 *	compile_p
 */
int
compile_p(source)
	char			*source;
{
	int			status;

#ifdef PASCAL_105
	extern Const_intP	original_sw_rel_passes;
	static Const_intP	saved_sw_rel_passes;
	static char		*saved_iropt_path;

	/* Special-case for Pascal 1.05 for Sun-4 target:
	 *	treat it as if the compiler passes were 4.0 passes,
	 *	except must use regular 3.x linker pass & flags.
	 *	(All of which is overridden if -release or -relpasses was used)
	 */
	if ( ( (target_arch.value->value == ARCH_SUN4)  ||
	       (target_arch.value->value == ARCH_SUN4C)
	     )
				&&
	     (original_sw_rel_passes->value == SW_REL_DFLT) )
	{
		special_pascal_105_case = TRUE;
		saved_sw_rel_passes = target_sw_release[R_PASSES].value;
		target_sw_release[R_PASSES].value = &sw_release_40_explicit;
		saved_iropt_path = program.iropt_pc.path;
		program.iropt_pc.path = "/usr/lib/iropt-pc";
	}
#endif /*PASCAL_105*/

	set_flag(do_cpp);
	status= compile_pi(source);
	reset_flag(do_cpp);

#ifdef PASCAL_105
	if ( special_pascal_105_case )
	{
		program.iropt_pc.path = saved_iropt_path;
		target_sw_release[R_PASSES].value = saved_sw_rel_passes;
		special_pascal_105_case = FALSE;
	}
#endif /*PASCAL_105*/

	return status;
}	/* compile_p */


/*
 *	setup_pc0_for_3x
 *		3.x pc0 has different arguments from 4.0 pc0, therefore the
 *		argument template has to be changed.
 */
char *
setup_pc0_for_3x(source, file)
	char			*source;
	char			*file;
{
	register int i;
	static Template pc0_3x_template[PGM_TEMPLATE_LENGTH] =
		{ MINUS_O_TEMPLATE, OUT_TEMPLATE, OPTION_TEMPLATE, IN_TEMPLATE};

	(void)setup_pc0();

	for (i = 0;  i < PGM_TEMPLATE_LENGTH;  i++)
	{
		program.pc0.template[i] = pc0_3x_template[i];
	}

	return NULL;
}	/* setup_pc0_for_3x */


/*
 *	setup_pc0_for_non_3x
 *		The post-3.x pc0 produces two outfiles;
 *		one iropt file (containing IR for the Text segment)
 *		and one assembly file (containing the Data segment).
 */
char *
setup_pc0_for_non_3x(source, file)
	char			*source;
	char			*file;
{
	(void)setup_pc0();
	append_list(&program.pc0.infile, file);
	/* program.pc0.outfile is referenced in setup_as_for_pc(), to get the
	 * name of the extra infile.
	 */
	program.pc0.outfile= temp_file_name(&program.pc0, &suffix.s, 0);
	append_list(&program.pc0.options,
			source= temp_file_name(&program.pc0, &suffix.ir, 0));

	return source;
}	/* setup_pc0_for_iropt */


/*
 *	setup_pc0
 */
char *
setup_pc0()
{
	char	buffer[LOCAL_STRING_LENGTH];

	if (!program.pc0.has_been_initialized)
	{
		program.pc0.has_been_initialized = TRUE;
		if (optimizer_level > OPTIM_NONE)
		{
			if (target_sw_release[R_PASSES].value->value
				== SW_REL_3X)
				{
				append_list(&program.pc0.permanent_options,
						"-O");
			}
			else
			{
				/* a 4.x target release */
				(void)sprintf(buffer, "-O%c", optimizer_level);
				append_list(&program.pc0.permanent_options,
						make_string(buffer));
			}
		}
		if (is_on(warning))
			append_list(&program.pc0.permanent_options, "-w");
		if (is_on(checkC))
			append_list(&program.pc0.permanent_options, "-C");
		if (is_on(checkH))
			append_list(&program.pc0.permanent_options, "-H");
		if (is_on(checkV))
			append_list(&program.pc0.permanent_options, "-V");
		if (debugger.value)
			append_list(&program.pc0.permanent_options, "-g");
		if ( is_on(handle_misalignment) &&
		     (optimizer_level < OPTIM_IROPT_P) )
		{
			append_list(&program.pc0.permanent_options, "-m");
		}
	}

#ifdef BROWSER
	if (is_on(code_browser)) {
		append_list(&program.pc0.infile, "-cb");
		}
#endif

	return NULL;
}	/* setup_pc0 */

/* 
 *
 *	setup_cppas
 *
 */
char *
setup_cppas(original_source, source)
	char	*original_source;
	char	*source;

{
	/* we are reaching new heights in hacking 
	 * we need to copy cpp options to cppas
	 * because we can't have a -I option for bot
	 * cpp and cppas
	 */
	program.cppas.permanent_options = program.cpp.permanent_options;
	program.cppas.trailing_options = program.cpp.trailing_options;


	return NULL;

} /* setup_cppas */


/*
 *	setup_inline_for_pc
 *		Make sure inline runs with the proper .il file for Pascal.
 */
char *
setup_inline_for_pc()
{
	setup_inline();
	append_list(&program.inline.options, "-i");
	append_list(&program.inline.options,
			scan_Qpath_and_vroot("/usr/lib/pc2.il",TRUE));
	return NULL;
}	/* setup_inline_for_pc */


static char *to_as_file;

/*
 *	setup_cat_for_pc
 *		This is used when "pc -O3 -S" has been given.
 *		The assembly source from pc0 must be combined with the one
 *		from c2.
 */
char *
setup_cat_for_pc(source, file)
	char			*source;
	char			*file;
{
	append_list(&program.cat.infile, program.pc0.outfile);
	append_list(&program.cat.infile, file);

	if  (target_arch.value->value == ARCH_SUN386) 
	{
		if ( (requested_suffix == &suffix.s)
				&&
		     (optimizer_level ==  OPTIM_NONE))
		{
			program.cat.outfile = to_as_file = 
			outfile_name(source, &suffix.s, (ProgramP)NULL);
		}
		else
		{
			program.cat.outfile = to_as_file =
				temp_file_name(&program.as, &suffix.s, 0);
		}
	}
	else
	{
		program.cat.outfile = outfile_name(source, &suffix.s, (ProgramP)NULL);
	}

	return  program.cat.outfile;
}	/* setup_cat_for_pc */

/*
 *	setup_as_for_pc
 *		This is used to feed "as" the infiles from both pc0 and c2,
 *		when iropt is used.
 *
 *		Also used for sun386 .. whose "as" does not take more than
 *		one input file.
 */
char *
setup_as_for_pc(source, file)
	char			*source;
	char			*file;
{
	(void)setup_as();

	if (target_arch.value->value == ARCH_SUN386)
	{
		append_list(&program.as.infile, to_as_file);
	}
	else
	{
		if (target_sw_release[R_PASSES].value->value == SW_REL_3X)
		{
			if ( (optimizer_level > OPTIM_C2) ||
			     is_on(statement_count))
			{
				append_list(&program.as.infile,
						program.pc0.outfile);
			}
		}
		else
		{
			/* a 4.x target release */
			append_list(&program.as.infile, program.pc0.outfile);
		}

		append_list(&program.as.infile, file);
	}

	program.as.outfile= outfile_name(source, &suffix.o, (ProgramP)NULL);
	if ((source_infile_count == 1) && (product.value == &executable))
		append_list(&files_to_unlink, make_string(program.as.outfile));

	return program.as.outfile;
}	/* setup_as_for_pc */

/*
 *	setup_asS_for_pc
 */
char *
setup_asS_for_pc(source, file)
	char			*source;
	char			*file;
{
	setup_asS();
	if (target_sw_release[R_PASSES].value->value == SW_REL_3X)
	{
		if ( (optimizer_level > OPTIM_C2) || is_on(statement_count))
			append_list(&program.asS.infile, program.pc0.outfile);
	}
	else
	{
		/* a 4.x target release */
		append_list(&program.asS.infile, program.pc0.outfile);
	}

	append_list(&program.asS.infile, file);
	program.asS.outfile= outfile_name(source, &suffix.s, (ProgramP)NULL);
	return program.asS.outfile;
}	/* setup_asS_for_pc */

/*
 *	setup_pc3
 */
char *
setup_pc3()
{
	register ListP		p;

	if (!program.pc3.has_been_initialized)
	{
		program.pc3.has_been_initialized = TRUE;
		if (is_on(warning))
			append_list(&program.pc3.permanent_options, "-w");
		append_list(&program.pc3.permanent_options,
			scan_Qpath_and_vroot("/usr/lib/pcexterns.o", TRUE));
	}
	for (p= infile_o; p != NULL; p= p->next)
	{
		/* lets not append any -X options  
		 * there are certain cases where the infiles
		 * do begin with -X (i.e. -Bxxxx )
		 * pc3 should only have only true input files
		 */
		if (*p->value != '-')
		{
			append_list(&program.pc3.infile, p->value);
		}
	}
	return "";
}	/* setup_pc3 */

/*
 *	setup_ld_for_pc
 */
char *
setup_ld_for_pc()
{
	static char	*libraries[] =
				{ "-lpc",   "-lm",   "-lc",   NULL };
	static char	*profiled_libraries[] =
				{ "-lpc_p", "-lm_p", "-lc_p", NULL };

#ifdef i386
	append_list(&program.ld.permanent_options, "-Bstatic");
#endif i386

	setup_ld( (char **)NULL, libraries, profiled_libraries );

	return "";
}

/*
 *	pc_doit
 */
void
pc_doit()
{
	set_requested_suffix(&suffix.pi);
	do_infiles();
	if ((exit_status == 0) && (product.value == &executable) &&
	    (infile_o != NULL))
	{
		requested_suffix= &suffix.none;
		if (source_infile_count > 1)
		{
			(void)printf("Linking:\n");
		}
		clear_program_options();
		exit_status|= run_steps("", pc_steps, FALSE, "");
		if (exit_status != 0)
		{
			do_not_unlink_ld_infiles();
		}
	}
}	/* pc_doit */

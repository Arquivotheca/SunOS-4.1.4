#ifndef lint
static	char sccsid[] = "@(#)setup_cc.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "driver.h"

/*
 *	compile_i
 */
int
compile_i(source)
	char			*source;
{
	int			status;

#ifdef BROWSER
	global_flag.code_browser = global_flag.code_browser_seen;
#endif
	clear_program_options();

	if (requested_suffix == &suffix.s)
	{
		set_flag(do_cat);
	}
	if (optimizer_level > OPTIM_C2 || is_on(statement_count))
	{
		status= run_steps(source, c_iropt_steps, TRUE, "c");
	}
	else
	{
		status= run_steps(source, c_no_iropt_steps, TRUE, "c");
	}
	reset_flag(do_cat);
#ifdef BROWSER
	reset_flag(code_browser);
#endif
	return status;
}	/* compile_i */

/*
 *	compile_c
 */
int
compile_c(source)
	char			*source;
{
	int			status;

	set_flag(do_cpp);
	if (is_on(statement_count))
	{
		setup_tcov_file(source);
	}
	status= compile_i(source);
	reset_flag(do_cpp);
	return status;
}	/* compile_c */

/*
 *	setup_cpp_for_cc
 */
char *
setup_cpp_for_cc()
{
	if (product.value == &preprocessed2)
	{
		append_list(&program.cpp.options, "-P");
	}
	return NULL;
}	/* setup_cpp_for_cc */

/*
 *	setup_ccom
 */
char *
setup_ccom()
{
	if (!program.ccom.has_been_initialized)
	{
		program.ccom.has_been_initialized = TRUE;
		if (profile.touched && (optimizer_level < OPTIM_IROPT_P))
		{
			char *ccom_profile_flag;
			switch (target_arch.value->value)
			{
			case ARCH_SUN2:
			case ARCH_SUN3X:
			case ARCH_SUN3:
				switch (target_sw_release[R_PASSES].value->value)
				{
				case SW_REL_3X:
					ccom_profile_flag = "-XP";/* 3.x flag */
					break;
				default:
					ccom_profile_flag = "-p"; /* 4.x flag */
					break;
				}
				break;
			
			case ARCH_SUN386:
				ccom_profile_flag = "-XP"; /*3.x flag, for now*/
				break;

			case ARCH_FOREIGN:
			default:
				/* Other ccom's (e.g. Sun-4) always gets
				 * new-style flag.
				 */
				ccom_profile_flag = "-p";	/* 4.x flag */
				break;
			}
			append_list(&program.ccom.permanent_options,
					ccom_profile_flag);
		}

		if (is_on(pic_code))
			append_list(&program.ccom.permanent_options, "-Xk");
		if (is_on(PIC_code))
			append_list(&program.ccom.permanent_options, "-XK");
		if (debugger.value == &adb)
			append_list(&program.ccom.permanent_options, "-XG");
		if (debugger.value == &dbx)
			append_list(&program.ccom.permanent_options, "-Xg");

		if (optimizer_level < OPTIM_IROPT_P)
		{
			/* Since the optimization level is 0 or 1, "cg" is not
			 * going to be run, so add these flags to "ccom".
			 */
			if (is_on(generate_nonzero_activation_records) &&
			    is_off(statement_count) )
			{
				append_list(&program.ccom.permanent_options,
						"-bnzero");
			}
			if ( is_on(fstore) )
			{
				append_list(&program.ccom.permanent_options,
						"-fstore");
			}
			if ( is_on(handle_misalignment) )
			{
				append_list(&program.ccom.permanent_options,
						"-m");
			}
			if (target_sw_release[R_PASSES].value->value
				>= SW_REL_41 )
			{
				if ( is_on(doubleword_aligned_doubles) )
				{
					append_list(&program.ccom.permanent_options,
						"-d");
				}
			}
		}

		switch (target_arch.value->value)
		{
		case ARCH_SUN2:
		case ARCH_SUN3X:
		case ARCH_SUN3:
			if (is_on(long_offset) &&
			    (optimizer_level < OPTIM_IROPT_P))
			{
				char *ccom_J_flag;
				switch (target_sw_release[R_PASSES].value->value)
			
				{
				case SW_REL_3X: ccom_J_flag = "-XJ";	break;
				default:	ccom_J_flag = "-J" ;	break;
				}
				append_list(&program.ccom.permanent_options,
						ccom_J_flag);
			}
			append_list(&program.ccom.permanent_options,
					float_mode.value->name);
			break;
		case ARCH_SUN386:
			if(requested_suffix == &suffix.s)
			{
				append_list(&program.ccom.permanent_options,
						"-Xl");
			}
			if(is_on(as_R))
			{
				append_list(&program.ccom.permanent_options, "-XR");
			}
			break;
		case ARCH_SUN4C:
		case ARCH_SUN4:
			if (is_on(sparc_sdata))
				append_list(&program.ccom.permanent_options,
						"-Xx");
			break;
		case ARCH_FOREIGN:
		default:
			/* do nothing here. */
			break;
		}

		switch (target_arch.value->value)
		{
		case ARCH_SUN2:
		case ARCH_SUN3X:
		case ARCH_SUN3:
			append_list(&program.ccom.permanent_options,
					get_processor_flag(target_arch.value));
			break;
		case ARCH_SUN386:
		case ARCH_SUN4C:
		case ARCH_SUN4:
		case ARCH_FOREIGN:
		default:
			/* do nothing here. */
			break;
		}
	}

#ifdef BROWSER
	if (is_on(code_browser)) {
		append_list(&program.ccom.options, "-cb");
		}
#endif

	return NULL;
}	/* setup_ccom */

/*
 *	setup_cat_for_cc
 *		This is used when "cc -O3 -S" has been given.
 *		The assembly source from ccom must be combined with the one
 *		from c2.
 */
char *
setup_cat_for_cc(source, file)
	char			*source;
	char			*file;
{
	append_list(&program.cat.infile, program.ccom.outfile);
	append_list(&program.cat.infile, file);
	return  program.cat.outfile=
			outfile_name(source, &suffix.s, (ProgramP)NULL);
}	/* setup_cat_for_cc */

/*
 *	setup_as_for_cc
 *		This is used to feed "as" the infiles from both ccom and c2,
 *		when iropt is used.
 */
char *
setup_as_for_cc(source, file)
	char			*source;
	char			*file;
{
	(void)setup_as();
	if ( (optimizer_level > OPTIM_C2) || is_on(statement_count))
		append_list(&program.as.infile, program.ccom.outfile);
	append_list(&program.as.infile, file);
	program.as.outfile= outfile_name(source, &suffix.o, (ProgramP)NULL);
	if ((source_infile_count == 1) && (product.value == &executable))
		append_list(&files_to_unlink, make_string(program.as.outfile));
	return program.as.outfile;
}	/* setup_as_for_cc */

/*
 *	setup_asS_for_cc
 *		This is used to feed "as" the infiles from both ccom and c2,
 *		when iropt is used, for SPARC.
 */
char *
setup_asS_for_cc(source, file)
	char			*source;
	char			*file;
{
	setup_asS();
	if (optimizer_level > OPTIM_C2 || is_on(statement_count))
		append_list(&program.asS.infile, program.ccom.outfile);
	append_list(&program.asS.infile, file);
	program.asS.outfile= outfile_name(source, &suffix.s, (ProgramP)NULL);
	return program.asS.outfile;

}	/* setup_asS_for_cc */

/*
 *	setup_ld_for_cc
 */
char *
setup_ld_for_cc()
{
	static char	*libraries[] =          { "-lc",   NULL };
	static char	*profiled_libraries[] = { "-lc_p", NULL };

	setup_ld( (char **)NULL, libraries, profiled_libraries );

	return "";
}

/*
 *	cc_doit
 */
void
cc_doit()
{
	set_requested_suffix(&suffix.i);
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
}	/* cc_doit */

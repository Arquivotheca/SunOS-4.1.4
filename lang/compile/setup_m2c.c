#ifndef lint
static	char sccsid[] = "@(#)setup_m2c.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "driver.h"

/*
 *	compile_def
 */
int
compile_def(source)
	char			*source;

{
	int			status;

	clear_program_options();
	if (target_sw_release[R_PASSES].value->value >=  SW_REL_40)
	{
		status = run_steps(source, def_steps_non_3x, FALSE, "");
	}
	else
	{
		status = run_steps(source, def_steps_3x, FALSE, "");
	}
	return status;

}	/* compile_def */

/*
 *	compile_mod
 */
int
compile_mod(source)
	char			*source;
{
	int			status;

	clear_program_options();
	if (target_sw_release[R_PASSES].value->value >=  SW_REL_40)
	{
		if (optimizer_level > OPTIM_C2 || is_on(statement_count))
		{
			status = run_steps(source, mod_iropt_steps, FALSE, "ir");
		}
		else
		{
			status = run_steps(source, mod_no_iropt_steps, FALSE, "ir");
		}
	}
	else
	{
		status = run_steps(source, mod_rel3x_steps, FALSE, "");
	}
	return status;

}	/* compile_mod */

/*
 *	setup_m2cfe
 */
char *
setup_m2cfe()
{
	char			buffer[MAXPATHLEN];
	register ListP		cp;

	if (!program.m2cfe.has_been_initialized)
	{
		program.m2cfe.has_been_initialized = TRUE;
		if (optimizer_level > OPTIM_NONE)
		{
			append_list(&program.m2cfe.permanent_options, "-O");
		}
		/* else if no optimization append -noregs option */
		else
		{
			append_list(&program.m2cfe.permanent_options, "-noregs");
		}

		if (debugger.touched)
			append_list(&program.m2cfe.permanent_options, "-g");
		if (is_on(trace))
			append_list(&program.m2cfe.permanent_options, "-v");
		if (is_on(statement_count))
			append_list(&program.m2cfe.permanent_options, "-a");
		/* The driver converts the list of -M options to one -M, with
		 * all the dirs.
		 */
		(void)strcpy(buffer, "-M");
		for (cp= module_list; cp != NULL; cp= cp->next)
		{
			(void)strcat(buffer, cp->value+2);
			(void)strcat(buffer, " ");
		}
		if (is_off(no_default_module_list))
		{
			(void)strcat(buffer, ". /usr/lib/modula2");
			if (profile.touched)
				(void)strcat(buffer, "_p");
		}
		else
			if (module_list)
				/* Remove extra trailing space */
				buffer[strlen(buffer)-1]= '\0';
		if (buffer[2] != '\0')
			append_list(&program.m2cfe.permanent_options,
					make_string(buffer));
	}
	return NULL;
}	/* setup_m2cfe */



/*
 *	setup_m2cfe_for_3x_def
 *		Start m2cfe with -def option and basename of source filename
 */
char *
setup_m2cfe_for_3x_def(source)
	char			*source;
{
	register char	*p;
	register char	*q;
	register int	 i;	

	static Template mod_def_template[PGM_TEMPLATE_LENGTH] =
		{ OPTION_TEMPLATE, IN_TEMPLATE, STDOUT_TEMPLATE};

	for (i = 0;  i < PGM_TEMPLATE_LENGTH;  i++)
	{
		program.m2cfe.template[i] = mod_def_template[i];
	}

	p= get_memory(strlen(source)+1);
	if ((q= rindex(source, '/')) == NULL)
		q= source;
	else
		q++;	/* point PAST the '/' */
	(void)strcpy(p, q);
	if ((q= rindex(p, '.')) != NULL)
		*q= '\0';
	append_list(&program.m2cfe.options, "-def");
	append_list(&program.m2cfe.options, p);
	return NULL;

}	/* setup_m2cfe_for_3x_def */


/*
 *	setup_m2cfe_for_non_3x_def
 *		Start m2cfe with -def option and basename of source filename
 */
char *
setup_m2cfe_for_non_3x_def(source)
	char			*source;
{
	register char	*p;
	register char	*q;
	register int	 i;	

	static Template mod_def_template[PGM_TEMPLATE_LENGTH] =
		{ OPTION_TEMPLATE, IN_TEMPLATE, OUT_TEMPLATE};

	for (i = 0;  i < PGM_TEMPLATE_LENGTH;  i++)
	{
		program.m2cfe.template[i] = mod_def_template[i];
	}

	p= get_memory(strlen(source)+1);
	if ((q= rindex(source, '/')) == NULL)
		q= source;
	else
		q++;	/* point PAST the '/' */
	(void)strcpy(p, q);
	if ((q= rindex(p, '.')) != NULL)
		*q= '\0';
	append_list(&program.m2cfe.options, "-def");
	append_list(&program.m2cfe.options, p);
	return NULL;

}	/* setup_m2cfe_for_non_3x_def */


/*
 *	setup_m2cfe_for_non_3x_mod
 *		one iropt file
 *		and one assembly file
 */
char *setup_m2cfe_for_non_3x_mod(source)
	char			*source;
{
	register char		*p;
	register char		*q;

	p= get_memory(strlen(source)+1);
	if ((q= rindex(source, '/')) == NULL)
		q= source;
	else
		q++;	/* point PAST the '/' */
	(void)strcpy(p, q);
	if ((q= rindex(p, '.')) != NULL)
		*q= '\0';
	append_list(&program.m2cfe.options, "-mod");
	append_list(&program.m2cfe.options, p);
	
	program.m2cfe.outfile2= temp_file_name(&program.m2cfe, &suffix.s, 0);

	return NULL;
}	/* setup_m2cfe_for_non_3x_mod */



/*
 *	setup_m2cfe_for_3x_mod
 *		Start m2cfe with "-mod" and basename of source filename
 */
char *setup_m2cfe_for_3x_mod(source)
	char			*source;
{
	register char		*p;
	register char		*q;

	p= get_memory(strlen(source)+1);
	if ((q= rindex(source, '/')) == NULL)
		q= source;
	else
		q++;	/* point PAST the '/' */
	(void)strcpy(p, q);
	if ((q= rindex(p, '.')) != NULL)
		*q= '\0';
	append_list(&program.m2cfe.options, "-mod");
	append_list(&program.m2cfe.options, p);
	return NULL;
}	/* setup_m2cfe_for_3x_mod */


/*
 *	setup_mf1
 */
char *
setup_mf1()
{
	if (!program.mf1.has_been_initialized)
	{
		program.mf1.has_been_initialized = TRUE;
		f1_to_mf1_copy();
		switch (target_arch.value->value)
		{
		case ARCH_SUN2:
		case ARCH_SUN3X:
		case ARCH_SUN3:
			if (is_on(long_offset))
				append_list(&program.mf1.permanent_options,
						"-J");
			if (profile.touched)
				append_list(&program.mf1.permanent_options,
						"-p");
			append_list(&program.mf1.permanent_options,
					float_mode.value->name);
			append_list(&program.mf1.permanent_options,
					get_processor_flag(target_arch.value));
			break;
		case ARCH_SUN4C:
		case ARCH_SUN4:
			if (profile.touched)
				append_list(&program.mf1.permanent_options,
					"-p");
			break;
		case ARCH_FOREIGN:
		default:
			break;
		}
	}
	return NULL;
}	/* setup_mf1 */

/*
 *	setup_as_for_m2c
 */
char *
setup_as_for_m2c(source, file)
	char			*source;
	char			*file;
{
	setup_as();
	append_list(&program.as.infile, file);
	program.as.outfile= outfile_name(source, &suffix.o, (ProgramP)NULL);
	return program.as.outfile;

}	/* setup_as_for_m2c */


/*
 *	setup_asS_for_m2c
 */
char *
setup_asS_for_m2c(source, file)
	char			*source;
	char			*file;
{
	setup_asS();
	append_list(&program.asS.infile, file);
	program.asS.outfile= outfile_name(source, &suffix.s, (ProgramP)NULL);
	return program.asS.outfile;
}	/* setup_asS_for_m2c */

/*
 *	setup_m2l_for_m2c
 */
char *
setup_m2l_for_m2c()
{
	register ListP		p;
	char			buffer[LOCAL_STRING_LENGTH];

	if (!program.m2l.has_been_initialized)
	{
		program.m2l.has_been_initialized = TRUE;
#ifdef sun386
		append_list(&program.ld.permanent_options, "-Bstatic");
#endif  sun386
		for (p= module_list; p != NULL; p= p->next)
			append_list(&program.m2l.permanent_options, p->value);
		if (is_off(no_default_module_list))
		{
			append_list(&program.m2l.permanent_options, "-M.");
			if (profile.touched)
				append_list(&program.m2l.permanent_options,
						"-M/usr/lib/modula2_p");
			else
				append_list(&program.m2l.permanent_options,
						"-M/usr/lib/modula2");
		}
		if (is_on(verbose))
			append_list(&program.m2l.permanent_options, "-v");
		if (is_on(trace))
			append_list(&program.m2l.permanent_options, "-trace");
		buffer[0] = '-';
		strcpy(&buffer[1], target_arch.value->extra);
		append_list(&program.m2l.permanent_options,
				make_string(buffer));
		append_list(&program.m2l.permanent_options, "-X");
	}
	for (p= program.ld.permanent_options; p != NULL; p= p->next)
		append_list(&program.m2l.permanent_options, p->value);
	for (p= program.ld.trailing_options; p != NULL; p= p->next)
		append_list(&program.m2l.trailing_options, p->value);
	program.m2l.outfile= (outfile == NULL) ? "a.out" : outfile;
	for (p= infile; p != NULL; p= p->next)
	{
		if (STR_EQUAL("-e", p->value))
		{
			p= p->next;
			continue;
		}
		if (STR_EQUAL(program.m2l.outfile, p->value))
			fatal("Outfile %s would overwrite infile", p->value);
	}

	/* Add crt0.o routine */
	append_list(&program.m2l.infile,
			scan_Qpath_and_vroot(profile.value->extra, TRUE));

	/* Add crt1.o routine */
	if ( (target_arch.value->value != ARCH_SUN4) && 
	     (target_arch.value->value != ARCH_SUN4C) &&
	     (target_arch.value->value != ARCH_SUN386) &&
	     float_mode.value->extra
	   )
	{
		append_list(&program.m2l.infile,
			scan_Qpath_and_vroot(float_mode.value->extra, TRUE));
	}

	/* Add bb_link.o routine, if -a is enabled */
	if (is_on(statement_count))
	{
		append_list(&program.m2l.infile,
			scan_Qpath_and_vroot("/usr/lib/bb_link.o", TRUE));
	}

	/* Do not append /usr/lib/f... search path if the release is 3x,
	 * or the arch is sun386, sun4, or sun4c
	 */
	if ( (target_sw_release[R_PATHS].value->value != SW_REL_3X) 
				&& 
		(target_arch.value->value != ARCH_SUN386)
				&& 
		(target_arch.value->value != ARCH_SUN4C)
				&& 
		(target_arch.value->value != ARCH_SUN4) )
	{
		/* Add /usr/lib/f... search path for libraries. */
		(void)sprintf(buffer, "-L/usr/lib/%s",
					float_mode.value->name+1);
		append_list(&program.m2l.infile, make_string(buffer));
	}
	for (p= infile_o; p != NULL; p= p->next)
	{
		append_list(&program.m2l.infile, p->value);
	}
	if (debugger.touched)
	{
		append_list(&program.m2l.infile, "-lg");
	}
	append_list(&program.m2l.infile, profile.touched ? "-lc_p":"-lc");
	return "";
}	/* setup_m2l_for_m2c */


static char *to_as_file;

/*
 *      setup_cat_for_mod
 *              The assembly source from  must be combined with the one
 *              from c2.
 */
char *
setup_cat_for_mod(source, file)
	char                    *source;
	char                    *file;
{
        append_list(&program.cat.infile, program.m2cfe.outfile2);
	append_list(&program.cat.infile, file);

	if  (requested_suffix != &suffix.s)
	{
		program.cat.outfile = to_as_file =
			temp_file_name(&program.as, &suffix.s, 0);
	}
	
	/* this case is for sun4's with -S and -O */
	else if ( ( (target_arch.value->value == ARCH_SUN4)  ||
		    (target_arch.value->value == ARCH_SUN4C) 
		  )
				&&
		  (optimizer_level > OPTIM_NONE ) )
	{
		program.cat.outfile = to_as_file =
			temp_file_name(&program.as, &suffix.s, 0);
	
	}
	else
	{
		program.cat.outfile = outfile_name(source, &suffix.s, (ProgramP)NULL);
	}
	return program.cat.outfile;

} /* setup_cat_for_mod */



/*
 *	m2c_doit
 */
void
m2c_doit()
{
	/* The default is to not run the linker for m2c */
	if (product.value == &executable)
		product.value= &object;
	if (is_on(do_m2l))
		product.value= &executable;
	set_requested_suffix(&suffix.none);
	do_infiles();
	if (!exit_status && (product.value == &executable))
	{
		requested_suffix= &suffix.none;
		if (source_infile_count > 1)
		{
			(void)printf("Linking:\n");
		}
		clear_program_options();
		exit_status|= run_steps("", link_m2c_steps, FALSE, "");
		if (exit_status != 0)
		{
			do_not_unlink_ld_infiles();
		}
	}
}	/* m2c_doit */

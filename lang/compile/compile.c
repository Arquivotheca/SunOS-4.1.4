#ifndef lint
static	char sccsid[] = "@(#)compile.c 1.122 89/04/14 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 *	[ The comments which used to be here are probably 'way out of date;
 *	 see the /lib/compile Internal Architecture document instead ]
 */

#include "driver.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef BROWSER
#include "../browser/cblib/src/cb_init.h"
#endif
		        
#if   RELEASE < 40
#  ifdef sparc
#    include <alloca.h>
#  endif
#else  /* RELEASE >= 40 */
#  include <alloca.h>
#endif /* RELEASE < 40 */

#ifndef BROWSER
+++ -DBROWSER is to be left ON! +++
#endif

static Bool	target_arch_set_from_processor_type = FALSE;
static OptionP	processor_type_option;	/* valid when above flag is TRUE. */

#ifdef PASCAL_105
Const_intP	original_sw_rel_passes;
#endif /*PASCAL_105*/

#ifdef NSE
  static char	*ENV_nse_variant    = NULL;
  static char	*ENV_target_arch    = NULL;
  static char	*ENV_target_release = NULL;
  static char	TARGET_ARCH_varname[]    = "TARGET_ARCH";
  static char	TARGET_RELEASE_varname[] = "TARGET_RELEASE";
#  define IN_AN_NSE_VARIANT(ENV_nse_variant)  (ENV_nse_variant != ((char*)NULL))
#  ifdef DEBUG
    static Bool	consulted_NSE = FALSE;
#  endif
#endif /*NSE*/


/*
 *	Allocate memory with error checking
 */
char *
get_memory(size)
	int			size;
{
	register char		*result= malloc((unsigned)size);

	if (result == NULL)
		fatal("Out of memory");
	return result;
}

/*
 *      A set of simple functions that are used to determine if
 *      a particular OS, driver release and DRIVER are active
 *      i.e. if f77 and 4.0 and 1.2
 */
int
check_release_version_driver(release,version,driver_value)
	int     release;
	char    *version;
	int     driver_value;

{
        return
			( (target_sw_release[R_PASSES].value->value == release)
						&&
				( ! strcmp(base_driver_release,version))
						&&
				(driver.value->value == driver_value) );
}


/*
 *	Allocate right amount of memory and copy string to it
 */
char *
make_string(string)
	register char		*string;
{
	return strcpy(get_memory(strlen(string)+1), string);
}

/*
 *	append_list_with_suffix
 *		Append one string to the end of the list
 */
void
append_list_with_suffix(list_head, value, suffix)
	register ListP		*list_head;
	char			*value;
	SuffixP			suffix;
{
	register ListP		new_list_cell= (ListP)get_memory(sizeof(List));
	register ListP		list_tail;

	new_list_cell->value= value;
	new_list_cell->next= (ListP)NULL;
	new_list_cell->suffix= suffix;
	if (*list_head == NULL)
		*list_head= new_list_cell;
	else
	{
		for ( list_tail= *list_head;
		      list_tail->next != NULL;
		      list_tail= list_tail->next
		    )
		      ;
		list_tail->next= new_list_cell;
	}
}

/*
 *	get_file_suffix
 *		Extract the suffix from a string by locating the last '.'
 */
char *
get_file_suffix(name)
	char			*name;
{
	register char		*dot= rindex(name, '.');

	if (dot == NULL)
		return "";
	return make_string(dot+1);
}	/* get_file_suffix */

/*
 *	print_help_prefix
 *		Print the prefix for the "compile -help" command
 */
static void
print_help_prefix(drivers)
	register int		drivers;
{
	register int		help_prefix_width= 0;

	if ((drivers & DRIVER_C) != 0)
	{
		(void)printf("C");
		help_prefix_width++;
	}
	if ((drivers & DRIVER_F) != 0)
	{
		(void)printf("F");
		help_prefix_width++;
	}
	if ((drivers & DRIVER_L) != 0)
	{
		(void)printf("L");
		help_prefix_width++;
	}
	if ((drivers & DRIVER_M) != 0)
	{
		(void)printf("M");
		help_prefix_width++;
	}
	if ((drivers & DRIVER_P) != 0)
	{
		(void)printf("P");
		help_prefix_width++;
	}
	if ((drivers & HIDE_OPTION) != 0)
	{
		(void)printf("[H]");
		help_prefix_width+= 3;
	}
	(void)printf("%*s", MAX_HELP_PREFIX_WIDTH - help_prefix_width, "");
}

/*
 *	print_help
 */
static void
print_help()
{
	register OptionP	op;
	register int		indent= HELP_STRING_INDENT_DEPTH;
	register int		head;
	register SuffixP	sp;
	register char		*rest;
	register ProgramP	pp;
	register int		mask;

	for (op= options; op && (op->type != end_of_list); op++)
	{
	    if (driver.value == &dummy)
	    {
		print_help_prefix(op->drivers);
	    }
	    else
	    {
		if (((op->drivers & driver.value->value) == 0) ||
		    ((op->drivers & HIDE_OPTION) != 0))
			continue;
	    }

	    switch (op->type)
	    {
		case help_option:
			(void)printf("%s:%*sPrints this message\n",
				op->name, indent-strlen(op->name), "");
			break;
		case infile_option:
			(void)printf("%sX:%*s%s\n",
				op->name, indent-1-strlen(op->name),
				"", op->help);
			break;
		case lint1_option:
		case lint_i_option:
		case lint_n_option:
			(void)printf("%s:%*s%s\n", op->name,
				indent-strlen(op->name), "", op->help);
			break;
		case make_lint_lib_option:
			(void)printf("%s:%*sMake lint library\n",
				op->name, indent-strlen(op->name), "");
			break;
		case module_option:
			(void)printf("%s X:%*sForce module to load from specified file\n",
				op->name, indent-2-strlen(op->name), "");
			break;
		case module_list_option:
			(void)printf("%sX:%*sAdd directory to path used when looking for modules\n",
				op->name, indent-1-strlen(op->name), "");
			break;
		case optimize_option:
			(void)printf("%s:%*sGenerate optimized code\n",
				op->name, indent-strlen(op->name), "");
			break;
		case outfile_option:
			(void)printf("%s file:%*sSet name of output file\n",
				     op->name, indent-5-strlen(op->name), "");
			break;
		case pass_on_lint_option:
			(void)printf("%s:%*s%s\n", op->name,
				indent-strlen(op->name), "", op->help);
			break;
		case pass_on_select_option:
			(void)printf("%*s The -Qoption group of options\n",
				indent, "");
			for ( pp= (ProgramP)&program;
			      pp != &program.sentinel_program_field;
			      pp++ )
			{
				if (pp->name == NULL)
				{
					continue;
				}
				mask= (op->drivers&pp->drivers)|
					((op->drivers|pp->drivers)&HIDE_OPTION);
				if (driver.value == &dummy)
				{
					print_help_prefix(mask);
				}
				else
				{
					if (((mask & driver.value->value) == 0)
					    || ((mask & HIDE_OPTION) != 0))
						continue;
				}
				(void)printf("%s %s X:%*sPass option X on to program %s\n",
						op->name, pp->name,
						indent-3-strlen(op->name)
							-strlen(pp->name),
						"",pp->name);
			}
			break;
		case pass_on_1_option:
			head= 1;
			rest= "";
			goto pass_on;
		case pass_on_1t12_1t_option_pc:
		case pass_on_1t12_1t_option:
		case pass_on_1t_option:
			head= 1;
			rest= "X";
			goto pass_on;
		case pass_on_12_option:
			head= 1;
			rest= " X";
			goto pass_on;
		case pass_on_1to_option:
			head= 0;
			rest= "X";
			goto pass_on;
	pass_on:
			(void)printf("%s%s:%*s",
				op->name, rest,
				indent-strlen(op->name)-strlen(rest), "");
			if (op->help)
			{
				(void)printf("%s\n", op->help);
			}
			else
			{
				if (!head && (rest[0] == ' '))
				{
					rest++;
				}
				(void)printf("Pass option '%s%s' on to program %s\n",
					head ? op->name : "",
					rest, op->program->name);
			}
			break;
		case produce_option:
			(void)printf("%*s The -Qproduce group of options\n",
				indent, "");
			for ( sp= (SuffixP)&suffix;
			      sp != &suffix.sentinel_suffix_field;
			      sp++)
			{
				if (sp->suffix[0] == '\0')
				{
					continue;
				}
				mask= (op->drivers&sp->out_drivers) |
				      ((op->drivers|sp->out_drivers)&HIDE_OPTION);
				if (driver.value == &dummy)
				{
					print_help_prefix(mask);
				}
				else
				{
					if (((mask & driver.value->value) == 0)
					    || ((mask & HIDE_OPTION) != 0))
					{
						continue;
					}
				}
				(void)printf("%s .%s:%*sProduce type .%s file (%s)\n",
					op->name, sp->suffix,
					indent-2-strlen(op->name)
						-strlen(sp->suffix),
					"", sp->suffix, sp->help);
			}
			break;
		case path_option:
			(void)printf("%s X:%*sLook for compiler passes in directory X first\n",
					op->name, indent-2-strlen(op->name),"");
			break;
		case run_m2l_option:
			(void)printf("%s X:%*sRun the Modula-2 linker on the specified root module\n",
					op->name, indent-2-strlen(op->name),"");
			break;
		case load_m2l_option:
			(void)printf("%s X:%*sLoad the specified Modula-2 module\n",
					op->name, indent-2-strlen(op->name),"");
			break;
		case set_int_arg_option:
			(void)printf("%s:%*s", op->name,
				indent-strlen(op->name), "");
			if (op->help)
			{
				(void)printf("%s\n", op->help);
			}
			else
			{
				(void)printf("Sets %s\n",
					op->variable->help, op->constant->name);
			}
			break;
		case set_named_int_arg_option:
		case set_target_arch_option1:
		case set_target_proc_option1:
			(void)printf("%s:%*s", op->name,
				indent-strlen(op->name), "");
			if (op->help)
			{
				(void)printf("%s\n", op->help);
			}
			else
			{
				(void)printf("Sets value of %s to %s\n",
					op->variable->help, op->constant->name);
			}
			break;
		case set_target_arch_option2:
			(void)printf("%s arch-type:%*sSet target architecture to arch-type\n",
				     op->name, indent-11-strlen(op->name), "");
			break;
		case set_sw_release_option:
#ifdef TELL_ABOUT_SW_RELEASE_OPTION
			(void)printf("%s release:%*sSet target s/w release #\n",
					op->name, indent-9-strlen(op->name),
					"");
#endif /*TELL_ABOUT_SW_RELEASE_OPTION*/
			break;
		case temp_dir_option:
			(void)printf("%sdir:%*sSet directory for temporary files to <dir>\n",
				op->name, indent-3-strlen(op->name), "");
			break;
		default:
			fatal("Internal error: help optype %d?", (int)op->type);
	    }
	}

	if (driver.value != &xlint)
	{
		(void)printf("All other options are passed down to ld.\n");
	}

	for (sp= (SuffixP)&suffix; sp != &suffix.sentinel_suffix_field; sp++)
	{
		if (sp->suffix[0] == '\0')
		{
			continue;
		}
		if (driver.value == &dummy)
		{
			print_help_prefix(sp->in_drivers);
		}
		else
		{
			if (((sp->in_drivers & driver.value->value) == 0) ||
			    ((sp->in_drivers & HIDE_OPTION) != 0))
			{
				continue;
			}
		}
		(void)printf("Suffix '%s':%*s%s\n",
			sp->suffix, indent-9-strlen(sp->suffix), "", sp->help);
	}

	(void)printf("'file.X=.Y' will read the file 'file.X' but treat it as if it had suffix 'Y'\n");

	if (driver.value != &xlint)
	{
		(void)printf("%s:%*sEnvironment variable with floating point option\n",
			FLOAT_OPTION, indent-strlen(FLOAT_OPTION), "");
	}
}	/* print_help */

/*
 *	lint_lib
 *		Transform "-lx" notation to corresponding lint library
 */
char *
lint_lib(lib)
	char			*lib;
{
	char			path[MAXPATHLEN];
	char			*lint_path;

	switch (target_sw_release[R_PATHS].value->value)
	{ 

	    case SW_REL_DFLT:
		/* sw release is not yet defined, lets postpone
		 * setting the lint_path until after the sw release
		 * is determined. This will only happen for -lXXX
		 * options on the command line. We set the sw release
		 * after parsing the command line
		 */
		lint_path = DUMMY_FLAG;
		break;
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
		fatal("Invalid software release specified for lint library");
        }

	(void)sprintf(path, "%s/lint/llib%s.ln", lint_path, lib);

	return scan_Qpath_and_vroot(path, FALSE);
}	/* lint_lib */

/*
 *	handle_infile
 *		If argument cannot be recognized as an option handle_infile()
 *		gets it.
 */
static void
handle_infile(file, extra_drivers)
	register char		*file;
	int			extra_drivers;
{
	register char		*suffix_string;
	register char		*p;
	register SuffixP	suffixp;

	if (file[0] == '-')
	{
		/* If this is an "-l" style infile we assign it a suffix. */
		/* For lint it is translated to a proper filename. */
		if (driver.value == &xlint)
		{
			file= lint_lib(file);
			suffix_string= suffix.ln.suffix;
		}
		else
			suffix_string= suffix.a.suffix;
	}
	else
	{
		/* else we get the suffix from the filename itself */
		suffix_string= get_file_suffix(file);
		/* Remove "foo.x=.o" type options */
		if ((p= index(file, '=')) != NULL)	*p= '\0';
	}

	/* Inline files are filtered out.
	 * The inline program is activated with them as args.
	 */
	if (STR_EQUAL(suffix_string, suffix.il.suffix))
	{
		set_flag(do_inline);
		append_list(&program.inline.permanent_options, "-i");
		append_list(&program.inline.permanent_options, file);
		append_list(&report, file);
		return;
	}

	/* Check if the suffix of the file is known to the driver */
	for (suffixp= (SuffixP)&suffix; suffixp->suffix != NULL; suffixp++)
	{
		if ( (STR_EQUAL(suffixp->suffix, suffix_string)) &&
		     ( ( (suffixp->in_drivers|extra_drivers) &
			 driver.value->value) != 0 )
		   )
		{
			/* If we know about the suffix we add the file to the
			 * infile list.
			 */
			append_list_with_suffix(&infile, file, suffixp);
			infile_count++;
			if ((suffixp->in_drivers&SOURCE_SUFFIX) == SOURCE_SUFFIX)
			{
				source_infile_count++;
			}
			return;
		}
	}

	/* We pass unknown files to one of: cpp, lint1, or ld. */

	/* If this is the "cc" driver, and we've already seen the -P or -E
	 * flag, then pass all filenames with unknown suffixes to CPP.
	 * The user is on his/her own if he/she requests it.
	 * E.g.:     cc -E foo.y
	 */
	if ( (driver.value == &cc) &&
	     ( (product.value == &preprocessed) ||
	       (product.value == &preprocessed2) )
	   )
	{
		/* Treat it as if it had a ".c" suffix to start with,
		 * and wish the user "Good Luck".
		 */
		append_list_with_suffix(&infile, file, &suffix.c);
		infile_count++;
		source_infile_count++;
	}
	else
	{
		if (driver.value == &xlint)
		{
			warning("File with unknown suffix (%s) passed to lint1",
					file);
			append_list_with_suffix(&infile, file, &suffix.c);
			infile_count++;
			source_infile_count++;
		}
		else
		{
			warning("File with unknown suffix (%s) passed to ld",
					file);
			append_list_with_suffix(&infile, file, &suffix.o);
			infile_count++;
		}
	}
}	/* handle_infile */


static void
check_for_redefinition(named_intp, constp, arg1, arg2, arg3)
	register Named_intP	named_intp;
	register Const_intP	constp;
	char		*arg1;	/* first  argument from command line */
	char		*arg2;	/* second argument from command line */
	char		*arg3;	/* third  argument from command line */
{
	/* If the given Named_int is about to be redefined, issue a warning. */
	if ( named_intp->touched &&
	     !named_intp->redefine_ok &&
	     (named_intp->value != constp) )
	{
		warning("\"%s%s%s\" redefines %s from \"%s\" to \"%s\"",
			arg1,
			(arg2 == (char *)NULL) ?"" : arg2,
			(arg3 == (char *)NULL) ?"" : arg3,
			named_intp->help,
			named_intp->value->name,
			constp->name);
	}
}


static void
set_named_int_option(options, argument)
	register Option		*options;
	char			*argument;
{
	/* Handle options that set state which has multiple values.
	 * Warn about redefinitions.
	 */
	check_for_redefinition(options->variable, options->constant,
				argument, (char *)NULL, (char *)NULL );
	set_named_int(*(options->variable), options->constant);
}


/*
 *	sw_release_lookup()
 *		Lookup the software release number given among the known
 *		Const_int's, and return a pointer to one of them.
 */
Const_intP
sw_release_lookup(string)
	register char *string;
{
	register int	i;

	for (i = 0;   known_sw_releases[i] != NULL;   i++)
	{
		if ( STR_EQUAL(string, known_sw_releases[i]->name))
		{
			return known_sw_releases[i];
		}
	}

	fatal("Unknown target software release \"%s\"", string);
	/*NOTREACHED*/
}


static void
illegal_optimize_option(opt_level)
	char opt_level;
{
	fatal("Illegal optimization option \"-O%c\"", opt_level);
}


static ProgramP
lookup_program(program_name)
	char *program_name;
{
	register ProgramP	pp;

	for ( pp= (ProgramP)&program;
	      pp != &program.sentinel_program_field;
	      pp++)
	{
		if ((pp->name != NULL) && strcmp(program_name, pp->name) == 0)
		{
			return pp;
		}
	}

	/* didn't find it. */
	return (ProgramP)NULL;
}


static void
set_a_target_sw_release_value(which, release_name, arg1, arg2, arg3)
	register int	which;
	register char	*release_name;
	char		*arg1;	/* first  argument from command line */
	char		*arg2;	/* second argument from command line */
	char		*arg3;	/* third  argument from command line */
{
	register Const_intP	constp;

	constp = sw_release_lookup(release_name);
	check_for_redefinition(&target_sw_release[which], constp,
				arg1, arg2, arg3);
	set_named_int(target_sw_release[which], constp);

	if (is_on(debug))
	{
		fprintf(stderr, "[rel[%d]=\"%s\" (%s%s%s)]\n",
			which, target_sw_release[which].value->name,
			arg1, arg2, arg3);
	}
}


static void
set_all_target_sw_release_values(release_name, arg1, arg2, arg3)
	register char	*release_name;
	char		*arg1;	/* first  argument from command line */
	char		*arg2;	/* second argument from command line */
	char		*arg3;	/* third  argument from command line */
{
	register int i;

	/* Set all "R_elements" components of the target S/W release to
	 * the given value.
	 */
	for ( i = 0;   i < R_elements;   i++)
	{
		set_a_target_sw_release_value(i, release_name,
						arg1, arg2, arg3);
	}
}


static void
set_all_defaulted_target_sw_release_values(release_name, arg1, arg2, arg3)
	register char	*release_name;
	char		*arg1;	/* first  argument from command line */
	char		*arg2;	/* second argument from command line */
	char		*arg3;	/* third  argument from command line */
{
	register int i;

	/* Set all "R_elements" components of the target S/W release to
	 * the given value.
	 */
	for ( i = 0;   i < R_elements;   i++)
	{
		if (target_sw_release[i].value->value == SW_REL_DFLT)
		{
			set_a_target_sw_release_value(i, release_name,
							arg1, arg2, arg3);
		}
	}
}


/*
 *	skip_leading_minus()
 */
static char *
skip_leading_minus(arch_name)
      char *arch_name;
{
      /* This is mostly used to skip the leading '-' from target
       * architecture names.  A minus, if present, is probably a historical
       * artifact, present because "make"'s TARGET_ARCH variable carries
       * around a '-' prefix.
       */
      if (*arch_name == '-')	return  arch_name + 1;
      else			return  arch_name;
}


/*
 *	lookup_architecuture()
 */
static Const_intP
lookup_architecture(arch_name)
	char		*arch_name;
{
	register int	i;
	register int	len;
	char		buffer[LOCAL_STRING_LENGTH];

	for (i = 0;   known_architectures[i] != NULL;   i++)
	{
		len = strlen(known_architectures[i]->extra);
		if ( STR_EQUAL_N(arch_name, known_architectures[i]->extra, len)
		   )
		{
			switch (arch_name[len])
			{
			case '\0': /* the names were an exact match. */
				return known_architectures[i];

			default:
				fatal("Unknown Sun target architecture \"%s\"",
					arch_name);
				break;
			}
		}
	}

#ifdef CROSS
	/* Didn't find it.  Assume it's a foreign architecture type. */
	sprintf(buffer, "-target %s", arch_name);
	arch_foreign.name  = make_string(buffer);
	arch_foreign.extra = make_string(arch_name);
	return  &arch_foreign;
#else /*!CROSS*/
	fatal("Cross-compilers required for target architecture \"%s\"",
		arch_name);
#endif /*CROSS*/
}



/*
 *	set_target_architecture()
 */
static void
set_target_architecture(targ_arch_variable, arch_name, from_processor_type)
	Named_intP	targ_arch_variable;
	char		*arch_name;
	Bool		from_processor_type;
{
	register Const_intP	arch_constp;
	register int		length;

	/* Flag whether we are setting the target architecture type from a real
	 * architecture type (e.g. sun3), or a processor type (e.g. mc68020).
	 */
	target_arch_set_from_processor_type = from_processor_type;

	arch_constp = lookup_architecture(arch_name);

	/* Set the target architecture type. */
	set_named_int(*targ_arch_variable, arch_constp);

	if (is_on(debug))
	{
		fprintf(stderr, "[target_arch=\"%s\"]\n",
			target_arch.value->extra);
	}

}

static void
request_default_opt_level(arg)
	char	*arg;
{
	/* arg[0] is "-".
	 * arg[1] is "O" or "P".
	 */
	switch (arg[1])
	{
	case 'P':
		optimizer_level= OPTIM_IROPT_P;
		break;
	case 'O':
		/* Set the optimizer level later, based on the language type
		 * and the target architecture (which we don't necessarily
		 * know yet when we see the -O/-P).
		 */
		use_default_optimizer_level = TRUE;
		break;
	}
}

/*
 *	lookup_option
 *		Scan the options list for one particular option and process it
 */
static int
lookup_option(options, argc, argv, infile_ok, extra_drivers)
	register Option		*options;	/* list of valid options. */
	int			*argc;		/* the count of options given.*/
	char			*argv[];	/* text of the options given. */
	register int		infile_ok;
	register int		extra_drivers;
{
	register int		args_used= 1;
	register int		length;
	register char		*p;
	register SuffixP	sp;
	register ProgramP	pp;

	if (*argc <= 0)
	{
		fatal("No more arguments");
	}

	/* Scan the list of options and try to find the current one */
	for (  ;  options && (options->type != end_of_list); options++)
	{
	    if ((driver.value != NULL) &&
		(driver.value->value != 0) &&
		((options->drivers|extra_drivers) & driver.value->value) == 0)
	    {
		continue;
	    }

	    if ((strncmp(argv[0], options->name,
			length= strlen(options->name)) == 0))
	    {
		/* Some options takes extra argments that may not start with
		 * a "-".
		 */
		if (((options->drivers&NO_MINUS_OPTION) != 0) && (*argc > 1)
		    && (argv[1][0] == '-'))
		{
			warning("Extra argument for %s starts with '-'",
				     options->name);
		}
		/* Go to the action routine now that the option has been
		 * identified.
		 */
		switch (options->type)
		{
		    case help_option:
			print_help();
			exit(0);

		    case infile_option:
			/* This handles "-l" type options */
			/* Special case "pc -l" */
			if ((argv[0][0] == '-') && (argv[0][1] == 'l') &&
			    (argv[0][2] == '\0') && (driver.value == &pc))
			{
				append_list(&program.pc0.permanent_options,
						"-l");
				break;
			}
			handle_infile(argv[0], extra_drivers);
			goto exit_fn;

		    case lint1_option:
			append_list(&program.lint1.trailing_options, argv[0]);
			break;

		    case lint_i_option:
			if (length == strlen(argv[0]))
			{
				set_named_int(product, &lint1_file);
			}
			else	args_used= 0;
			break;

		    case lint_n_option:
			if (length == strlen(argv[0]))
			{
				set_flag(ignore_lc);
			}
			else	args_used= 0;
			break;

		    case make_lint_lib_option:
			/* This handles "lint -Cfoo" type options */
			append_list(&program.lint1.permanent_options, "-L");
			p= get_memory(strlen(argv[0])+8);
			(void)sprintf(p, "-Cllib-l%s", argv[0]+2);
			append_list(&program.lint1.permanent_options, p);
			p= get_memory(strlen(argv[0])+8);
			(void)sprintf(p, "llib-l%s.ln", argv[0]+2);
			outfile= p;
			requested_suffix= &suffix.ln;
			if (make_lint_lib_fd++ > 0)
			{
				fatal("Multiple -C options");
			}
			break;

		    case module_option:
			/* This handles "m2c -m X" type options */
			if (*argc <= 1)
			{
				fatal("-m option with no argument");
			}
			args_used++;
			append_list(&program.m2cfe.permanent_options, argv[0]);
			append_list(&program.m2cfe.permanent_options, argv[1]);
			append_list(&program.m2l.permanent_options, argv[0]);
			append_list(&program.m2l.permanent_options, argv[1]);
			break;

		    case module_list_option:
			/* This handles "m2c -MX" type options */
			/* If X is empty we clear the list */
			if (argv[0][2] == '\0')
			{
				module_list= NULL;
				set_flag(no_default_module_list);
			}
			else
			{
				append_list(&module_list, argv[0]);
			}
			break;

		    case optimize_option:
			/* This handles the "-OX" and "-PX" type options */
			/* Pickup the X */
			if (argv[0][2] == '\0')
			{
				/* No optimization level was specified in this
				 * optimization option (just plain "-O"/"-P").
				 */
				if (optimizer_level == OPTIM_NONE)
				{
					/*
					 * Set <level> to a default value.
					 */
					request_default_opt_level(argv[0]);
				}
				else
				{
					/* -O<level> was previously specified;
					 * retain the previous value of <level>.
					 * This allows the user to raise the
					 * optimization level without changing
					 * makefiles (e.g. make CC="cc -O2",
					 * where "${CC} -O foo.c" was already
					 * in the Makefile).
					 */
				}
			}
			else
			{
				optimizer_level= argv[0][2];
			}

			/* Validity of the optimization level is checked at
			 * in main(), after the host & target architectures
			 * are known to be set.
			 */

			break;

		    case outfile_option:
			if (*argc <= 1)
			{
				fatal("-o option but no filename");
			}
			args_used++;
			outfile= argv[1];
			break;

		    case pass_on_lint_option:
			if (length == strlen(argv[0]))
			{
				append_list(&program.lint1.permanent_options,
					argv[0]);
				append_list(&program.lint2.permanent_options,
					argv[0]);
			}
			else	args_used= 0;
			break;

		    case pass_on_select_option:
			/* Handle "-Qoption" type options */
			if (*argc <= 2)
			{
				fatal("%s option without arguments", argv[0]);
			}
			args_used+= 2;
			/* Find the program the option is intended for */
			if ( (pp = lookup_program(argv[1])) == NULL )
			{
				fatal("%s option with unknown program %s",
					argv[0], argv[1]);
			}
			else
			{
				append_list(&pp->trailing_options, argv[2]);
			}
			break;

		    case pass_on_1_option:
			/* cc -x	=> prog -x */
			if (length != strlen(argv[0]))
			{
				fatal("Unknown option \"%s\"", argv[0]);
			}
			/* Fall into */

		    case pass_on_1t_option:
			/* cc -xREST	=> prog -xREST */
			switch ( options->subtype )
			{
			case PASS_OPT_AS_FLAG:
				append_list(
					&options->program->permanent_options,
					argv[0]);
				/* special hack for XL_FLAG
				 * we need to also add -Q flag
				 * and run cppas instead of cpp
				 */
				if (! strcmp(argv[0], XL_FLAG))
				{
				append_list(
					&options->program->permanent_options,"-Q");
				set_flag(do_cppas);
				}
				break;
			case PASS_OPT_AS_INFILE:
				handle_infile(argv[0], extra_drivers);
				break;
			default:
				fatal("1t subtype %d?", options->subtype);
			}
			break;

		    case pass_on_1t12_1t_option:
			/*       cc -xREST	=> prog -xREST
			 * [or]  cc -x REST	=> prog -xREST
			 * [but] cc -x -REST    => [-x is ignored]
			 *
			 * Used for Sys-V compatibility, for -U and -D.
			 */
			p = NULL;
			switch ( strlen(argv[0]) )
			{
			case 2:	/* -x REST; make into -xREST, unless the next
				 * REST begins with "-", which makes it the
				 * next argument.
				 */
				if (*argc <= 1)
				{
					fatal("%s option without arguments",
						argv[0]);
				}
				/* if there are only two arguments
				 * the command line arguments are
				 * wrong, could be -D t.c or
				 *                 -D xxxxx 
				 */
				if (*argc == 2)
				{
					fatal("Invalid combination of command line arguments");
				}
				if (*argv[1] != '-')
				{
					args_used++;
					p = get_memory(2 + strlen(argv[1])
							 + 1/*'\0'*/ );
					strcpy(p,   argv[0]);
					strcpy(p+2, argv[1]);
				}
				break;
			default:/* -xREST; use as-is. */
				p = argv[0];
				break;
			}
			if (p != NULL)
			{
				append_list(&options->program->trailing_options, p);
			}
			break;

		case pass_on_1t12_1t_option_pc:
			/*	 cc -configREST   => prog -xREST
			 * [or]  cc -config REST  => prog -xREST
			 * [but] cc -config -REST => [-x is ignored]
			 *
			 * used to pass option ro pc0 in pascal
			 */
			p = NULL;
			switch (strlen(argv[0]) )
			{
			case 7:
				if (*argc <= 1)
				{
					fatal("%s option without arguments", argv[0]);
				}
				if (*argv[1] != '-')
				{
					args_used++;
					p = get_memory(7 + strlen(argv[1]) + 1/*'0'*/ );
					strcpy(p,   argv[0]);
					strcpy(p+7, argv[1]);
				}
				break;
			default: /* -xREST; use as-is. */
				p = argv[0];
				break;
			}
 
			if (p != NULL)
			{
				append_list(&options->program->trailing_options, p);
			}
			break;

		    case pass_on_12_option:
			/* cc -x REST	=> prog -x REST */
			if (*argc <= 1)
			{
				fatal("%s option without arguments", argv[0]);
			}
			args_used++;
			append_list(&options->program->permanent_options,
				argv[0]);
			append_list(&options->program->permanent_options,
				argv[1]);
			break;

		    case pass_on_1to_option:
			/* cc -xREST	=> prog REST */
			if (argv[0][length] != '\0')
			{
				append_list(&options->program->permanent_options,
					argv[0]+length);
			}
			break;

		    case produce_option:
			/* Handle "-Qproduce" type options */
			if (*argc <= 1)
			{
				fatal("%s option without argument", argv[0]);
			}
			args_used++;
			/* Scan the legal suffixes to find the one specified */
			for (sp= (SuffixP)&suffix; sp->suffix != NULL; sp++)
			{
				if (strncmp(argv[1]+1, sp->suffix,
				      strlen(sp->suffix)) == 0)
				{
					set_named_int(product, NULL);
					requested_suffix= sp;
					goto found_suffix;
				}
			}
			fatal("Don't know how to produce %s", argv[1]);
		      found_suffix:
			break;

		    case path_option:
			/* Handle "-Qpath" type options */
			if (*argc <= 1)
			{
				fatal("%s option without argument", argv[0]);
			}
			args_used++;
			add_dir_to_path(argv[1], &program_path, -1);
			last_program_path++;
			break;

		    case run_m2l_option:
			/* Handle "m2c -e" type options */
			if (is_on(root_module_seen))
			{
				fatal("Multiple %s options", argv[0]);
			}
			set_flag(root_module_seen);
			/* Fall into !!! */

		    case load_m2l_option:
			/* Handle "m2c -E" type options */
			if (*argc <= 1)
			{
				fatal("%s option without argument", argv[0]);
			}
			if (product.touched &&
			    !product.redefine_ok &&
			    (product.value != &executable))
			{
				warning("%s: %s redefines %s from %s to %s",
					program_name,
					argv[0],
					product.help,
					product.value->name,
					executable.name);
			}
			set_named_int(product, &executable);
			/* Pass option to "m2l" in sequence with other infiles*/
			append_list_with_suffix(&infile, argv[0], &suffix.o);
			append_list_with_suffix(&infile, argv[1], &suffix.o);
			args_used++;
			set_flag(do_m2l);
			break;

		    case set_int_arg_option:
			/* Handle options that only set some binary state in
			 * the driver.
			 */
			if ( (options->constant) != (Const_intP)NULL )
			{
				*((char *)(options->variable))= 1;
			}
			else
			{
				*((char *)(options->variable))= 0;
			}
			break;

		    case set_named_int_arg_option:
			/* Handle options that set state that has multiple
			 * values.  Warn about redefinitions.
			 */
			set_named_int_option(options, argv[0]);
			break;

		    case set_target_arch_option1:
			/* Handle 1-arg target-architecture-setting option. */

			/* Set the target architecture type. */
			set_target_architecture(options->variable, &argv[0][1],
						FALSE);
			break;

		    case set_target_arch_option2:
			/* Handle 2-arg target-architecture-setting option. */

			if (*argc <= 1)
			{
				fatal("missing architecture after \"-target\"");
			}
			args_used++;

			/* Set the target architecture type. */
			/* The reference to skip_leading_minus() below is to
			 * handle the possible instance of a historical artifact
			 * (leading '-' on target architecture name).  See that
			 * routine for more comments.
			 */
			set_target_architecture(options->variable,
						skip_leading_minus(argv[1]),
						FALSE);
			break;

		    case set_target_proc_option1:
			/* Handle target-processor-setting option. */

			processor_type_option = options;

			/* Set the target architecture type. */
			set_target_architecture(options->variable,
					options->constant->extra, TRUE);
			break;

		    case set_sw_release_option:
			if (*argc <= 1)
			{
				fatal("missing release# after \"-release\"");
			}
			args_used++;
			if ( options->subtype == R_all )
			{
				/* Set all of the options. */
				set_all_target_sw_release_values(argv[1],
							argv[0], " ", argv[1]);
			}
			else
			{
				set_a_target_sw_release_value( options->subtype,
						argv[1], argv[0], " ", argv[1]);
			}
			break;

		    case temp_dir_option:
			/* Handle "-temp=" type options */
			if (argv[0][length] == '\0')
			{
				fatal("Bad %s option", argv[0]);
			}
			temp_dir= make_string(argv[0]+length);
			break;

		    default:
			fatal("Internal error");
		}
		goto exit_fn;
	    }
	}
	/* We couldn't find that option. Maybe it is an input file name? */
	if (!infile_ok || (argv[0][0] == '-'))
	{
		/* Unknown option. Pass to ld */
		append_list(&program.ld.permanent_options, argv[0]);
		if (driver.value == &xlint)
		{
			fatal("Option %s not recognized", argv[0]);
		}
		else
		{
			warning("Option %s passed to ld", argv[0]);
		}
	}
	else
	{
		/* We assume it is an input file name. */
		handle_infile(argv[0], extra_drivers);
	}
exit_fn:
	*argc -= args_used;
	return args_used;
}	/* lookup_option */

/*
 *	lookup_lint_option
 *		Split bundled options then
 *		scan the options list for one particular option and process it
 */
static int
lookup_lint_option(options, argc, argv, infile_ok, extra_drivers)
	Option			*options;
	int			*argc;
	char			*argv[];
	int			infile_ok;
	int			extra_drivers;
{
	int			args_used;
	char			**new_argv;
	int			new_argc= 0;
	char			**ap;
	char			*p;

	if ((args_used= lookup_option(options, argc, argv, infile_ok,
				extra_drivers)) > 0)
	{
		return args_used;
	}

	/* Split option word into a list of single char options */
	ap= new_argv= (char **)alloca(sizeof(char *)*((strlen(argv[0])*2)+1));
	for (p= argv[0]+1; *p != '\0'; p++)
	{
		*ap= get_memory(3);
		ap[0][0]= '-';
		ap[0][1]= *p;
		ap[0][2]= '\0';
		ap++;
		new_argc++;
	}
	*ap= NULL;
	while (new_argc > 0)
	{
		new_argv+= lookup_option(options, &new_argc, new_argv,
				infile_ok, extra_drivers);
	}
	*argc-= 1;
	return 1;
/*
a
b
c
h
i	set product
n	set ignore_lc
u
v	
x
z
*/
}


/*
 *	lookup_one_flag_option
 *		Lookup a single option using lookup_option().
 */
static void
lookup_one_flag_option(option_name, prepend_minus)
char	*option_name;
Bool	prepend_minus;
{
	char	option[LOCAL_STRING_LENGTH];
	char	*argv[2];
	int	argc;

	if (prepend_minus)
	{
		option[0]= '-';
		(void)strcpy(&option[1], option_name);
		argv[0] = option;
	}
	else
	{
		argv[0] = option_name;
	}

	argv[1] = NULL;
	argc = 1;
	(void)lookup_option(options, &argc, argv, 0, 0);
}


/*
 *	default_float_mode
 *		Figure out what to use as the default float mode
 *		Use the FLOAT_OPTION environment variable
 */
static Const_intP
default_float_mode()
{
	register char		*float_option= getenv(FLOAT_OPTION);

	if (float_option && (driver.value != &xlint))
	{
		lookup_one_flag_option(float_option, (float_option[0] != '-'));
		if (float_mode.value == NULL)
			fatal("%s value not ok:%s", FLOAT_OPTION, float_option);
		return float_mode.value;
	}
	return &fsoft;
}	/* default_float_mode */

/*
 *	check_float_arguments
 *		Check if the float mode/target_arch combination is legal
 */
static void
check_float_arguments()
{
	if (float_mode.value == NULL) 
	{
		float_mode.value = default_float_mode();
	}

	if ( ( (target_arch.value->value == ARCH_SUN2) &&
	       (float_mode.value == &ffpa) )
			||
	     ( (target_arch.value->value == ARCH_SUN3X) &&
	       (float_mode.value == &fsky) )
			||
	     ( (target_arch.value->value == ARCH_SUN3) &&
	       (float_mode.value == &fsky) )
			||
	     ( (target_arch.value->value == ARCH_SUN386) &&
	       (float_mode.value != &fsoft) )
			||
	     ( (target_arch.value->value == ARCH_SUN4C) &&
	       (float_mode.value != &fsoft) )
			||
	     ( (target_arch.value->value == ARCH_SUN4) &&
	       (float_mode.value != &fsoft) )
	   )
	{
		(void)fprintf(stderr,
			"%s: \"%s\" is invalid for the %s target architecture\n",
			program_name,
			float_mode.value->name,
			target_arch.value->extra);
		cleanup(0);
		exit(1);
	}
}	/* check_float_arguments */

/*
 *	cleanup
 *		Remove temp files before we exit
 */
void
cleanup(abort)
	int			abort;
{
	register ListP		cp;

	/* We saved our reports till now to avoid conflicts with other
	 * reporters.
	 */
	for (cp= report; cp != NULL; cp= cp->next)
	{
		report_dependency(cp->value);
#ifdef NSE
		report_libdep(cp->value, "COMP");  
#endif /*NSE*/
	}

	if (is_on(remove_tmp_files))
	{
		for (cp= files_to_unlink; cp != NULL; cp= cp->next)
		{
			if (cp->value != NULL)
			{
				if ( ( is_on(verbose) || is_on(dryrun) ) &&
				     is_on(show_rm_commands)
				   )
				{
					(void)fprintf(stderr,
							"rm %s\n", cp->value);
				}
				if (abort)
				{
					(void)truncate(cp->value, 0);
				}
				/* if dryrun is on DO NOT remove 
				 * any files 
				 */
				if ( is_off(dryrun) )
				{
					(void)unlink(cp->value);
				}
			}
		}
	}
}	/* cleanup */

/*
 *	fatal error reporting routine
 */
/*VARARGS1*/
void
warning(msg, a, b, c, d, e, f, g)
	char		*msg;
{
	(void)fprintf(stderr, "%s: Warning: ", program_name);
	(void)fprintf(stderr, msg, a, b, c, d, e, f, g);
	(void)fprintf(stderr, "\n");
}

/*
 *	fatal error reporting routine
 */
/*VARARGS1*/
void
fatal(msg, a, b, c, d, e)
	char		*msg;
{
	(void)fprintf(stderr, "%s: ", program_name);
	(void)fprintf(stderr, msg, a, b, c, d, e);
	(void)fprintf(stderr, "\n");
	cleanup(0);
	exit(1);
}

/*
 *	abort_program
 */
static void
abort_program()
{
	cleanup(1);
	exit(1);
}	/* abort_program */

#if RELEASE == 32
#  define SIGNAL_TYPE	int
#else /* RELEASE != 32 */
#  define SIGNAL_TYPE	void
#endif /* RELEASE == 32 */
/*
 *	set_signal sets a handler for a signal if the signal is not ignored
 */
void
set_signal(sig, handler)
	int		sig;
	SIGNAL_TYPE	(*handler)();
{
	SIGNAL_TYPE	(*old_handler)();

	old_handler= signal(sig, handler);
	if (old_handler == SIG_IGN)
	{
		(void)signal(sig, SIG_IGN);
	}
}


/*
 *	Divine the target processor type from the target architecture type.
 */
char *
get_processor_type(archvalue)
	Const_intP archvalue;
{
	switch (archvalue->value)
	{
	case ARCH_SUN2:		return "mc68010";
	case ARCH_SUN3X:	return "mc68020"; /* Really!  020, not 030! */
	case ARCH_SUN3:		return "mc68020";
	case ARCH_SUN386:	return "i386";
	case ARCH_SUN4C:	return "sparc";
	case ARCH_SUN4:		return "sparc";
	}
}


/*
 *	Get the target processor flag for the given target architecture type.
 */
char *
get_processor_flag(archvalue)
	Const_intP archvalue;
{
	char			buffer[LOCAL_STRING_LENGTH];

	/* The target processor flag is just "-" plus the processor type. */
	buffer[0] = '-';
	strcpy(&buffer[1], get_processor_type(archvalue));
	return make_string(buffer);
}

enum ShellType { SHELL_SH, SHELL_CSH };

static enum ShellType
guess_shell_type()
{
	/* To make an educated guess as to which shell the user is using,
	 * we examine the SHELL environment variable.
	 * If it ends in "sh" but not "csh", we'll assume that the user
	 * is using a Bourne or Bourne-compatible Shell (ksh, etc).
	 * Otherwise, we'll assume that it's a C-Shell, since it is
	 * probably still in more common use (by non-AT&T UNIX users,
	 * anyway).
	 * This method is NOT foolproof, but does provide a reasonable guess.
	 */

	register char	*shell_name;
	register int	len;

	shell_name = getenv("SHELL");

	if ( (shell_name != NULL) &&
	     ( (len = strlen(shell_name)) >= 2 ) &&
	     ( strcmp(&shell_name[len-2], "sh") == 0  ) &&
	     ( (len < 3) ||
	       (strcmp(&shell_name[len-3], "csh") != 0) )
	   )
	{
		return SHELL_SH;
	}
	else	return SHELL_CSH;
}


/*
 *	Returns TRUE if the path given is to a directory; FALSE otherwise.
 */
static Bool
is_existing_directory(path)
	char	*path;
{
	struct stat statbuf;

	return ( (stat(path, &statbuf) == 0) && 
		 ((statbuf.st_mode & S_IFMT) == S_IFDIR));
}


/*
 *	Figure out what host we are on.
 */
static void
set_host()
{
	/* Set the host architecture type.
	 * On m68k machines, this used to be done with a routine
	 * that could tell a 68020 from a 68010.   This is unnecessary
	 * since we always ship separate binaries for 68010s and 68020s.
	 */
#if   defined(sun4c)
	host_arch.value= &arch_sun4c;
#elif   defined(sun4)
	host_arch.value= &arch_sun4;
#elif defined(sun3x)
	host_arch.value= &arch_sun3x;
#elif defined(sun3)
	host_arch.value= &arch_sun3;
#elif defined(sun386)
	host_arch.value= &arch_sun386;
#elif defined(sun2)
	host_arch.value= &arch_sun2;
#else
	@@@ error -- missing case @@@
#endif

	if (is_on(debug))
	{
		fprintf(stderr, "[host_arch = \"%s\"]\n",
				host_arch.value->extra);
	}
}


static void
set_target()
{
	/* Set the target architecture type.
	 * If it was specified on the command line, we'll use the one given.
	 * If not, and we're in an NSE "variant", use the target architecture
	 * which it specifies.  Otherwise, we assume that the target
	 * architecture is the same as the host architecture (i.e. this is a
	 * native compilation).
	 */

	if (target_arch.value == NULL)
	{
		/* Target architecture was NOT specified explicitly by user,
		 * or by an NSE environment.
		 */
		target_arch = host_arch;
		set_target_architecture(&target_arch,
					host_arch.value->extra, FALSE);
	}
	else
	{
		/* Target architecture was specified explicitly on the command
		 * line or by the containing NSE environment; use the one given.
		 */
	}
}


static void
set_host_and_target()
{
	set_host();
	set_target();
}


/*
 *	Set a couple of the s/w release types, if defaulted.
 */
static void
set_lib_paths_and_option_types(root_path)
	char *root_path;
{
	Const_intP  target_base_OS_release;

	target_base_OS_release =
		get_target_base_OS_version(root_path, driver.value);

	/* "base_driver_release" is a global variable telling what 
	 * driver release we are using.
	 */
	base_driver_release = 
		get_base_driver_version(root_path, driver.value);

	/* This is here just for the sake of debug info... */
	if (is_on(debug))
	{
		fprintf(stderr,"[driver version = %s (%s)]\n",
			base_driver_release, debug_info_filename);
		fprintf(stderr, "[rel[%d]=\"%s\" /*vroot */]\n", R_VROOT,
			target_sw_release[R_VROOT].value->name);
	}


	/* This is necessary so we know where to look for the various compiler
	 * passes and libraries, as different S/W releases put them in
	 * different places.
	 */
	if (target_sw_release[R_PATHS].value->value == SW_REL_DFLT)
	{
		target_sw_release[R_PATHS].value = target_base_OS_release;
	}

	if (is_on(debug))
	{
		fprintf(stderr, "[rel[%d]=\"%s\" /*paths */]\n", R_PATHS,
			target_sw_release[R_PATHS].value->name);
	}

	/* And this is needed so we know what options to pass to the various
	 * compiler passes; as compiler passes in different S/W releases expect
	 * different options.
	 */
	if (target_sw_release[R_PASSES].value->value == SW_REL_DFLT)
	{
		target_sw_release[R_PASSES].value = target_base_OS_release;
	}
	if (is_on(debug))
	{
		fprintf(stderr, "[rel[%d]=\"%s\" /*passes*/]\n", R_PASSES,
			target_sw_release[R_PASSES].value->name);
	}

}


/*
 *	Setup the environment required for native compilation.
 */
setup_for_native_compile()
{
	set_lib_paths_and_option_types("");
}


#ifdef CROSS
/*
 *	Setup the environment required for cross compilation.
 */
setup_for_cross_compile()
{
	char			*old_vroot_path;
	int			len;
	char			*arch_dir;
	char			*cross_vroot;

	if ( (old_vroot_path = getenv(get_vroot_name())) == NULL )
	{
		old_vroot_path = "";
	}

	if ( (arch_dir = getenv("CROSS_COMPILATION_HOME")) == NULL )
	{
		arch_dir = "/usr/arch";
	}

	cross_vroot = (char *)alloca(  strlen(arch_dir)
				     + 1 /*"/"*/
				     + strlen(host_arch.value->extra)
				     + 1 /*"/"*/
				     + strlen(target_arch.value->extra)
				     + strlen(target_sw_release[R_VROOT].value->extra)
				     + 1 /*'\0'*/
				    );

	(void)sprintf(cross_vroot, "%s/%s/%s%s",
			arch_dir,
			host_arch.value->extra,
			target_arch.value->extra,
			target_sw_release[R_VROOT].value->extra);

	if ( is_existing_directory(cross_vroot) )
	{
		/* <whew> -- the cross-tools for the target_arch the user has
		 * requested are (probably) installed.
		 * Go ahead and set up VIRTUAL_ROOT for cross-compilation.
		 */
		register char	*new_vroot_path;
		new_vroot_path = get_memory(  strlen(cross_vroot)
					    + 1 /*":"*/
					    + strlen(old_vroot_path)
					    + 1 /*'\0'*/
					   );
		(void)sprintf(new_vroot_path, "%s:%s",
				cross_vroot, old_vroot_path);

		/* If we are insisting on pure cross-compilation (i.e., no
		 * files from our regular root filesystem are to be used),
		 * then cut off any trailing ":" from the VIRTUAL_ROOT.
		 */
		if ( is_on(pure_cross) &&
		     (new_vroot_path[strlen(new_vroot_path)-1]==':')
		   )
		{
			new_vroot_path[strlen(new_vroot_path)-1] = '\0';
		}

		setenv(get_vroot_name(), new_vroot_path);

		if (is_on(verbose) || is_on(dryrun))
		{
			/* The C-Shell and the Bourne Shell (and its variants)
			 * use different commands to set environment variables.
			 * In order for us to print the Shell command(s) to set
			 * the VIRTUAL_ROOT environment variable, we should
			 * take an educated guess as to which shell the user is
			 * using.
			 *
			 * For SH,  we'll use "<var>=<value>; export <var>".
			 * For CSH, we'll use "setenv <var> <value>".
			 */
			switch ( guess_shell_type() )
			{
			case SHELL_CSH:
				(void)fprintf(stderr, "setenv %s \"%s\"\n",
					get_vroot_name(), new_vroot_path);
				break;
			case SHELL_SH:
				(void)fprintf(stderr, "%s=\"%s\"; export %s\n",
						get_vroot_name(),
						new_vroot_path,
						get_vroot_name());
				break;
			}
		}

		set_lib_paths_and_option_types(cross_vroot);
	}
	else
	{
		/* Hmmm... the user has requested cross-compilation, yet the
		 * directory which would have become the head of the
		 * VIRTUAL_ROOT path doesn't even exist.  The user must be
		 * attempting a cross-compilation without even having any
		 * cross-tools installed for that target_arch.  Stop now,
		 * or we will end up doing a native compilation and the user
		 * probably won't know until he/she tries to execute the linked
		 * object file.
		 *
		 * There are a couple of exceptions to this; see below.
		 *
		 */
		if ( ( (host_arch.value->value == ARCH_SUN3X) ||
		       (host_arch.value->value == ARCH_SUN3 ) )
					&&
		     (target_arch.value->value == ARCH_SUN2)
					&&
		     target_arch_set_from_processor_type )
		{
			/*
			 * This is the first exception to the above.
			 *
			 * The user is on a Sun3 or Sun3x host, and requested
			 * by using the -mc68010 flag that we generate code for
			 * an MC68010 target processor.  In this case, he/she
			 * may not have actually wanted full cross-compilation,
			 * but just to generate 68010 code for the modules being
			 * compiled.  The native compiler can handle this, so
			 * give him/her the benefit of the doubt.
			 * We allow this exception as a form of backward
			 * compatibility.
			 */

			set_lib_paths_and_option_types("");

			/* If the user was trying to get an MC68010 executable,
			 * we'd better warn him/her, and force a "-c" option,
			 * i.e. force the "product" to be relocatable ".o"
			 * files, since there is no way we can generate a real
			 * mc68010 executable without the cross-compiler's
			 * mc68010 libraries.
			 */
			if (product.value->value == GOAL_EXE)
			{
				warning("\"-mc68010\" implies \"-c\"");
				product.value = &object;
			}
		} else if (driver.value == &xlint)
		{
			/* The second exception to this is "lint"; it has
			 * cross-lint'ing builtin via its "-target=<TARGET>"
			 * option.  So, if there is no appropriate virtual root
			 * directory, try to use the native lint.
			 */
			set_lib_paths_and_option_types("");
		}
		else
		{
			fatal("Cannot cross-compile for \"%s%s\"; directory \"%s\" doesn't exist.",
				target_arch.value->extra,
				target_sw_release[R_VROOT].value->extra,
				cross_vroot);
		}
	}

	if ((driver.value == &xlint) && (target_arch.value != &arch_foreign))
	{
		char	option[LOCAL_STRING_LENGTH];
		sprintf(option, "-host=%s", target_arch.value->extra);
		append_list(&program.lint1.permanent_options,
				make_string(option));
		sprintf(option, "-target=%s", target_arch.value->extra);
		append_list(&program.lint1.permanent_options,
				make_string(option));
	}
}
#endif /*CROSS*/


/*
 *	Check for conflict in the arguments given (besides the float/
 *	target_arch conflicts).
 */
static void
check_for_option_conflicts()
{
	static char *OptionConflict = "%s conflicts with %s. %s turned off.";

	/* Check for option conflicts */

	check_float_arguments();

	if (target_arch.value->value != ARCH_SUN4 && target_arch.value->value != ARCH_SUN4C)
	{
		/* The misalignment-handling and "trust me that doubles are
		 * all aligned on doubleword boundaries" flags are currently
		 * only for the Sun-4 target architecture, and must be ignored
		 * for all other architectures.  Simply turning them off here
		 * for non-Sun4 targets is more direct than checking the target
		 * architecture everywhere in the code the flags are checked.
		 */
		if ( is_on(handle_misalignment) )
		{
			reset_flag(handle_misalignment);
		}
		if ( is_on(doubleword_aligned_doubles) )
		{
			reset_flag(doubleword_aligned_doubles);
		}
	}

	if (debugger.touched)
	{
		if (is_on(statement_count))
		{
			warning(OptionConflict, "-a", "-g", "-a");
			reset_flag(statement_count);
		}
		if (optimizer_level > OPTIM_NONE)
		{
			warning(OptionConflict, "-O", "-g", "-O");
			optimizer_level= OPTIM_NONE;
		}
		if (is_on(as_R))
		{
			warning(OptionConflict, "-R", "-g", "-R");
			reset_flag(as_R);
		}
	}

	if (is_on(statement_count))
	{
		/* This conflict is no longer a conflice in 4.1 */
		/* per request by aoki                          */
		if (optimizer_level > OPTIM_NONE)
		{
			warning(OptionConflict, "-O", "-a", "-O");
			optimizer_level= OPTIM_NONE;
		}
		if (is_on(as_R))
		{
			warning(OptionConflict, "-R", "-a", "-R");
			reset_flag(as_R);
		}
	}

	if ( target_arch_set_from_processor_type )
	{
		/* If we have to guess the target architecture type from a
		 * target processor type which is non-unique, then give 'em
		 * a warning.
		 */
		switch (target_arch.value->value)
		{
		case ARCH_SUN2:
			break;
		case ARCH_SUN3:
		case ARCH_SUN3X:
		case ARCH_SUN386:
		case ARCH_SUN4:
		case ARCH_SUN4C:
			if (product.value->value == GOAL_EXE)
			{
				warning("target architecture option \"%s\" assumed, from \"%s\".",
				target_arch.value->name,
				processor_type_option->name);

			}
			break;
		default:
			/* We had to guess at the target architecture type
			 * by looking at the target processor type.
			 * This is worth a warning.
			 */
			warning("target architecture option \"%s\" assumed, from \"%s\".",
				target_arch.value->name,
				processor_type_option->name);
			break;
		}
	}

	/* Naming "an" output file makes no sense, if this compilation is
	 * going to result in multiple output files (e.g., a ".o" for each
	 * input ".c").
	 */
	if ((outfile != NULL) && (source_infile_count > 1) &&
	    (product.value != &executable))
	{
		warning("-o option ignored");
		outfile= NULL;
	}
}


/*
 *	Make any changes necessary to our data structures, so that we can
 *	access the target files properly under Release 3.x  file organization.
 */
static void
setup_for_R3X_file_paths()
{
	   /* Under 3.x, floating-point "crt1" routines are under /lib.  */

	f68881.extra = "/lib/Mcrt1.o";
	ffpa.extra   = "/lib/Wcrt1.o";
	fsky.extra   = "/lib/Scrt1.o";
	fsoft.extra  = "/lib/Fcrt1.o";

	if (is_off(sys5_flag))  /* is bsd version */
           {
	    /* Under 3.x, "crt0" routines are under /lib.  */
	    gprof.extra   = "/lib/gcrt0.o";
	    no_prof.extra = "/lib/crt0.o" ;
	    prof.extra    = "/lib/mcrt0.o";

	   }

	else /* is sys5 version */
           {
	    /* Under 3.x and System 5, "crt0" routines are under /usr/5lib.  */
	    gprof.extra   = "/usr/5lib/gcrt0.o";
	    no_prof.extra = "/usr/5lib/crt0.o" ;
	    prof.extra    = "/usr/5lib/mcrt0.o";
           }
}

static void
setup_for_R4X_file_paths()
{
	   /* Under 4.0, floating-point "crt1" routines are under /usr/lib.  */

	   f68881.extra = "/usr/lib/Mcrt1.o";
	   ffpa.extra   = "/usr/lib/Wcrt1.o";
	   fsky.extra   = "/usr/lib/Scrt1.o";
	   fsoft.extra  = "/usr/lib/Fcrt1.o";

	   /* Under 4.0, "crt0" routines are under /usr/lib.  */
	   gprof.extra   = "/usr/lib/gcrt0.o";
	   no_prof.extra = "/usr/lib/crt0.o";
	   prof.extra    = "/usr/lib/mcrt0.o";

}





static OptimLevel
default_optimization_level(sw_release_passes, targ_arch_type, driver_type)
	int		sw_release_passes;
	int		targ_arch_type;
	int		driver_type;
{
	OptimLevel	opt_level;

	/* pascal 4.X should default to -O2 */
	if ((driver_type == DRIVER_P) && (sw_release_passes >= SW_REL_40))
	{
		opt_level= OPTIM_IROPT_P;
	}
	/* modula2 4.X should default to -O3 */
	else if ((driver_type == DRIVER_M) && (sw_release_passes >= SW_REL_40))
	{
		opt_level= OPTIM_IROPT_O;
	}
	/* fortran should default to -O3 */
	else if (driver_type == DRIVER_F)
	{
		opt_level= OPTIM_IROPT_O;
	}
	/* sun4 or sw release >= 4.1 should default to -O2 */
	else if ( (targ_arch_type == ARCH_SUN4) ||
		  (targ_arch_type == ARCH_SUN4C) ||
		  (sw_release_passes >= SW_REL_41) )
	{
		opt_level = OPTIM_IROPT_P;
	}
	else	opt_level = OPTIM_C2;

	return  opt_level;
}


static OptimLevel
legal_optimization_level(opt_level, sw_release_passes, targ_arch_type,
				driver_type)
	OptimLevel	opt_level;
	int		sw_release_passes;
	int		targ_arch_type;
	int		driver_type;
{
	switch (opt_level)
	{
	case OPTIM_NONE:	/* (0) NO opt */
	case OPTIM_C2:		/* (1) c2 opt, only */
		break;
	case OPTIM_IROPT_P:	/* (2) iropt -P opt */
	case OPTIM_IROPT_O:	/* (3) iropt -O opt */
		/* for sun386 and the c compiler use only 01 level */
		if (targ_arch_type == ARCH_SUN386 && driver_type == DRIVER_C)
		{
			opt_level = OPTIM_C2;
			break;
		}

#ifndef FORCE_ACCEPT_O2O3O4 /* do the following checks, only if we are not */
			    /* forcing the host to allow -O2, -O3, and -O4 */
			    /* (the "force" mode is used for debugging).   */
		/* These levels are allowable for Fortran on any S/W release,
		 * for any language under 4.0, and for any language on a Sun4
		 * under any version of the operating system.
		 */
		if ( (driver_type == DRIVER_F)
				||
#ifdef PASCAL_105
		     /* also allow higher optimization levels for Pascal 1.05 */
		     ( ((driver_type == DRIVER_C)||(driver_type == DRIVER_P)) &&
		       (targ_arch_type == ARCH_SUN4) )
#else /*!PASCAL_105*/
		     ( (driver_type == DRIVER_C) &&
		       ((targ_arch_type == ARCH_SUN4C) || 
		       (targ_arch_type == ARCH_SUN4)) )
#endif /*PASCAL_105*/
				||
		     (sw_release_passes != SW_REL_3X) )
		{
		     /* These optimization levels are OK for these. */
		}
		else
		{
			/* Back down to the default 3.x optimization level. */
			opt_level = OPTIM_C2;
		}
#endif /*!FORCE_ACCEPT_O2O3O4*/
		break;
	case OPTIM_IROPT_O_TRACK_PTRS:	/* (4) iropt -O, with pointer-tracking*/
		/* for sun386 and the c compiler use only 01 level */
		if (targ_arch_type == ARCH_SUN386 && driver_type == DRIVER_C)
		{
			opt_level = OPTIM_C2;
			break;
		}
#ifndef FORCE_ACCEPT_O2O3O4
		/* This level is allowable only on post-3.x (i.e. 4.0)
		 * compilers, or on 3.2 (i.e. Sys4-3.2L or Sys4-3.2+)
		 * compilers on a Sun4.
		 */

		if ( (sw_release_passes != SW_REL_3X) ||
		     (targ_arch_type == ARCH_SUN4C) || 
		     (targ_arch_type == ARCH_SUN4)
		   )
		{
		     /* All OK. */
		}
		else
		{
			/* Back down to the default optimization level. */
			opt_level =
				default_optimization_level(sw_release_passes,
					targ_arch_type, driver_type);
		}
#endif /*!FORCE_ACCEPT_O2O3O4*/
		break;
	default:
		illegal_optimize_option(opt_level);
	}

	return opt_level;
}


static void
set_global_flag_defaults()
{
	/* All are OFF by default; so only turn ON the exceptions. */

	set_flag(show_rm_commands);	/* OK to echo "rm" cmds */
	set_flag(remove_tmp_files);	/* remove all /tmp files, by default. */

#if   defined(FORCE_SYS5)
	set_flag(sys5_flag);		/* Force SysV default orientation. */
#  ifdef DEBUG
	if (is_on(debug))	fprintf(stderr, "[orientation=sys5(forced)]\n");
#  endif /*DEBUG*/
#elif defined(FORCE_UCB)
	reset_flag(sys5_flag);		/* Force UCB  default orientation. */
#  ifdef DEBUG
	if (is_on(debug))	fprintf(stderr, "[orientation=ucb(forced)]\n");
#  endif /*DEBUG*/
#else
	uninitialize_flag(sys5_flag);	/* Leave uninitialized until we know
					 * the target software release number.
					 */
#endif /* forced UCB or SYS5 orientations */
}


#ifdef NSE
static void
consult_NSE_environment()
{
	static char NSE_env_variable_not_set[] =
		"env variable %s not set, in NSE variant \"%s\"";

	ENV_nse_variant = getenv("NSE_VARIANT");

	if (IN_AN_NSE_VARIANT(ENV_nse_variant))
	{
		/* We're in an NSE variant. */

		/* Get the target architecture type and target release number
		 * specified by NSE -- if any.
		 */

		if ( (ENV_target_arch = getenv(TARGET_ARCH_varname)) == NULL )
		{
			fatal(NSE_env_variable_not_set,
				TARGET_ARCH_varname, ENV_nse_variant);
		}
		else
		{
			/* If the target architecture starts with '-' in the
			 * environment, drop the '-' (which is a historical
			 * artifact).
			 */
			ENV_target_arch = skip_leading_minus(ENV_target_arch);

			if (target_arch.value == NULL)
			{
				/* Target architecture was NOT specified
				 * explicitly by user.  Use the NSE TARGET_ARCH
				 * target architecture.
				 */
				set_target_architecture(&target_arch,
							ENV_target_arch, FALSE);
			}
		}

		if ( (ENV_target_release = getenv(TARGET_RELEASE_varname))
			== NULL)
		{
			fatal(NSE_env_variable_not_set,
				TARGET_RELEASE_varname, ENV_nse_variant);
		}
		else
		{
			/* Set all target s/w release values which are (so far)
			 * defaulted, i.e. NOT specified explicitly by the user,
			 * to the NSE TARGET_RELEASE value.
			 */
			set_all_defaulted_target_sw_release_values(
				ENV_target_release,
				TARGET_RELEASE_varname,"=",ENV_target_release);
		}
	}
	else
	{
		/* We're in a normal, non-NSE environment.  Use the regular
		 * defaults for target architecture and release number.
		 */
	}

#  ifdef DEBUG
    consulted_NSE = TRUE;
#  endif
}
#endif NSE


static void
define_arch_and_sw_release()
{
	Const_intP	arch_of_tools_in_root_filesystem;

#ifdef NSE
	/* Consult the NSE environment for default target architecture and
	 * target software release -- if it sets them at all.
	 */
	consult_NSE_environment();
#endif

	set_host_and_target();

#ifdef PASCAL_105
	/* Save the initial target s/w release.  If it was given on the 
	 * command line, it's as given.  If not, it is the default value,
	 * BEFORE we end up setting it via get_target_base_OS_version()
	 * in setup_for_*_compile().  */

	original_sw_rel_passes = target_sw_release[R_PASSES].value;

#endif /*PASCAL_105*/

#ifdef CROSS
#  ifdef NSE
#    ifdef DEBUG
	if (!consulted_NSE) internal_fatal("skipped NSE check!");
#    endif
	/* If we're in an activated NSE environment, then the tools which
	 * appear to be in the root filesystem are actually cross-tools for
	 * the target architecture.
	 *
	 * If we're not in an activated NSE environment,the tools in the root
	 * filesystem are actually the native tools.
	 */
	if ( IN_AN_NSE_VARIANT(ENV_nse_variant) )
	{
		arch_of_tools_in_root_filesystem =
			lookup_architecture(ENV_target_arch);
	}
	else
	{
		arch_of_tools_in_root_filesystem = host_arch.value;
	}
#  else /*!NSE*/
	arch_of_tools_in_root_filesystem = host_arch.value;
#  endif /*NSE*/


	/* We have native compilation only if:
	 *	the tools in the root filesystem are those needed for the
	 *		target architecture,
	 *  AND the S/W release for which the tools in the root filesystem are
	 *		intended match the target S/W release.
	 *
	 * Otherwise, we have cross-compilation.
	 */
	if ( (arch_of_tools_in_root_filesystem->value ==
		target_arch.value->value)
			&&
	     ( (target_sw_release[R_VROOT].value->value == SW_REL_DFLT) ||
	       (target_sw_release[R_VROOT].value->value ==
			get_target_base_OS_version("", driver.value)->value) )
	   )
	{
		setup_for_native_compile();
	}
	else
	{
		/* We're doing a cross-compilation. */
		setup_for_cross_compile();
	}
#else /*!CROSS*/
	setup_for_native_compile();
#endif /*CROSS*/
} /* define_arch_and_sw_release() */



void
check_sys5_flag_status()
{

	if is_initialized(sys5_flag)
	{
#ifdef DEBUG
		if (is_on(debug))
		{
			fprintf(stderr,"[orientation=%s]\n",
					(is_on(sys5_flag) ? "sys5" : "ucb") );
		}
#endif /*DEBUG*/
	}
	else
	{
		/* we'll use the default orientation based on the
		 * target software release.
		 */
		if (target_sw_release[R_PATHS].value->value <= SW_REL_40)
		{
			reset_flag(sys5_flag);	/* bsd, for 4.0 and previous.*/
#  ifdef DEBUG
			if (is_on(debug))
			{
				fprintf(stderr,"[orientation=ucb(rel<=4.0)]\n");
			}
#  endif /*DEBUG*/
		}
		else
		{
			reset_flag(sys5_flag);	/* ucb for 4.1  */
#  ifdef DEBUG
			if (is_on(debug))
			{
				fprintf(stderr,"[orientation=ucb(rel>=4.1)]\n");
			}
#  endif /*DEBUG*/
		}
	}

}  /* check_sys5_flag_status */



/*
 *	main()
 */
void
main(argc, argv)
	int			argc;
	char			*argv[];
{
	char			*p;

	set_signal(SIGINT,  abort_program);
	set_signal(SIGTERM, abort_program);
	set_signal(SIGQUIT, abort_program);
	set_signal(SIGHUP,  abort_program);

	set_global_flag_defaults();

	/* Do not buffer output */
	(void)setlinebuf(stdout);
	(void)setlinebuf(stderr);

	scan_vroot_first();

	argv_for_passes= (char **)get_memory(
		(argc+MAX_NUMBER_OF_DRIVER_SUPPLIED_ARGS)*sizeof(char *));



#ifdef BROWSER
	if (getenv(CB_INIT_ENV_VAR) != NULL) {
		set_flag(code_browser_seen);
	}
#endif

	/* Strip path from program name */
	if ((p= rindex(argv[0], '/')) != NULL)
	{
		argv[0]= p+1;
	}
	program_name= argv[0];

	/* If the program name starts with "x" we provide "-dryrun";
	 * if it starts with "X" or "-" we provide "-dryrun -normcmds" (which
	 * dryrun's without showing the file-removal lines).
	 * These are debugging aids for the /lib/compile maintainer, not
	 * features for users.
	 */
	switch (argv[0][0])
	{
	case 'X':
	case '-':
		reset_flag(show_rm_commands); /* don't echo "rm" cmds */
		/* fall through to 'x' case! */
	case 'x':
		set_flag(dryrun);
		(argv[0])++;
		break;
	default:
		/* The usual case -- nothing to do, here. */
		break;
	}

	/* Lookup the program name */
	argv+= lookup_option(drivers, &argc, argv, 0, DRIVER_all);

	if (driver.value == NULL)
	{
		fatal("Unknown driver %s", argv[-1]);
	}

	/* Process all the command line options */
	while (argc > 0)
	{
		if (driver.value == &xlint)
			argv+= lookup_lint_option(options, &argc, argv, 1, 0);
		else
			argv+= lookup_option(options, &argc, argv, 1, 0);
	}

	if (driver.value == &dummy)
	{
		exit(0);
	}

	if (is_on(do_dependency))
	{
		set_named_int(product, &preprocessed);
	}


	/* OK, now that we have parsed all the command line
	 * options, lets set the target sw architecture and sw release
	 */
	define_arch_and_sw_release(); 
	check_sys5_flag_status();


	switch (target_sw_release[R_PATHS].value->value) 
	{
	   case SW_REL_3X: 
		    setup_for_R3X_file_paths();
		    break;
	   case SW_REL_40:
	   case SW_REL_41:
		    setup_for_R4X_file_paths();
		    break;
           default:
	            fatal("Invalid target sw release %d",target_sw_release[R_PATHS].value->value);
	}

	if (use_default_optimizer_level)
	{
		/* Plain "-O" was specified, and no specific optimization
		 * level (such as "-O2") was given.  Use the default level
		 * for this combination of S/W release, target architecture,
		 * and driver type.
		 */
		optimizer_level =
			default_optimization_level(
				target_sw_release[R_PASSES].value->value,
				target_arch.value->value,
				driver.value->value );
	}
	else
	{
		optimizer_level =
			legal_optimization_level(
				optimizer_level,
				target_sw_release[R_PASSES].value->value,
				target_arch.value->value,
				driver.value->value);
	}

	check_for_option_conflicts();

	/* Call action routine for this driver */
	if (driver.value->name != NULL)
		(*((int (*)())(driver.value->name)))();
	cleanup(0);
	exit(exit_status);
}	/* main */

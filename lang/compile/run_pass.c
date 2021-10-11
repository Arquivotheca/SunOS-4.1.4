/* @(#)run_pass.c 1.1 94/10/31 SMI */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "driver.h"
#include <sys/wait.h>
#include <sys/file.h>
#include <ctype.h>

#define PIPE_RD_FD 0
#define PIPE_WR_FD 1


/*
 *	outfile_name
 *		Figure out what the outfile name should be
 */
char *
outfile_name(source, out_suffix, programp)
	register char		*source;
	register SuffixP	out_suffix;
	ProgramP		programp;
{
	register char		*p;
	register char		*result;

	if (make_lint_lib_fd && (programp == &program.lint1))
	{
		return "";
	}

	if ((outfile != NULL) && (product.value != &executable))
	{
		/* If a "-o" option was given */
		if (strcmp(get_file_suffix(outfile), out_suffix->suffix))
			fatal("Suffix mismatch between -o and produced file (should produce .%s)",
				out_suffix->suffix);
		result= outfile;
	}
	else
	{
		/* We derive the outfile name from the infile name */
		if ((p= rindex(source, '/')) != NULL)
			source= p+1;
		if ((p= rindex(source, '.')) != NULL)
			*p= 0;
		result= get_memory(strlen(source)+strlen(out_suffix->suffix)+2);
		(void)sprintf(result, "%s.%s", source, out_suffix->suffix);
		if (p != NULL)
			*p= '.';
	}
	if (STR_EQUAL(result, source))
		fatal("Outfile would overwrite infile %s", result);
	return result;

}	/* outfile_name */

/*
 *	temp_file_name
 *		Construct one temp file name, and register it to make sure it
 *		will be cleaned up.
 */
char *
temp_file_name(program, suffix, mark)
	register ProgramP	program;
	register SuffixP	suffix;
	register char		mark;
{
	register char		*string;
	char			buffer[MAXPATHLEN];
	static int		process_number= -1;

	if (process_number == -1)
	{
		if (is_on(testing_driver))
		{
			process_number = 0;
		}
		else
		{
			process_number = getpid();
		}
	}

	if (mark == 0)
	{
		(void)sprintf(buffer, "%s/%s.%05d.%d.%s",
			temp_dir, program->name, process_number,
			temp_file_number++, suffix->suffix);
	}
	else
	{
		(void)sprintf(buffer, "%s/%s.%05d.%c.%d.%s",
			temp_dir, program->name, process_number, mark, temp_file_number++, suffix->suffix);
	}
	string= make_string(buffer);
	append_list(&temp_files_named, string);
	return string;
}	/* temp_file_name */

/*
 *	wait_program
 *		Wait for a process to terminate.
 *		Report times for it if we need to after it exits.
 */
static int
wait_program(wait_step, step)
	register StepP		wait_step;
	register StepP		step;
{
	union wait		wait_status;
	struct timeval		stop;
	struct rusage		usage;
	struct timeval		cpu;
	struct timeval		elapsed;
	register int		process;
	register StepP		p;
	register StepP		q;

	process= wait3(&wait_status, 0, &usage);

	/* Find step for process that returned */
	for (p= wait_step; p <= step; p++)
		if (p->process == process)
			break;

	/* And mark it as terminated */
	p->process= -1;
	/* If it returned an error we kill all other running processes */
	if ((wait_status.w_T.w_Termsig != 0) || (wait_status.w_T.w_Retcode))
	{
		for (q= wait_step; q <= step; q++)
		{
			if (q->process > 0)
			{
				(void)kill(q->process, SIGKILL);
				q->killed= TRUE;
			}
		}
	}

	if (is_on(time_run))
	{
		/* Report program runtime */
		(void)gettimeofday(&stop, (struct timezone *)NULL);
		cpu.tv_sec= usage.ru_utime.tv_sec+usage.ru_stime.tv_sec;
		cpu.tv_usec= usage.ru_utime.tv_usec+usage.ru_stime.tv_usec;
		if (cpu.tv_usec > 1000000)
		{
			cpu.tv_usec-= 1000000;
			cpu.tv_sec++;
		}
		elapsed.tv_sec= stop.tv_sec-p->start.tv_sec;
		elapsed.tv_usec= stop.tv_usec-p->start.tv_usec;
		if (elapsed.tv_usec < 0)
		{
			elapsed.tv_usec+= 1000000;
			elapsed.tv_sec--;
		}
		(void)fprintf(stderr, "%s: time U:%d.%01ds+S:%d.%01ds=%d.%01ds REAL:%d.%01ds %d%%. ",
			p->program->name,
			usage.ru_utime.tv_sec, usage.ru_utime.tv_usec/100000,
			usage.ru_stime.tv_sec, usage.ru_stime.tv_usec/100000,
			cpu.tv_sec, cpu.tv_usec/100000,
			elapsed.tv_sec, elapsed.tv_usec/100000,
			(cpu.tv_sec*1000+cpu.tv_usec/1000)*100/
				(elapsed.tv_sec*1000+elapsed.tv_usec/1000));
		(void)fprintf(stderr, "core T:%dk D:%dk. io IN:%db OUT:%db. pf IN:%dp OUT:%dp.\n",
			usage.ru_ixrss/1024, usage.ru_idrss/1024,
			usage.ru_inblock, usage.ru_oublock,
			usage.ru_majflt, usage.ru_minflt);
	}

	/* Report processes that returned errors */
	if (!p->killed && (wait_status.w_T.w_Termsig != 0))
	{
		(void)fprintf(stderr, "%s: Fatal error in %s: %s%s\n",
			program_name,
			p->program->name,
			sys_siglist[wait_status.w_T.w_Termsig],
			wait_status.w_T.w_Coredump?" (core dumped)":"");
		return 1;
	}

	return wait_status.w_T.w_Retcode;
}	/* wait_program */


/*
 *	use_temp_file
 *		Mark one temp file as used.
 *		Only files that are used will be removed by the driver.
 */
static char *
use_temp_file(file)
	register char		*file;
{
	register ListP		tp;

	/* Move file from "temp_files_named" to "files_to_unlink" */
	for (tp= temp_files_named; tp != NULL; tp= tp->next)
		if (tp->value == file)
		{
			append_list(&files_to_unlink, file);
			tp->value= NULL;
			return file;
		}
	return file;
}	/* use_temp_file */

/*
 *	append_list_to_argv
 *		Take a list, and add all its strings to the current argv
 *		being built.
 */
static void
append_list_to_argv(p, consp)
	register char		***p;
	register ListP		consp;
{
	for (;consp != NULL; consp= consp->next)
		*(*p)++= use_temp_file(consp->value);
}	/* append_list_to_argv */


/*
 *	scan_Qpath_and_vroot
 *		Take a path and transform it using Qpath and VIRTUAL_ROOT.
 *		Also maybe save it for later dependency reporting.
 */
char *
scan_Qpath_and_vroot(file, report_file)
	register char		*file;
	register Bool		report_file;
{
	register int		result;
	char			buffer[MAXPATHLEN];
	register char		*slash;
	char			*path;

	/* If filename contains a "/" and we have a path defined, we strip
	 * the path from the filename and tack it on as the last component
	 * of the path.
	 */
	if ((program_path != NULL) && ((slash= rindex(file, '/')) != NULL))
	{
		(void)strncpy(buffer, file, slash-file);
		buffer[slash-file]= '\0';
		/* Also put the full path to this file in the search path,
		 * in case it isn't under the Qpath.
		 */
		add_dir_to_path(buffer, &program_path, last_program_path);
		result= access_vroot(slash+1, F_OK, program_path, VROOT_DEFAULT);
	}
	else
	{
		result= access_vroot(file, F_OK, (pathpt)NULL, VROOT_DEFAULT);
	}

	if (result == 0)
	{
		/* Found the file. */
		/* Get the true path from the vroot subsystem */
		get_vroot_path(&path, (char **)NULL, (char **)NULL);
		(void)strcpy(buffer, path);
		if (report_file)
		{
			/* Get the name with the path but not the vroot for
			 * dependency reporting.
			 */
			get_vroot_path((char **)NULL, &path, (char **)NULL);
			append_list(&report, make_string(path));
		}
	}
	else
	{
		/* Didn't find the file.
		 * If we are "-dryrun", just report the plain file.
		 */
		if (is_on(dryrun))
			(void)strcpy(buffer, file);
		else
			fatal("Cannot find %s", file);
	}
	return make_string(buffer);
}	/* scan_Qpath_and_vroot */


/*
 *	scan_Qpath_only
 *		Take one path and transform it using the Qpath path.
 *		Also maybe save it for later dependency reporting.
 */
char *
scan_Qpath_only(file, report_file)
	register char		*file;
	register Bool		report_file;
{
	register int		result;
	char			buffer[MAXPATHLEN];
	register char		*slash;
	char			*path;

	/* If filename contains a "/" and we have a path defined, we strip
	 * the path from the filename and tack it on as the last component
	 * of the path.
	 */
	if ((program_path != NULL) && ((slash= rindex(file, '/')) != NULL))
	{
		(void)strncpy(buffer, file, slash-file);
		buffer[slash-file]= '\0';
		/* Also put the full path to this file in the search path,
		 * in case it isn't under the Qpath.
		 */
		add_dir_to_path(buffer, &program_path, last_program_path);
		result= access_vroot(slash+1, F_OK, program_path, (pathpt)NULL);
	}
	else
	{
		result= -1;
	}

	if (result == 0)
	{
		/* Found the file. */
		/* Get the true path from the vroot subsystem */
		get_vroot_path(&path, (char **)NULL, (char **)NULL);
		(void)strcpy(buffer, path);
		if (report_file)
		{
			/* Get the name with the path but not the vroot for
			 * dependency reporting.
			 */
			get_vroot_path((char **)NULL, &path, (char **)NULL);
			append_list(&report, make_string(path));
		}
		return  make_string(buffer);
	}
	else
	{
		/* Didn't find the file.  Return NULL to report that. */
		return  (char *)NULL;
	}
}	/* scan_Qpath_only */


/*
 *	build_argv
 *		Build the argument list for the process to be called
 */
static void
build_argv(argv, step, this_pipe, std_in, std_out, unlink_candidates,
		previous_pipe)
	char		**argv;
	StepP		step;
	int		this_pipe[2];
	char		**std_in;
	char		**std_out;
	ListP		*unlink_candidates;
	int		*previous_pipe;
{
	char			**vp= argv;
	Template		*tp;

	/* Get the true name of the executable */
	*vp++= scan_Qpath_and_vroot(step->program->path, FALSE);
	*vp++= step->program->name;
	this_pipe[PIPE_RD_FD]= this_pipe[PIPE_WR_FD]= -1;
	*std_in= *std_out= NULL;
	/* Build the argv vector.
	 * The template from the program description is used to determine
	 * the order of args.
	 * It also specifies whether it is OK to use stdin/stdout or not.
	 */
	for (tp= step->program->template; *tp != '\0'; tp++)
	    switch (*tp)
	{
		case IN_TEMPLATE:
			append_list_to_argv(&vp, step->program->infile);
			/* Temp files are removed as soon as one step has
			 * read them
			 */
			append_list(unlink_candidates,
					step->program->infile->value);
			break;
		case STDIN_TEMPLATE:
			if (*previous_pipe == -1)
			{
				append_list(unlink_candidates,
						step->program->infile->value);
				*std_in= use_temp_file(
						step->program->infile->value);
			}
			break;
		case OUT_TEMPLATE:
			*vp++= use_temp_file(step->program->outfile);
			break;
		case OUT2_TEMPLATE:
			*vp++= use_temp_file(step->program->outfile2);
			break;
		case STDOUT_TEMPLATE:
			/* Use stdout only if this is not the last step */
			if ((step+1)->program &&
			    ((step+1)->program->template[0] == STDIN_TEMPLATE)&&
			    is_on(pipe_ok))
			{
				if (pipe(this_pipe) == -1)
				{
					fatal("Trouble opening pipe");
				}
			}
			else
			{
				if ( (step->program->outfile != NULL) &&
				     !(make_lint_lib_fd &&
				     (step->program == &program.lint1))
				   )
				{
					*std_out= use_temp_file(
							step->program->outfile);
				}
			}
			break;
		case OPTION_TEMPLATE:
			append_list_to_argv(&vp, step->program->options);
			append_list_to_argv(&vp,
				step->program->permanent_options);
			append_list_to_argv(&vp,
				step->program->trailing_options);
			break;
		case MINUS_TEMPLATE:
			*vp++= "-";
			break;
		case MINUS_O_TEMPLATE:
			*vp++= "-o";
			break;
	}

	*vp= NULL;

	/* Print trace if verbose is on */
	if ( is_on(verbose) || is_on(dryrun) )
	{
		Bool first_arg;
		(void)fprintf(stderr, "%s ", *argv);
		/* If argument contains whitespace or is empty we quote it */
		for (first_arg = TRUE, vp= argv+2;  *vp != NULL;  vp++)
		{
			char	*q;
			Bool	space= FALSE;

			if (vp[0][0] == '\0')
				space= TRUE;
			for (q= *vp; *q != '\0'; q++)
			{
				if (isspace(*q))
				{
					space= TRUE;
					break;
				}
			}
			if (first_arg)
			{
				first_arg = FALSE;
			}
			else
			{
				(void)putc(' ', stderr);
			}
			(void)fprintf(stderr, space ? "\"%s\"":"%s", *vp);
		}
		if (*std_in != NULL)
			(void)fprintf(stderr, " <%s", *std_in);
		if ( make_lint_lib_fd && (step->program == &program.lint1)
		     && (this_pipe[PIPE_RD_FD] == -1))
			(void)fprintf(stderr, " >>%s", outfile);
		if (*std_out != NULL)
			(void)fprintf(stderr, " >%s", *std_out);
		if (this_pipe[PIPE_RD_FD] != -1)
			(void)fprintf(stderr, " | ");
		else
			(void)fprintf(stderr, "\n");
	}
}


static int
close_if_open(fd)
	int fd;
{
	if (fd == -1)	return 0;
	else
	{
		/** if(is_on(debug)) fprintf(stderr,"[close fd %d]", fd); **/
		return close(fd);
	}
}


/*
 *	run_steps
 *		Run all the steps that are relevant to compile this source file.
 */
int
run_steps(source, step, do_cb, source_suffix)
	register char		*source;	/* the previous out_file. */
	register StepP		step;
	Bool			do_cb;
	char			*source_suffix;
{
	Step			current_steps[sizeof(program)/sizeof(Program)];
	StepP			final_step= NULL;
	StepP			wait_step= current_steps;
	StepP			p;
	StepP			q;
	char			*out_file= source;
	char			*proposed_out_file;
	int			previous_pipe;
	int			this_pipe[2];
	char			*std_out= NULL;
	char			*std_in= NULL;
	ListP			unlink_candidates= NULL;
	int			status= 0;
	int			processes_running= 0;

	if ( (requested_suffix == &suffix.i) &&
	     (product.value == &preprocessed/*-E*/) )
	{
	     /* As an exception, we can run "cc -E" on a file of any suffix
	      * (doesn't have to be ".c").
	      */
	     current_steps[0] = anything_thru_cc_E_steps[0];
	     final_step = &current_steps[0];
	}
	else if ( (requested_suffix == &suffix.i) &&
		  ( (product.value == &preprocessed2/*-P*/) &&
		    (strcmp(get_file_suffix(source), suffix.i.suffix) != 0 ) )
		)
	{
	     /* As an exception, we can run "cc -P" on a file of any suffix
	      * (doesn't have to be ".c") -- except for an input file with
	      * a ".i" suffix, since it would get overwritten.
	      */
	     current_steps[0] = anything_thru_cc_P_steps[0];
	     final_step = &current_steps[0];
	}
	else
	{
		/* First run thru the steps, and extract the ones we are going
		 * to use this time. 
		 */
		for ( p= step, q= current_steps;
		      p->program != &program.sentinel_program_field;
		      p++)
		{
			if ((p->expression == NULL) || (*p->expression)())
			{
				/* Remember the last step which produces the
				 * suffix we want.
				 */
				if (p->out_suffix == requested_suffix)
					final_step= q;
				*q++= *p;
			}
		}
		if (final_step == NULL)
		{
			fatal("Cannot go from .%s to .%s",
				get_file_suffix(source),
				requested_suffix->suffix);
		}
	}
	(final_step+1)->program= NULL;


	/* Then run the steps */
	previous_pipe= -1;
	for (step= current_steps; step->program && (status == 0); step++)
	{
		step->program->outfile= NULL;
		proposed_out_file= NULL;

		/* Run the setup for the step if any */
		if (step->setup != NULL)
			proposed_out_file= (*step->setup)(source, out_file);

		/* Run the setup for the program if any and the step didn't
		 * do it.
		 */
		if ((proposed_out_file == NULL) &&
		    (step->program->setup != NULL))
			proposed_out_file=
				(*step->program->setup)(source, out_file);
		if (proposed_out_file != NULL)
		{
			/* The setup function is responsible for setting up the
			 * infile.
			 */
			out_file= proposed_out_file;
		}
		else
		{
			/* The setup routines don't care about the outfile name.
			 * The outfile name here is from the previous step;
			 * use it as input.
			 */
			append_list(&step->program->infile, out_file);

			if ( ( (product.value == &executable) &&
			       (step->out_suffix == &suffix.o)) ||
			     ( ((step+1)->program == NULL) &&
			       ( (product.value != &executable) ||
				 (driver.value == &m2c)
			       )
			     )
			   )
			{
				/* If this is the last step (or last before ld)
				 * we use real outname.
				 */
				out_file= outfile_name(source, requested_suffix,
							step->program);
				if ((source_infile_count == 1) &&
				    (product.value == &executable) &&
				    (driver.value != &m2c))
					/* The command "cc foo.c" removes
					 * "foo.o" after linking it.
					 */
					append_list(&files_to_unlink,
							make_string(out_file));
			}
			else
			{
				/* This is intermediate outfile */
				out_file= temp_file_name(step->program,
							  step->out_suffix, 0);
			}
			step->program->outfile= out_file;
		}

		/* Execute the program for the step */
		build_argv(argv_for_passes, step, this_pipe, &std_in, &std_out,
				&unlink_candidates, &previous_pipe);

		if (is_on(dryrun))
		{
			(void)close_if_open(this_pipe[PIPE_RD_FD]);
			(void)close_if_open(this_pipe[PIPE_WR_FD]);
			goto end_loop;
		}

		if (is_on(time_run))
		{
			(void)gettimeofday(&step->start,
						(struct timezone *)NULL);
		}

		step->killed= FALSE;

		switch (step->process= fork())
		{

		    case -1:
			fatal("fork failed");

		    case 0:	/* Child */
			if (std_out != NULL)
			{
				int	mode= 0666;

				/* If we need to create file to write, do it. */
				if (STR_EQUAL_N(temp_dir, std_out,
						strlen(temp_dir)))
						{
					/* Create temp files with fewer
					 * permissions.
					 */
					mode= 0600;
				}
				if ( (this_pipe[PIPE_WR_FD]=
					/* opening /tmp file; deliberately
					 * use open(), not open_vroot().
					 */
					open(std_out,
						O_WRONLY|O_CREAT|O_TRUNC, mode)
				     ) < 0)
				{
					(void)fprintf(stderr,
						"%s: Cannot open %s for output\n",
						program_name, std_out);
					_exit(1);
				}
			}
			if (this_pipe[PIPE_WR_FD] > 0)
			{
				(void)dup2(this_pipe[PIPE_WR_FD],
						fileno(stdout));
			}
			if ( make_lint_lib_fd &&
			     (step->program == &program.lint1) &&
			     (std_out == NULL) &&
			     (this_pipe[PIPE_WR_FD] <= 0) )
			{
				(void)dup2(make_lint_lib_fd, fileno(stdout));
			}
			(void)close_if_open(this_pipe[PIPE_RD_FD]);

			/* Open the infile if we need to */
			if (std_in != NULL)
			{
				if ( (previous_pipe=
					/* opening /tmp file; deliberately
					 * use open(), not open_vroot().
					 */
					open(std_in, O_RDONLY, 0644)
				     ) < 0)
				{
					(void)fprintf(stderr,
						"%s: Cannot open %s for input\n",
						program_name, std_in);
					_exit(1);
				}
			}
			if (previous_pipe > 0)
			{
				(void)dup2(previous_pipe, fileno(stdin));
			}
			execve(*argv_for_passes, argv_for_passes+1, environ);
			(void)fprintf(stderr, "%s: Cannot exec %s\n",
					program_name, *argv_for_passes);
			_exit(1);
			break;

		    default:	/* Parent */
			processes_running++;
			break;
		}

		/* This step is done.
		 * Close its input & output pipes, if used.
		 */
		(void)close_if_open(previous_pipe);
		(void)close_if_open(this_pipe[PIPE_WR_FD]);

		/* If there are no steps reading piped output from this step,
		 * then wait for all running steps to complete before going on.
		 */
		if (this_pipe[PIPE_RD_FD] == -1)
		{
			while (processes_running > 0)
			{
				status|= wait_program(wait_step, step);
				processes_running--;
			}
			wait_step= step;
		}

	end_loop:
		if ( is_on(remove_tmp_files) &&
		     ((previous_pipe= this_pipe[PIPE_RD_FD]) == -1) )
		{
			/* If we are removing temp files (which is standard
			 * procedure), and this was the last leg of a pipeline,
			 * we remove all consumed tempfiles.
			 * Files must be on both "files_to_unlink" and the
			 * local "unlink_candidates" list to get removed.
			 */
			for (; unlink_candidates != NULL;
			     unlink_candidates= unlink_candidates->next)
			{
				ListP cp;

				for (cp= files_to_unlink; cp != NULL;
				     cp= cp->next)
				{
					if (cp->value ==
						unlink_candidates->value)
						{
						if ( ( is_on(verbose)  ||
						       is_on(dryrun)
						      ) &&
						      is_on(show_rm_commands)
						   ){
							(void)fprintf(stderr,
								"rm %s\n",
								unlink_candidates->value);
						}
						(void)unlink(unlink_candidates->value);
						cp->value= NULL;
						break;
					}
				}
			}
		}
	}
	step--;
	if (step->out_suffix->collect != NULL)
		/* And offer what we just produced for collection */
		/* Collected files will be postprocessed */
		(*step->out_suffix->collect)(out_file, step->out_suffix);
	return status;
}	/* run_steps */

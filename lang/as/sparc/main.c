static char sccs_id[] = "@(#)main.c	1.1\t10/31/94";
#include <stdio.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/exec.h>	/* for TV_SUN4 */
#include "structs.h"
#include "bitmacros.h"
#include "errors.h"
#include "globals.h"
#include "lex.h"	/* for curr_flp */
#include "segments.h"
#include "alloc.h"
#include "node.h"
#include "emit.h"
#include "obj.h"
#include "check_nodes.h"
#include "disassemble.h"
#include "relocs.h"
#include "scan_symbols.h"
#include "version_file.h"
#include "optimize.h"

#ident "@(#)RELEASE as 4.1"

static Bool	add_input_file();	/* forward declaration */
static void	pass_init();		/* forward declaration */
static void	pass_input();		/* forward declaration */
static void	pass_post_input();	/* forward declaration */
static void	pass_optimize();	/* forward declaration */
static void	pass_assemble();	/* forward declaration */
static void	pass_finish();		/* forward declaration */
static void	statistics();		/* forward declaration */
static void	init();			/* forward declaration */
static void	cleanup();		/* forward declaration */
void		terminal_cleanup();	/* forward declaration */
static void	catchsigs();		/* forward declaration */
static void	chk_objfilename();	/* forward declaration */

static struct filelist *file_list;	/* ptr to begin of input file list */



main(argc, argv)
	int	argc;
	char   *argv[];
{
	register int	 ac;
	register char   *p;	/* temp. char ptr */
	char argchar[2];
	char	argsign;
	Options options;
	Bool	argerror;
	Bool	argdone;
	Bool	any_opt_to_do = FALSE;
	Bool	print_initial_info = FALSE;
	int	i;
	static char *Usage =
#ifdef DEBUG
#  ifdef CHIPFABS
		"usage: %s [-V] [-d] [-da] [-F[O][if]#] [-O[#][sd][[~][A-Z]]...]\n\t\t[-S[CLRSclrs]] [-o objfile] [-L] [-R] [-Q]\n\t\t[-P [[-Ipath] [-Dname] [-Dname=def] [-Uname]]...] file.s...\n";
#  else /*!CHIPFABS*/
		"usage: %s [-V] [-d] [-da] [-O[#][sd][[~][A-Z]]...]\n\t\t[-S[CLRSclrs]] [-o objfile] [-L] [-R] [-Q]\n\t\t[-P [[-Ipath] [-Dname] [-Dname=def] [-Uname]]...] file.s...\n";
#  endif /*CHIPFABS*/
#else /*!DEBUG*/
#  ifdef CHIPFABS
		"usage: %s [-F[O][if]#] [-O[#]] [-S[C]] [-o objfile] [-L] [-R] [-Q]\n\t\t[-P [[-Ipath] [-Dname] [-Dname=def] [-Uname]]...] file.s...\n";
#  else /*!CHIPFABS*/
		"usage: %s [-O[#]] [-S[C]] [-o objfile] [-L] [-R] [-Q]\n\t\t[-P [[-Ipath] [-Dname] [-Dname=def] [-Uname]]...] file.s...\n";
#  endif /*CHIPFABS*/
#endif /*DEBUG*/

#ifdef CHIPFABS
	Bool	optimal_fab_workarounds_requested = FALSE;
#endif /*CHIPFABS*/

#ifdef DEBUG
		/* print_alloc_stats==TRUE if to print allocation stats @ end.*/
	Bool	print_alloc_stats = FALSE;
	extern	Bool	debug_tkn_count;
#endif /*DEBUG*/

	pgm_name = argv[0];

	/* default: all undefined symbols are assumed to be Global,
	 * at end of assembly.
	 */
	bzero(&options, sizeof(options));	/* reset all options */

	/* The number of arguments passed to the program is an absolute
	 * upper bound on the number of args which might passed on to the
	 * preprocessor.
	 */
	cpp_argv = (char **)as_malloc(argc * sizeof(char *));
	cpp_argv[cpp_argc++] = "cpp";	/* CPP's argv[0]. */

	for(argerror = FALSE, ac = 1;    ac < argc;     ac++)
	{
		/* the "!= '\0'" check allows "-" by itself to be taken as
		 * a filename below, later to be interpreted to mean stdin.
		 */
		if ( ( ((argsign = *argv[ac]) == '-') &&
		       (*(argv[ac]+1) != '\0'))   ||
		     (argsign == '+')
		   )
		{
			for ( argdone = FALSE;
			      !argdone && (*(++(argv[ac])) != '\0');
			    ) 
			{
				switch ( *argv[ac] )
				{

				case 'D':	/* arg for preprocessor */
				case 'I':	/* arg for preprocessor */
				case 'U':	/* arg for preprocessor */
					*(argv[ac]-1) = '-';
					cpp_argv[cpp_argc++] = argv[ac]-1;
					/* skip all chars in arg after -I/D/U */
					argdone = TRUE;
					break;

#ifdef CHIPFABS
				case 'F':	/* chip fab#s to assemble for */
					/* Have option "-F[O]<n>", "-F[O]f<n>",
					 * "-F[O]i<n>", or just "-FO".
					 */
					p = argv[ac]+1;  /* p --> after "-F" */
					if (*p == 'O')
					{
						optimal_fab_workarounds_requested = TRUE;
						p++;
					}
					switch (*p)
					{
					case 'i':	/* IU fab# */
						iu_fabno = atoi(++p);
						break;
					case 'f':	/* FPU fab# */
						fpu_fabno = atoi(++p);
						break;
					case '\0':
						/* probably just gave "-FO";
						 * so this is OK.
						 */
						break;
					default:
						if ((*p >= '0') && (*p <= '9'))
						{
							/* set both fab#s = N */
							iu_fabno = fpu_fabno =
								*p - '0';
						}
						else	argerror = TRUE;
						break;
					}

					/* Make sure there are no extra chars
					 * at the end of the option.
					 */
					if ( (*p != '\0') && (*(p+1) != '\0') )
					{
						argerror = TRUE;
					}

					argdone = TRUE;
					break;
#endif /*CHIPFABS*/

				case 'L':	/* don't output symbols
						 * beginning with 'L'
						 */
					if (argsign == '-')	/*on */
					{
						options.opt_L_lbls = TRUE;
					}
					else	options.opt_L_lbls = FALSE;
					break;

				case 'O':	/* optimization */
					argdone = TRUE;
					if ( ! optimization_options(
							(argsign == '-'),
							argv[ac] )
					   )
					{
						argerror = TRUE;
						break;
					}

					if (argsign == '-')	/*on */
					{
						/* there will be at least
						 * some optimization to do.
						 */
						any_opt_to_do = TRUE;
					}
					break;

				case 'P':	/* running the C preprocessor*/
					/* "-P" means "on", "+P" means "off". */
					options.opt_preproc = (argsign == '-');
					break;

				case 'Q':	/* quick-and-dirty assembly */
					/* This option should ONLY be used by
					 * the compiler driver, and should
					 * disappear when the -compiled flag
					 * is supplied by the compiler driver.
					 */
					if (argsign == '-')
					{
						/* -Q: turn ON quick-mode. */
						assembly_mode = ASSY_MODE_QUICK;
					}
					else
					{
						/* +Q: turn OFF quick-mode. */
						assembly_mode =
							ASSY_MODE_BUILD_LIST;
					}
					break;

				case 'R':	/* read-only Data */
					make_data_readonly = TRUE;
					break;

				case 'S':	/* Disassembly option */
					argdone = TRUE;
					if ( disassembly_options(
							(argsign == '-'),
							argv[ac] ) )
					{
						do_disassembly = (argsign=='-');
					}
					else	argerror = TRUE;
					break;

#ifdef DEBUG
				case 'T':	/* token-count dump */
					debug_tkn_count = (argsign == '-');
					break;
#endif

				case 'V':	/* give version */
					fprintf(stderr, "version %s",
						asm_version);
					if (TV_SUN4 != 1)
					{
#ifdef CHIPFABS
						fprintf(stderr,
						    " (TV=0x%02x:iu%d/fpu%d)",
						    get_toolversion(),
						    iu_fabno, fpu_fabno);
#else /*CHIPFABS*/
						fprintf(stderr, " (TV=0x%02x)",
							TV_SUN4);
#endif /*CHIPFABS*/
					}
#ifdef DEBUG
					fputs(asm_make_date, stderr);
#endif /*DEBUG*/
					putc('\n', stderr);
					exit(0);
					break;

#ifdef DEBUG
				case 'd':	/* Debugging option */
					p = argv[ac] + 1; /* --> after 'd'. */
					switch (*p)
					{
					case 'a':
						print_alloc_stats =
							(argsign == '-');
						break;
					case 'i':
						/* initial debug info */
						print_initial_info =
							(argsign == '-');
						break;
					default:
						debug = (argsign == '-');
					}
					argdone = TRUE;
					break;
#endif /*DEBUG*/

				case 'k':	/* generate pic */
					picflag = TRUE;
					break;

				case 'o':	/* output object-file name */
					objfilename = argv[++ac];
					chk_objfilename(objfilename);
					/* don't scan filename arg */
					argdone = TRUE;
					break;

				case 's':
					if ( (strcmp(argv[ac], "sparc") == 0) ||
					     (strcmp(argv[ac], "sun4") == 0)  )
					{
						/* just ignore these options.*/
						argdone = TRUE;
					}
					else
					{
						/* unknown option. */
						error(ERR_UNK_OPT, NULL, 0,
							argv[ac]);
						argerror = TRUE;
						argdone = TRUE;
					}
					break;

				default:
					argchar[0] = *argv[ac];
					argchar[1] = '\0';
					error(ERR_UNK_OPT, NULL, 0,&argchar[0]);
					argerror = TRUE;
				}
			}
		}
		else
		{
			/* it must be an input filename */

			if ( ! add_input_file(argv[ac], options) )
			{
				argerror = TRUE;
			}
		}
	}

	if ( file_list == NULL )
	{
		/* there were no input filenames given */
		error(ERR_NO_INPUT, NULL, 0);
		argerror = TRUE;
	}


	switch (assembly_mode)
	{
	case ASSY_MODE_UNKNOWN:
		/* Neither -Q nor +Q was specified on the command line. */

		if (optimization_level > 0)
		{
		 	assembly_mode = ASSY_MODE_BUILD_LIST;
		}
#ifdef CHIPFABS
		else if (optimal_fab_workarounds_requested)
		{
			/* -FO was specified on the cmd line. */
			assembly_mode = ASSY_MODE_BUILD_LIST;
		}
#endif /*CHIPFABS*/
		else
		{
			/* Turn on list-building (therefore all of the checks
			 * and correct fab-workarounds), by default.
			 */
			assembly_mode = ASSY_MODE_BUILD_LIST;
				/* (this used to be ASSY_MODE_QUICK) */
		}
		break;

	case ASSY_MODE_QUICK:
		/* Here: -Q must have been specified on the cmd line. */
		if (optimization_level != 0)
		{
			/* Both -O and -Q were specified, but
			 * -O and -Q are mutually exclusive.
			 */
			error(ERR_OPT_COMBO, NULL, 0, "-O", "-Q");
			argerror = TRUE;
		}
#ifdef CHIPFABS
		if (optimal_fab_workarounds_requested)
		{
			/* Both -FO and -Q were specified, but
			 * -FO and -Q are mutually exclusive.
			 */
			error(ERR_OPT_COMBO, NULL, 0, "-FO", "-Q");
			argerror = TRUE;
		}
#endif /*CHIPFABS*/
		break;

	case ASSY_MODE_BUILD_LIST:
		/* Here: +Q must have been specified on the cmd line. */
		/* Nothing more to do here. */
		break;
	}


	if ( argerror )
	{
		fprintf(stderr, Usage, pgm_name);
		/* terminal_cleanup() will see that error_count is non-zero
		 * and give a non-zero exit status.
		 */
	}
	else
	{
#ifdef DEBUG
		if (debug || print_initial_info)
		{
			fputs("[", stderr);
#  ifdef CHIPFABS
			fprintf(stderr, "iu_fabno=%d, fpu_fabno=%d, ",
				iu_fabno, fpu_fabno);
#  endif /*CHIPFABS*/
			fprintf(stderr, "optlvl=%d, ", optimization_level);

			fputs("assy_mode=", stderr);
			switch(assembly_mode)
			{
			case ASSY_MODE_UNKNOWN:
				fputs("unknown!", stderr);
				break;
			case ASSY_MODE_BUILD_LIST:
				fputs("build_list",stderr);
				break;
			case ASSY_MODE_QUICK:
				fputs("quick",stderr);
				break;
			}
			fputs("]\n", stderr);
		}
#endif /*DEBUG*/

		pass_init();
#ifdef CHIPFABS
		/* modify opcodes, depending on target chip versions */
		setup_fab_opcodes();
#endif /*CHIPFABS*/
		pass_input();
		pass_post_input();
		if(any_opt_to_do)	pass_optimize();
		pass_assemble();
		pass_finish();
#ifdef DEBUG
		statistics(print_alloc_stats);
#endif /*DEBUG*/
	}

	terminal_cleanup();	/* no return from terminal_cleanup */
	/*NOTREACHED*/
}

static void
pass_init()
{
	extern void lex_init(), errors_init();

	catchsigs();
	lex_init();
	nodes_init();		/* must call AFTER lex_init() */
	segments_init();	/* must call AFTER lex_init() */
	errors_init();
	obj_init();
}

static void
pass_input()
{
	pass = PASS_INPUT;
	/* "curr_flp" is passed (globally) to the lexical analyzer */
	curr_flp = file_list;
	yyparse();
}

static void
pass_post_input()
{
	pass = PASS_POST_INPUT;
	scan_symbols_after_input();
}

static void
pass_optimize()
{
	pass = PASS_OPTIMIZE;
	if (error_count == 0)
	{
		curr_flp = file_list;
		optimize_all_segments();
	}
}

static void
pass_assemble()
{
	pass = PASS_ASSEMBLE;
	if (error_count == 0)
	{
		curr_flp = file_list;
		do_for_each_segment(check_list_for_errors);
#ifdef CHIPFABS
		do_for_each_segment(perform_fab_workarounds);
#endif /*CHIPFABS*/
		do_for_each_segment(emit_list_of_nodes);

		scan_symbols_after_assy();
	}
}

static void
pass_finish()
{
	pass = PASS_FINISH;
	if (error_count == 0)
	{
		/* resolve_relocations must be done AFTER
		 * scan_symbols_after_assy() !
		 */
		resolve_relocations();

		write_obj_file();
	}
}

#ifdef DEBUG
static void
statistics(print_alloc_stats)
	Bool print_alloc_stats;
{
	if (print_alloc_stats)	alloc_statistics();
}
#endif /*DEBUG*/


static Bool
add_input_file(filename, opts)		/* returns TRUE if all seems OK */
	register char *filename;
	Options opts;
{
	register int filename_len;	/* (temp) length of filename */
	static struct filelist *tailp=NULL;/* ptr to last input file on list */
	struct filelist *old_tailp;	/* temp ptr to "    "    "   "   "   */
	extern char *strcpy();

	filename_len = strlen(filename);

	old_tailp = tailp;
	tailp = (struct filelist *)as_malloc(sizeof(struct filelist));
	tailp->fl_fnamep  = (char *)as_malloc(filename_len+1);
	strcpy(tailp->fl_fnamep, filename);
	tailp->fl_options = opts;
	tailp->fl_next    = NULL;

	if (old_tailp == NULL)	file_list = tailp;
	else			old_tailp->fl_next = tailp;

	return TRUE;
}


static void
chk_objfilename(outfilename)
	register char *outfilename;
{
	register char *extp;	/* pointer to '.' extension on filename */
	register int len;	/* length of object file name		*/

	/* Do not allow the assembler to overwrite an existing file which
	 * probably contains source code (determined by its name)
	 */

	if( (len = strlen(outfilename)) >= 2)
	{
		/* extp -> next-to-last character of output file name */
		extp = outfilename + len - 2;

		if (*extp == '.')
		{
			switch (*(extp+1))	/* char after the '.' */
			{
			case 's':	/* assembly	*/
			case 'S':	/* assembly	*/
			case 'c':	/* C		*/
			case 'p':	/* Pascal	*/
			case 'f':	/* Fortran	*/
			case 'F':	/* Fortran	*/
			case 'y':	/* Yacc		*/
			case 'l':	/* Lex		*/
				if ( access(outfilename, F_OK) == 0 )
				{
					fprintf(stderr,
						"%s: would overwrite \"%s\"\n",
						pgm_name, outfilename);
					terminal_cleanup();
					/* no return from terminal_cleanup */
					/*NOTREACHED*/
				}
				else
				{
					/* if file doesn't already exist, let
					 * the user get away with it.
					 */
				}
			}
		}
	}
}



static void
cleanup()
{
	extern void unlink_pp_tmp_file();
	extern void obj_cleanup();

	close_input_file();

	/* remove /tmp file from preprocessor, if there was one */
	unlink_pp_tmp_file();

	/* remove /tmp file(s) from object-file preparation, if any */
	obj_cleanup();
}


void
terminal_cleanup()
{
	cleanup();

	if (error_count != 0)
	{
		/* remove object file, so that the user doesn't think that
		 * it's really up-to-date.
		 */
		unlink(objfilename);
		exit(1);
	}
	else	exit(0);
}


static void
catchsigs()
{
	register int *sigp;
	static int sigs_to_catch[] =
	    { SIGHUP, SIGINT, SIGPIPE, SIGALRM, SIGTERM, SIGXCPU, SIGXFSZ, 0 };

	for (sigp = &sigs_to_catch[0];   *sigp != 0;    sigp++)
	{
		/* Setup to catch the signal, but if it was already being
		 * IGNORED (e.g. process is in background), leave it ignored.
		 */
		if ( signal(*sigp, terminal_cleanup) == SIG_IGN )
		{
			signal(*sigp, SIG_IGN);
		}
	}
}

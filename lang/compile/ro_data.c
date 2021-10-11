#ifndef lint
static	char sccsid[] = "@(#)ro_data.c 1.53 88/09/26 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "driver.h"

	/* Suffixes. */
	Suffixes	suffix=
	{
		{ "a", DRIVER_CFMP, DRIVER_none, NULL, collect_o,		"Object library" },
		{ "c", SOURCE(DRIVER_CFLMP), DRIVER_C, compile_c, NULL,		"C source" },
		{ "def", SOURCE(DRIVER_M), DRIVER_none, compile_def, NULL,	"Module definitions" },
		{ "f", SOURCE(DRIVER_CFMP), DRIVER_F, compile_f, NULL,		"F77 source" },
		{ "F", SOURCE(DRIVER_CFMP), DRIVER_none, compile_F, NULL,	"F77 source for cpp" },
		{ "il", DRIVER_CFMP, DRIVER_none, NULL, NULL,			"Inline expansion file" },
		{ "i", SOURCE(DRIVER_CL), DRIVER_C, compile_i, NULL,		"C source after cpp" },
		{ "ln", DRIVER_L, DRIVER_L, NULL, collect_ln,			"Lint library" },
		{ "mod", SOURCE(DRIVER_M), DRIVER_none, compile_mod, NULL,	"Modula-2 source" },
		{ "o", DRIVER_CFMP, DRIVER_CFMP, NULL, collect_o,		"Object file" },
		{ "pas", SOURCE(DRIVER_P), DRIVER_none, compile_p, NULL,	"Pascal source" },
		{ "pi", SOURCE(DRIVER_P), DRIVER_P, compile_pi, NULL,		"Pascal source after cpp" },
		{ "p", SOURCE(DRIVER_CFMP), DRIVER_none, compile_p, NULL,	"Pascal source" },
		{ "r", SOURCE(DRIVER_CFMP), DRIVER_none, compile_r, NULL,	"Ratfor source" },
		{ "sym", DRIVER_none, DRIVER_M, NULL, NULL,			"Modula2 Symbol file" },
		{ "s", SOURCE(DRIVER_CFMP), DRIVER_CFMP, compile_s, NULL,	"Assembler source" },
		{ "so", DRIVER_CFMP, DRIVER_none, NULL,	collect_o, "Shared object" },
		{ "S", SOURCE(DRIVER_CFMP), DRIVER_none, compile_S, NULL,	"Assembler source for cpp" },
		{ "ir", DRIVER_none, DRIVER_none, NULL, NULL,			"IR Intermediate file" },
		{ "none", DRIVER_none, DRIVER_none, NULL, NULL,			"Intermediate file" },
	};


	/* Drivers. */
	Const_int	cc=		{ DRIVER_C, (char *)cc_doit, "c"      };
	Const_int	dummy=		{ DRIVER_none, NULL,         "(none)" };
	Const_int	f77=		{ DRIVER_F, (char *)f77_doit,"fortran"};
	Const_int	xlint=		{ DRIVER_L, (char *)lint_doit,"lint"  };
	Const_int	m2c=		{ DRIVER_M, (char *)m2c_doit,"modula2"};
	Const_int	pc=		{ DRIVER_P, (char *)pc_doit, "pascal" };


	/* Debuggers. */
	Const_int	adb=		{ DEBUG_ADB, "adb", NULL };
	Const_int	dbx=		{ DEBUG_DBX, "dbx", NULL };


	/* Compilation "goals". */
	Const_int	assembler=	{ GOAL_S, "assembly-source", NULL };
	Const_int	preprocessed=	{ GOAL_I, "source", NULL };
	Const_int	preprocessed2=	{ GOAL_I, "source", NULL };
	Const_int	executable=	{ GOAL_EXE, "executable", NULL };
	Const_int	lint1_file=	{ GOAL_L, "lint", NULL };
	Const_int	object=		{ GOAL_O, "object", NULL };


	/* Floating-point types. */
	Const_int	fswitch=	{ FLOAT_SWITCH, "-fswitch", NULL };
	/* (the rest are in rw_data.c) */


	/* System architectures. */
	Const_int	arch_sun2=	{ ARCH_SUN2,  "-target sun2", "sun2" };
	Const_int	arch_sun3x=	{ ARCH_SUN3X, "-target sun3x","sun3x"};
	Const_int	arch_sun386=	{ ARCH_SUN386,"-target sun386",
								      "sun386"};
	Const_int	arch_sun3=	{ ARCH_SUN3,  "-target sun3", "sun3" };
	Const_int	arch_sun4c=	{ ARCH_SUN4C,  "-target sun4c", "sun4c" };
	Const_int	arch_sun4=	{ ARCH_SUN4,  "-target sun4", "sun4" };

	Const_intP	known_architectures[]=
					{
#if defined(CROSS) || defined(sun2)
					   &arch_sun2,
#endif /*defined(CROSS) || defined(sun2)*/
#if defined(CROSS) || defined(sun3x)
					   &arch_sun3x, /* must come before 3*/
#endif /*defined(CROSS) || defined(sun3x)*/
#if defined(CROSS) || defined(sun386)
					   &arch_sun386,/* must come before 3*/
#endif /*defined(CROSS) || defined(sun386)*/
#if defined(CROSS) || defined(sun3)
					   &arch_sun3,
#endif /*defined(CROSS) || defined(sun3)*/
#if defined(CROSS) || defined(sun4c)
					   &arch_sun4c,
#endif /*defined(CROSS) || defined(sun4c)*/
#if defined(CROSS) || defined(sun4)
					   &arch_sun4,
#endif /*defined(CROSS) || defined(sun4)*/
					   (Const_intP)NULL
					};


	/* Target software releases. */
	Const_int	sw_release_default=
					{ SW_REL_DFLT, "",         ""     };

	Const_int	sw_release_3x_implicit=
					{ SW_REL_3X,   "3.x",      ""     };
	Const_int	sw_release_41_implicit=
					{ SW_REL_41,   "4.1",      ""     };

	Const_int	sw_release_3x_explicit=
					{ SW_REL_3X,   "3.x",      "R3.2" };
	Const_int	sw_release_32_explicit=
					{ SW_REL_3X,   "3.2",      "R3.2" };
	Const_int	sw_release_34_explicit=
					{ SW_REL_3X,   "3.4",      "R3.2" };
	Const_int	sw_release_35_explicit=
					{ SW_REL_3X,   "3.5",      "R3.2" };
	Const_int	sw_release_sys4_explicit=
					{ SW_REL_3X,   "sys4-3.2", "R3.2" };
	Const_int	sw_release_40_implicit=
					{ SW_REL_40,   "4.0",      ""     };
	Const_int	sw_release_40_explicit=
					{ SW_REL_40,   "4.0",      "R4.0" };
	Const_int	sw_release_41_explicit=
					{ SW_REL_41,   "4.1",      "R4.1" };

	Const_intP	known_sw_releases[]=
					{
						&sw_release_3x_explicit,
						&sw_release_32_explicit,
						&sw_release_34_explicit,
						&sw_release_35_explicit,
						&sw_release_sys4_explicit,
						&sw_release_40_explicit,
						&sw_release_41_explicit,
						(Const_intP)NULL
					};



#define OPTION(name, driver, type, help) \
	{ (name), (driver), (type), 0, NULL, NULL, NULL, (help) },

	/* OPTION_SET_SW_REL: used to set one (or all) of the internal
	 *	Software Release options.
	 */
#define OPTION_SET_SW_REL(name, driver, which, help) \
	{ (name), (driver), set_sw_release_option, (which), NULL, NULL, (help)},

	/* OPTION_SET0: presence of the cmd-line flag causes the given global
	 *	flag to be set to 0 (FALSE).
	 */
#define OPTION_SET0(name, driver, flag, help) \
	{ (name), (driver), set_int_arg_option, 0, (Named_intP)(&global_flag.flag), NULL, (Const_intP)0, (help)},

	/* OPTION_SET1: presence of the cmd-line flag causes the given global
	 *	flag to be set to 1 (TRUE).
	 */
#define OPTION_SET1(name, driver, flag, help) \
	{ (name), (driver), set_int_arg_option, 0, (Named_intP)(&global_flag.flag), NULL, (Const_intP)1, (help)},

	/* OPTION_SET ... */
#define OPTION_SET(name, driver, flag, arg, help) \
	{ (name), (driver), set_named_int_arg_option, 0, &(flag), NULL, &(arg), (help)},

	/* OPTION_TARGET_ARCH1 causes the variable pointed to by "flag" to be
	 *	given the (Const_int) value pointed to by "arg".
	 */
#define OPTION_TARGET_ARCH1(name, driver, flag, arg, help) \
	{ (name), (driver), set_target_arch_option1, 0, &(flag), NULL, &(arg), (help)},

	/* OPTION_TARGET_ARCH2:
	 *	option_target_arch2:   -target {TARGET}  ==> (lookup {TARGET})
	 *	causes the (target architecture) variable pointed to by "flag"
	 *	to be given a (Const_int) value, which is determied by looking
	 *	up the name given by the following argument.
	 */
#define OPTION_TARGET_ARCH2(name, driver, flag, help) \
	{ (name), (driver), set_target_arch_option2, 0, &(flag), NULL, NULL, (help)},

	/* OPTION_TARGET_PROC1 causes the variable pointed to by "flag" to be
	 *	given the (Const_int) value pointed to by "arg".
	 *	It also triggers special handling for target processor types
	 *	(target architecture types are normally used, not processor
	 *	 types).
	 */
#define OPTION_TARGET_PROC1(name, driver, flag, arg, help) \
	{ (name), (driver), set_target_proc_option1, 0, &(flag), NULL, &(arg), (help)},

	/* OPTION_PASS_ON_1:
	 *	pass_on_1_option:	-x	=> prog -x
	 */
#define	OPTION_PASS_ON_1(name, driver, prog, help) \
	{ (name), (driver), pass_on_1_option, 0, NULL, &(program.prog), NULL, (help)},

	/* OPTION_PASS_ON_1T:
	 *	pass_on_1t_option:	-xREST	=> prog -xREST
	 */
#define	OPTION_PASS_ON_1T(name, driver, prog, help) \
	{ (name), (driver), pass_on_1t_option, PASS_OPT_AS_FLAG, NULL, &(program.prog), NULL, (help)},

	/* OPTION_PASS_ON_1T_INFILE:
	 *	pass_on_1t_option:	-xREST	=> prog -xREST
	 *				(but -xREST is passed with the infiles)
	 */
#define	OPTION_PASS_ON_1T_INFILE(name, driver, prog, help) \
	{ (name), (driver), pass_on_1t_option, PASS_OPT_AS_INFILE, NULL, &(program.prog), NULL, (help)},


	/* OPTION_PASS_ON_1T_OR_12_AS_1T:
	 *	pass_on_1t12_1t_option:		-xREST	=> prog -xREST
	 *				 [or]	-x REST	=> prog -xREST
	 */
#define	OPTION_PASS_ON_1T_OR_12_AS_1T(name, driver, prog, help) \
	{ (name), (driver), pass_on_1t12_1t_option, 0, NULL, &(program.prog), NULL, (help)},

	/* OPTION_PASS_ON_1T_OR_12_AS_1T_PC:
	 * 	pass_on_1t12_1t_option_pc: 	-configREST => prog -configREST
	 *				   [or] -config REST=> prog -configREST
	 */
#define OPTION_PASS_ON_1T_OR_12_AS_1T_PC(name, driver, prog, help) \
	{ (name), (driver), pass_on_1t12_1t_option_pc, 0, NULL, &(program.prog), NULL, (help)},

	/* OPTION_PASS_ON_1TO:
	 *	pass_on_1to_option:	-xREST	=> prog REST
	 */
#define	OPTION_PASS_ON_1TO(name, driver, prog, help) \
	{ (name), (driver), pass_on_1to_option, 0, NULL, &(program.prog), NULL, (help)},


	/* OPTION_PASS_ON_12:
	 *	pass_on_12_option:	-x REST	=> prog -x REST
	 */
#define	OPTION_PASS_ON_12(name, driver, prog, help) \
	{ (name), (driver), pass_on_12_option, 0, NULL, &(program.prog), NULL, (help)},
#define END_OF_OPTIONS() { NULL, 0, end_of_list, 0, NULL, NULL, NULL, NULL}


Option options[]=
{
/*
 *	Order is important in this table!
 *
 *	Long strings *must* come before short ones, since the first partial
 *	match is used.
 *
 *	Options are listed in this table in alphabetical order, except when
 *	this violates the above rule.  (This is by convention, not necessity).
 */

	OPTION_PASS_ON_1("-66", DRIVER_F, f77pass1, "Report non FORTRAN-66 constructs as errors")

	OPTION_PASS_ON_12("-align", NO_MINUS(DRIVER_CFMP), ld, "Page align and pad symbol X (for ld)")
	OPTION_PASS_ON_1("-ansi", DRIVER_F, f77pass1, "Report non-ANSI extensions.")
	OPTION_PASS_ON_12("-assert", DRIVER_C, ld, "Specify link time assertion")
	OPTION("-a", DRIVER_L, pass_on_lint_option, "Report assignments of long values to int variables")

#ifndef sun386
	OPTION_SET1("-a", DRIVER_CFM, statement_count, "Prepare to count # of times each basic block is executed")
#else
	OPTION_SET1("-a", DRIVER_F, statement_count, "Prepare to count # of times each basic block is executed")
#endif

#ifndef sun386
	OPTION_PASS_ON_1TO("-A", DRIVER_C, as, "Obsolete. Use \"-Qoption as X\".")
	OPTION_PASS_ON_12("-A", HIDE(DRIVER_FMP), ld, "Incremental linking")
	OPTION_SET1("-bnzero", HIDE(DRIVER_C), generate_nonzero_activation_records, "Generate code with nonzero activation_records")
#endif /*!sun386*/

	OPTION("-b", DRIVER_L, pass_on_lint_option, "Report break statements that can not be reached")
	OPTION_PASS_ON_1("-b", DRIVER_P, pc0, "Buffer 'output' in blocks, not lines")

	/* "-B" is treated like an "infile", to keep args to ld correct in the
	 * face of command lines like:  "cc -Bstatic -lfoo.a -Bdynamic -lbar.a".
	 */
#ifdef sun386
	OPTION_PASS_ON_1T_INFILE("-B", DRIVER_C, ld,"Specify the binding types")
#else
	OPTION_PASS_ON_1T_INFILE("-B", DRIVER_CFMP, ld,"Specify the binding types")
#endif

	OPTION_PASS_ON_1T_OR_12_AS_1T_PC("-config", HIDE(DRIVER_P), cpp, "Define cppas symbol X (for cppas)")
        OPTION_PASS_ON_1("-cond", HIDE(DRIVER_P), cpp , "Apollo specific flag")
#ifdef BROWSER
	OPTION_SET1("-cb", DRIVER_CFP, code_browser_seen, "Collect information for code browser")
#endif
	OPTION_SET("-c", DRIVER_CFP, product, object, "Produce '.o' file. Do not run ld.")
	OPTION("-c", DRIVER_L, pass_on_lint_option, "Complain about casts with questionable portability")
	OPTION_PASS_ON_1("-C", DRIVER_C, cpp, "Make cpp preserve C style comments")

	OPTION_PASS_ON_1("-C", DRIVER_F, f77pass1, "Generate code to check subscripts")

	OPTION_SET1("-C", DRIVER_P, checkC, "Generate code to check subscripts and subranges")
	OPTION("-C", DRIVER_L, make_lint_lib_option, NULL)
#ifndef sun386
	OPTION_SET1("-dalign", DRIVER_CFMP, doubleword_aligned_doubles, "Assume that all double-prec f.p. values are doubleword aligned.")
#endif
	OPTION_SET1("-debug", HIDE(DRIVER_all), debug, "[internal debugging]")
	OPTION_SET1("-dryrun", DRIVER_all, dryrun, "Show but do not execute the cmds constructed by the driver")

#ifndef sun386
	OPTION_SET0("-dl", HIDE(DRIVER_C), sparc_sdata, "Generate all data-segment references \"long\" for SPARC")
	OPTION_SET1("-ds", HIDE(DRIVER_C), sparc_sdata, "Generate Small-Data Segment references for SPARC")

	OPTION_PASS_ON_1("-d", DRIVER_F, f77pass1, "Debug/trace option")
	OPTION_PASS_ON_1("-d", HIDE(DRIVER_CMP), ld, "Force definition of common")
#endif /*!sun386*/

	OPTION_PASS_ON_1T_OR_12_AS_1T("-D", DRIVER_CFLP, cpp, "Define cpp symbol X (for cpp)")
	OPTION_PASS_ON_1("-e", DRIVER_F, f77pass1, "Recognize extended (132 character) source lines")
	OPTION("-e", NO_MINUS(DRIVER_M), run_m2l_option, NULL)
	OPTION("-E", NO_MINUS(DRIVER_M), load_m2l_option, NULL)
	OPTION_SET("-E", DRIVER_C, product, preprocessed, "Run source thru cpp, output to stdout")
#ifndef sun386
	OPTION_SET("-f68881", DRIVER_CFMP, float_mode, f68881, NULL)
	OPTION_SET("-ffpa", DRIVER_CFMP, float_mode, ffpa, NULL)
	OPTION_SET("-fsky", DRIVER_CFMP, float_mode, fsky, NULL)
	OPTION_SET("-fsoft", DRIVER_CFMP, float_mode, fsoft, NULL)
#endif
	OPTION_SET1("-fstore", DRIVER_CF, fstore, "Force assignments to write to memory")
#ifndef sun386
	OPTION_SET("-fswitch", DRIVER_CFMP, float_mode, fswitch, NULL)
#endif
	OPTION_PASS_ON_1("-fsingle2", DRIVER_C, ccom, "Pass float values as float not double")
	OPTION_PASS_ON_1("-fsingle", DRIVER_C, ccom, "Use single precision arithmetic when 'float' only")
#ifndef sun386
	OPTION_PASS_ON_1("-f", DRIVER_F, f77pass1, "Force non-standard alignment of 8-byte quantities")
#endif

	OPTION_SET("-F", DRIVER_F, product, preprocessed2,"Run cpp/ratfor only")

	OPTION_SET("-go", DRIVER_C, debugger, adb, "Generate extra information for adb")
	OPTION_SET("-g", DRIVER_CFMP, debugger, dbx, "Generate extra information for dbx")
	OPTION("-host=", DRIVER_L, lint1_option, "specify host to lint1")
	OPTION("-help", DRIVER_all, help_option, NULL)
	OPTION("-h", DRIVER_L, pass_on_lint_option, "Be heuristic")
	OPTION_SET1("-H", DRIVER_P, checkH, "Generate code to check heap pointers")

#if defined(CROSS) || defined(i386)
	OPTION_TARGET_PROC1("-i386", HIDE(DRIVER_all), target_arch, arch_sun386, "Generate 80386 .o's")
#endif /*defined(CROSS) || defined(i386)*/

	OPTION_PASS_ON_1("-i2", DRIVER_F, f77pass1, "Make integers be two bytes by default")
	OPTION_PASS_ON_1("-i4", DRIVER_F, f77pass1, "Make integers be four bytes by default")

	OPTION_PASS_ON_12("-i", DRIVER_P, pc0, "Produce list of module")
	OPTION("-i", DRIVER_L, lint_i_option, "Run lint pass1 only. Leave '.ln' files")
	OPTION_PASS_ON_1T("-I", DRIVER_CFLP, cpp, "Add directory X to cpp include path (for cpp)")

#ifndef sun386
	OPTION_SET1("-J", DRIVER_CMP, long_offset, "Generate long offsets for switch/case statements")
#endif /*!sun386*/

	OPTION_SET0("-keeptmp", HIDE(DRIVER_all), remove_tmp_files, "keep /tmp files [for debugging]")
	OPTION_PASS_ON_1("-keys", DRIVER_M, m2l, "Report detailed result of consistency checks by m2l")
	OPTION_PASS_ON_1("-list", DRIVER_M, m2cfe, "Generate listing")
	OPTION("-l", DRIVER_CFM, infile_option, "Read object library (for ld)")
	OPTION("-l", DRIVER_L, infile_option, "Read definition of object library")
	OPTION("-l", DRIVER_P, infile_option, "Read object library (for ld) or generate listing")
	OPTION_PASS_ON_1T_OR_12_AS_1T("-L", DRIVER_CFM, ld, "Add directory X to ld library path (for ld)")
	OPTION_PASS_ON_1("-L", DRIVER_P, pc0, "Map upper case letters to lower case in identifiers and keywords")
	OPTION_SET1("-m4", DRIVER_F, do_m4, "Run source thru m4")

#ifndef sun386
#if defined(CROSS) || defined(mc68000)	/* mc68000, not mc680x0 here */
	OPTION_TARGET_PROC1("-mc68010", HIDE(DRIVER_all), target_arch, arch_sun2, "Generate 68010 .o's")

/* define mc68020 architecture for sun3 or sun3x */
#ifdef sun3x 
	OPTION_TARGET_PROC1("-mc68020", HIDE(DRIVER_all), target_arch, arch_sun3x, "Generate 68020 .o's")
#else
	OPTION_TARGET_PROC1("-mc68020", HIDE(DRIVER_all), target_arch, arch_sun3, "Generate 68020 .o's")
#endif

	OPTION_TARGET_PROC1("-m68010", HIDE(DRIVER_all), target_arch, arch_sun2, "Generate 68010 .o's")

/* define m68020 architecture for sun3 or sun3x */
#ifdef sun3x 
	OPTION_TARGET_PROC1("-m68020", HIDE(DRIVER_all), target_arch, arch_sun3x, "Generate 68020 .o's")
#else
	OPTION_TARGET_PROC1("-m68020", HIDE(DRIVER_all), target_arch, arch_sun3, "Generate 68020 .o's")
#endif

#endif /*defined(CROSS) || defined(mc68000)*/

	OPTION_SET1("-misalign", DRIVER_CFP, handle_misalignment, "Handle misalinged Sun-4 data w/pessimistic code")
#endif /*!sun386*/

	OPTION_PASS_ON_12("-map", NO_MINUS(DRIVER_M), m2l, "Generate modula-2 link map file")
	OPTION("-m", NO_MINUS(DRIVER_M), module_option, NULL)
	OPTION("-M", DRIVER_M, module_list_option, NULL)
	OPTION_SET1("-M", DRIVER_C, do_dependency, "Collect dependencies")
	OPTION_PASS_ON_1T("-Nd", DRIVER_F, f77pass1, "Set size of structure-decl nesting compiler table to X")
	OPTION_PASS_ON_1T("-Nl", DRIVER_F, f77pass1, "Set maximum  number of continuation lines to X")
	OPTION_PASS_ON_1("-nobounds", DRIVER_M, m2cfe, "Do not compile array bound checking code")
	OPTION_PASS_ON_1("-norange", DRIVER_M, m2cfe, "Do not compile range checking code")
	OPTION_SET0("-normcmds", HIDE(DRIVER_all), show_rm_commands, "Don't bother to show \"rm\" commands in -dryrun or -verbose output")
	OPTION_SET1("-noc2", DRIVER_CFM, no_c2, "Don't do peephole optimization, even if -O is used")
	OPTION("-n", DRIVER_L, lint_n_option, "Do not check against C library")

	OPTION_PASS_ON_1("-n", HIDE(DRIVER_CFMP), ld, "Make shared")
	OPTION_PASS_ON_1T("-Nc", DRIVER_F, f77pass1, "Set size of do loop nesting compiler table to X")
	OPTION_PASS_ON_1T("-Nn", DRIVER_F, f77pass1, "Set size of identifier compiler table to X")
	OPTION_PASS_ON_1T("-Nq", DRIVER_F, f77pass1, "Set size of equivalenced variables compiler table to X")
	OPTION_PASS_ON_1T("-Ns", DRIVER_F, f77pass1, "Set size of statement numbers compiler table to X")
	OPTION_PASS_ON_1T("-Nx", DRIVER_F, f77pass1, "Set size of external identifier compiler table to X")

	OPTION_PASS_ON_1("-N", HIDE(DRIVER_CFMP), ld, "Do not make shared")

	OPTION_SET1("-onetrip", DRIVER_F, onetrip, "Perform DO loops at least once")

	OPTION("-o", NO_MINUS(DRIVER_CFLMP), outfile_option, NULL)
	OPTION("-O", DRIVER_CFMP, optimize_option, NULL)
#ifndef sun386
	OPTION_SET1("-purecross", DRIVER_all, pure_cross, "Pure cross-compilation; use no files from /")
#endif
	OPTION_SET1("-pipe", DRIVER_all, pipe_ok, "Use pipes instead of temp files, when possible")
	OPTION_SET1("-pic", DRIVER_C, pic_code, "Generate pic code with short offset")
	OPTION_SET1("-pic", HIDE(DRIVER_F), pic_code, "Generate pic code with short offset")
	OPTION_SET1("-PIC", DRIVER_C, PIC_code, "Generate pic code with long offset")
	OPTION_SET1("-PIC", HIDE(DRIVER_F), PIC_code, "Generate pic code with long offset")
	OPTION_SET("-pg", DRIVER_CFMP, profile, gprof, "Prepare to collect data for the gprof program")
	OPTION_SET("-p", DRIVER_CFMP, profile, prof, "Prepare to collect data for the prof program")

#ifdef sun386
	OPTION("-p", DRIVER_L, pass_on_lint_option, "Check for non-portable constructs")
#endif

	OPTION_SET("-P", DRIVER_C, product, preprocessed2, "Run source thru cpp, output to file.i")

	OPTION("-P", DRIVER_F, optimize_option, NULL)

	OPTION_PASS_ON_1("-P", DRIVER_P, pc0, "Use partial evaluation semantics for and/or")
	OPTION("-Qoption",  NO_MINUS(DRIVER_all), pass_on_select_option, NULL)
	OPTION("-Qpath",    NO_MINUS(DRIVER_all), path_option, NULL)
	OPTION("-Qproduce", NO_MINUS(DRIVER_all), produce_option, NULL)
        OPTION("-q", DRIVER_L, pass_on_lint_option, "Do not complain about Sun-specific C constructs")

	OPTION_SET_SW_REL("-release",   NO_MINUS(DRIVER_CFLMP), R_all,    NULL)
	OPTION_SET_SW_REL("-relvroot",  NO_MINUS(DRIVER_CFLMP), R_VROOT,  NULL)
	OPTION_SET_SW_REL("-relpaths",  NO_MINUS(DRIVER_CFLMP), R_PATHS,  NULL)
	OPTION_SET_SW_REL("-relpasses", NO_MINUS(DRIVER_CFLMP), R_PASSES, NULL)


#ifndef sun386
	OPTION_PASS_ON_1("-r", HIDE(DRIVER_CFMP), ld, "Make relocatable")
#endif /*!sun386*/

	OPTION_SET1("-R", DRIVER_CMP, as_R, "Merge data segment into text segment (for as)")

#ifndef sun386
	OPTION_PASS_ON_1TO("-R", DRIVER_F, ratfor, "Obsolete. Use -Qoption ratfor opt.")
#endif /*!sun386*/

#if defined(CROSS) || defined(sparc)

/* define sparc architecture for sun4 or sun4c */
#ifdef sun4c
	OPTION_TARGET_PROC1("-sparc", HIDE(DRIVER_all), target_arch, arch_sun4c, "Generate SPARC .o's")
#else
	OPTION_TARGET_PROC1("-sparc", HIDE(DRIVER_all), target_arch, arch_sun4, "Generate SPARC .o's")
#endif

#endif /*defined(CROSS) || defined(sparc)*/

#if defined(CROSS) || defined(sun2)
	OPTION_TARGET_ARCH1("-sun2", DRIVER_all, target_arch, arch_sun2, "Generate code for a Sun2 system")
#endif /*defined(CROSS) || defined(sun2)*/

#if defined(CROSS) || defined(sun3x)
	OPTION_TARGET_ARCH1("-sun3x", DRIVER_all, target_arch, arch_sun3x, "Generate code for a Sun3x system")
#endif /*defined(CROSS) || defined(sun3x)*/

#if defined(CROSS) || defined(sun386)
	OPTION_TARGET_ARCH1("-sun386", DRIVER_all, target_arch, arch_sun386, "Generate code for a Sun386 system")
#endif /*defined(CROSS) || defined(sun386)*/

#if defined(CROSS) || defined(sun3)
	OPTION_TARGET_ARCH1("-sun3", DRIVER_all, target_arch, arch_sun3, "Generate code for a Sun3 system")
#endif /*defined(CROSS) || defined(sun3)*/

#if defined(CROSS) || defined(sun4c)
	OPTION_TARGET_ARCH1("-sun4c", DRIVER_all, target_arch, arch_sun4c, "Generate code for a Sun4c system")
#endif /*defined(CROSS) || defined(sun4c)*/

#if defined(CROSS) || defined(sun4)
	OPTION_TARGET_ARCH1("-sun4", DRIVER_all, target_arch, arch_sun4, "Generate code for a Sun4 system")
#endif /*defined(CROSS) || defined(sun4)*/

	/* -sys5 is an undocumented flag for testing purposes only */
	/* using this flag sets compile for sys5 as opposed to bsd version */
	OPTION_SET1("-sys5", HIDE(DRIVER_all), sys5_flag, "Set driver to sys5 version" ) 
#ifdef BROWSER
	OPTION_SET1("-sb", DRIVER_CFP, code_browser_seen, "Collect information for code browser")
#endif

	OPTION_PASS_ON_1("-s", DRIVER_P, pc0, "Accept Standard Pascal only")
	OPTION_PASS_ON_1("-s", HIDE(DRIVER_CFM), ld, "Strip")
	OPTION_SET("-S", DRIVER_CFMP, product, assembler, "Produce '.s' file. Do not run \"as\".")
	OPTION("-target=", DRIVER_L, lint1_option, "specify target to lint1")
	OPTION_TARGET_ARCH2("-target", DRIVER_all, target_arch, "specify target architecture")
	OPTION("-temp=", DRIVER_all, temp_dir_option, NULL)
	OPTION_SET1("-test", HIDE(DRIVER_all), testing_driver, "Testing driver")
	OPTION_SET1("-time", DRIVER_all, time_run, "Report the execution time for the compiler passes")
	OPTION_SET1("-trace", DRIVER_M, trace, "Show compiler actions")
	OPTION_PASS_ON_1("-t",  HIDE(DRIVER_CFMP),  ld, "Trace ld")
	OPTION_PASS_ON_12("-Tdata", HIDE(DRIVER_CFMP), ld, "Set data address")
	OPTION_PASS_ON_12("-Ttext", HIDE(DRIVER_CFMP), ld, "Set text address")
	OPTION_PASS_ON_12("-T",     HIDE(DRIVER_CFMP), ld, "Set text address")

	/* -ucb is an undocumented flag for testing purposes only */
	/* using this flag sets compile for ucb as opposed to sys5 version */
	OPTION_SET0("-ucb", HIDE(DRIVER_all), sys5_flag, "Set driver to bsd version" ) 

	OPTION_PASS_ON_1("-u", DRIVER_F, f77pass1, "Make the default type be 'undefined', not 'integer'")

	OPTION_PASS_ON_12("-u", HIDE(DRIVER_CMP), ld, "Undefine")
	OPTION("-u", DRIVER_L, pass_on_lint_option, "Library mode")
	OPTION_PASS_ON_1T_OR_12_AS_1T("-U", DRIVER_CLP, cpp, "Delete initial definition of cpp symbol X (for cpp)")

	OPTION_PASS_ON_1("-U", DRIVER_F, f77pass1, "Do not map upper case letters to lower case")

	OPTION_SET1("-verbose", DRIVER_L, verbose, "Report which programs the driver invokes")

#ifndef sun386
	OPTION_SET1("-vpa", DRIVER_F, vpa, "Invoke pre- and post-processing for Sun VPA")
#endif /*!sun386*/

	OPTION_SET1("-v", DRIVER_CFMP, verbose, "Report which programs the driver invokes")
	OPTION("-v", DRIVER_L, pass_on_lint_option, "Do not complain about unused formals")
	OPTION_SET1("-V", DRIVER_P, checkV, "Report hard errors for non standard Pascal constructs")

	OPTION_PASS_ON_1("-w66", DRIVER_F, f77pass1, "Do not print FORTRAN 66 compatibility warnings")

	OPTION_PASS_ON_1("-w", DRIVER_C, ccom,     "Do not print warnings")

	OPTION_PASS_ON_1("-w", DRIVER_F, f77pass1, "Do not print warnings")

	OPTION_SET1(     "-w", DRIVER_P, warning,  "Do not print warnings")
        OPTION_PASS_ON_1("-xl", HIDE(DRIVER_P), pc0, "Apollo specific flag")
	OPTION("-x", DRIVER_L, pass_on_lint_option, "Report variables referred to by extern but not used")
	OPTION_PASS_ON_1("-x", HIDE(DRIVER_CFMP), ld, "Remove local symbols")
	OPTION_PASS_ON_1("-X", HIDE(DRIVER_CFMP), ld, "Save most local symbols")
	OPTION_PASS_ON_1T("-y",HIDE(DRIVER_CFMP), ld, "Trace symbol")

	OPTION("-z", DRIVER_L, pass_on_lint_option, "Do not complain about referenced but not defined structs")
	OPTION_PASS_ON_1("-z", DRIVER_P, pc0, "Prepare to collect data for the pxp program")
	OPTION_PASS_ON_1("-z", HIDE(DRIVER_CFM), ld, "Make demand load")
	OPTION_PASS_ON_1("-Z", DRIVER_P, pc0, "Initialize local storage to zero")

	END_OF_OPTIONS()
};

#define DRIVER(name, drivers, value) { (name), (drivers), set_named_int_arg_option, 0, &driver, NULL, (value), NULL},
Option drivers[]=
{
	DRIVER("cc",      DRIVER_C, &cc)
	DRIVER("compile", HIDE(0),  &dummy)
	DRIVER("f77",     DRIVER_F, &f77)
	DRIVER("lint",    DRIVER_L, &xlint)
	DRIVER("m2c",     DRIVER_M, &m2c)
	DRIVER("pc",      DRIVER_P, &pc)
	END_OF_OPTIONS()
};

/*
 *	A set of simple functions that are used to determine if
 *	a particular compiler step should be used this time
 */
static int
cppas_expr()
{
	return is_on(do_cppas);
}

static int
not_cppas_expr()
{
	return ( is_off(do_cppas) && is_on(do_cpp) );
}

static int
cpp_expr()
{
	return is_on(do_cpp);
}

static int
cat_expr()
{
#ifdef sun386
	optim_after_cat = c2_expr();
#endif
	return is_on(do_cat) &&
	       !( ((target_arch.value->value == ARCH_SUN4C) ||
	          (target_arch.value->value == ARCH_SUN4)) &&
	          (optimizer_level > OPTIM_NONE) );
}

static int
inline_expr()
{
	/* Must always run "inline" if handle_misalignment is on. */

	return	( ( is_on(do_inline) && (driver.value != &xlint) )
			||
		  is_on(handle_misalignment) );
}

static int
lint_expr()
{
	return driver.value == &xlint;
}

static int
m4_expr()
{
	return is_on(do_ratfor) && is_on(do_m4);
}

static int
nlint_expr()
{
	return !lint_expr();
}

static int
nsparc_expr()
{
	return (target_arch.value->value != ARCH_SUN4) &&
		(target_arch.value->value != ARCH_SUN4C) &&
		(optimizer_level > OPTIM_NONE);
}

static int
c2_expr()
{
	return (optimizer_level > OPTIM_NONE) &&
	       (driver.value != &xlint) &&
	       (target_arch.value->value != ARCH_SUN4C) &&
	       (target_arch.value->value != ARCH_SUN4);
}

static int
ratfor_expr()
{
	return is_on(do_ratfor);
}


static int
m2c_cat_expr()
{
	return	(set_flag(do_cat));
}


static int
sparc_Soption_expr()
{
	return	is_on(do_cat) &&
		(optimizer_level > OPTIM_NONE) &&
		((target_arch.value->value == ARCH_SUN4C) ||
		(target_arch.value->value == ARCH_SUN4)) &&
		(driver.value != &xlint);
}

static int
nsparc_Soption_expr()
{
	return	!( is_on(do_cat) &&
		   (optimizer_level > OPTIM_NONE) &&
		   ((target_arch.value->value == ARCH_SUN4C)  ||
		   (target_arch.value->value == ARCH_SUN4)) ) &&
		(driver.value != &xlint);
}

static int
do_source_expr()
{
	return  (product.value == &assembler) &&
		(optimizer_level > OPTIM_NONE) &&
		((target_arch.value->value == ARCH_SUN4C) ||
		(target_arch.value->value == ARCH_SUN4));
}

static int
do_object_expr()
{
	return (product.value == &object) || (product.value == &executable);
}

static int
vpa_expr()
{
	return is_on(vpa);
}

#define STEP(prog, suff, expr, setup) \
	{&program.prog, &suffix.suff, (expr), (setup)},

#define END_OF_STEPS() { &program.sentinel_program_field, NULL, NULL, NULL}

#ifdef sun386
Step c_iropt_steps[]= {
	STEP(cpp, i, cpp_expr, setup_cpp_for_cc)
	STEP(ccom, s, nlint_expr, setup_ccom_for_iropt)
	STEP(iropt, none, NULL, NULL) 
        STEP(cg, s, NULL, setup_cg_for_tcov) 
	STEP(inline, s, inline_expr, NULL)
	STEP(c2, s, nsparc_expr, NULL)
	STEP(cat, s, cat_expr, setup_cat_for_cc)
	STEP(as, o, nsparc_Soption_expr, setup_as_for_cc)
	STEP(asS, s, sparc_Soption_expr, setup_asS_for_cc)
	END_OF_STEPS()
};

#else
Step c_iropt_steps[]=
{
	STEP(cpp,      i,    cpp_expr,            setup_cpp_for_cc)
	STEP(ccom,     s,    nlint_expr,          setup_ccom_for_iropt)
	STEP(iropt,    ir,   NULL,                NULL)
	STEP(cg,       s,    NULL,                setup_cg_cc_for_tcov)
	STEP(inline,   s,    inline_expr,         setup_inline)
	STEP(c2,       s,    nsparc_expr,         NULL)
	STEP(cat,      s,    cat_expr,            setup_cat_for_cc)
	STEP(as,       o,    nsparc_Soption_expr, setup_as_for_cc)
	STEP(asS,      s,    sparc_Soption_expr,  setup_asS_for_cc)
	END_OF_STEPS()
};
#endif

Step c_no_iropt_steps[]=
{
	STEP(cpp,      i,    cpp_expr,            setup_cpp_for_cc)
	STEP(ccom,     s,    nlint_expr,          NULL)
	STEP(lint1,    ln,   lint_expr,           NULL)
	STEP(inline,   s,    inline_expr,         setup_inline)
	STEP(c2,       s,    c2_expr,             NULL)
	STEP(as,       o,    nsparc_Soption_expr, setup_as_for_cc)
	STEP(asS,      s,    sparc_Soption_expr,  setup_asS_for_cc)
	END_OF_STEPS()
};

Step def_steps_3x[]=
{
	STEP(m2cfe, sym, NULL, setup_m2cfe_for_3x_def)
	END_OF_STEPS()
};

Step def_steps_non_3x[]=
{
	STEP(m2cfe, sym, NULL, setup_m2cfe_for_non_3x_def)
	END_OF_STEPS()
};


#ifdef sun386 
Step f_iropt_steps[]=
{
	STEP(cpp,      f,    cpp_expr,            NULL)
	STEP(m4,       f,    m4_expr,             NULL)
	STEP(ratfor,   f,    ratfor_expr,         NULL)
	STEP(vpaf77,   none, vpa_expr,            NULL)
	STEP(f77pass1, none, NULL,                NULL)
	STEP(iropt,    none, NULL,                NULL)
	STEP(cg,       s,    NULL,                setup_cg_f77_for_tcov)
	STEP(inline,   s,    inline_expr,         NULL)
	STEP(cat,      s,    cat_expr,            setup_cat_for_f77)
	STEP(c2,       s,    c2_expr,             setup_c2_for_f77)
	STEP(as,       o,    nsparc_Soption_expr, setup_as_for_f77)
	STEP(asS,      s,    sparc_Soption_expr,  setup_asS_for_f77)
	END_OF_STEPS()
};

Step f_no_iropt_steps[]=
{
	STEP(cpp,      f,    cpp_expr,            NULL)
	STEP(m4,       f,    m4_expr,             NULL)
	STEP(ratfor,   f,    ratfor_expr,         NULL)
	STEP(vpaf77,   none, vpa_expr,            NULL)
	STEP(f77pass1, s,    NULL,                setup_f77pass1)
	STEP(inline,   s,    inline_expr,         NULL)
	STEP(cat,      s,    cat_expr,            setup_cat_for_f77)
	STEP(c2,       s,    c2_expr,             setup_c2_for_f77)
	STEP(as,       o,    nsparc_Soption_expr, setup_as_for_f77)
	STEP(asS,      s,    sparc_Soption_expr,  setup_asS_for_f77)
	END_OF_STEPS()
};

#else
Step f_iropt_steps[]=
{
	STEP(cpp,      f,    cpp_expr,            NULL)
	STEP(m4,       f,    m4_expr,             NULL)
	STEP(ratfor,   f,    ratfor_expr,         NULL)
	STEP(vpaf77,   none, vpa_expr,            NULL)
	STEP(f77pass1, none, NULL,                NULL)
	STEP(iropt,    ir,   NULL,                setup_iropt_f77)
	STEP(cg,       s,    NULL,                setup_cg_f77)
	STEP(inline,   s,    inline_expr,         setup_inline)
	STEP(c2,       s,    c2_expr,             NULL)
	STEP(cat,      s,    cat_expr,            setup_cat_for_f77)
	STEP(as,       o,    nsparc_Soption_expr, setup_as_for_f77)
	STEP(asS,      s,    sparc_Soption_expr,  setup_asS_for_f77)
	END_OF_STEPS()
};

Step f_no_iropt_steps[]=
{
	STEP(cpp,      f,    cpp_expr,            NULL)
	STEP(m4,       f,    m4_expr,             NULL)
	STEP(ratfor,   f,    ratfor_expr,         NULL)
	STEP(vpaf77,   none, vpa_expr,            NULL)
	STEP(f77pass1, s,    NULL,                NULL)
	STEP(inline,   s,    inline_expr,         setup_inline)
	STEP(c2,       s,    c2_expr,             NULL)
	STEP(cat,      s,    cat_expr,            setup_cat_for_f77)
	STEP(as,       o,    nsparc_Soption_expr, setup_as_for_f77)
	STEP(asS,      s,    sparc_Soption_expr,  setup_asS_for_f77)
	END_OF_STEPS()
};
#endif

Step link_steps[]=
{
	STEP(ld,       none, NULL,                NULL)
	STEP(vpald,    none, vpa_expr,            NULL)
	END_OF_STEPS()
};

Step link_m2c_steps[]=
{
	STEP(m2l,      none, NULL,                NULL)
	END_OF_STEPS()
};

Step lint_steps[]=
{
	STEP(cat,      none, NULL,                setup_cat_for_lint)
	STEP(lint2,    none, NULL,                NULL)
	END_OF_STEPS()
};

Step mod_rel3x_steps[]=       /* for release 3.x modula2 */
{
	STEP(m2cfe,    none, NULL,                setup_m2cfe_for_3x_mod)
	STEP(mf1,      s,    NULL,                NULL)
	STEP(inline,   s,    inline_expr,         setup_inline)
	STEP(c2,       s,    c2_expr,             NULL)
	STEP(asS,      s,    do_source_expr,      setup_asS_for_m2c)
	STEP(as,       o,    do_object_expr,      NULL)
	END_OF_STEPS()
};

Step mod_no_iropt_steps[]=	/* for release 4.x modula2 */
{
	STEP(m2cfe,    ir,   NULL,                setup_m2cfe_for_non_3x_mod)
	STEP(cg,       s,    NULL,                setup_cg_mod)
	STEP(inline,   s,    inline_expr,         setup_inline) 
	STEP(c2,       s,    c2_expr,             NULL)
	STEP(cat,      s,    m2c_cat_expr,        setup_cat_for_mod)
	STEP(as,       o,    do_object_expr,      setup_as_for_m2c) 
	STEP(asS,      s,    sparc_Soption_expr,  setup_asS_for_m2c) 
	END_OF_STEPS()
};

Step mod_iropt_steps[]=		/* for release 4.x modula2 */
{
	STEP(m2cfe,    ir,   NULL,                setup_m2cfe_for_non_3x_mod)
	STEP(iropt,    ir,   NULL,                setup_iropt_mod)
	STEP(cg,       s,    NULL,                setup_cg_mod)
	STEP(inline,   s,    inline_expr,         setup_inline)
	STEP(c2,       s,    c2_expr,             NULL)
	STEP(cat,      s,    m2c_cat_expr,        setup_cat_for_mod)
	STEP(as,       o,    do_object_expr,      setup_as_for_m2c)
	STEP(asS,      s,    sparc_Soption_expr,  setup_asS_for_m2c) 
	END_OF_STEPS()
};


Step p_rel3x_steps[]=		/* for release 3.x Pascal */
{
	STEP(cpp,      pi,   cpp_expr,            NULL)
	STEP(pc0,      none, NULL,                setup_pc0_for_3x)
	STEP(f1,       s,    NULL,                NULL)
	STEP(inline,   s,    NULL,                setup_inline_for_pc)
	STEP(c2,       s,    c2_expr,             NULL)
	STEP(asS,      s,    do_source_expr,      setup_asS_for_pc)
	STEP(as,       o,    do_object_expr,      NULL)
	END_OF_STEPS()
};

#ifdef sun386
Step p_no_iropt_steps[]= 
{
	STEP(cppas,     pi,     cppas_expr,	NULL)
	STEP(cpp,       pi,     not_cppas_expr,	NULL)
	STEP(pc0, 	ir,	NULL, 		setup_pc0_for_non_3x)
	STEP(cg, 	s, 	NULL,		setup_cg_for_tcov)
	STEP(inline,	s,	NULL,		setup_inline_for_pc)
	STEP(cat, 	s,	cat_expr, 	setup_cat_for_pc)
	STEP(c2, 	s, 	c2_expr, 	NULL)
	STEP(asS, 	s, 	do_source_expr, setup_asS_for_pc)
	STEP(as, 	o, 	do_object_expr, NULL)
	END_OF_STEPS()
};

Step p_iropt_steps[]= 
{
	STEP(cppas,     pi,     cppas_expr,	NULL)
	STEP(cpp,       pi,     not_cppas_expr,	NULL)
	STEP(pc0, 	ir, 	NULL, 		setup_pc0_for_non_3x)
	STEP(iropt, 	none, 	NULL, 		NULL)
	STEP(cg, 	s, 	NULL, 		setup_cg_for_tcov)
	STEP(inline, 	s, 	NULL, 		setup_inline_for_pc)
	STEP(cat, 	s, 	cat_expr, 	setup_cat_for_pc)
	STEP(c2, 	s, 	c2_expr, 	NULL)
	STEP(asS, 	s, 	do_source_expr, setup_asS_for_pc)
	STEP(as, 	o, 	do_object_expr, NULL)
	END_OF_STEPS()
};

#else /* !sun386 */
Step p_no_iropt_steps[]=	/* for release 4.x Pascal */
{
	STEP(cppas,    pi,   cppas_expr,	NULL)
	STEP(cpp,      pi,   not_cppas_expr,	NULL)
	STEP(pc0,      none, NULL,                setup_pc0_for_non_3x)
	STEP(cg,       s,    NULL,                setup_cg_pc)
	STEP(inline,   s,    NULL,                setup_inline_for_pc)
	STEP(c2,       s,    nsparc_expr,         NULL)
	STEP(cat,      s,    cat_expr,            setup_cat_for_pc)
	STEP(asS,      s,    do_source_expr,      setup_asS_for_pc)
	STEP(as,       o,    do_object_expr,      setup_as_for_pc)
	END_OF_STEPS()
};

Step p_iropt_steps[]=		/* for release 4.x Pascal */
{
	STEP(cppas,    pi,   cppas_expr,	NULL)
	STEP(cpp,      pi,   not_cppas_expr,	NULL)
	STEP(pc0,      none, NULL,                setup_pc0_for_non_3x)
	STEP(iropt,    ir,   NULL,                setup_iropt_pc)
	STEP(cg,       s,    NULL,                setup_cg_pc)
	STEP(inline,   s,    NULL,                setup_inline_for_pc)
	STEP(c2,       s,    nsparc_expr,         NULL)
	STEP(cat,      s,    cat_expr,            setup_cat_for_pc)
	STEP(as,       o,    nsparc_Soption_expr, setup_as_for_pc)
	STEP(asS,      s,    sparc_Soption_expr,  setup_asS_for_pc)
	END_OF_STEPS()
};
#endif

Step pc_steps[]=
{
	STEP(pc3,    none, NULL,                NULL)
	STEP(ld,     none, NULL,                NULL)
	END_OF_STEPS()
};

Step s_steps[]=
{
	STEP(cpp,    s,    cpp_expr,            NULL)
	STEP(as,     o,    NULL,                NULL)
	END_OF_STEPS()
};

	/* The following is used for pushing anything, no matter what its
	 * suffix, through "cc -E".
	 */
Step anything_thru_cc_E_steps[]=
{
	STEP(cpp,    s,    cpp_expr,            NULL)
	END_OF_STEPS()
};

	/* The following is used for pushing anything, no matter what its
	 * suffix, through "cc -P".
	 */
Step anything_thru_cc_P_steps[]=
{
	STEP(cpp,    s,    cpp_expr,           setup_cpp_for_cc)
	END_OF_STEPS()
};

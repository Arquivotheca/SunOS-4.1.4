/* @(#)driver.h 1.77 88/11/19 SMI */
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/param.h>
#include <report.h>
#include <vroot.h>

#ifdef sparc
#include <alloca.h>
#endif

typedef unsigned char Bool;
#define FALSE       0
#define TRUE        1
#define UNINIT_BOOL 2

#define STR_EQUAL(a,b)		(strcmp((a),(b)) == 0)
#define STR_EQUAL_N(a,b,n)	(strncmp((a),(b),(n)) == 0)

/*
 *	Const_int defines the integer constant values that Named_int can assume.
 *
 *	The "name" field is used when printing error messages.
 *
 *	The "extra" field holds an optional value associated with the int
 *	constant, to wit:
 *
 *	Type of	
 *	Const_int		Use of "extra"
 *	--------------		-----------------------------------------------
 *	float_mode		the run time startup routine for the mode
 *	profile			the run time startup routine for the mode
 *	target_sw_release[R_VROOT]:
 *				the value (string) to be used in the VROOT path
 *	system architecture
 *				the name of the architecture
 *	drivers			the name of the product to which they belong
 */
typedef struct
{
	int		value;
	char		*name;
	char		*extra;
} Const_int, *Const_intP;

/*
 *	Named_int cells are used to hold slightly complex integer values.
 *	The value field is always a reference to a Const_int.
 *	The help field is used when printing error messages.
 */
typedef struct
{
	Const_intP	value;
	char		*help;
	Bool		touched    :1;
	Bool		redefine_ok:1;
} Named_int, *Named_intP;

/*
 *	The Suffix struct holds information about one suffix the driver knows
 *	about.	Fields:
 *		suffix		Points to the plain suffix string (w/o the ".")
 *		in_drivers	2 sets (bitmasks) that define which drivers
 *		out_drivers		consume/produce files with this suffix
 *					(DRIVER_* values).
 *		compile		Function that compiles files with this suffix.
 *		collect		Function that collects files with this suffix
 *					for postprocessing.
 *		help		String describing the suffix.
 */
typedef struct
{
	char		*suffix;
	short int	in_drivers;
	short int	out_drivers;
	int		(*compile)();
	void		(*collect)();
	char		*help;
} Suffix, *SuffixP;

/*
 *	There is one variable of the type Suffixes.
 *	It contains one entry for each suffix that the driver
 *	is prepared to handle. The individual suffix cells
 *	are described above.
 */
typedef struct
{
	Suffix		a;
	Suffix		c;
	Suffix		def;
	Suffix		f;
	Suffix		F;
	Suffix		il;
	Suffix		i;
	Suffix		ln;
	Suffix		mod;
	Suffix		o;
	Suffix		pas;
	Suffix		pi;
	Suffix		p;
	Suffix		r;
	Suffix		sym;
	Suffix		s;
	Suffix		so;
	Suffix		S;
	Suffix		ir;
	Suffix		none;
	Suffix		sentinel_suffix_field;
} Suffixes;

/*
 *	The List struct is used for building linked lists of strings.
 *	It is possible to associate a suffix with each string.
 */
typedef struct List
{
	char		*value;
	SuffixP		suffix;
	struct List	*next;
} List, *ListP;


typedef unsigned char Template;
/* Values for the Template (program argument template) */
#define IN_TEMPLATE		1
#define OUT_TEMPLATE		2
#define OPTION_TEMPLATE		3
#define STDOUT_TEMPLATE		4
#define STDIN_TEMPLATE		5    /* must be the 1st template,if it is used*/
#define MINUS_O_TEMPLATE	6
#define MINUS_TEMPLATE		7
#define OUT2_TEMPLATE		8

/*
 *	The Program struct contains information about one program
 *	that the driver is prepared to invoke.
 *	Fields:
 *		name		Short name of program
 *		path		Path to the program
 *		drivers		bitmask describing which drivers use this prog
 *		setup		Function called to prepare for run of program
 *					(determined dynamically)
 *		permanent_options Options used for all runs of program
 *		trailing_options  Options used for all runs, but specified on
 *					cmd line with -Qoption, therefore need
 *					to be on the end of the option list.
 *		options		list of options with which program will be run
 *		infile		List of infiles for program (dynamic)
 *		outfile		The file produced by the program
 *		outfile2	A second file produced by the program
 *		has_been_initialized
 *				Flags if permanent_options has been set yet
 *		template	Vector that describes the order in which
 *				options should be passed to the program
 *	The lists options and infile and the value outfile are set for each
 *	run of the program by the setup functions.
 */
#define PGM_TEMPLATE_LENGTH 5
typedef struct
{
	char		*name;
	char		*path;
	short int	drivers;
	char		*(*setup)();
	ListP		permanent_options;
	ListP		trailing_options;
	ListP		options;
	ListP		infile;
	char		*outfile;
	char		*outfile2;
	Bool		has_been_initialized :1;
	Template	template[PGM_TEMPLATE_LENGTH];
} Program, *ProgramP;

/*
 *	There is one variabe of the type Programs.
 *	Each program that the driver is prepared to invoke is described here.
 */
typedef struct
{
	Program		cppas;
	Program		cpp;
	Program		m4;
	Program		ratfor;
	Program		lint1;
	Program		cat;
	Program		lint2;
	Program		m2cfe;
	Program		ccom;
	Program		pc0;
	Program		f1;
	Program		mf1;
	Program		vpaf77;
	Program		f77pass1;
	Program		iropt;
	Program		cg;
	Program		inline;
	Program		c2;
	Program		as;
	Program		asS;
	Program		pc3;
	Program		ld;
	Program		vpald;
	Program		m2l;
	Program		sentinel_program_field;
} Programs;

/*
 *	The Step struct is used to build vectors that describes
 *	which programs should run in what order for one particular compile
 *	Fields:
 *		program		References the program to run
 *		out_suffix	The suffix of the file produced
 *		expression	Expression that determines if step is relevant
 *		setup		Function that performs special setup for step
 *		start		Time program started. Used when reporting
 *				runtime.
 *		process		Process id of process running program
 *		killed		Indicated if the process has been killed
 */
typedef struct
{
	ProgramP	program;
	SuffixP		out_suffix;
	int		(*expression)();
	char		*(*setup)();
	struct timeval	start;
	short int	process;
	Bool		killed	:1;
} Step, *StepP;

/*
 *	This enum defines the special actions that can
 *	be used for processing command line options
 *	after they have been recognized.
 */
typedef enum
{
	end_of_list= 1,		/* The last option in the list of options */
	help_option,		/* Show help message */
	infile_option,		/* This option is an infile */
	lint1_option,		/* pass to lint1 */
	lint_i_option,		/* Special check for lint -n & -i to make sure*/
	lint_n_option,		/*	they do not have more options trailing*/
	make_lint_lib_option,	/* Make lint library option */
	module_option,		/* Force load modula module */
	module_list_option,	/* m2c search path option */
	optimize_option,	/* -O/-P options */
	outfile_option,		/* next arg is the outfile */
	pass_on_lint_option,	/* Pass to lint1 & lint2 */
	pass_on_select_option,	/* -Qoption handler */
	pass_on_1_option,	/* -x		=> prog -x */
	pass_on_1t12_1t_option,	/* -xREST	=> prog -xREST */
				/* [or] -x REST => prog -xREST */
	pass_on_1t12_1t_option_pc,
				/* -config REST => prog -configREST */
				/* [or] -configREST => prog -configREST */
	pass_on_1t_option,	/* -xREST	=> prog -xREST */
	pass_on_12_option,	/* -x REST	=> prog -x REST */
	pass_on_1to_option,	/* -xREST	=> prog REST */
	produce_option,		/* -Qproduce handler */
	path_option,		/* -Qpath handler */
	run_m2l_option,		/* m2c -e handler */
	load_m2l_option,	/* m2c -E handler */
	set_int_arg_option,	/* Handle simple boolean options */
	set_named_int_arg_option, /* Handle multiple choice options */
	set_target_arch_option1,/* -{TARGET}		(set target_arch) */
	set_target_arch_option2,/* -target {TARGET}	(set target_arch) */
	set_target_proc_option1,/* -{PROCESSOR}		(set target processor
				 *			 type & target_arch)
				 */
	set_sw_release_option,	/* next arg is the target S/W release */
	temp_dir_option		/* Handle -temp option */
} Options;

/*
 *	The Option struct describes one legal option for the driver.
 *
 *	Fields:
 *		name		String with the option expected as an argument
 *		drivers		Bitmask (of Z_* values) describing drivers
 *					for which option is legal.
 *		type		Of option. Determines what action is taken when
 *					option is seen.
 *		subtype		Extra value, for use in decoding option.
 *		variable	Associated Named_int
 *		program		Program to pass option to
 *		constant	Associated Const_int
 *		help		String describing option
 */
typedef struct
{
	char		*name;
	int		drivers :16;
	Options		type    :8;
	int		subtype :8;
	Named_intP	variable;
	ProgramP	program;
	Const_intP	constant;
	char		*help;
} Option, *OptionP;

/*
 *	Various global binary flags; listed in alphabetical order.
 */
typedef struct
{
	Bool		as_R;
	Bool		checkC;
	Bool		checkH;
	Bool		checkV;
#ifdef BROWSER
	Bool		code_browser;
	Bool		code_browser_seen;
#endif
	Bool		debug;		/* (for internal debugging) */
	Bool		do_cppas;
	Bool		do_cpp;
	Bool		do_dependency;
	Bool		do_cat;
	Bool		do_inline;
	Bool		do_m2l;
	Bool		do_m4;
	Bool		do_ratfor;
	Bool		doing_mod_file;
	Bool		doubleword_aligned_doubles;
	Bool		dryrun;
	Bool		fstore;
	Bool		generate_nonzero_activation_records;
	Bool		handle_misalignment;
	Bool		ignore_lc;
	Bool		junk;
	Bool		long_offset;
	Bool		no_c2;
	Bool		no_default_module_list;
	Bool		onetrip;
	Bool		pic_code;
	Bool		PIC_code;
	Bool		pipe_ok;
	Bool		pure_cross;
	Bool		remove_tmp_files;
	Bool		root_module_seen;
	Bool		show_rm_commands;	/* OK to echo "rm" cmds */
	Bool		statement_count;
	Bool		sparc_sdata;
	Bool            sys5_flag;              /* For defining sys5 setup */
	Bool		testing_driver;		/* For testing the driver */
	Bool		time_run;
	Bool		trace;
	Bool		verbose;
	Bool		vpa;
	Bool		warning;
} Global_flag;

#define FLOAT_OPTION "FLOAT_OPTION"
#define MAX_HELP_PREFIX_WIDTH 10
#define HELP_STRING_INDENT_DEPTH 21
#define MAX_NUMBER_OF_DRIVER_SUPPLIED_ARGS 32
#define LOCAL_STRING_LENGTH 1000
#define DUMMY_FLAG "/usr/lib"
#define DUMMY_LENGTH 8
#define XL_FLAG "-xl"

#define set_named_int(i, v) { (i).value= (v); (i).touched= TRUE;}
#define append_list(list, value) \
		append_list_with_suffix(list, value, (SuffixP)NULL)

#define uninitialize_flag(flag)	global_flag.flag= UNINIT_BOOL
#define set_flag(flag)		global_flag.flag= TRUE
#define reset_flag(flag)	global_flag.flag= FALSE
#define is_initialized(flag)	(global_flag.flag != UNINIT_BOOL)
#define is_uninitialized(flag)	(!is_initialized(flag))
#ifdef DEBUG
#  define chk_uninit(flag)	( is_initialized(flag) || \
				  (fatal("uninit flag"),FALSE) )
#  define is_off(flag)		(chk_uninit(flag),(global_flag.flag == FALSE))
#  define is_on(flag)		(chk_uninit(flag),(global_flag.flag == TRUE ))
#else /*!DEBUG*/
#  define is_off(flag)		(global_flag.flag == FALSE)
#  define is_on(flag)		(global_flag.flag == TRUE )
#endif /*DEBUG*/

/*
 *	The following macros are used to set bitmask fields
 */
#define HIDE_OPTION	0x0100		/* Do not show option (for -help
					 * option)
					 */
#define NO_MINUS_OPTION	0x0200		/* Option takes extra arg that must not
					 * start with "-"
					 */
#define SOURCE_SUFFIX	0x0400		/* Suffix is suffixes that should be
					 * traced when compiling
					 */
#define HIDE(x) ((x) | HIDE_OPTION)
#define NO_MINUS(x) ((x) | NO_MINUS_OPTION)
#define SOURCE(x) ((x) | SOURCE_SUFFIX)

/* Basic values for the different drivers */
#define DRIVER_none	0x00
#define DRIVER_C	0x01			/* cc */
#define DRIVER_F	0x02			/* f77 */
#define DRIVER_L	0x04			/* lint */
#define DRIVER_M	0x08			/* m2c */
#define DRIVER_P	0x10			/* pc */
/* Combinations of the above */
#define DRIVER_all	(DRIVER_C|DRIVER_F|DRIVER_L|DRIVER_M|DRIVER_P)
#define DRIVER_CF	(DRIVER_C|DRIVER_F)
#define DRIVER_CFLM	(DRIVER_C|DRIVER_F|DRIVER_L|DRIVER_M)
#define DRIVER_CFLMP	(DRIVER_C|DRIVER_F|DRIVER_L|DRIVER_M|DRIVER_P)
#define DRIVER_CFLP	(DRIVER_C|DRIVER_F|DRIVER_L|DRIVER_P)
#define DRIVER_CFM	(DRIVER_C|DRIVER_F|DRIVER_M)
#define DRIVER_CFMP	(DRIVER_C|DRIVER_F|DRIVER_M|DRIVER_P)
#define DRIVER_CFP	(DRIVER_C|DRIVER_F|DRIVER_P)
#define DRIVER_CL	(DRIVER_C|DRIVER_L)
#define DRIVER_CLP	(DRIVER_C|DRIVER_L|DRIVER_P)
#define DRIVER_CMP	(DRIVER_C|DRIVER_M|DRIVER_P)
#define DRIVER_CP	(DRIVER_C|DRIVER_P)
#define DRIVER_FL	(DRIVER_F|DRIVER_L)
#define DRIVER_FMP	(DRIVER_F|DRIVER_M|DRIVER_P)
#define DRIVER_LP	(DRIVER_L|DRIVER_P)
#define DRIVER_MP	(DRIVER_M|DRIVER_P)

#define GOAL_EXE		1
#define GOAL_I			2
#define GOAL_L			3
#define GOAL_O			4
#define GOAL_S			5

#define DEBUG_ADB		1
#define DEBUG_DBX		2

#define PROF_NONE		1
#define PROF_OLD		2
#define PROF_NEW		3

#define ARCH_FOREIGN		1	/* Foreign (unknown) architecture. */
#define ARCH_SUN2		20	/* Sun-2 */
#define ARCH_SUN3		30	/* Sun-3 */
#define ARCH_SUN3X		31	/* Sun-3x */
#define ARCH_SUN4		40	/* Sun-4 */
#define ARCH_SUN4C		41	/* Sun-4c */
#define ARCH_SUN386		386	/* Sun-386 */

/* SW_REL_* values after SW_REL_DFLT are assumed to be in ascending order,
 * chronologically by release.  (The absolute values don't matter, though)
 */
#define SW_REL_DFLT		1	/* default */
#define SW_REL_3X		30	/* "3.x": 3.2, 3.4, 3.5, etc. */
#define SW_REL_40		40	/* 4.0 */
#define SW_REL_41		41	/* 4.1 */

/* DRIVER_REL version numbers 
 * this is used when we need to look up a specific
 * driver version number, instead of the OS release number
 * as above
 */
#define DRIVER_REL_10 		"1.0"
#define DRIVER_REL_11 		"1.1"
#define DRIVER_REL_12 		"1.2"
#define DRIVER_REL_20 		"2.0"
#define DRIVER_REL_40 		"4.0"

#define FLOAT_68881		10
#define FLOAT_FPA		20
#define FLOAT_SKY		30
#define FLOAT_SOFT		40
#define FLOAT_SWITCH		50

/* The driver assumes that the following values are ordered according to
 * increasing numerical value.
 */
typedef char OptimLevel;
#define OPTIM_NONE		 '0'
#define OPTIM_C2		 '1'
#define OPTIM_IROPT_P		 '2'
#define OPTIM_IROPT_O		 '3'
#define OPTIM_IROPT_O_TRACK_PTRS '4'

/* Used as subtypes for options:
 */
#define PASS_OPT_AS_FLAG   0
#define PASS_OPT_AS_INFILE 1

/* Library routine declarations */
extern	char			*alloca();
extern	char			**environ;
extern	int			errno;
extern	char			*getenv();
extern	char			*getwd();
extern	char			*index();
extern	char			*malloc();
extern	char			*rindex();
extern	char			*sprintf();
extern	char			*strcat();
extern	char			*strcpy();
extern	char			*strncpy();
extern 	char			*sys_siglist[];

/* from ro_data.c & rw_data.c */
extern	Option			options[];
extern	Option			drivers[];
extern	Step			c_iropt_steps[];
extern	Step			c_no_iropt_steps[];
extern	Step			def_steps_3x[];
extern	Step			def_steps_non_3x[];
extern	Step			f_iropt_steps[];
extern	Step			f_no_iropt_steps[];
extern	Step			link_steps[];
extern	Step			link_m2c_steps[];
extern	Step			lint_steps[];
extern	Step			mod_rel3x_steps[];
extern	Step			mod_iropt_steps[];
extern	Step			mod_no_iropt_steps[];
extern	Step			p_rel3x_steps[];   /* for release 3.x Pascal */
extern	Step			p_no_iropt_steps[];/* for release 4.x Pascal */
extern	Step			p_iropt_steps[];   /* for release 4.x Pascal */
extern	Step			pc_steps[];
extern	Step			s_steps[];
extern	Step			anything_thru_cc_E_steps[];
extern	Step			anything_thru_cc_P_steps[];
extern	Const_intP		known_sw_releases[];
extern	Const_intP		known_architectures[];

/* from compile.c */
extern	void			append_list_with_suffix();
extern 	int			check_release_version_driver();
extern	void			cleanup();
extern  void			define_arch_and_sw_release();
extern	void			fatal();
extern	char			*get_file_suffix();
extern	char			*get_memory();
extern	char			*get_processor_type();
extern	char			*get_processor_flag();
extern	char			*lint_lib();
extern	char			*make_string();
extern	Const_intP		sw_release_lookup();
extern	void			warning();

/* from driver_versions.c */
extern	Const_intP		get_target_base_OS_version();
extern	char			*get_base_driver_version();

/* from run_pass.c */
extern	char			*outfile_name();
extern	char			*temp_file_name();
extern	int			run_steps();

/* from setup*.c */
extern	void			cc_doit();
extern	void			clear_program_options();
extern	void			collect_ln();
extern	void			collect_o();
extern	int			compile_c();
extern	int			compile_def();
extern	int			compile_f();
extern	int			compile_F();
extern	int			compile_i();
extern	int			compile_l();
extern	int			compile_mod();
extern	int			compile_r();
extern	int			compile_p();
extern	int			compile_pi();
extern	int			compile_S();
extern	int			compile_s();
extern	void			do_infiles();
extern	void			do_not_unlink_ld_infiles();
extern	void			f1_to_mf1_copy();
extern	void			f77_doit();
extern	char			*get_crt0_name();
extern	void			lint_doit();
extern	void			m2c_doit();
extern	void			pc_doit();
extern	char			*scan_Qpath_and_vroot();
extern	char			*scan_Qpath_only();
extern	void			setup_tcov_file();
extern	char			*setup_as();
extern	char			*setup_as_for_cc();
extern	char			*setup_as_for_f77();
extern	char			*setup_as_for_m2c();
extern	char			*setup_as_for_pc();
extern	void			setup_asS();
extern	char			*setup_asS_for_cc();
extern	char			*setup_asS_for_f77();
extern	char			*setup_asS_for_m2c();
extern	char			*setup_asS_for_pc();
extern	char			*setup_c2();
extern	char			*setup_c2_for_f77();
extern	char			*setup_ccom();
extern	char			*setup_ccom_for_iropt();
extern	char			*setup_cat_for_cc();
extern	char			*setup_cat_for_mod();
extern	char			*setup_cat_for_pc();
extern	char			*setup_cat_for_f77();
extern	char			*setup_cat_for_lint();
extern	char			*setup_cg_all();
extern	char			*setup_cg_f77();
extern	char			*setup_cg_mod();
extern	char			*setup_cg_pc();
extern	char			*setup_cg_for_tcov();
extern	char			*setup_cg_cc_for_tcov();
extern	char			*setup_cg_f77_for_tcov();
extern  char                    *setup_cppas();
extern	char			*setup_cpp();
extern	char			*setup_cpp_for_cc();
extern	char			*setup_f1();
extern	char			*setup_f77pass1();
extern	char			*setup_inline();
extern	char			*setup_inline_for_pc();
extern	char			*setup_iropt_all();
extern	char			*setup_iropt_f77();
extern	char			*setup_iropt_mod();
extern	char			*setup_iropt_pc();
extern	char			*setup_ld();
extern	char			*setup_ld_for_cc();
extern	char			*setup_ld_for_f77();
extern	char			*setup_ld_for_pc();
extern	char			*setup_link_step();
extern	char			*setup_lint_l_special();
extern	char			*setup_m2cfe();
extern	char			*setup_m2cfe_for_3x_def();
extern	char			*setup_m2cfe_for_non_3x_def();
extern	char			*setup_m2cfe_for_3x_mod();
extern	char			*setup_m2cfe_for_non_3x_mod();
extern	char			*setup_m2l_for_m2c();
extern	char			*setup_mf1();
extern	char			*setup_pc0();
extern	char			*setup_pc0_for_3x();
extern	char			*setup_pc0_for_non_3x();
extern	char			*setup_pc3();
extern	char			*setup_vpald();
extern	void			set_requested_suffix();

/* global variables */
/* First, all the named_ints and the Const_ints they reference */
extern	Named_int	debugger;
extern	Const_int	adb;
extern	Const_int	dbx;

extern	Named_int	driver;
extern	Const_int	cc;
extern	Const_int	dummy;
extern	Const_int	f77;
extern	Const_int	m2c;
extern	Const_int	pc;
extern	Const_int	xlint;

extern	Named_int	float_mode;
extern	Const_int	ffpa;
extern	Const_int	fsky;
extern	Const_int	fsoft;
extern	Const_int	fswitch;
extern	Const_int	f68881;

extern	Named_int	host_arch;
extern	Named_int	target_arch;
extern	Const_int	arch_sun2;
extern	Const_int	arch_sun386;
extern	Const_int	arch_sun3x;
extern	Const_int	arch_sun3;
extern	Const_int	arch_sun4c;
extern	Const_int	arch_sun4;
extern	Const_int	arch_foreign;

			/* Note that if the following "R_" constants are
			 * modified, the corresponding initializations of
			 * target_sw_release[] (in rw_data.c) must be modified.
			 */
#define R_VROOT    0
#define R_PATHS    1
#define R_PASSES   2
#define R_elements 3	/* number of elements */
			/* R_all is used when referring to all R_elements 
			 * elements of target_sw_release[], as a group.
			 */
#define R_all	   R_elements
extern	Named_int	target_sw_release[R_elements];
extern	Const_int	sw_release_default;		/* default */
extern	Const_int	sw_release_3x_explicit;		/* 3.x */
extern	Const_int	sw_release_40_explicit;		/* 4.0 */
extern	Const_int	sw_release_41_explicit;		/* 4.1 */
extern	Const_int	sw_release_3x_implicit;		/* 3.x */
extern	Const_int	sw_release_40_implicit;		/* 4.0 */
extern	Const_int	sw_release_41_implicit;		/* 4.1 */

extern	Named_int	product;
extern	Const_int	lint1_file;
extern	Const_int	preprocessed;	/* -E output, to stdout */
extern	Const_int	preprocessed2;	/* -P output, to file.i */
extern	Const_int	object;
extern	Const_int	assembler;
extern	Const_int	executable;

extern	Named_int	profile;
extern	Const_int	gprof;
extern	Const_int	no_prof;
extern	Const_int	prof;

extern	char		**argv_for_passes;
extern	char		*base_driver_release;
#ifdef BROWSER
extern	char		*browser_options;
#endif
extern  char            *debug_info_filename;
extern	int		exit_status;
extern	ListP		files_to_unlink;
extern	Global_flag	global_flag;
extern	ListP		infile;
extern	int		infile_count;
extern	ListP		infile_ln;
extern	ListP		infile_o;
extern	char		*iropt_files[3];
extern	int		last_program_path;
extern	char		*ld_to_vpa_file;
extern	int		make_lint_lib_fd;
extern	ListP		module_list;
extern	int		optim_after_cat;
extern	OptimLevel	optimizer_level;
extern	char		*outfile;
extern	Programs	program;
extern	char		*program_name;
extern	pathpt		program_path;
extern	ListP		report;
extern	SuffixP		requested_suffix;
extern  char		*to_as_file;
extern	int		source_infile_count;
extern	Suffixes	suffix;
extern	char		*tcov_file;
extern	char		*temp_dir;
extern	int		temp_file_number;
extern	ListP		temp_files_named;
extern	int		use_default_optimizer_level;

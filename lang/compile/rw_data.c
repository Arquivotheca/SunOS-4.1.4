#ifndef lint
static	char sccsid[] = "@(#)rw_data.c 1.1 94/10/31 SMI";
#endif
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "driver.h"

	/* The following crt0.o data must be R/W, in order to handle both 3.2
	 * and 4.0 source organizations.
	 */
	Const_int	f68881=	{ FLOAT_68881, "-f68881", NULL };
	Const_int	ffpa=	{ FLOAT_FPA,   "-ffpa",   NULL };
	Const_int	fsky=	{ FLOAT_SKY,   "-fsky",   NULL };
	Const_int	fsoft=	{ FLOAT_SOFT,  "-fsoft",  NULL };

	Const_int	gprof=	{ PROF_NEW,    "gprof",  NULL };
	Const_int	no_prof={ PROF_NONE,   NULL,     NULL  };
	Const_int	prof=	{ PROF_OLD,    "prof",   NULL };

	Const_int	arch_foreign=	{ ARCH_FOREIGN, NULL, NULL };

	Named_int	debugger=	{ NULL, "debug", 0, 0 };
	Named_int	driver=		{ NULL, "", 0, 0 };
	Named_int	float_mode=	{ NULL, "floating point option", 0, 0 };
	Named_int	host_arch=	{ NULL, "host architecture", 0, 0 };
	Named_int	product=	{ &executable, "produce", 0, 0 };
	Named_int	profile=	{ &no_prof, "profile", 0, 0 };
	Named_int	target_arch=	{ NULL, "target architecture", 0, 0 };
	Named_int	target_sw_release[R_elements]=
					{ { &sw_release_default,
						"target software release[0]",
						0, 0 },
					  { &sw_release_default,
						"target software release[1]",
						0, 0 },
					  { &sw_release_default,
						"target software release[2]",
						0, 0 }
					};

	char		**argv_for_passes;
	char		*base_driver_release;
	char		*debug_info_filename;
	int		exit_status;
	ListP		files_to_unlink;
	Global_flag	global_flag;
	ListP		infile;
	int		infile_count;
	ListP		infile_ln;
	ListP		infile_o;
	char		*iropt_files[3];
	int		last_program_path;
	char		*ld_to_vpa_file;
	int		make_lint_lib_fd;
	ListP		module_list;
	int		optim_after_cat;
	char		optimizer_level= OPTIM_NONE;
	char		*outfile;
	char		*program_name= "compile";
	pathpt		program_path;
	ListP		report;
	SuffixP		requested_suffix;
	int		source_infile_count;
	char		*tcov_file;
	char		*temp_dir= "/tmp";
	int		temp_file_number;
	ListP		temp_files_named;
	char		*to_as_file;
	int		use_default_optimizer_level= 0;

#ifdef lint
	char            *sys_siglist[1];
	char            **environ;
#endif lint


Programs program=
{
	{ "cppas", "/usr/ucb/cppas", DRIVER_P, setup_cppas, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ OPTION_TEMPLATE, STDOUT_TEMPLATE, IN_TEMPLATE } },
	{ "cpp", "/lib/cpp", DRIVER_CFLP, setup_cpp, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ STDOUT_TEMPLATE, OPTION_TEMPLATE, IN_TEMPLATE } },
	{ "m4", "/usr/bin/m4", DRIVER_F, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, 0,
		{ STDIN_TEMPLATE, STDOUT_TEMPLATE, OPTION_TEMPLATE } },
	{ "ratfor", "/usr/bin/ratfor", DRIVER_F, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ STDIN_TEMPLATE, STDOUT_TEMPLATE, OPTION_TEMPLATE } },
	{ "lint1", "/usr/lib/lint/lint1", DRIVER_L, NULL, NULL, NULL,NULL,NULL,NULL,NULL,0,
		{ STDIN_TEMPLATE, STDOUT_TEMPLATE, OPTION_TEMPLATE } },
	{ "cat", "/bin/cat", HIDE(DRIVER_FL), NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ STDOUT_TEMPLATE, OPTION_TEMPLATE, IN_TEMPLATE } },
	{ "lint2", "/usr/lib/lint/lint2", DRIVER_L, NULL, NULL, NULL,NULL,NULL,NULL,NULL,0,
		{ IN_TEMPLATE, OPTION_TEMPLATE } },
	{ "m2cfe", "/usr/lib/modula2/m2cfe", DRIVER_M, setup_m2cfe, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ OPTION_TEMPLATE, IN_TEMPLATE, OUT_TEMPLATE, OUT2_TEMPLATE}},
	{ "ccom", "/lib/ccom", DRIVER_C, setup_ccom, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ STDIN_TEMPLATE, STDOUT_TEMPLATE, MINUS_TEMPLATE, OPTION_TEMPLATE } },
	{ "pc0", "/usr/lib/pc0", DRIVER_P, setup_pc0, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ OUT_TEMPLATE, MINUS_O_TEMPLATE, OPTION_TEMPLATE, IN_TEMPLATE} },
	{ "f1", "/usr/lib/f1", DRIVER_P, setup_f1, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ STDOUT_TEMPLATE, OPTION_TEMPLATE, IN_TEMPLATE } },
	{ "mf1", "/usr/lib/modula2/f1", DRIVER_M, setup_mf1, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ STDOUT_TEMPLATE, OPTION_TEMPLATE, IN_TEMPLATE } },
	{ "vpaf77", "/usr/lib/vpaf77", DRIVER_F, NULL, NULL, NULL, NULL, NULL, NULL,NULL,0,
		{ STDIN_TEMPLATE, STDOUT_TEMPLATE, OPTION_TEMPLATE } },
	{ "f77pass1", "/usr/lib/f77pass1", DRIVER_F, setup_f77pass1, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ OPTION_TEMPLATE, IN_TEMPLATE, OUT_TEMPLATE } },

	{ "iropt", "/usr/lib/iropt", DRIVER_CFMP, setup_iropt_all, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ OPTION_TEMPLATE, MINUS_O_TEMPLATE,OUT_TEMPLATE,IN_TEMPLATE} },

	{ "cg", "/usr/lib/cg", DRIVER_CFP, setup_cg_all, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ STDOUT_TEMPLATE, OPTION_TEMPLATE, IN_TEMPLATE } },

	{ "inline", "/usr/lib/inline", DRIVER_CFMP, NULL, NULL, NULL,NULL,NULL,NULL,NULL, 0,
		{ STDIN_TEMPLATE, STDOUT_TEMPLATE, OPTION_TEMPLATE } },

#  ifdef sun386
	{ "optim", "/lib/optim", DRIVER_CFMP, setup_c2, NULL, NULL, NULL, NULL, NULL, NULL, 0,
#  else /*!sun386*/
	{ "c2", "/lib/c2",    DRIVER_CFMP, setup_c2, NULL, NULL, NULL, NULL, NULL, NULL, 0,
#  endif /*sun386*/
		{ STDIN_TEMPLATE, STDOUT_TEMPLATE, OPTION_TEMPLATE } },
	{ "as", "/bin/as", DRIVER_CFMP, setup_as, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ MINUS_O_TEMPLATE, OUT_TEMPLATE,OPTION_TEMPLATE,IN_TEMPLATE} },
	{ "asS", "/bin/as", DRIVER_CFMP, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ STDOUT_TEMPLATE, OPTION_TEMPLATE, IN_TEMPLATE } },
	{ "pc3", "/usr/lib/pc3", DRIVER_P, setup_pc3, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ OPTION_TEMPLATE, IN_TEMPLATE } },
	{ "ld", "/bin/ld", DRIVER_CFP, setup_link_step, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ OPTION_TEMPLATE, MINUS_O_TEMPLATE,OUT_TEMPLATE,IN_TEMPLATE} },
	{ "vpald", "/usr/lib/vpald", DRIVER_F, setup_vpald, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ OPTION_TEMPLATE, MINUS_O_TEMPLATE,OUT_TEMPLATE,IN_TEMPLATE} },
	{ "m2l", "/usr/bin/m2l", DRIVER_M, setup_link_step, NULL, NULL, NULL, NULL, NULL, NULL, 0,
		{ OPTION_TEMPLATE, IN_TEMPLATE,MINUS_O_TEMPLATE,OUT_TEMPLATE} },
};

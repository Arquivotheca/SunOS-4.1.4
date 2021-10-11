#ident  "@(#)module_conf.c 1.1 94/10/31 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <machine/module.h>

/*
 * Eventually, when we want to edit all our kernel
 * configuration files, we can make the various
 * bits of code here for each module type #ifdef'd
 * on a symbol from the config file.
 */
 
/************************************************************************
 *		add extern declarations for module drivers here
 ************************************************************************/

extern int	ross_module_identify();
extern void	ross_module_setup();

extern int	vik_module_identify();
extern void	vik_module_setup();

extern int	tsu_module_identify();
extern void	tsu_module_setup();

extern int	swift_module_identify();
extern void	swift_module_setup();

extern int	ross625_module_identify();
extern void	ross625_module_setup();

/************************************************************************
 *		module driver table
 ************************************************************************/

struct module_linkage module_info[] = {
	/*
	 * The ross625_module_identify() must be called before
	 * ross_module_identify() since ross_...() will return 1
	 * for the 625 and I want to avoid modifying module_ross.c
	 */
	{ ross625_module_identify,	ross625_module_setup },
	{ ross_module_identify, 	ross_module_setup },
	{ vik_module_identify, 		vik_module_setup },
	{ tsu_module_identify,		tsu_module_setup },
	{ swift_module_identify,	swift_module_setup },

/************************************************************************
 *		add module driver entries here
 ************************************************************************/
};
int	module_info_size = sizeof module_info / sizeof module_info[0];

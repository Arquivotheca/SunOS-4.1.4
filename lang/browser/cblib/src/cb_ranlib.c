#ident "@(#)cb_ranlib.c 1.1 94/10/31 SMI"

#ifndef lint
#ifdef INCLUDE_COPYRIGHT
static char _copyright_notice_[] =
"Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved. \
Sun considers its source code as an unpublished, proprietary \
trade secret, and it is available only under strict license \
provisions.  This copyright notice is placed here only to protect \
Sun in the event the source is deemed a published work.  Dissassembly, \
decompilation, or other means of reducing the object code to human \
readable form is prohibited by the license agreement under which \
this code is provided to the user or company in possession of this \
copy. \
RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the \
Government is subject to restrictions as set forth in subparagraph \
(c)(1)(ii) of the Rights in Technical Data and Computer Software \
clause at DFARS 52.227-7013 and in similar clauses in the FAR and \
NASA FAR Supplement.";
#endif

#ifdef INCLUDE_COPYRIGHT_REFERENCE
static char _copyright_notice_reference_[] =
"Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved. \
See copyright notice in cb_copyright.o in the libsb.a library.";
#endif
#endif


#ifndef cb_sun_c_tags_h_INCLUDED
#include "cb_sun_c_tags.h"
#endif

#ifndef cb_directory_h_INCLUDED
#include "cb_directory.h"
#endif

#ifndef cb_enter_h_INCLUDED
#include "cb_enter.h"
#endif

#ifndef cb_file_h_INCLUDED
#include "cb_file.h"
#endif

#ifndef cb_graph_h_INCLUDED
#include "cb_graph.h"
#endif

#ifndef cb_init_h_INCLUDED
#include "cb_init.h"
#endif

#ifndef cb_libc_h_INCLUDED
#include "cb_libc.h"
#endif

#ifndef cb_misc_h_INCLUDED
#include "cb_misc.h"
#endif

#ifndef cb_string_h_INCLUDED
#include "cb_string.h"
#endif

#ifndef cb_write_h_INCLUDED
#include "cb_write.h"
#endif

static	int		cb_ranlib_library_seen = 0;
static	char		*cb_ranlib_library_name;
static	char		*cb_ranlib_unit_pattern;
static	int		cb_ranlib_lineno; /* Dummy slot */

/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	void		cb_ranlib_start_library();
extern	void		cb_ranlib_symbol();
extern	void		cb_ranlib_exit();

/*
 *	cb_ranlib_start_library(library, language,
 *				major_version, minor_version, unit_pattern)
 */
void
cb_ranlib_start_library(library, language,
				major_version, minor_version, unit_pattern)
	char		*library;
	char		*language;
	int		major_version;
	int		minor_version;
	char		*unit_pattern;
{
	static int	first_time = 1;
	char		*p;

	if (first_time) {
		first_time = 0;
		cb_enter_start(&cb_ranlib_lineno,
			       language,
			       major_version,
			       minor_version);
		cb_enter_set_multiple();
	}
	if (cb_ranlib_library_seen) {
		cb_enter_pop_file();
		cb_ranlib_library_seen = 0;
	}

	if ((p = rindex(library, '/')) != NULL) {
		library = p+1;
	}
	cb_ranlib_library_name = cb_string_unique(library);
	cb_ranlib_unit_pattern = unit_pattern;
}

/*
 *	cb_ranlib_symbol(symbol)
 *		Called from ranlib for each .cb style .stab
 */
void
cb_ranlib_symbol(symbol, tag)
	char			*symbol;
	unsigned int		tag;
{
	char			*buffer;
	Cb_file_write		cb_file;
	Cb_directory		directory;
	Cb_file			file;
	register unsigned	hash;
	char			*p;
	char			*q;
	char			*name;
	char			path[MAXPATHLEN];
	Cb_refd_dir		mark;
	static char		*node_name;

	if (!cb_ranlib_library_seen) {
		cb_ranlib_library_seen = 1;
		cb_enter_push_file(cb_ranlib_library_name, 0);
		cb_make_absolute_path(cb_ranlib_library_name, 0, path);
		buffer = alloca(strlen(cb_ranlib_unit_pattern) +
				strlen(path) +
				100);
		(void)sprintf(buffer, cb_ranlib_unit_pattern, path);
		(void)cb_enter_symbol(buffer, 0, tag);

		node_name = cb_graph_node_head(path);
		(void)sprintf(path, CB_EX_GRAPHER_NODE_PATTERN, node_name);
		(void)cb_enter_symbol(path,
			      1,
			      (unsigned int)cb_grapher_executable_file_node);
	}

	if ((q = strrchr(symbol, '/')) == NULL) {
		q = symbol-1;
	}
	p = alloca(strlen(q+1)+1);
	(void)strcpy(p, q+1);
	if ((q = strrchr(p, '.')) != NULL) {
		*q = 0;
	}
	if ((q = strrchr(p, '.')) != NULL) {
		*q = 0;
		q[-1] = 'o';
	}
	(void)sprintf(path,
		      CB_EX_GRAPHER_ARC_PATTERN2,
		      CB_EX_GRAPHER_FILES_TAG,
		      node_name,
		      p);
	(void)cb_enter_symbol(path,
			      1,
			      (unsigned int)cb_grapher_executable_object_file_arc);

	directory = cb_directory_home();
	cb_init_read(directory);
	mark = cb_directory_find_file(directory->init,
				      symbol,
				      1,
				      1);
	if (mark != NULL) {
		cb_directory_mark(mark);
	}

	cb_file = cb_enter_get_cb_file();
	if (cb_file->refd_files == NULL) {
		cb_file->refd_files = array_create(cb_file->heap);
		array_expand(cb_file->refd_files, 10);
	}

	hash = cb_string2hash(symbol);
	p = rindex(symbol, '.');
	*p = 0;
	q = rindex(symbol, '.');
	*q = 0;
	name = cb_string_unique(symbol);
	*p = '.';
	*q = '.';
	file = cb_file_create(name,
			      hash,
			      cb_directory_create(cb_get_wd(0)),
			      cb_get_sub_dir_string(CB_NEW_DIR_ID));
	/*
	 * This is a bogus use of the focus field.
	 * It just happens to be available.
	 */
	if (file->focus != NULL) {
		return;
	}
	file->focus = (Array)1;
	file->src.name = name;
	array_append(cb_file->refd_files, (Array_value)file);

	/* Compute the hash value */
	p = rindex(name, '/');
	if (p == NULL) {
		p = name;
	} else {
		p++;
	}
	while (*p != '\0') {
		hash += (unsigned int)*p++;
	}
	cb_file->hashs[(int)cb_refd_files_section] += hash;
}

/*
 *	cb_ranlib_exit()
 */
void
cb_ranlib_exit()
{
	if (cb_ranlib_library_seen) {
		cb_enter_pop_file();
		cb_ranlib_library_seen = 0;
	}
}

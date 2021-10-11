#ident "@(#)cb_directory.c 1.1 94/10/31 SMI"

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


#ifndef cb_cb_h_INCLUDED
#include "cb_cb.h"
#endif

#ifndef cb_directory_h_INCLUDED
#include "cb_directory.h"
#endif

#ifndef cb_file_h_INCLUDED
#include "cb_file.h"
#endif

#ifndef cb_heap_h_INCLUDED
#include "cb_heap.h"
#endif

#ifndef cb_io_h_INCLUDED
#include "cb_io.h"
#endif

#ifndef cb_libc_h_INCLUDED
#include "cb_libc.h"
#endif

#ifndef cb_misc_h_INCLUDED
#include "cb_misc.h"
#endif

#ifndef cb_rw_h_INCLUDED
#include "cb_rw.h"
#endif

#ifndef cb_string_h_INCLUDED
#include "cb_string.h"
#endif

#ifndef hash_h_INCLUDED
#include "hash.h"
#endif

#ifndef MAXNAMLEN
#include <dirent.h>
#endif



/*
 *	Data
 */
static	Hash		directory_table;

/*
 *	Table of contents
 */
extern	Cb_directory	cb_directory_create();
extern	Cb_directory	cb_directory_home();
extern	char		*cb_directory_home_path();
extern	void		cb_directory_flush();
extern	int		cb_directory_flush_fn();
extern	void		cb_directory_resynch();
extern	void		cb_directory_resynch_trim();
extern	void		cb_directory_resynch_append();

/*
 *	cb_directory_create(directory_name)
 *		This will create and return a unique directory object
 *		containing "directory_name".
 */
Cb_directory
cb_directory_create(directory_name)
	char		*directory_name;
{
	char		buffer[MAXPATHLEN];
	Cb_directory	directory;
	Cb_heap		heap;
	
	if (directory_table == NULL) {
		directory_table = hsh_create(10,
					     hsh_int_equal,
					     hsh_int_hash,
					     (Hash_value)NULL,
					     cb_init_heap(0));
	}
	cb_make_absolute_path(directory_name, FALSE, buffer);
	directory_name = cb_string_unique(buffer);
	directory = (Cb_directory)hsh_lookup(directory_table,
					     (Hash_value)directory_name);
	if (directory == NULL) {
		directory = cb_allocate(Cb_directory, directory_table->heap);
		bzero((char *)directory, sizeof(Cb_directory_rec));
		heap = directory_table->heap;
		directory->name = directory_name;
		directory->heap = heap;
		directory->new_files = array_create(heap);
		directory->old_files = array_create(heap);
		directory->refd_files = array_create(heap);
		(void)hsh_insert(directory_table,
				 (Hash_value)directory_name,
				 (Hash_value)directory);
	}
	return directory;
}

/*
 *	cb_directory_home()
 *		This will return the "home" directory object.
 */
Cb_directory
cb_directory_home()
{
	return cb_directory_create(".");
}

/*
 *	cb_directory_home_path()
 *		This will return the "home" directory object.
 */
char *
cb_directory_home_path()
{
	Cb_directory	directory;
	static char	*path = NULL;

	if (path == NULL) {
		directory = cb_directory_home();
		path = directory->name;
	}
	return path;
}

/*
 *	cb_directory_flush()
 */
void
cb_directory_flush()
{
	if (directory_table != NULL) {
		(void)hsh_iterate(directory_table,
				  cb_directory_flush_fn,
				  0);
	}
}

/*
 *	cb_directory_flush_fn(name, directory)
 */
/*ARGSUSED*/
static int
cb_directory_flush_fn(name, directory)
	char		*name;
	Cb_directory	directory;
{
	array_trim(directory->refd_files, 0);
	array_trim(directory->old_files, 0);
	array_trim(directory->new_files, 0);
	directory->init = (Cb_init)0;
	directory->refd_read = 0;
	directory->root_read = 0;
	directory->refd_marked = 0;
	directory->root_marked = 0;
	directory->split_happened = 0;
	directory->is_local_dir = 0;
}

/*
 *	cb_directory_resynch()
 *		This will ensure that all files are on the correct
 *		directory list.
 */
void
cb_directory_resynch()
{
	(void)hsh_iterate(directory_table,
			  (Hash_Int_func)cb_directory_resynch_trim, 0);
	(void)hsh_iterate(cb_file_table_get(),
			  (Hash_Int_func)cb_directory_resynch_append, 0);
}

/*
 *	cb_directory_resynch_trim(dir_name, directory)
 *		This will trim the files lists in "directory".
 */
/* ARGSUSED */
static void
cb_directory_resynch_trim(dir_name, directory)
	char			*dir_name;
	Cb_directory		directory;
{
	array_trim(directory->old_files, 0);
	array_trim(directory->new_files, 0);
	array_trim(directory->refd_files, 0);
}

/*
 *	cb_directory_resynch_append(zilch, file)
 *		This will thread "file" onto the appropriate list in "file".
 */
/* ARGSUSED */
static void
cb_directory_resynch_append(zilch, file)
	int		zilch;
	Cb_file		file;
{
	Cb_directory	directory;
	char		*sub_dir;
	static char	*locked_sub_dir;
	static char	*new_sub_dir;
	static char	*old_sub_dir;
	static char	*refd_sub_dir;

	if (locked_sub_dir == NULL) {
		locked_sub_dir = cb_get_sub_dir_string(CB_LOCKED_DIR_ID);
		new_sub_dir = cb_get_sub_dir_string(CB_NEW_DIR_ID);
		old_sub_dir = cb_get_sub_dir_string(CB_OLD_DIR_ID);
		refd_sub_dir = cb_get_sub_dir_string(CB_REFD_DIR_ID);
	}

	directory = file->directory;
	sub_dir = file->sub_dir;
	if (sub_dir == locked_sub_dir) {
		array_append(directory->new_files, (Array_value)file);
	} else if (sub_dir == new_sub_dir) {
		array_append(directory->new_files, (Array_value)file);
	} else if (sub_dir == old_sub_dir) {
		array_append(directory->old_files, (Array_value)file);
	} else if (sub_dir == refd_sub_dir) {
		array_append(directory->refd_files, (Array_value)file);
	}
}


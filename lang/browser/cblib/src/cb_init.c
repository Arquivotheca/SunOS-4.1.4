#ident "@(#)cb_init.c 1.1 94/10/31 SMI"

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

#ifndef cb_heap_h_INCLUDED
#include "cb_heap.h"
#endif

#ifndef cb_init_h_INCLUDED
#include "cb_init.h"
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

#ifndef cb_string_h_INCLUDED
#include "cb_string.h"
#endif


extern	void	cb_init_read();
extern	char	*cb_init_get_arg();
extern	void	cb_init_parse();
extern	void	cb_init_add_import();
extern	void	cb_init_warn();
extern	void	cb_init_dump();
extern	void	cb_init_add_parent();


static int resynch = 0;

/*
 * cb_init_resynch()
 *	This will force .cbinit files to be reread.
 */
void
cb_init_resynch()
{
	resynch++;
}

/*
 * cb_init_read(directory)
 *	This will read in the contents of the .cbinit file associated with
 *	"directory" and insert it into the directory object.
 */
void
cb_init_read(directory)
	Cb_directory	directory;
{
	char		*file_name;
	char		full_name[MAXPATHLEN];
	Cb_heap		heap;
	Cb_import	import;
	Cb_init		init;
	int		in_file;
	char		*init_file;
	Cb_partition	partition;
	struct stat	status;

	heap = directory->heap;
	file_name = getenv(CB_INIT_ENV_VAR);
	if (file_name == NULL) {
		(void)sprintf(full_name, "%s/%s",
			      directory->name, CB_INIT_DEFAULT_FILE_NAME);
		file_name = full_name;
	}
	status.st_mtime = 0;
	if ((directory->init != NULL) &&
	    ((directory->init->resynch == resynch) ||
	     (stat(file_name, &status) < 0) ||
	     (status.st_mtime == directory->init->timestamp))) {
		return;
	}
	init = cb_allocate(Cb_init, heap);
	init->split = 0;
	init->resynch = resynch;
	init->imports = array_create(heap);
	init->partitions = array_create(heap);

	/* Process the .cbinit file */
	in_file = open(file_name, O_RDONLY);
	if (in_file >= 0) {
		if (status.st_mtime == 0) {
			cb_fstat(in_file, &status);
		}
		init_file = cb_mmap_file_private(in_file,
					(int)status.st_size);
		if (init_file != NULL) {
			cb_init_parse(init_file,
				      (int)status.st_size,
				      file_name,
				      init,
				      heap);
		}
		cb_close(in_file);
		init->timestamp = status.st_mtime;
	}

	/* Create default import records. */
	import = cb_allocate(Cb_import, heap);
	import->dir = cb_directory_create(".");
	import->dir->is_local_dir = 1;
	import->parent = FALSE;
	array_append(init->imports, (Array_value)import);

	/* Create default partition: */
	partition = cb_allocate(Cb_partition, heap);
	partition->dir = cb_directory_create(".");
	partition->prefix = "/";
	partition->prefix_length = 1;
	partition->new_root_made = FALSE;
	array_append(init->partitions, (Array_value)partition);
	directory->init = init;
}

/*
 * cb_init_parse(file_buffer, file_length, file_name, init, heap)
 *	This will parse the buffer pointed to by "file_buffer" for 
 *	"file_length" bytes and store the results into "init".
 */
static void
cb_init_parse(file_buffer, file_length, file_name, init, heap)
	char		*file_buffer;
	int		file_length;
	char		*file_name;
	Cb_init		init;
	Cb_heap		heap;
{
	char		*arg;
	int		line_number = 0;
	char		*line_end = file_buffer - 1;
	char		*file_end = file_buffer + file_length;
	Cb_partition	partition;
	char		*path;
	char		*pointer;
	char		*prefix;
	char		buffer[MAXPATHLEN];

	line_number = 0;
	while (1) {
		if (line_end == NULL) {
			return;
		}
		file_buffer = line_end + 1;
	continuation:
		line_end = strchr(file_buffer, '\n');
		if ((line_end >= file_end) || (file_buffer >= file_end)) {
			return;
		}
		if ((line_end > file_buffer + 2) && (line_end[-1] == '\\')) {
			line_end[-1] = ' ';
			line_end[0] = ' ';
			goto continuation;
		}
		line_number++;
		pointer = file_buffer;
		arg = cb_init_get_arg(&pointer);
		if (arg == NULL) {
			continue;
		} else if (arg[0] == '#') {
			/* Comment line: */
			continue;
		} else if (hsh_string_equal(arg, "split")) {
			arg = cb_init_get_arg(&pointer);
			if (arg == NULL) {
				cb_init_warn(file_name, line_number,
					     "Missing arg");
				continue;
			}
			init->split = atoi(arg);
		} else if (hsh_string_equal(arg, "export")) {
			prefix = cb_init_get_arg(&pointer);
			if (prefix == NULL) {
				cb_init_warn(file_name, line_number,
					     "Missing prefix");
				continue;
			}
			arg = cb_init_get_arg(&pointer);
			if ((arg != NULL) && hsh_string_equal(arg, "into")) {
				path = cb_init_get_arg(&pointer);
			} else {
				path = arg;
			}
			if (path == NULL) {
				cb_init_warn(file_name, line_number,
					     "Missing path");
				continue;
			}

			/* Build child import and partition record: */
			cb_make_absolute_path(prefix, 0, buffer);
			prefix = cb_string_unique(buffer);
			partition = cb_allocate(Cb_partition, heap);
			partition->dir = cb_directory_create(path);
			partition->prefix = prefix;
			partition->prefix_length = strlen(prefix);
			partition->new_root_made = FALSE;
			array_append(init->partitions,
				     (Array_value)partition);

			cb_init_add_import(init, partition->dir, heap);
		} else if (hsh_string_equal(arg, "import")) {
			arg = cb_init_get_arg(&pointer);
			if (arg == NULL) {
				cb_init_warn(file_name, line_number,
					     "Missing arg");
				continue;
			}

			cb_init_add_import(init,
					   cb_directory_create(arg),
					   heap);
		} else {
			cb_init_warn(file_name, line_number,
				     "Bad command '%s'", arg);
		}
	}
}

/*
 *	cb_init_add_import(init, dir, heap)
 */
static void
cb_init_add_import(init, dir, heap)
	Cb_init		init;
	Cb_directory	dir;
	Cb_heap		heap;
{
	int		i;
	Cb_import	import;

	for (i = array_size(init->imports)-1; i >= 0; i--) {
		if (((Cb_import)ARRAY_FETCH(init->imports, i))->dir == dir) {
			return;
		}
	}

	import = cb_allocate(Cb_import, heap);
	import->dir = dir;
	import->parent = FALSE;
	array_append(init->imports, (Array_value)import);
}

/*
 * cb_init_get_arg(pointer_ptr)
 *	This will return the next word pointed to by "pointer_ptr".  A word
 *	a sequence of printing characters surrounded by whitespace.  The
 *	pointer pointed to by "pointer_ptr" is updated to point to the end
 *	of the current word.  If no word is found, NULL is returned.
 */
static char *
cb_init_get_arg(pointer_ptr)
	char		**pointer_ptr;
{
	register char	chr;
	register char	*pointer;
	register char	*start;

	pointer = *pointer_ptr;
	do {
		chr = *pointer++;
	} while ((chr == ' ') || (chr == '\t'));
	start = --pointer;
	do {
		chr = *pointer++;
	} while (('!' <= chr) && (chr <= '~'));
	if (--pointer == start) {
		start = NULL;
	} else {
		start = cb_string_unique_n(start, pointer - start);
	}
	*pointer_ptr = pointer;
	return start;
}

/*
 * cb_init_warn(file_name, line_number, message, arg)
 *	This will print "file_name", "line_number" and "message" with "arg"
 *	as an error message.
 */
/*VARARGS1*/
static void
cb_init_warn(file_name, line_number, message, arg)
	char		*file_name;
	int		line_number;
	char		*message;
	char		*arg;
{
	(void)fprintf(stderr, "\"%s\", line %d: ", file_name, line_number);
	(void)fprintf(stderr, message, arg);
	(void)fprintf(stderr, "\n");
}

#ifdef CB_INIT_DUMP
/*
 *	cb_init_dump(init)
 *		This will dump out the contents of "init".
 */
void
cb_init_dump(init)
	Cb_init		init;
{
	int		index;
	Cb_import	import;
	Cb_partition	partition;
	int		size;

	(void)printf("Split: %d\n", init->split);
	size = array_size(init->imports);
	for (index = 0; index < size; index++) {
		import = (Cb_import)array_fetch(init->imports, index);
		(void)printf("Import %d: %s %s\n",
			     index,
			     import->dir->name,
			     (import->parent ? "(Parent)" : ""));
	}
	size = array_size(init->partitions);
	for (index = 0; index < size; index++) {
		partition = (Cb_partition)array_fetch(init->partitions,
						      index);
		(void)printf("Partition %d: %s into %s\n",
			     index,
			     partition->prefix,
			     partition->dir->name);
	}
	(void)fflush(stdout);
}
#endif

/*
 *	cb_init_add_parent(init, import)
 */
void
cb_init_add_parent(init, import)
	Cb_init		init;
	Cb_import	import;
{
	int		i;
	Cb_import	new;
	char		path[MAXPATHLEN];

	for (i = 0; i < array_size(init->imports); i++) {
		if ((Cb_import)ARRAY_FETCH(init->imports, i) == import) {
			new = cb_allocate(Cb_import, init->imports->heap);
			(void)sprintf(path,
				      "%s/%s",
				      import->dir->name,
				      CB_DIRECTORY);
			new->dir = cb_directory_create(path);
			new->parent = 1;
			new->added_parent = 0;
			array_insert(init->imports, i + 1, (Array_value)new);
			return;
		}
	}
}

